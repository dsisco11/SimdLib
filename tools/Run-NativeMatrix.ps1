<#
.SYNOPSIS
Builds or consumes fingerprinted native compiler cells.
.DESCRIPTION
Build creates validation artifacts and manifests. Test validates those manifests
and runs CTest without configuring or building. Benchmark operations reuse only
the existing Release trees. Coverage is an independent Clang Debug cell.
#>
[CmdletBinding()]
param(
    [ValidateSet('Build', 'Test', 'BuildBenchmarks', 'RunBenchmarks')]
    [string]$Action = 'Build',
    [ValidateSet('All', 'Release', 'Debug', 'Coverage')]
    [string]$Cell = 'All',
    [ValidateSet('All', 'Msvc', 'ClangCl', 'ClangCoverage')]
    [string]$Compiler = 'All',
    [string]$TestRegex = '',
    [string]$TestLabel = '',
    [string[]]$InjectFailure = @('None')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Pipeline.Common.psm1') -Force

$repositoryRoot = Get-PipelineRepositoryRoot
$pipelineRoot = Join-Path $repositoryRoot 'out/pipeline'
$cmake = (Get-Command cmake -ErrorAction Stop).Source
$ctest = (Get-Command ctest -ErrorAction Stop).Source
$visualStudio = Initialize-PipelineVisualStudioEnvironment
$ninja = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'

<#
.SYNOPSIS
Returns native cells selected by compiler, configuration, and operation.
.PARAMETER CompilerName
Requested native compiler.
.PARAMETER CellScope
Requested configuration scope.
.PARAMETER Operation
Requested pipeline operation.
#>
function Resolve-NativeCells {
    param(
        [Parameter(Mandatory)][string]$CompilerName,
        [Parameter(Mandatory)][string]$CellScope,
        [Parameter(Mandatory)][string]$Operation
    )
    $compilers = switch ($CompilerName) {
        'Msvc' { @('msvc') }
        'ClangCl' { @('clangcl') }
        'ClangCoverage' { @('clang-coverage') }
        default { @('msvc', 'clangcl', 'clang-coverage') }
    }
    $cells = [System.Collections.Generic.List[object]]::new()
    foreach ($compilerKey in $compilers) {
        if ($compilerKey -eq 'clang-coverage') {
            if ($Operation -notin @('BuildBenchmarks', 'RunBenchmarks') -and $CellScope -in @('All', 'Coverage')) {
                $cells.Add([pscustomobject]@{
                        Compiler = $compilerKey; Key = 'debug-coverage'; Preset = 'clang-debug-coverage'
                        BuildProfile = 'Debug'; Generator = 'Ninja'; Consumer = $false; Coverage = $true
                    })
            }
            continue
        }
        if ($CellScope -in @('All', 'Release')) {
            $presetPrefix = if ($compilerKey -eq 'msvc') { 'msvc' } else { 'clangcl' }
            $cells.Add([pscustomobject]@{
                    Compiler = $compilerKey; Key = 'release'; Preset = "$presetPrefix-release-exhaustive"
                    BuildProfile = 'Release'; Generator = if ($compilerKey -eq 'msvc') { 'Visual Studio 17 2022' } else { 'Ninja' }
                    Consumer = $true; Coverage = $false
                })
        }
        if ($Operation -notin @('BuildBenchmarks', 'RunBenchmarks') -and $CellScope -in @('All', 'Debug')) {
            $presetPrefix = if ($compilerKey -eq 'msvc') { 'msvc' } else { 'clangcl' }
            $cells.Add([pscustomobject]@{
                    Compiler = $compilerKey; Key = 'debug'; Preset = "$presetPrefix-debug-diagnostics"
                    BuildProfile = 'Debug'; Generator = if ($compilerKey -eq 'msvc') { 'Visual Studio 17 2022' } else { 'Ninja' }
                    Consumer = $true; Coverage = $false
                })
        }
    }
    return $cells.ToArray()
}

<#
.SYNOPSIS
Returns immutable compiler identity for one cell.
.PARAMETER BuildCell
Native cell definition.
#>
function Get-NativeCompilerIdentity {
    param([Parameter(Mandatory)]$BuildCell)
    $commandName = if ($BuildCell.Compiler -eq 'msvc') { 'cl.exe' } elseif ($BuildCell.Compiler -eq 'clangcl') { 'clang-cl.exe' } else { 'clang++.exe' }
    $command = Get-Command $commandName -ErrorAction Stop
    $version = if ($BuildCell.Compiler -eq 'msvc') {
        $command.FileVersionInfo.ProductVersion
    } else {
        (& $command.Source --version | Select-Object -First 1).Trim()
    }
    return [ordered]@{ id = $BuildCell.Compiler; path = $command.Source; version = $version }
}

<#
.SYNOPSIS
Creates and validates one native fingerprint artifact location.
.PARAMETER BuildCell
Native cell definition.
#>
function Initialize-NativeArtifact {
    param([Parameter(Mandatory)]$BuildCell)
    $compilerIdentity = Get-NativeCompilerIdentity -BuildCell $BuildCell
    $fingerprint = [ordered]@{
        schema = 'simdlib.build-cell-fingerprint.v1'
        platform = 'windows-x64'
        compiler = $compilerIdentity
        configuration = [ordered]@{
            key = $BuildCell.Key; preset = $BuildCell.Preset; buildProfile = $BuildCell.BuildProfile
            sanitizer = 'none'; coverage = $BuildCell.Coverage; generator = $BuildCell.Generator
            cxxStandard = '20-and-23-register'
        }
        dependencies = [ordered]@{
            cmakeVersion = (& $cmake --version | Select-Object -First 1).Trim()
            ninjaPath = if ($BuildCell.Generator -eq 'Ninja') { $ninja } else { '' }
            visualStudio = $visualStudio
            catch2Commit = '2b60af89e23d28eefc081bc930831ee9d45ea58b'
        }
        requiredCpuFeatures = @('sse4.2', 'avx2', 'fma', 'bmi1', 'bmi2')
    }
    $json = $fingerprint | ConvertTo-Json -Depth 8 -Compress
    $digest = Get-PipelineTextDigest -Text $json
    $root = Join-Path $pipelineRoot "windows-$($BuildCell.Compiler)/$($BuildCell.Key)-$($digest.Substring(0, 16))"
    $provenance = Join-Path $root 'provenance'
    $fingerprintPath = Join-Path $provenance 'fingerprint.json'
    New-Item -ItemType Directory -Path $provenance -Force | Out-Null
    if (Test-Path -LiteralPath $fingerprintPath) {
        $existing = Get-Content -LiteralPath $fingerprintPath -Raw
        if ($existing -ne $json) { throw "Fingerprint collision at $root" }
    } else {
        Set-PipelineTextFile -Path $fingerprintPath -Content $json
    }
    return [pscustomobject]@{
        Id = "$($BuildCell.Compiler)-$($BuildCell.Key)"; Definition = $BuildCell; Fingerprint = $digest
        Root = $root; Build = Join-Path $root 'build'; Consumer = Join-Path $root 'consumer'
        Reports = Join-Path $root 'reports'; Provenance = $provenance; FingerprintPath = $fingerprintPath
        CompilerIdentity = $compilerIdentity
    }
}

<#
.SYNOPSIS
Returns whether any supported CI indicator is nonempty.
#>
function Test-CiEnvironment {
    foreach ($name in @('CI', 'GITHUB_ACTIONS', 'GITLAB_CI', 'TF_BUILD', 'BUILDKITE', 'CIRCLECI', 'JENKINS_URL', 'TEAMCITY_VERSION')) {
        if ([Environment]::GetEnvironmentVariable($name)) { return $true }
    }
    return $false
}

<#
.SYNOPSIS
Runs the CMake test-inventory recorder or validator.
.PARAMETER Mode
RECORD or VALIDATE.
.PARAMETER TestDirectory
CTest tree.
.PARAMETER InventoryPath
Owned inventory file.
.PARAMETER Configuration
Optional multi-config configuration.
#>
function Invoke-TestInventory {
    param(
        [Parameter(Mandatory)][ValidateSet('RECORD', 'VALIDATE')][string]$Mode,
        [Parameter(Mandatory)][string]$TestDirectory,
        [Parameter(Mandatory)][string]$InventoryPath,
        [string]$Configuration = ''
    )
    $arguments = @("-DMODE=$Mode", "-DTEST_DIRECTORY=$TestDirectory", "-DINVENTORY_FILE=$InventoryPath", "-DCMAKE_CTEST_COMMAND=$ctest")
    if ($Configuration) { $arguments += "-DCONFIGURATION=$Configuration" }
    $arguments += @('-P', (Join-Path $repositoryRoot 'cmake/RecordTestInventory.cmake'))
    & $cmake @arguments
    if ($LASTEXITCODE -ne 0) { throw "CTest inventory $Mode failed for $TestDirectory" }
}

<#
.SYNOPSIS
Returns a file hash or the manifest marker for an absent optional file.
.PARAMETER Path
File to hash.
#>
function Get-OptionalFileHash {
    param([Parameter(Mandatory)][string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return 'none' }
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

<#
.SYNOPSIS
Writes an atomic completed-operation manifest for one native cell.
.PARAMETER Artifact
Resolved cell artifact.
.PARAMETER Operation
Completed operation identity.
#>
function Write-NativeManifest {
    param([Parameter(Mandatory)]$Artifact, [Parameter(Mandatory)][string]$Operation)
    $mainInventory = Join-Path $Artifact.Provenance 'main-test-artifacts.inventory'
    $consumerInventory = Join-Path $Artifact.Provenance 'consumer-test-artifacts.inventory'
    $codegenIndex = Join-Path $Artifact.Provenance 'codegen-records.index'
    $mainMetadata = Join-Path $Artifact.Build 'CTestTestfile.cmake'
    $consumerMetadata = Join-Path $Artifact.Consumer 'CTestTestfile.cmake'
    $manifestName = if ($Operation -eq 'build-benchmarks') { 'benchmark-build.manifest' } else { 'validation-build.manifest' }
    $manifestPath = Join-Path $Artifact.Provenance $manifestName
    $lines = @(
        'schema=simdlib.build-manifest.v1', "operation=$Operation", 'status=complete',
        "source_revision=$(Get-PipelineRevision -RepositoryRoot $repositoryRoot)",
        "source_digest=$(Get-PipelineSourceDigest -RepositoryRoot $repositoryRoot)",
        "fingerprint_sha256=$($Artifact.Fingerprint)", "fingerprint_document=$($Artifact.FingerprintPath)",
        "compiler_id=$($Artifact.Definition.Compiler)", "compiler=$($Artifact.CompilerIdentity.version)", 'base_image=none',
        "preset=$($Artifact.Definition.Preset)", "build_profile=$($Artifact.Definition.BuildProfile)", 'sanitizer=none',
        "build_directory=$($Artifact.Build)", "consumer_directory=$($Artifact.Consumer)",
        "cmake_cache_sha256=$(Get-OptionalFileHash -Path (Join-Path $Artifact.Build 'CMakeCache.txt'))",
        'required_cpu_features=sse4.2,avx2,fma,bmi1,bmi2',
        "main_test_inventory=$mainInventory", "main_test_inventory_sha256=$(Get-OptionalFileHash -Path $mainInventory)",
        "main_ctest_metadata_sha256=$(Get-OptionalFileHash -Path $mainMetadata)",
        "consumer_test_inventory=$consumerInventory", "consumer_test_inventory_sha256=$(Get-OptionalFileHash -Path $consumerInventory)",
        "consumer_ctest_metadata_sha256=$(Get-OptionalFileHash -Path $consumerMetadata)",
        "codegen_record_index=$codegenIndex", "codegen_record_index_sha256=$(Get-OptionalFileHash -Path $codegenIndex)"
    )
    Set-PipelineTextFile -Path $manifestPath -Content (($lines -join "`n") + "`n")
}

<#
.SYNOPSIS
Validates a native build manifest and every artifact index it binds.
.PARAMETER Artifact
Resolved cell artifact.
.PARAMETER Operation
Expected completed operation.
#>
function Assert-NativeManifest {
    param([Parameter(Mandatory)]$Artifact, [Parameter(Mandatory)][string]$Operation)
    $manifestName = if ($Operation -eq 'build-benchmarks') { 'benchmark-build.manifest' } else { 'validation-build.manifest' }
    $path = Join-Path $Artifact.Provenance $manifestName
    $manifest = Read-PipelineManifest -Path $path
    $expected = @{
        schema = 'simdlib.build-manifest.v1'; operation = $Operation; status = 'complete'
        fingerprint_sha256 = $Artifact.Fingerprint; fingerprint_document = $Artifact.FingerprintPath
        compiler_id = $Artifact.Definition.Compiler; preset = $Artifact.Definition.Preset
        build_profile = $Artifact.Definition.BuildProfile; sanitizer = 'none'
    }
    foreach ($key in $expected.Keys) {
        if ($manifest[$key] -ne $expected[$key]) { throw "Manifest $path has mismatched $key" }
    }
    $sourceDigest = Get-PipelineSourceDigest -RepositoryRoot $repositoryRoot
    if ($manifest.source_digest -ne $sourceDigest) { throw "Build manifest is stale for current source inputs: $path" }
    $cache = Join-Path $Artifact.Build 'CMakeCache.txt'
    if ($manifest.cmake_cache_sha256 -ne (Get-OptionalFileHash -Path $cache)) { throw "Build manifest is stale for CMake cache: $path" }
    if ($Operation -eq 'build-validation') {
        foreach ($pair in @(
                @('main_test_inventory', 'main_test_inventory_sha256'),
                @('consumer_test_inventory', 'consumer_test_inventory_sha256'),
                @('codegen_record_index', 'codegen_record_index_sha256')
            )) {
            if ($manifest[$pair[1]] -ne (Get-OptionalFileHash -Path $manifest[$pair[0]])) { throw "Artifact index is missing or stale: $($manifest[$pair[0]])" }
        }
        $configuration = if ($Artifact.Definition.Compiler -eq 'msvc') { $Artifact.Definition.BuildProfile } else { '' }
        Invoke-TestInventory -Mode VALIDATE -TestDirectory $Artifact.Build -InventoryPath $manifest.main_test_inventory -Configuration $configuration
        if ($Artifact.Definition.Consumer) {
            Invoke-TestInventory -Mode VALIDATE -TestDirectory $Artifact.Consumer -InventoryPath $manifest.consumer_test_inventory -Configuration $configuration
        }
        & $cmake "-DRECORD_INDEX=$($manifest.codegen_record_index)" -P (Join-Path $repositoryRoot 'cmake/ValidateCodegenRecords.cmake')
        if ($LASTEXITCODE -ne 0) { throw "Generated-code records are stale for $($Artifact.Id)" }
    }
    return $manifest
}

<#
.SYNOPSIS
Configures and builds one native validation cell.
.PARAMETER Artifact
Resolved cell artifact.
#>
function Build-NativeValidationCell {
    param([Parameter(Mandatory)]$Artifact)
    if ($InjectFailure -contains 'All' -or $InjectFailure -contains $Artifact.Id) { throw "Intentional native failure: $($Artifact.Id)" }
    New-Item -ItemType Directory -Path $Artifact.Reports, $Artifact.Provenance -Force | Out-Null
    $env:SIMDLIB_BUILD_DIRECTORY = $Artifact.Build
    $configureArguments = @('--preset', $Artifact.Definition.Preset, '-S', $repositoryRoot)
    if (Test-CiEnvironment) { $configureArguments = @('--fresh') + $configureArguments }
    Invoke-PipelineCommand -FilePath $cmake -ArgumentList $configureArguments -LogPath (Join-Path $Artifact.Reports 'main-configure.log')
    $buildArguments = @('--build', $Artifact.Build, '--parallel', '--target', 'ExhaustiveArtifacts')
    if ($Artifact.Definition.Compiler -eq 'msvc') { $buildArguments += @('--config', $Artifact.Definition.BuildProfile) }
    Invoke-PipelineCommand -FilePath $cmake -ArgumentList $buildArguments -LogPath (Join-Path $Artifact.Reports 'main-build.log')

    if ($Artifact.Definition.Consumer) {
        $consumerArguments = @('-S', (Join-Path $repositoryRoot 'tests/consumer'), '-B', $Artifact.Consumer, "-DSIMDLIB_SOURCE_DIR=$repositoryRoot", '-DSIMDLIB_BUILD_REGISTER_CONSUMER=ON')
        if ($Artifact.Definition.Compiler -eq 'msvc') {
            $consumerArguments += @('-G', 'Visual Studio 17 2022', '-A', 'x64', "-DCMAKE_CONFIGURATION_TYPES=$($Artifact.Definition.BuildProfile)")
        } else {
            $consumerArguments += @('-G', 'Ninja', "-DCMAKE_BUILD_TYPE=$($Artifact.Definition.BuildProfile)", "-DCMAKE_CXX_COMPILER=$((Get-Command clang-cl.exe).Source)", "-DCMAKE_MAKE_PROGRAM=$ninja")
        }
        Invoke-PipelineCommand -FilePath $cmake -ArgumentList $consumerArguments -LogPath (Join-Path $Artifact.Reports 'consumer-configure.log')
        $consumerBuildArguments = @('--build', $Artifact.Consumer, '--parallel')
        if ($Artifact.Definition.Compiler -eq 'msvc') { $consumerBuildArguments += @('--config', $Artifact.Definition.BuildProfile) }
        Invoke-PipelineCommand -FilePath $cmake -ArgumentList $consumerBuildArguments -LogPath (Join-Path $Artifact.Reports 'consumer-build.log')
    }

    $mainInventory = Join-Path $Artifact.Provenance 'main-test-artifacts.inventory'
    $consumerInventory = Join-Path $Artifact.Provenance 'consumer-test-artifacts.inventory'
    $configuration = if ($Artifact.Definition.Compiler -eq 'msvc') { $Artifact.Definition.BuildProfile } else { '' }
    Invoke-TestInventory -Mode RECORD -TestDirectory $Artifact.Build -InventoryPath $mainInventory -Configuration $configuration
    if ($Artifact.Definition.Consumer) {
        Invoke-TestInventory -Mode RECORD -TestDirectory $Artifact.Consumer -InventoryPath $consumerInventory -Configuration $configuration
    } else {
        Set-PipelineTextFile -Path $consumerInventory -Content ''
    }
    $records = @(Get-ChildItem -LiteralPath $Artifact.Build -Filter '*.record.json' -File -Recurse -ErrorAction SilentlyContinue | Sort-Object FullName | ForEach-Object FullName)
    Set-PipelineTextFile -Path (Join-Path $Artifact.Provenance 'codegen-records.index') -Content $(if ($records.Count) { ($records -join "`n") + "`n" } else { '' })
    Write-NativeManifest -Artifact $Artifact -Operation 'build-validation'
}

<#
.SYNOPSIS
Validates host ISA support required by native runtime tests.
#>
function Assert-NativeCpuFeatures {
    $features = [ordered]@{
        'sse4.2' = [System.Runtime.Intrinsics.X86.Sse42]::IsSupported
        avx2 = [System.Runtime.Intrinsics.X86.Avx2]::IsSupported
        fma = [System.Runtime.Intrinsics.X86.Fma]::IsSupported
        bmi1 = [System.Runtime.Intrinsics.X86.Bmi1]::IsSupported
        bmi2 = [System.Runtime.Intrinsics.X86.Bmi2]::IsSupported
    }
    foreach ($feature in $features.Keys) { if (-not $features[$feature]) { throw "Host CPU does not expose required feature: $feature" } }
}

<#
.SYNOPSIS
Runs tests and optional coverage reporting for one validated native cell.
.PARAMETER Artifact
Resolved cell artifact.
#>
function Test-NativeCell {
    param([Parameter(Mandatory)]$Artifact)
    [void](Assert-NativeManifest -Artifact $Artifact -Operation 'build-validation')
    Assert-NativeCpuFeatures
    New-Item -ItemType Directory -Path $Artifact.Reports -Force | Out-Null
    if ($Artifact.Definition.Coverage) {
        & $cmake "-DBINARY_DIRECTORY=$($Artifact.Build)" -P (Join-Path $repositoryRoot 'cmake/ResetCoverage.cmake')
        if ($LASTEXITCODE -ne 0) { throw "Coverage reset failed for $($Artifact.Id)" }
        $env:LLVM_PROFILE_FILE = Join-Path $Artifact.Build 'ctest-%p-%m.profraw'
    }
    $testArguments = @('--test-dir', $Artifact.Build, '--output-on-failure', '--output-junit', (Join-Path $Artifact.Reports 'main-test.xml'))
    if ($Artifact.Definition.Compiler -eq 'msvc') { $testArguments += @('-C', $Artifact.Definition.BuildProfile) }
    if ($TestRegex) { $testArguments += @('--tests-regex', $TestRegex) }
    if ($TestLabel) { $testArguments += @('--label-regex', $TestLabel) }
    Invoke-PipelineCommand -FilePath $ctest -ArgumentList $testArguments -LogPath (Join-Path $Artifact.Reports 'main-test.log')
    if ($Artifact.Definition.Consumer) {
        $consumerArguments = @('--test-dir', $Artifact.Consumer, '--output-on-failure', '--output-junit', (Join-Path $Artifact.Reports 'consumer-test.xml'))
        if ($Artifact.Definition.Compiler -eq 'msvc') { $consumerArguments += @('-C', $Artifact.Definition.BuildProfile) }
        Invoke-PipelineCommand -FilePath $ctest -ArgumentList $consumerArguments -LogPath (Join-Path $Artifact.Reports 'consumer-test.log')
    }
    if ($Artifact.Definition.Coverage) {
        $coverageManifest = Get-ChildItem -LiteralPath $Artifact.Build -Filter 'coverage-targets-*.txt' -File | Select-Object -First 1
        if (-not $coverageManifest) { throw "Coverage target manifest is missing below $($Artifact.Build)" }
        $arguments = @(
            "-DBINARY_DIRECTORY=$($Artifact.Build)", "-DSOURCE_DIRECTORY=$repositoryRoot",
            "-DCOVERAGE_MANIFEST=$($coverageManifest.FullName)", "-DLLVM_PROFDATA=$((Get-Command llvm-profdata.exe).Source)",
            "-DLLVM_COV=$((Get-Command llvm-cov.exe).Source)", "-DLLVM_READOBJ=$((Get-Command llvm-readobj.exe).Source)",
            '-P', (Join-Path $repositoryRoot 'cmake/GenerateCoverageReport.cmake')
        )
        Invoke-PipelineCommand -FilePath $cmake -ArgumentList $arguments -LogPath (Join-Path $Artifact.Reports 'coverage-report.log')
    }
}

<#
.SYNOPSIS
Builds benchmarks in an already validated native Release tree.
.PARAMETER Artifact
Resolved Release artifact.
#>
function Build-NativeBenchmarks {
    param([Parameter(Mandatory)]$Artifact)
    [void](Assert-NativeManifest -Artifact $Artifact -Operation 'build-validation')
    $arguments = @('--build', $Artifact.Build, '--parallel', '--target', 'BenchmarkArtifacts')
    if ($Artifact.Definition.Compiler -eq 'msvc') { $arguments += @('--config', 'Release') }
    Invoke-PipelineCommand -FilePath $cmake -ArgumentList $arguments -LogPath (Join-Path $Artifact.Reports 'benchmark-build.log')
    Write-NativeManifest -Artifact $Artifact -Operation 'build-benchmarks'
}

<#
.SYNOPSIS
Runs the benchmark executable from a validated benchmark manifest.
.PARAMETER Artifact
Resolved Release artifact.
#>
function Run-NativeBenchmarks {
    param([Parameter(Mandatory)]$Artifact)
    [void](Assert-NativeManifest -Artifact $Artifact -Operation 'build-benchmarks')
    Assert-NativeCpuFeatures
    $benchmark = Get-ChildItem -LiteralPath $Artifact.Build -Filter 'Benchmarks.exe' -File -Recurse | Select-Object -First 1
    if (-not $benchmark) { throw "Required benchmark executable is missing below $($Artifact.Build)" }
    Invoke-PipelineCommand -FilePath $benchmark.FullName -ArgumentList @('[simdlib][benchmark]', '--benchmark-samples', '25') -LogPath (Join-Path $Artifact.Reports 'benchmark-execution.txt')
}

if (($TestRegex -or $TestLabel) -and $Action -ne 'Test') { throw '-TestRegex and -TestLabel are valid only for Test.' }
if ($Cell -eq 'Coverage' -and $Compiler -notin @('All', 'ClangCoverage')) { throw 'Coverage is owned by the native Clang coverage compiler.' }
if ($Action -in @('BuildBenchmarks', 'RunBenchmarks') -and $Cell -notin @('All', 'Release')) { throw 'Benchmark operations use Release cells only.' }

$cells = @(Resolve-NativeCells -CompilerName $Compiler -CellScope $Cell -Operation $Action)
if ($cells.Count -eq 0) { throw 'The native compiler and cell selections identify no operation cells.' }
$artifacts = @($cells | ForEach-Object { Initialize-NativeArtifact -BuildCell $_ })
$failures = [System.Collections.Generic.List[string]]::new()
foreach ($artifact in $artifacts) {
    try {
        Write-Host "Native operation: action=$Action cell=$($artifact.Id) root=$($artifact.Root)"
        switch ($Action) {
            'Build' { Build-NativeValidationCell -Artifact $artifact }
            'Test' { Test-NativeCell -Artifact $artifact }
            'BuildBenchmarks' { Build-NativeBenchmarks -Artifact $artifact }
            'RunBenchmarks' { Run-NativeBenchmarks -Artifact $artifact }
        }
    } catch {
        Write-Error -ErrorAction Continue "$($artifact.Id): $_"
        $failures.Add($artifact.Id)
    }
}
if ($failures.Count) { throw "Native operation $Action failed: $($failures -join ', ')" }
Write-Host "Native operation passed: action=$Action cells=$($artifacts.Count)"
