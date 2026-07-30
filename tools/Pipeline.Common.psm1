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
Returns the exact configure presets owned by the unified default validation matrix.
.PARAMETER SelectedCompilers
Canonical user-facing compiler names selected by the caller.
#>
function Get-PipelineDefaultValidationPresets {
    param([Parameter(Mandatory)][string[]]$SelectedCompilers)

    $presets = [System.Collections.Generic.List[string]]::new()
    foreach ($compiler in $SelectedCompilers) {
        switch ($compiler) {
            'Msvc' {
                $presets.Add('msvc-release-exhaustive')
                $presets.Add('msvc-debug-diagnostics')
            }
            'ClangCl' { $presets.Add('clangcl-release-exhaustive') }
            'ClangCoverage' { $presets.Add('clang-debug-coverage') }
            'Gcc13' { $presets.Add('gcc13-core-release-exhaustive') }
            'Gcc14' { $presets.Add('gcc14-release-exhaustive') }
            'Clang22' {
                $presets.Add('clang22-release-exhaustive')
                $presets.Add('clang22-debug-asan-ubsan')
            }
            default { throw "Unknown compiler identity in the default validation matrix: $compiler" }
        }
    }
    return $presets.ToArray()
}

<#
.SYNOPSIS
Reports whether a configure preset belongs to the unified default validation matrix.
.PARAMETER Preset
Configure preset name to classify.
#>
function Test-PipelineDefaultValidationPreset {
    param([Parameter(Mandatory)][string]$Preset)

    $allDefaultPresets = Get-PipelineDefaultValidationPresets -SelectedCompilers @(
        'Msvc', 'ClangCl', 'ClangCoverage', 'Gcc13', 'Gcc14', 'Clang22')
    return $Preset -in $allDefaultPresets
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
        $relativeFiles = @($files | ForEach-Object {
                [System.IO.Path]::GetRelativePath($root, $_).Replace('\', '/')
            })
        [Array]::Sort($relativeFiles, [System.StringComparer]::Ordinal)
        foreach ($relative in $relativeFiles) {
            $file = Join-Path $root $relative.Replace('/', [System.IO.Path]::DirectorySeparatorChar)
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
Resolves a manifest artifact path into the host repository.
.PARAMETER RepositoryRoot
Absolute SimdLib source tree.
.PARAMETER Path
Host, repository-relative, or canonical `/workspace` container path.
#>
function Resolve-PipelineArtifactPath {
    param(
        [Parameter(Mandatory)][string]$RepositoryRoot,
        [Parameter(Mandatory)][string]$Path
    )

    $root = [System.IO.Path]::GetFullPath($RepositoryRoot)
    if (Test-Path -LiteralPath $Path) {
        $candidate = $Path
    } elseif ($Path -match '^/workspace/out/(?<relative>.+)$') {
        $candidate = Join-Path (
            Join-Path $root 'out/pipeline') $Matches.relative
    } elseif (-not [System.IO.Path]::IsPathRooted($Path)) {
        $candidate = Join-Path $root $Path
    } else {
        throw "Manifest artifact path is not host-accessible: $Path"
    }
    $resolved = [System.IO.Path]::GetFullPath($candidate)
    if (-not $resolved.StartsWith(
            $root + [System.IO.Path]::DirectorySeparatorChar,
            [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Manifest artifact path escapes the repository: $Path"
    }
    return $resolved
}

<#
.SYNOPSIS
Creates the unified-receipt entry for a completed repository audit.
.PARAMETER RepositoryRoot
Absolute SimdLib source tree.
.PARAMETER AuditPath
Machine-readable repository audit result.
.PARAMETER ExpectedSourceDigest
Canonical source digest the audit must own.
#>
function New-PipelineRepositoryAuditEntry {
    param(
        [Parameter(Mandatory)][string]$RepositoryRoot,
        [Parameter(Mandatory)][string]$AuditPath,
        [Parameter(Mandatory)][string]$ExpectedSourceDigest
    )
    if (-not (Test-Path -LiteralPath $AuditPath -PathType Leaf)) {
        throw "Repository audit result is missing: $AuditPath"
    }
    $audit = Get-Content -LiteralPath $AuditPath -Raw | ConvertFrom-Json
    if ($audit.schema -ne 'simdlib.repository-audit.v1' -or
        $audit.status -ne 'complete' -or
        $audit.sourceDigest -ne $ExpectedSourceDigest) {
        throw "Repository audit result is stale or incompatible: $AuditPath"
    }
    return [ordered]@{
        path = [System.IO.Path]::GetRelativePath($RepositoryRoot, $AuditPath).Replace('\', '/')
        sha256 = (Get-FileHash -LiteralPath $AuditPath -Algorithm SHA256).Hash.ToLowerInvariant()
        sourceDigest = [string]$audit.sourceDigest
    }
}

<#
.SYNOPSIS
Validates the repository-audit entry bound into a unified build receipt.
.PARAMETER RepositoryRoot
Absolute SimdLib source tree.
.PARAMETER Entry
Receipt entry containing path, hash, and source digest.
.PARAMETER ExpectedSourceDigest
Canonical source digest required by the consuming operation.
#>
function Assert-PipelineRepositoryAuditEntry {
    param(
        [Parameter(Mandatory)][string]$RepositoryRoot,
        [Parameter(Mandatory)][object]$Entry,
        [Parameter(Mandatory)][string]$ExpectedSourceDigest
    )
    if (-not $Entry -or $Entry.sourceDigest -ne $ExpectedSourceDigest) {
        throw 'Unified build receipt does not contain the current repository audit.'
    }
    $auditPath = Join-Path $RepositoryRoot ([string]$Entry.path)
    if (-not (Test-Path -LiteralPath $auditPath -PathType Leaf)) {
        throw "Receipt repository audit is missing: $auditPath"
    }
    $auditHash = (Get-FileHash -LiteralPath $auditPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($auditHash -ne $Entry.sha256) {
        throw "Receipt repository audit changed after the unified build: $auditPath"
    }
    $audit = Get-Content -LiteralPath $auditPath -Raw | ConvertFrom-Json
    if ($audit.schema -ne 'simdlib.repository-audit.v1' -or
        $audit.status -ne 'complete' -or
        $audit.sourceDigest -ne $ExpectedSourceDigest) {
        throw "Receipt repository audit is incomplete or stale: $auditPath"
    }
    return $auditPath
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
    'Get-PipelineDefaultValidationPresets',
    'Test-PipelineDefaultValidationPreset',
    'Get-PipelineSourceDigest',
    'Get-PipelineTextDigest',
    'Set-PipelineTextFile',
    'Read-PipelineManifest',
    'Resolve-PipelineArtifactPath',
    'New-PipelineRepositoryAuditEntry',
    'Assert-PipelineRepositoryAuditEntry',
    'Invoke-PipelineCommand',
    'Initialize-PipelineVisualStudioEnvironment',
    'Get-PipelineRevision',
    'Invoke-PipelineChildOperations'
)
