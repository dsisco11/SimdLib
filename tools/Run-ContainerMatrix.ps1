<#
.SYNOPSIS
Builds or consumes fingerprinted Linux compiler cells.
.DESCRIPTION
Each invocation owns one action. Build creates all selected validation
artifacts, Test consumes them without compilation, benchmark actions share the
Release trees, RecordCodegen creates an isolated diagnostic fingerprint, and
InspectEnvironment performs no project build.
#>
[CmdletBinding()]
param(
    [ValidateSet('Build', 'Test', 'RecordCodegen', 'BuildBenchmarks', 'RunBenchmarks', 'InspectEnvironment', 'Clean')]
    [string]$Action = 'Build',
    [ValidateSet('All', 'Release', 'Debug', 'AsanUbsan')]
    [string]$Cell = 'All',
    [ValidateSet('All', 'Gcc13', 'Gcc14', 'Clang22')]
    [string]$Compiler = 'All',
    [ValidateRange(1, 32)]
    [int]$MaxParallel = 3,
    [switch]$SkipImageBuild,
    [switch]$NoImageCache,
    [string]$TestRegex = '',
    [string]$TestLabel = '',
    [string[]]$InjectFailure = @('None'),
    [ValidateRange(0, 86400)]
    [int]$CancelAfterSeconds = 0
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Pipeline.Common.psm1') -Force
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$composeFile = Join-Path $repositoryRoot 'compose.yml'
$pipelineRoot = Join-Path $repositoryRoot 'out/pipeline'
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)

if (-not (Get-Command docker -CommandType Application -ErrorAction SilentlyContinue)) {
    throw 'Docker CLI is required to run the container compiler matrix, but docker was not found on PATH.'
}

if (-not $env:SIMDLIB_BUILD_REVISION) {
    $env:SIMDLIB_BUILD_REVISION = (& git -C $repositoryRoot rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to determine the SimdLib revision for operation provenance.'
    }
}
if ($IsLinux -or $IsMacOS) {
    $env:SIMDLIB_HOST_UID = (& id -u).Trim()
    $env:SIMDLIB_HOST_GID = (& id -g).Trim()
}

<#
.SYNOPSIS
Invokes Docker and rejects a nonzero exit code.
.PARAMETER Arguments
Arguments passed directly to Docker.
#>
function Invoke-DockerChecked {
    param([Parameter(Mandatory)][string[]]$Arguments)
    & docker @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "docker $($Arguments -join ' ') failed with exit code $LASTEXITCODE"
    }
}

<#
.SYNOPSIS
Returns the selected compiler service names.
.PARAMETER CompilerName
User-facing compiler selection.
#>
function Resolve-Services {
    param([Parameter(Mandatory)][string]$CompilerName)
    switch ($CompilerName) {
        'Gcc13' { @('gcc13') }
        'Gcc14' { @('gcc14') }
        'Clang22' { @('clang22') }
        default { @('gcc13', 'gcc14', 'clang22') }
    }
}

