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
Reads the canonical validation matrix.
.PARAMETER RepositoryRoot
Absolute SimdLib source tree.
#>
function Get-PipelineValidationMatrix {
    param([string]$RepositoryRoot = (Get-PipelineRepositoryRoot))

    $path = Join-Path $RepositoryRoot 'tools/validation-matrix.json'
    $matrix = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    if ($matrix.schema -ne 'simdlib.validation-matrix.v1') {
        throw "Unsupported validation matrix schema in $path"
    }
    return $matrix
}

<#
.SYNOPSIS
Returns compiler names in matrix-owned deterministic order for one platform.
.PARAMETER Platform
Validation runner platform.
#>
function Get-PipelineValidationCompilers {
    param([Parameter(Mandatory)][ValidateSet('native', 'container')][string]$Platform)

    $matrix = Get-PipelineValidationMatrix
    $available = @($matrix.cells.PSObject.Properties |
        Where-Object { $_.Value.platform -eq $Platform } |
        ForEach-Object { $_.Value.compiler } | Select-Object -Unique)
    return @($matrix.compilerOrder | Where-Object { $_ -in $available })
}
<#
.SYNOPSIS
Returns the ordered cells assigned to one canonical matrix operation.
.PARAMETER Operation
Operation name from the validation matrix.
.PARAMETER RepositoryRoot
Absolute SimdLib source tree.
#>
function Get-PipelineValidationOperationCells {
    param(
        [Parameter(Mandatory)][string]$Operation,
        [string]$RepositoryRoot = (Get-PipelineRepositoryRoot)
    )

    $matrix = Get-PipelineValidationMatrix -RepositoryRoot $RepositoryRoot
    $operationProperty = $matrix.operations.PSObject.Properties[$Operation]
    if (-not $operationProperty) {
        throw "Validation matrix does not define operation $Operation"
    }
    $seen = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::Ordinal)
    return @(
        foreach ($cellId in @($operationProperty.Value)) {
            if (-not $seen.Add([string]$cellId)) {
                throw "Validation matrix operation $Operation duplicates cell $cellId"
            }
            $cellProperty = $matrix.cells.PSObject.Properties[[string]$cellId]
            if (-not $cellProperty) {
                throw "Validation matrix operation $Operation references unknown cell $cellId"
            }
            $cell = $cellProperty.Value.PSObject.Copy()
            Add-Member -InputObject $cell -NotePropertyName MatrixCell `
                -NotePropertyValue ([string]$cellId) -Force
            $cell
        }
    )
}

<#
.SYNOPSIS
Resolves runner-facing cells from canonical matrix operations and filters.
.PARAMETER Platform
Runner platform to select.
.PARAMETER CompilerNames
Canonical user-facing compiler names.
.PARAMETER CellScope
Requested configuration or instrumentation scope.
.PARAMETER Operation
Runner operation name.
#>
function Resolve-PipelineValidationCells {
    param(
        [Parameter(Mandatory)][ValidateSet('native', 'container')][string]$Platform,
        [Parameter(Mandatory)][string[]]$CompilerNames,
        [Parameter(Mandatory)][string]$CellScope,
        [Parameter(Mandatory)][string]$Operation
    )

    $operationName = switch ($Operation) {
        { $_ -in @('BuildCompilerContracts', 'TestCompilerContracts') } { 'compilerContracts'; break }
        'RecordCodegen' { 'optionalDiagnostics'; break }
        { $_ -in @('BuildBenchmarks', 'RunBenchmarks') } { 'benchmarks'; break }
        'Test' { 'defaultTests'; break }
        default { 'defaultBuild' }
    }
    $matrix = Get-PipelineValidationMatrix
    $candidateIds = [System.Collections.Generic.List[string]]::new()
    foreach ($name in @($operationName, 'optionalDebug', 'coverage', 'sanitizer')) {
        $property = $matrix.operations.PSObject.Properties[$name]
        if ($property) {
            foreach ($cellId in @($property.Value)) {
                if (-not $candidateIds.Contains([string]$cellId)) {
                    $candidateIds.Add([string]$cellId)
                }
            }
        }
    }

    $operationIds = @($matrix.operations.PSObject.Properties[$operationName].Value)
    return @(
        foreach ($cellId in $candidateIds) {
            $cell = $matrix.cells.PSObject.Properties[$cellId].Value
            if ($cell.platform -ne $Platform -or $cell.compiler -notin $CompilerNames) {
                continue
            }
            $scopeMatches = switch ($CellScope) {
                'All' { $cellId -in $operationIds }
                'Release' { $cell.profile -in @('RELEASE', 'COMPILER_CONTRACTS') }
                'Debug' {
                    $cell.profile -in @('DEBUG', 'CODEGEN_DIAGNOSTIC') -and
                    $cell.instrumentation -eq 'none'
                }
                'Coverage' { $cell.profile -eq 'COVERAGE' }
                'AsanUbsan' { $cell.instrumentation -eq 'asan-ubsan' }
                default { $false }
            }
            if (-not $scopeMatches) { continue }
            if ($Operation -in @('BuildBenchmarks', 'RunBenchmarks',
                    'BuildCompilerContracts', 'TestCompilerContracts',
                    'RecordCodegen') -and $cellId -notin $operationIds) {
                continue
            }

            $runnerCompiler = switch ($cell.compiler) {
                'Msvc' { 'msvc' }
                'ClangCl' { 'clangcl' }
                'ClangCoverage' { 'clang-coverage' }
                default { ([string]$cell.compiler).ToLowerInvariant() }
            }
            $definition = [ordered]@{
                MatrixCell = [string]$cellId
                Key = [string]$cell.artifactKey
                Preset = [string]$cell.preset
                BuildProfile = [string]$cell.configuration
                Consumer = [bool]$cell.consumer
                Coverage = $cell.instrumentation -eq 'coverage'
                Instrumentation = [string]$cell.instrumentation
                Sanitizer = if ($cell.instrumentation -eq 'asan-ubsan') { 'asan-ubsan' } else { 'none' }
                CodegenMode = [string]$cell.codegenMode
                Aggregate = [string]$cell.aggregate
            }
            if ($Platform -eq 'native') {
                $definition.Compiler = $runnerCompiler
                $definition.Generator = [string]$cell.generator
            } else {
                $definition.Service = $runnerCompiler
            }
            [pscustomobject]$definition
        }
    )
}
<#
.SYNOPSIS
Returns the exact configure presets owned by the unified default validation matrix.
.PARAMETER SelectedCompilers
Canonical user-facing compiler names selected by the caller.
#>
function Get-PipelineDefaultValidationPresets {
    param([Parameter(Mandatory)][string[]]$SelectedCompilers)

    return @(
        Get-PipelineValidationOperationCells -Operation defaultBuild |
            Where-Object compiler -in $SelectedCompilers |
            ForEach-Object preset
    )
}

<#
.SYNOPSIS
Reports whether a configure preset belongs to the unified default validation matrix.
.PARAMETER Preset
Configure preset name to classify.
#>
function Test-PipelineDefaultValidationPreset {
    param([Parameter(Mandatory)][string]$Preset)

    $allDefaultPresets = Get-PipelineValidationOperationCells `
        -Operation defaultBuild | ForEach-Object preset
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
Returns the reviewed files that own pipeline-tooling validation.
.PARAMETER RepositoryRoot
Absolute SimdLib source tree.
#>
function Get-PipelineToolingInputs {
    param([Parameter(Mandatory)][string]$RepositoryRoot)

    $root = [System.IO.Path]::GetFullPath($RepositoryRoot)
    $matrix = Get-PipelineValidationMatrix -RepositoryRoot $root
    $classes = @($matrix.toolingValidation.inputClasses.PSObject.Properties)
    if ($classes.Count -eq 0) { throw 'Validation matrix defines no tooling-input classes.' }
    $owned = [System.Collections.Generic.List[object]]::new()
    $relativeOwners = @{}
    foreach ($class in $classes) {
        foreach ($declaredPath in @($class.Value)) {
            $path = Join-Path $root ([string]$declaredPath)
            if (Test-Path -LiteralPath $path -PathType Container) {
                $files = @(Get-ChildItem -LiteralPath $path -File -Recurse | Sort-Object FullName)
            } elseif (Test-Path -LiteralPath $path -PathType Leaf) {
                $files = @((Get-Item -LiteralPath $path))
            } else {
                throw "Pipeline-tooling input is missing: $declaredPath"
            }
            foreach ($file in $files) {
                $relative = [System.IO.Path]::GetRelativePath($root, $file.FullName).Replace('\', '/')
                if ($relativeOwners.ContainsKey($relative)) {
                    throw "Pipeline-tooling input $relative belongs to both $($relativeOwners[$relative]) and $($class.Name)"
                }
                $relativeOwners[$relative] = $class.Name
                $owned.Add([pscustomobject]@{
                        Class = [string]$class.Name
                        RelativePath = $relative
                        FullName = $file.FullName
                    })
            }
        }
    }
    return @($owned | Sort-Object Class, RelativePath)
}

<#
.SYNOPSIS
Computes the digest of the reviewed pipeline-tooling input set.
.PARAMETER RepositoryRoot
Absolute SimdLib source tree.
#>
function Get-PipelineToolingDigest {
    param([Parameter(Mandatory)][string]$RepositoryRoot)

    $stream = [System.IO.MemoryStream]::new()
    try {
        foreach ($input in Get-PipelineToolingInputs -RepositoryRoot $RepositoryRoot) {
            $record = "$($input.Class)`0$($input.RelativePath)`0$((Get-FileHash -LiteralPath $input.FullName -Algorithm SHA256).Hash.ToLowerInvariant())`n"
            $bytes = $script:Utf8NoBom.GetBytes($record)
            $stream.Write($bytes, 0, $bytes.Length)
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
Creates the unified-receipt entry for completed pipeline-tooling validation.
.PARAMETER RepositoryRoot
Absolute SimdLib source tree.
.PARAMETER ResultPath
Machine-readable pipeline-tooling validation result.
.PARAMETER ExpectedToolingDigest
Canonical tooling digest the result must own.
#>
function New-PipelineValidationEntry {
    param(
        [Parameter(Mandatory)][string]$RepositoryRoot,
        [Parameter(Mandatory)][string]$ResultPath,
        [Parameter(Mandatory)][string]$ExpectedToolingDigest
    )
    if (-not (Test-Path -LiteralPath $ResultPath -PathType Leaf)) {
        throw "Pipeline-tooling validation result is missing: $ResultPath"
    }
    $result = Get-Content -LiteralPath $ResultPath -Raw | ConvertFrom-Json
    if ($result.schema -ne 'simdlib.pipeline-tooling-validation.v1' -or
        $result.status -ne 'complete' -or
        $result.toolingDigest -ne $ExpectedToolingDigest) {
        throw "Pipeline-tooling validation result is stale or incompatible: $ResultPath"
    }
    return [ordered]@{
        path = [System.IO.Path]::GetRelativePath($RepositoryRoot, $ResultPath).Replace('\', '/')
        sha256 = (Get-FileHash -LiteralPath $ResultPath -Algorithm SHA256).Hash.ToLowerInvariant()
        status = [string]$result.status
        schema = [string]$result.schema
        toolingDigest = [string]$result.toolingDigest
    }
}

<#
.SYNOPSIS
Validates the pipeline-tooling entry bound into a unified build receipt.
.PARAMETER RepositoryRoot
Absolute SimdLib source tree.
.PARAMETER Entry
Receipt entry containing result identity and tooling digest.
.PARAMETER ExpectedToolingDigest
Canonical tooling digest required by the consuming operation.
#>
function Assert-PipelineValidationEntry {
    param(
        [Parameter(Mandatory)][string]$RepositoryRoot,
        [Parameter(Mandatory)][AllowNull()][object]$Entry,
        [Parameter(Mandatory)][string]$ExpectedToolingDigest
    )
    if (-not $Entry -or
        $Entry.schema -ne 'simdlib.pipeline-tooling-validation.v1' -or
        $Entry.status -ne 'complete' -or
        $Entry.toolingDigest -ne $ExpectedToolingDigest) {
        throw 'Unified build receipt does not contain current pipeline-tooling validation.'
    }
    $resultPath = Join-Path $RepositoryRoot ([string]$Entry.path)
    if (-not (Test-Path -LiteralPath $resultPath -PathType Leaf)) {
        throw "Receipt pipeline-tooling validation is missing: $resultPath"
    }
    $resultHash = (Get-FileHash -LiteralPath $resultPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($resultHash -ne $Entry.sha256) {
        throw "Receipt pipeline-tooling validation changed after the unified build: $resultPath"
    }
    $result = Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
    if ($result.schema -ne $Entry.schema -or
        $result.status -ne $Entry.status -or
        $result.toolingDigest -ne $ExpectedToolingDigest) {
        throw "Receipt pipeline-tooling validation is incomplete or stale: $resultPath"
    }
    return $resultPath
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
    'Get-PipelineValidationMatrix',
    'Get-PipelineValidationCompilers',
    'Get-PipelineValidationOperationCells',
    'Resolve-PipelineValidationCells',
    'Get-PipelineDefaultValidationPresets',
    'Test-PipelineDefaultValidationPreset',
    'Get-PipelineSourceDigest',
    'Get-PipelineToolingInputs',
    'Get-PipelineToolingDigest',
    'Get-PipelineTextDigest',
    'Set-PipelineTextFile',
    'Read-PipelineManifest',
    'Resolve-PipelineArtifactPath',
    'New-PipelineValidationEntry',
    'Assert-PipelineValidationEntry',
    'Invoke-PipelineCommand',
    'Initialize-PipelineVisualStudioEnvironment',
    'Get-PipelineRevision',
    'Invoke-PipelineChildOperations'
)
