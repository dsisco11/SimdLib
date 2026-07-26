Set-StrictMode -Version Latest

$script:Utf8NoBom = [System.Text.UTF8Encoding]::new($false)

<#
.SYNOPSIS
Returns the repository root owned by the pipeline tools.
#>
function Get-PipelineRepositoryRoot {
    return Split-Path -Parent $PSScriptRoot
}

<#
.SYNOPSIS
Computes the canonical digest of source inputs that affect build artifacts.
.PARAMETER RepositoryRoot
Absolute path to the SimdLib source tree.
#>
function Get-PipelineSourceDigest {
    param([Parameter(Mandatory)][string]$RepositoryRoot)
    $root = [System.IO.Path]::GetFullPath($RepositoryRoot)
    $files = [System.Collections.Generic.List[string]]::new()
    foreach ($name in @('CMakeLists.txt', 'CMakePresets.json', 'compose.yml', '.clang-format')) {
        $path = Join-Path $root $name
        if (Test-Path -LiteralPath $path -PathType Leaf) { $files.Add($path) }
    }
    foreach ($directory in @('include', 'cmake', 'tests', 'examples', 'benchmarks', 'containers', 'tools')) {
        $path = Join-Path $root $directory
        if (Test-Path -LiteralPath $path -PathType Container) {
            foreach ($file in Get-ChildItem -LiteralPath $path -File -Recurse) { $files.Add($file.FullName) }
        }
    }
    $stream = [System.IO.MemoryStream]::new()
    try {
        $orderedFiles = $files.ToArray()
        [Array]::Sort($orderedFiles, [System.StringComparer]::Ordinal)
        foreach ($file in $orderedFiles) {
            $relative = [System.IO.Path]::GetRelativePath($root, $file).Replace('\', '/')
            $relativeBytes = $script:Utf8NoBom.GetBytes($relative)
            $stream.Write($relativeBytes, 0, $relativeBytes.Length)
            $stream.WriteByte(0)
            $hash = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
            $hashBytes = $script:Utf8NoBom.GetBytes($hash)
            $stream.Write($hashBytes, 0, $hashBytes.Length)
            $stream.WriteByte(10)
        }
        return [Convert]::ToHexString(
            [System.Security.Cryptography.SHA256]::HashData($stream.ToArray())).ToLowerInvariant()
    } finally {
        $stream.Dispose()
    }
}

<#
.SYNOPSIS
Computes the lowercase SHA-256 digest of a UTF-8 string.
.PARAMETER Text
Text to hash.
#>
function Get-PipelineTextDigest {
    param([Parameter(Mandatory)][string]$Text)
    $bytes = $script:Utf8NoBom.GetBytes($Text)
    return [Convert]::ToHexString(
        [System.Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
}

<#
.SYNOPSIS
Writes UTF-8 text atomically.
.PARAMETER Path
Destination file.
.PARAMETER Content
Text to write.
#>
function Set-PipelineTextFile {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][AllowEmptyString()][string]$Content
    )
    $directory = Split-Path -Parent $Path
    if ($directory) { New-Item -ItemType Directory -Path $directory -Force | Out-Null }
    $temporary = "$Path.tmp-$PID"
    [System.IO.File]::WriteAllText($temporary, $Content, $script:Utf8NoBom)
    Move-Item -LiteralPath $temporary -Destination $Path -Force
}

<#
.SYNOPSIS
Reads a key-value build manifest.
.PARAMETER Path
Manifest path.
#>
function Read-PipelineManifest {
    param([Parameter(Mandatory)][string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Required build manifest is missing: $Path" }
    $values = @{}
    foreach ($line in Get-Content -LiteralPath $Path) {
        $separator = $line.IndexOf('=')
        if ($separator -gt 0) { $values[$line.Substring(0, $separator)] = $line.Substring($separator + 1) }
    }
    return $values
}

<#
.SYNOPSIS
Invokes a command, records its combined output, and preserves its exit code.
.PARAMETER FilePath
Executable to invoke.
.PARAMETER ArgumentList
Arguments passed without shell reinterpretation.
.PARAMETER LogPath
File that receives combined output.
#>
function Invoke-PipelineCommand {
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][string[]]$ArgumentList,
        [Parameter(Mandatory)][string]$LogPath
    )
    $directory = Split-Path -Parent $LogPath
    if ($directory) { New-Item -ItemType Directory -Path $directory -Force | Out-Null }
    & $FilePath @ArgumentList 2>&1 | Tee-Object -FilePath $LogPath
    if ($LASTEXITCODE -ne 0) { throw "$FilePath failed with exit code $LASTEXITCODE. Log: $LogPath" }
}

<#
.SYNOPSIS
Imports the installed Visual Studio x64 developer environment.
#>
function Initialize-PipelineVisualStudioEnvironment {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw "Visual Studio locator is missing: $vswhere" }
    $installation = (& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $installation) { throw 'A Visual Studio installation with the x64 C++ tools is required.' }
    $developerCommand = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
    $environmentLines = & cmd.exe /s /c "`"$developerCommand`" -no_logo -arch=x64 -host_arch=x64 && set"
    if ($LASTEXITCODE -ne 0) { throw 'Unable to initialize the Visual Studio x64 developer environment.' }
    foreach ($line in $environmentLines) {
        $separator = $line.IndexOf('=')
        if ($separator -gt 0) { [Environment]::SetEnvironmentVariable($line.Substring(0, $separator), $line.Substring($separator + 1), 'Process') }
    }
    return $installation
}

<#
.SYNOPSIS
Returns the current Git revision or a stable unknown marker.
.PARAMETER RepositoryRoot
Absolute source-tree path.
#>
function Get-PipelineRevision {
    param([Parameter(Mandatory)][string]$RepositoryRoot)
    $revision = (& git -C $RepositoryRoot rev-parse HEAD 2>$null).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $revision) { return 'unknown' }
    return $revision
}

<#
.SYNOPSIS
Runs independent PowerShell pipeline operations concurrently and aggregates failures.
.PARAMETER Operations
Objects with Id, Script, and Arguments properties.
.PARAMETER LogDirectory
Invocation-owned directory for child stdout and stderr logs.
#>
function Invoke-PipelineChildOperations {
    param(
        [Parameter(Mandatory)][object[]]$Operations,
        [Parameter(Mandatory)][string]$LogDirectory
    )
    if ($Operations.Count -eq 0) { return }
    New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null
    $pwsh = (Get-Command pwsh -ErrorAction Stop).Source
    $runs = [System.Collections.Generic.List[object]]::new()
    try {
        foreach ($operation in $Operations) {
            $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
            $startInfo.FileName = $pwsh
            $startInfo.UseShellExecute = $false
            $startInfo.RedirectStandardOutput = $true
            $startInfo.RedirectStandardError = $true
            foreach ($argument in @('-NoProfile', '-File', $operation.Script) + @($operation.Arguments)) {
                $startInfo.ArgumentList.Add([string]$argument)
            }
            $process = [System.Diagnostics.Process]::new()
            $process.StartInfo = $startInfo
            if (-not $process.Start()) { throw "Unable to start pipeline operation $($operation.Id)" }
            $runs.Add([pscustomobject]@{
                    Id = $operation.Id; Process = $process
                    StandardOutput = $process.StandardOutput.ReadToEndAsync()
                    StandardError = $process.StandardError.ReadToEndAsync()
                })
            Write-Host "Started pipeline operation: $($operation.Id)"
        }
        $failures = [System.Collections.Generic.List[string]]::new()
        foreach ($run in $runs) {
            $run.Process.WaitForExit()
            $stdout = $run.StandardOutput.GetAwaiter().GetResult()
            $stderr = $run.StandardError.GetAwaiter().GetResult()
            Set-PipelineTextFile -Path (Join-Path $LogDirectory "$($run.Id).stdout.log") -Content $stdout
            Set-PipelineTextFile -Path (Join-Path $LogDirectory "$($run.Id).stderr.log") -Content $stderr
            if ($stdout) { Write-Host $stdout.TrimEnd() }
            if ($stderr) { [Console]::Error.WriteLine($stderr.TrimEnd()) }
            if ($run.Process.ExitCode -ne 0) { $failures.Add("$($run.Id)=$($run.Process.ExitCode)") }
        }
        if ($failures.Count) { throw "Pipeline operations failed: $($failures -join ', '). Logs: $LogDirectory" }
    } finally {
        foreach ($run in $runs) {
            if (-not $run.Process.HasExited) {
                try { $run.Process.Kill($true); $run.Process.WaitForExit() } catch { Write-Warning "Unable to stop $($run.Id): $_" }
            }
            $run.Process.Dispose()
        }
    }
}

Export-ModuleMember -Function @(
    'Get-PipelineRepositoryRoot',
    'Get-PipelineSourceDigest',
    'Get-PipelineTextDigest',
    'Set-PipelineTextFile',
    'Read-PipelineManifest',
    'Invoke-PipelineCommand',
    'Initialize-PipelineVisualStudioEnvironment',
    'Get-PipelineRevision',
    'Invoke-PipelineChildOperations'
)