<#
.SYNOPSIS
Returns every build cell owned by the selected compilers and scope.
.PARAMETER Services
Selected Compose services.
.PARAMETER CellScope
Requested configuration scope.
.PARAMETER Operation
Requested pipeline operation.
#>
function Resolve-Cells {
    param(
        [Parameter(Mandatory)][string[]]$Services,
        [Parameter(Mandatory)][string]$CellScope,
        [Parameter(Mandatory)][string]$Operation
    )
    $cells = [System.Collections.Generic.List[object]]::new()
    foreach ($service in $Services) {
        if ($Operation -eq 'RecordCodegen') {
            if ($service -ne 'gcc13' -and $CellScope -in @('All', 'Debug')) {
                $cells.Add([pscustomobject]@{
                        Service = $service; Key = 'debug-codegen'; Preset = "$service-debug-codegen-diagnostic"
                        BuildProfile = 'Debug'; Sanitizer = 'none'; CodegenMode = 'RECORD'
                    })
            }
            if ($service -eq 'clang22' -and $CellScope -in @('All', 'AsanUbsan')) {
                $cells.Add([pscustomobject]@{
                        Service = $service; Key = 'asan-ubsan-codegen'; Preset = 'clang22-asan-ubsan-codegen-diagnostic'
                        BuildProfile = 'Debug'; Sanitizer = 'asan-ubsan'; CodegenMode = 'RECORD'
                    })
            }
            continue
        }
        if ($CellScope -in @('All', 'Release')) {
            $preset = if ($service -eq 'gcc13') { 'gcc13-core-release-exhaustive' } else { "$service-release-exhaustive" }
            $codegenMode = if ($service -eq 'gcc13') { 'OFF' } else { 'ENFORCE' }
            $cells.Add([pscustomobject]@{ Service = $service; Key = 'release'; Preset = $preset; BuildProfile = 'Release'; Sanitizer = 'none'; CodegenMode = $codegenMode })
        }
        if ($CellScope -in @('All', 'Debug')) {
            $preset = if ($service -eq 'gcc13') { 'gcc13-core-debug-diagnostics' } else { "$service-debug-diagnostics" }
            if ($CellScope -eq 'Debug' -or (Test-PipelineDefaultValidationPreset -Preset $preset)) {
                $cells.Add([pscustomobject]@{ Service = $service; Key = 'debug'; Preset = $preset; BuildProfile = 'Debug'; Sanitizer = 'none'; CodegenMode = 'OFF' })
            }
        }
        if ($service -eq 'clang22' -and $CellScope -in @('All', 'AsanUbsan')) {
            $cells.Add([pscustomobject]@{ Service = $service; Key = 'debug-asan-ubsan'; Preset = 'clang22-debug-asan-ubsan'; BuildProfile = 'Debug'; Sanitizer = 'asan-ubsan'; CodegenMode = 'OFF' })
        }
    }
    return $cells.ToArray()
}

<#
.SYNOPSIS
Reads immutable identity and labels from one local compiler image.
.PARAMETER Service
Compose service whose image is inspected.
#>
function Get-ImageMetadata {
    param([Parameter(Mandatory)][string]$Service)
    $imageName = "simdlib/${Service}:local"
    $raw = & docker image inspect $imageName
    if ($LASTEXITCODE -ne 0) {
        throw "Unable to inspect required image $imageName. Build it first."
    }
    $inspection = ($raw | ConvertFrom-Json)[0]
    $stableLabels = [ordered]@{}
    foreach ($property in @($inspection.Config.Labels.PSObject.Properties | Sort-Object Name)) {
        if ($property.Name -notlike 'com.docker.compose.*') {
            $stableLabels[$property.Name] = $property.Value
        }
    }
    $contentDocument = [ordered]@{
        architecture = $inspection.Architecture
        os = $inspection.Os
        layers = @($inspection.RootFS.Layers)
        config = [ordered]@{
            user = $inspection.Config.User
            environment = @($inspection.Config.Env)
            entrypoint = @($inspection.Config.Entrypoint)
            command = @($inspection.Config.Cmd)
            workingDirectory = $inspection.Config.WorkingDir
            labels = $stableLabels
        }
    }
    $contentJson = $contentDocument | ConvertTo-Json -Depth 8 -Compress
    $contentBytes = $utf8NoBom.GetBytes($contentJson)
    $contentIdentity = [Convert]::ToHexString(
        [System.Security.Cryptography.SHA256]::HashData($contentBytes)).ToLowerInvariant()
    [pscustomobject]@{
        Name = $imageName
        Id = $inspection.Id
        ContentIdentity = "sha256:$contentIdentity"
        BaseDigest = $inspection.Config.Labels.'org.simdlib.base.digest'
        ToolchainVersion = $inspection.Config.Labels.'org.opencontainers.image.version'
        CMakeSha256 = $inspection.Config.Labels.'org.simdlib.cmake.sha256'
        Catch2Commit = $inspection.Config.Labels.'org.simdlib.catch2.commit'
    }
}

