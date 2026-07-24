[CmdletBinding()]
param(
    [ValidateSet('Focused', 'Full', 'Feature', 'Sanitizer', 'Codegen', 'Debug', 'Benchmark')]
    [string]$Mode = 'Full',

    [ValidateSet('All', 'Gcc14', 'Clang22')]
    [string]$Compiler = 'All',

    [switch]$NoBuild,
    [switch]$NoCache,
    [switch]$DoctorOnly,

    [ValidateSet('None', 'Gcc14', 'Clang22', 'All')]
    [string]$InjectFailure = 'None',

    [ValidateRange(0, 86400)]
    [int]$CancelAfterSeconds = 0,

    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$composeFile = Join-Path $repositoryRoot 'compose.yml'
$artifactRoot = Join-Path $repositoryRoot 'out/container'

if (-not $env:SIMDLIB_BUILD_REVISION) {
    $env:SIMDLIB_BUILD_REVISION = (& git -C $repositoryRoot rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to determine the SimdLib revision for image provenance.'
    }
}

if ($IsLinux -or $IsMacOS) {
    $env:SIMDLIB_HOST_UID = (& id -u).Trim()
    $env:SIMDLIB_HOST_GID = (& id -g).Trim()
}

<#
.SYNOPSIS
Invokes Docker and fails immediately when the command cannot be started or
returns a nonzero exit code.
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
Starts one Compose service with redirected output so matrix services can run
concurrently while retaining independent logs.
#>
function Start-MatrixService {
    param(
        [Parameter(Mandatory)][string]$Service,
        [Parameter(Mandatory)][string]$Profile,
        [Parameter(Mandatory)][string]$ProjectName,
        [Parameter(Mandatory)][string[]]$ContainerArguments,
        [Parameter(Mandatory)][string]$LogDirectory,
        [Parameter(Mandatory)][bool]$FailIntentionally
    )

    $arguments = [System.Collections.Generic.List[string]]::new()
    foreach ($argument in @('compose', '--file', $composeFile, '--project-name', $ProjectName, '--profile', $Profile, 'run', '--rm', '--no-deps')) {
        $arguments.Add($argument)
    }
    if ($FailIntentionally) {
        foreach ($argument in @('--entrypoint', '/bin/sh', $Service, '-c', 'echo SIMDLIB_INTENTIONAL_MATRIX_FAILURE >&2; exit 23')) {
            $arguments.Add($argument)
        }
    }
    else {
        $arguments.Add($Service)
        foreach ($argument in $ContainerArguments) {
            $arguments.Add($argument)
        }
    }

    $processInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $processInfo.FileName = 'docker'
    $processInfo.UseShellExecute = $false
    $processInfo.RedirectStandardOutput = $true
    $processInfo.RedirectStandardError = $true
    foreach ($argument in $arguments) {
        $processInfo.ArgumentList.Add($argument)
    }

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $processInfo
    if (-not $process.Start()) {
        throw "Failed to start Compose service $Service"
    }

    [pscustomobject]@{
        Service = $Service
        Process = $process
        StandardOutput = $process.StandardOutput.ReadToEndAsync()
        StandardError = $process.StandardError.ReadToEndAsync()
        StandardOutputPath = Join-Path $LogDirectory "$Service.stdout.log"
        StandardErrorPath = Join-Path $LogDirectory "$Service.stderr.log"
    }
}

if ($Clean) {
    $resolvedArtifactRoot = [System.IO.Path]::GetFullPath($artifactRoot)
    $resolvedRepositoryRoot = [System.IO.Path]::GetFullPath($repositoryRoot)
    if (-not $resolvedArtifactRoot.StartsWith($resolvedRepositoryRoot + [System.IO.Path]::DirectorySeparatorChar)) {
        throw "Refusing to clean an artifact directory outside the repository: $resolvedArtifactRoot"
    }
    $containerIds = @(& docker ps --all --quiet --filter 'name=simdlib-register-')
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to enumerate SimdLib containers for cleanup.'
    }
    if ($containerIds.Count -ne 0) {
        Invoke-DockerChecked (@('container', 'rm', '--force') + $containerIds)
    }
    $networkIds = @(& docker network ls --quiet --filter 'name=simdlib-register-')
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to enumerate SimdLib networks for cleanup.'
    }
    if ($networkIds.Count -ne 0) {
        Invoke-DockerChecked (@('network', 'rm') + $networkIds)
    }
    foreach ($image in @('simdlib/gcc14:local', 'simdlib/clang22:local')) {
        & docker image inspect $image 2>$null | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Invoke-DockerChecked @('image', 'rm', $image)
        }
    }
    if (Test-Path -LiteralPath $resolvedArtifactRoot) {
        Remove-Item -LiteralPath $resolvedArtifactRoot -Recurse -Force
    }
    Write-Host "Removed SimdLib Compose containers, local images, and $resolvedArtifactRoot"
    exit 0
}

$services = switch ($Compiler) {
    'Gcc14' { @('gcc14') }
    'Clang22' { @('clang22') }
    default { @('gcc14', 'clang22') }
}
if ($Mode -eq 'Sanitizer') {
    if ($Compiler -eq 'Gcc14') {
        throw 'The sanitizer profile is owned by Clang 22; GCC 14 cannot be selected.'
    }
    $services = @('clang22')
}