<#
.SYNOPSIS
Creates the canonical fingerprint document for one build cell.
.PARAMETER BuildCell
Compiler/configuration cell being identified.
.PARAMETER Image
Immutable local image metadata.
#>
function New-FingerprintDocument {
    param([Parameter(Mandatory)]$BuildCell, [Parameter(Mandatory)]$Image)
    $requiredFlags = if ($BuildCell.Service -eq 'clang22') {
        [ordered]@{ cxx = '-stdlib=libc++'; linker = '-fuse-ld=lld --rtlib=compiler-rt --unwindlib=libunwind' }
    } else {
        [ordered]@{ cxx = ''; linker = '' }
    }
    [ordered]@{
        schema = 'simdlib.build-cell-fingerprint.v1'
        platform = 'linux-x64'
        compiler = $BuildCell.Service
        image = [ordered]@{ identity = $Image.ContentIdentity; name = $Image.Name; baseDigest = $Image.BaseDigest; toolchainVersion = $Image.ToolchainVersion }
        configuration = [ordered]@{
            key = $BuildCell.Key
            preset = $BuildCell.Preset
            buildProfile = $BuildCell.BuildProfile
            sanitizer = $BuildCell.Sanitizer
            codegenMode = $BuildCell.CodegenMode
            generator = 'Ninja'
            cxxStandard = 20
            cxxFlags = $requiredFlags.cxx
            linkerFlags = $requiredFlags.linker
        }
        dependencies = [ordered]@{ cmakeVersion = '4.4.0'; cmakeSha256 = $Image.CMakeSha256; catch2Commit = $Image.Catch2Commit }
        requiredCpuFeatures = @('sse4_2', 'avx2', 'fma', 'bmi1', 'bmi2')
    }
}

<#
.SYNOPSIS
Materializes and returns the fingerprinted artifact location for one cell.
.PARAMETER BuildCell
Compiler/configuration cell being located.
.PARAMETER Image
Immutable local image metadata.
#>
function Initialize-CellArtifact {
    param([Parameter(Mandatory)]$BuildCell, [Parameter(Mandatory)]$Image)
    $fingerprint = New-FingerprintDocument -BuildCell $BuildCell -Image $Image
    $json = $fingerprint | ConvertTo-Json -Depth 8 -Compress
    $bytes = $utf8NoBom.GetBytes($json)
    $digest = [Convert]::ToHexString([System.Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
    $compilerDirectoryName = "linux-$($BuildCell.Service)"
    $cellDirectoryName = "$($BuildCell.Key)-$($digest.Substring(0, 16))"
    $hostRoot = Join-Path $pipelineRoot "$compilerDirectoryName/$cellDirectoryName"
    $provenanceDirectory = Join-Path $hostRoot 'provenance'
    New-Item -ItemType Directory -Path $provenanceDirectory -Force | Out-Null
    [System.IO.File]::WriteAllText((Join-Path $provenanceDirectory 'fingerprint.json'), $json, $utf8NoBom)
    [pscustomobject]@{
        Service = $BuildCell.Service
        Key = $BuildCell.Key
        Id = "$($BuildCell.Service)-$($BuildCell.Key)"
        Preset = $BuildCell.Preset
        BuildProfile = $BuildCell.BuildProfile
        Sanitizer = $BuildCell.Sanitizer
        CodegenMode = $BuildCell.CodegenMode
        Fingerprint = $digest
        HostRoot = $hostRoot
        ContainerRoot = "/workspace/out/$compilerDirectoryName/$cellDirectoryName"
    }
}

<#
.SYNOPSIS
Starts one isolated Compose operation with independent output logs.
.PARAMETER CellArtifact
Resolved fingerprinted cell artifact.
.PARAMETER Operation
Entrypoint operation to execute.
.PARAMETER ProjectName
Unique Compose project for this invocation.
.PARAMETER LogDirectory
Invocation-owned log directory.
.PARAMETER FailIntentionally
Whether this operation is an aggregate-failure probe.
#>
function Start-CellOperation {
    param(
        [Parameter(Mandatory)]$CellArtifact,
        [Parameter(Mandatory)][string]$Operation,
        [Parameter(Mandatory)][string]$ProjectName,
        [Parameter(Mandatory)][string]$LogDirectory,
        [Parameter(Mandatory)][bool]$FailIntentionally
    )
    $arguments = [System.Collections.Generic.List[string]]::new()
    foreach ($argument in @('compose', '--file', $composeFile, '--project-name', $ProjectName, '--profile', 'compilers', 'run', '--rm', '--no-deps')) {
        $arguments.Add($argument)
    }
    if ($FailIntentionally) {
        foreach ($argument in @('--entrypoint', '/bin/sh', $CellArtifact.Service, '-c', 'echo SIMDLIB_INTENTIONAL_MATRIX_FAILURE >&2; exit 23')) {
            $arguments.Add($argument)
        }
    } else {
        $arguments.Add($CellArtifact.Service)
        foreach ($argument in @(
                '--operation', $Operation,
                '--preset', $CellArtifact.Preset,
                '--build-profile', $CellArtifact.BuildProfile,
                '--sanitizer', $CellArtifact.Sanitizer,
                '--codegen-mode', $CellArtifact.CodegenMode,
                '--artifact-root', $CellArtifact.ContainerRoot,
                '--fingerprint-sha256', $CellArtifact.Fingerprint
            )) {
            $arguments.Add($argument)
        }
        if ($Operation -eq 'test' -and $TestRegex) {
            $arguments.Add('--test-regex'); $arguments.Add($TestRegex)
        }
        if ($Operation -eq 'test' -and $TestLabel) {
            $arguments.Add('--test-label'); $arguments.Add($TestLabel)
        }
    }
    $processInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $processInfo.FileName = 'docker'
    $processInfo.UseShellExecute = $false
    $processInfo.RedirectStandardOutput = $true
    $processInfo.RedirectStandardError = $true
    foreach ($argument in $arguments) { $processInfo.ArgumentList.Add($argument) }
    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $processInfo
    if (-not $process.Start()) { throw "Failed to start $($CellArtifact.Id) operation $Operation" }
    [pscustomobject]@{
        Cell = $CellArtifact
        Operation = $Operation
        Process = $process
        StandardOutput = $process.StandardOutput.ReadToEndAsync()
        StandardError = $process.StandardError.ReadToEndAsync()
        StandardOutputPath = Join-Path $LogDirectory "$($CellArtifact.Id).$Operation.stdout.log"
        StandardErrorPath = Join-Path $LogDirectory "$($CellArtifact.Id).$Operation.stderr.log"
        Captured = $false
    }
}

<#
.SYNOPSIS
Completes one child process, writes its logs, and returns its exit code.
.PARAMETER Run
Running cell operation to complete.
#>
function Complete-CellOperation {
    param([Parameter(Mandatory)]$Run)
    if (-not $Run.Process.HasExited) { $Run.Process.WaitForExit() }
    if (-not $Run.Captured) {
        [System.IO.File]::WriteAllText($Run.StandardOutputPath, $Run.StandardOutput.GetAwaiter().GetResult(), $utf8NoBom)
        [System.IO.File]::WriteAllText($Run.StandardErrorPath, $Run.StandardError.GetAwaiter().GetResult(), $utf8NoBom)
        $Run.Captured = $true
    }
    return $Run.Process.ExitCode
}

<#
.SYNOPSIS
Runs cell operations with bounded concurrency and aggregate failure reporting.
.PARAMETER CellArtifacts
Resolved cells to execute.
.PARAMETER Operation
Entrypoint operation shared by the cells.
.PARAMETER ProjectName
Unique Compose project for this invocation.
.PARAMETER LogDirectory
Invocation-owned log directory.
#>
function Invoke-CellOperations {
    param(
        [Parameter(Mandatory)][object[]]$CellArtifacts,
        [Parameter(Mandatory)][string]$Operation,
        [Parameter(Mandatory)][string]$ProjectName,
        [Parameter(Mandatory)][string]$LogDirectory
    )
    $pending = [System.Collections.Generic.Queue[object]]::new()
    foreach ($cellArtifact in $CellArtifacts) { $pending.Enqueue($cellArtifact) }
    $running = [System.Collections.Generic.List[object]]::new()
    $allRuns = [System.Collections.Generic.List[object]]::new()
    $failed = [System.Collections.Generic.List[string]]::new()
    $deadline = if ($CancelAfterSeconds -gt 0) { (Get-Date).AddSeconds($CancelAfterSeconds) } else { $null }
    $cancelled = $false
    try {
        while ($pending.Count -gt 0 -or $running.Count -gt 0) {
            while ($pending.Count -gt 0 -and $running.Count -lt $MaxParallel) {
                $cellArtifact = $pending.Dequeue()
                $fail = $InjectFailure -contains 'All' -or $InjectFailure -contains $cellArtifact.Id
                $run = Start-CellOperation -CellArtifact $cellArtifact -Operation $Operation -ProjectName $ProjectName -LogDirectory $LogDirectory -FailIntentionally $fail
                $running.Add($run); $allRuns.Add($run)
                Write-Host "Started $($cellArtifact.Id) operation=$Operation"
            }
            if ($deadline -and (Get-Date) -ge $deadline) { $cancelled = $true; break }
            $completed = @($running | Where-Object { $_.Process.HasExited })
            if ($completed.Count -eq 0) { Start-Sleep -Milliseconds 100; continue }
            foreach ($run in $completed) {
                $exitCode = Complete-CellOperation -Run $run
                [void]$running.Remove($run)
                Write-Host "$($run.Cell.Id): operation=$Operation exit=$exitCode logs=$LogDirectory"
                if ($exitCode -ne 0) { $failed.Add($run.Cell.Id) }
            }
        }
    } finally {
        if ($cancelled) {
            foreach ($run in $running) {
                try { $run.Process.Kill($true); $run.Process.WaitForExit() }
                catch { Write-Warning "Process cancellation failed for $($run.Cell.Id): $_" }
            }
        }
        foreach ($run in $allRuns) {
            try { [void](Complete-CellOperation -Run $run) }
            catch { Write-Warning "Log capture failed for $($run.Cell.Id): $_" }
            $run.Process.Dispose()
        }
    }
    if ($cancelled) {
        throw [System.OperationCanceledException]::new("Container operation cancelled after $CancelAfterSeconds seconds. Logs: $LogDirectory")
    }
    if ($failed.Count -ne 0) {
        throw "Container operation $Operation failed: $($failed -join ', '). Logs: $LogDirectory"
    }
}

<#
.SYNOPSIS
Removes selected pipeline artifacts, images, and abandoned Compose resources.
.PARAMETER Services
Compiler services selected for cleanup.
#>
function Remove-PipelineState {
    param([Parameter(Mandatory)][string[]]$Services)
    $resolvedPipelineRoot = [System.IO.Path]::GetFullPath($pipelineRoot)
    $resolvedRepositoryRoot = [System.IO.Path]::GetFullPath($repositoryRoot)
    if (-not $resolvedPipelineRoot.StartsWith($resolvedRepositoryRoot + [System.IO.Path]::DirectorySeparatorChar, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean outside the repository: $resolvedPipelineRoot"
    }
    $containerIds = @(& docker ps --all --quiet --filter 'name=simdlib-container-')
    if ($LASTEXITCODE -ne 0) { throw 'Unable to enumerate SimdLib containers for cleanup.' }
    if ($containerIds.Count -ne 0) { Invoke-DockerChecked (@('container', 'rm', '--force') + $containerIds) }
    $networkIds = @(& docker network ls --quiet --filter 'name=simdlib-container-')
    if ($LASTEXITCODE -ne 0) { throw 'Unable to enumerate SimdLib networks for cleanup.' }
    if ($networkIds.Count -ne 0) { Invoke-DockerChecked (@('network', 'rm') + $networkIds) }
    foreach ($service in $Services) {
        $compilerRoot = [System.IO.Path]::GetFullPath((Join-Path $pipelineRoot "linux-$service"))
        if (-not $compilerRoot.StartsWith($resolvedPipelineRoot + [System.IO.Path]::DirectorySeparatorChar, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to clean unexpected compiler artifacts: $compilerRoot"
        }
        if (Test-Path -LiteralPath $compilerRoot) { Remove-Item -LiteralPath $compilerRoot -Recurse -Force }
        $image = "simdlib/${service}:local"
        & docker image inspect $image *> $null
        if ($LASTEXITCODE -eq 0) { Invoke-DockerChecked @('image', 'rm', $image) }
    }
    if ($Compiler -eq 'All') {
        $logsRoot = Join-Path $pipelineRoot 'logs'
        if (Test-Path -LiteralPath $logsRoot) { Remove-Item -LiteralPath $logsRoot -Recurse -Force }
    }
    Write-Host 'Removed selected pipeline artifacts, images, containers, and networks.'
}

$services = @(Resolve-Services -CompilerName $Compiler)
if ($Action -eq 'Clean') {
    Remove-PipelineState -Services $services
    exit 0
}
if ($NoImageCache -and ($SkipImageBuild -or $Action -notin @('Build', 'RecordCodegen', 'InspectEnvironment'))) {
    throw '-NoImageCache is only valid when Build, RecordCodegen, or InspectEnvironment owns the image build.'
}
if ($SkipImageBuild -and $Action -notin @('Build', 'RecordCodegen', 'InspectEnvironment')) {
    throw '-SkipImageBuild is only valid for Build, RecordCodegen, or InspectEnvironment.'
}
if (($TestRegex -or $TestLabel) -and $Action -ne 'Test') {
    throw '-TestRegex and -TestLabel are optional Test-only diagnostics.'
}
if ($Cell -eq 'AsanUbsan' -and 'clang22' -notin $services) {
    throw 'The ASan+UBSan cell is owned by Clang 22.'
}
if ($Action -eq 'RecordCodegen') {
    if ($Cell -notin @('All', 'Debug', 'AsanUbsan')) {
        throw 'Container codegen diagnostics use Debug or AsanUbsan cells only.'
    }
    if ($Compiler -eq 'Gcc13') {
        throw 'GCC 13 is core-only and owns no Register codegen diagnostic.'
    }
}

$selectedCellScope = if ($Action -eq 'InspectEnvironment') {
    if ($Cell -notin @('All', 'Release')) { throw 'Environment inspection is compiler-scoped and uses one Release identity per compiler.' }
    'Release'
} elseif ($Action -in @('BuildBenchmarks', 'RunBenchmarks')) {
    if ($Cell -notin @('All', 'Release')) { throw 'Benchmark operations only use Release cells.' }
    'Release'
} else {
    $Cell
}
$cells = @(Resolve-Cells -Services $services -CellScope $selectedCellScope -Operation $Action)
if ($cells.Count -eq 0) { throw 'The compiler and cell selections do not identify any operation cells.' }

$runId = "{0}-{1}-{2}" -f (Get-Date -Format 'yyyyMMdd-HHmmssfff'), $Action.ToLowerInvariant(), $PID
$projectName = "simdlib-container-$runId".ToLowerInvariant()
$imageBuildProjectName = 'simdlib-container-images'
$logDirectory = Join-Path $pipelineRoot "logs/$runId"
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
Write-Host "Container operation: action=$Action cells=$($cells.Count) maxParallel=$MaxParallel"

if ($Action -in @('Build', 'RecordCodegen', 'InspectEnvironment') -and -not $SkipImageBuild) {
    $buildArguments = @(
        'compose', '--file', $composeFile, '--project-name', $imageBuildProjectName,
        '--profile', 'compilers', 'build', '--provenance=false'
    )
    if ($NoImageCache) { $buildArguments += '--no-cache' }
    $buildArguments += $services
    Invoke-DockerChecked $buildArguments
}

$imageMetadata = @{}
foreach ($service in $services) {
    $imageMetadata[$service] = Get-ImageMetadata -Service $service
    Write-Host "$service image: config=$($imageMetadata[$service].Id) content=$($imageMetadata[$service].ContentIdentity)"
}
$cellArtifacts = @(
    foreach ($cellDefinition in $cells) {
        Initialize-CellArtifact -BuildCell $cellDefinition -Image $imageMetadata[$cellDefinition.Service]
    }
)
$operation = switch ($Action) {
    'Build' { 'build-validation' }
    'Test' { 'test' }
    'RecordCodegen' { 'record-codegen' }
    'BuildBenchmarks' { 'build-benchmarks' }
    'RunBenchmarks' { 'run-benchmarks' }
    'InspectEnvironment' { 'inspect-environment' }
}
try {
    Invoke-CellOperations -CellArtifacts $cellArtifacts -Operation $operation -ProjectName $projectName -LogDirectory $logDirectory
} finally {
    & docker compose --file $composeFile --project-name $projectName --profile compilers down --remove-orphans 2>$null | Out-Null
    if ($LASTEXITCODE -ne 0) { Write-Warning "Compose cleanup failed for project $projectName." }
}
Write-Host "Container operation passed. Logs: $logDirectory"