$profile = $Mode.ToLowerInvariant()
$preset = switch ($Mode) {
	'Focused' { 'container-focused' }
	'Sanitizer' { 'container-sanitize' }
	'Codegen' { 'container-codegen' }
	'Debug' { 'container-debug' }
	'Benchmark' { 'container-benchmark' }
	default { 'container-full' }
}
$configuration = if ($Mode -in @('Sanitizer', 'Debug')) { 'Debug' } else { 'Release' }
$sanitizer = if ($Mode -eq 'Sanitizer') { 'address-undefined' } else { 'none' }
$testLabel = if ($Mode -eq 'Feature') { 'AVX2|FMA|BMI|SCALAR' } else { $null }
$runId = "{0}-{1}-{2}" -f (Get-Date -Format 'yyyyMMdd-HHmmssfff'), $profile, $PID
$projectName = "simdlib-register-$runId".ToLowerInvariant()

Write-Host "Container matrix: mode=$Mode services=$($services -join ',') preset=$preset"

if (-not $NoBuild) {
    $buildArguments = @('compose', '--file', $composeFile, '--project-name', $projectName, '--profile', $profile, 'build')
    if ($NoCache) {
        $buildArguments += '--no-cache'
    }
    $buildArguments += $services
    Invoke-DockerChecked $buildArguments

    foreach ($service in $services) {
        $imageName = "simdlib/${service}:local"
        $imageIdentity = (& docker image inspect --format '{{.Id}} size={{.Size}}' $imageName).Trim()
        if ($LASTEXITCODE -ne 0) {
            throw "Unable to inspect rebuilt image $imageName."
        }
        Write-Host "$service image: $imageIdentity"
    }
}

$logDirectory = Join-Path $artifactRoot "logs/$runId"
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null

$runs = @()
$cancelled = $false
try {
    $createArguments = @(
        'compose', '--file', $composeFile, '--project-name', $projectName,
        '--profile', $profile, 'create', '--no-build'
    ) + $services
    Invoke-DockerChecked $createArguments

    foreach ($service in $services) {
        $containerOutput = "/workspace/out/$service/$profile"
        $containerArguments = @(
            '--preset', $preset,
            '--configuration', $configuration,
            '--sanitizer', $sanitizer,
            '--output-dir', $containerOutput
        )
        if ($DoctorOnly) {
            $containerArguments += '--doctor-only'
        }
        if ($Mode -eq 'Benchmark') {
            $containerArguments += '--run-benchmarks'
        }
        if ($testLabel) {
            $containerArguments += @('--test-label', $testLabel)
        }
        $failIntentionally = $InjectFailure -eq 'All' -or $InjectFailure.ToLowerInvariant() -eq $service
        $runs += Start-MatrixService -Service $service -Profile $profile -ProjectName $projectName -ContainerArguments $containerArguments -LogDirectory $logDirectory -FailIntentionally $failIntentionally
        Write-Host "Started $service"
    }

    $cancellationDeadline = if ($CancelAfterSeconds -gt 0) {
        (Get-Date).AddSeconds($CancelAfterSeconds)
    }
    else {
        $null
    }
    while ($runs.Process.HasExited -contains $false) {
        if ($cancellationDeadline -and (Get-Date) -ge $cancellationDeadline) {
            $cancelled = $true
            break
        }
        Start-Sleep -Milliseconds 200
    }

    if (-not $cancelled) {
        $failedServices = @()
        foreach ($run in $runs) {
            $standardOutput = $run.StandardOutput.GetAwaiter().GetResult()
            $standardError = $run.StandardError.GetAwaiter().GetResult()
            [System.IO.File]::WriteAllText($run.StandardOutputPath, $standardOutput)
            [System.IO.File]::WriteAllText($run.StandardErrorPath, $standardError)
            if ($run.Process.ExitCode -ne 0) {
                $failedServices += $run.Service
            }
            Write-Host "$($run.Service): exit=$($run.Process.ExitCode) logs=$logDirectory"
        }

        if ($failedServices.Count -ne 0) {
            throw "Container matrix failed: $($failedServices -join ', ')"
        }
    }
}
finally {
    & docker compose --file $composeFile --project-name $projectName --profile $profile down --remove-orphans 2>$null | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Compose cleanup failed for project $projectName."
    }
    foreach ($run in $runs) {
        try {
            if (-not $run.Process.WaitForExit(5000)) {
                $run.Process.Kill($true)
                $run.Process.WaitForExit()
            }
        }
        catch {
            Write-Warning "Process cleanup failed for $($run.Service): $_"
        }
        try {
            if (-not (Test-Path -LiteralPath $run.StandardOutputPath)) {
                [System.IO.File]::WriteAllText(
                    $run.StandardOutputPath,
                    $run.StandardOutput.GetAwaiter().GetResult())
            }
            if (-not (Test-Path -LiteralPath $run.StandardErrorPath)) {
                [System.IO.File]::WriteAllText(
                    $run.StandardErrorPath,
                    $run.StandardError.GetAwaiter().GetResult())
            }
        }
        catch {
            Write-Warning "Log capture failed for $($run.Service): $_"
        }
        $run.Process.Dispose()
    }
}

if ($cancelled) {
    throw [System.OperationCanceledException]::new(
        "Container matrix cancellation probe fired after $CancelAfterSeconds seconds. Logs: $logDirectory")
}

Write-Host "Container matrix passed. Logs: $logDirectory"
