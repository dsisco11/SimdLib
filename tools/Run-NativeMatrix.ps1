<#
.SYNOPSIS
Builds or consumes fingerprinted native compiler cells.
.DESCRIPTION
Build creates validation artifacts and manifests. Test validates those manifests
and runs CTest without configuring or building. RecordCodegen creates an
independent record-only diagnostic fingerprint. Benchmark operations reuse only
the existing Release trees. Coverage is an independent Clang Debug cell.
#>
[CmdletBinding()]
param(
    [ValidateSet('Build', 'Test', 'BuildCompilerContracts', 'TestCompilerContracts', 'RecordCodegen', 'BuildBenchmarks', 'RunBenchmarks')]
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

<#
.SYNOPSIS
Resolves and validates the Clang commands selected by the caller's PATH.
.PARAMETER CompilerName
Requested native compiler scope.
#>
function Resolve-RequestedClangCommands {
    param([Parameter(Mandatory)][string]$CompilerName)

    $commandNames = @(
        if ($CompilerName -in @('All', 'ClangCl')) { 'clang-cl.exe' }
        if ($CompilerName -in @('All', 'ClangCoverage')) { 'clang++.exe' }
    )
    $commands = [ordered]@{}
    if ($commandNames.Count -eq 0) { return ,$commands }

    foreach ($commandName in $commandNames) {
        $command = @(Get-Command $commandName -CommandType Application -ErrorAction Stop)[0]
        $versionLine = [string](@(& $command.Source --version 2>&1)[0])
        if ($versionLine -notmatch '\bclang version (?<major>\d+)(?:\.\d+)*') {
            throw "Unable to determine the Clang version selected for $commandName at $($command.Source): $versionLine"
        }
        if ([int]$Matches.major -lt 20) {
            throw "Clang 20 or newer is required for $commandName, but PATH selected $versionLine at $($command.Source)."
        }
        $commands[$commandName] = [pscustomobject]@{
            Name = $commandName
            Source = $command.Source
            Directory = Split-Path -Parent $command.Source
            Version = $versionLine.Trim()
        }
    }

    $directories = @($commands.Values.Directory | Select-Object -Unique)
    if ($directories.Count -gt 1) {
        throw "clang-cl and clang++ must come from one LLVM installation, but PATH selected: $($directories -join ', ')"
    }
    return $commands
}

<#
.SYNOPSIS
Restores the caller-selected LLVM directory after Visual Studio environment setup.
.PARAMETER Commands
Validated Clang commands captured before Visual Studio initialization.
#>
function Restore-RequestedClangCommands {
    param([Parameter(Mandatory)][System.Collections.IDictionary]$Commands)

    if ($Commands.Count -eq 0) { return }
    $selectedDirectory = [string]@($Commands.Values.Directory)[0]
    $pathSeparator = [System.IO.Path]::PathSeparator
    $remainingEntries = @($env:PATH -split [regex]::Escape([string]$pathSeparator) | Where-Object {
            $_ -and -not [string]::Equals(
                $_.TrimEnd('\', '/'), $selectedDirectory.TrimEnd('\', '/'),
                [System.StringComparison]::OrdinalIgnoreCase)
        })
    $env:PATH = (@($selectedDirectory) + $remainingEntries) -join $pathSeparator

    foreach ($entry in $Commands.GetEnumerator()) {
        $resolved = @(Get-Command $entry.Key -CommandType Application -ErrorAction Stop)[0]
        if (-not [string]::Equals(
                $resolved.Source, $entry.Value.Source,
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Unable to restore caller-selected $($entry.Key): expected $($entry.Value.Source), resolved $($resolved.Source)."
        }
    }
}

$repositoryRoot = Get-PipelineRepositoryRoot
$pipelineRoot = Join-Path $repositoryRoot 'out/pipeline'
$cmake = (Get-Command cmake -ErrorAction Stop).Source
$ctest = (Get-Command ctest -ErrorAction Stop).Source
$requestedClangCommands = Resolve-RequestedClangCommands -CompilerName $Compiler
$visualStudio = Initialize-PipelineVisualStudioEnvironment
Restore-RequestedClangCommands -Commands $requestedClangCommands
$ninja = Join-Path $visualStudio 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
if (-not (Test-Path -LiteralPath $ninja -PathType Leaf)) {
    throw "Visual Studio's bundled Ninja executable is missing: $ninja"
}
$env:SIMDLIB_NINJA = $ninja

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

    $compilerNames = if ($CompilerName -eq 'All') {
        @(Get-PipelineValidationCompilers -Platform native)
    } else {
        @($CompilerName)
    }
    return @(Resolve-PipelineValidationCells -Platform native `
        -CompilerNames $compilerNames -CellScope $CellScope -Operation $Operation)
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
            sanitizer = $BuildCell.Sanitizer; instrumentation = $BuildCell.Instrumentation
            coverage = $BuildCell.Coverage; generator = $BuildCell.Generator
            codegenMode = $BuildCell.CodegenMode
            aggregate = $BuildCell.Aggregate
            consumerScope = if ($BuildCell.Consumer) { 'compiler-release' } else { 'none' }
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
Verifies that mandatory runtime-test labels and families exist in one native CTest tree.
.PARAMETER Artifact
Resolved native build-cell artifact.
#>
function Invoke-RuntimeTestInventoryAudit {
    param([Parameter(Mandatory)]$Artifact)
    $arguments = @(
        "-DTEST_DIRECTORY=$($Artifact.Build)",
        "-DCMAKE_CTEST_COMMAND=$ctest",
        "-DAUDIT_FILE=$(Join-Path $Artifact.Reports 'runtime-test-inventory.audit.txt')",
        '-DREGISTER_REQUIRED=ON'
    )
    if ($Artifact.Definition.Compiler -eq 'msvc') {
        $arguments += "-DCONFIGURATION=$($Artifact.Definition.BuildProfile)"
    }
    $arguments += @('-P', (Join-Path $repositoryRoot 'cmake/VerifyRuntimeTestInventory.cmake'))
    & $cmake @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Mandatory runtime-test inventory audit failed for $($Artifact.Id)"
    }
}


<#
.SYNOPSIS
Returns the canonical validation-matrix cell identifier for one native artifact.
.PARAMETER Artifact
Resolved native build-cell artifact.
#>
function Get-NativeValidationCellId {
    param([Parameter(Mandatory)]$Artifact)

    return [string]$Artifact.Definition.MatrixCell
}
<#
.SYNOPSIS
Audits generated target and CTest ownership for one native build cell.
.PARAMETER Artifact
Resolved native build-cell artifact.
#>
function Invoke-NativeValidationInventoryAudit {
    param([Parameter(Mandatory)]$Artifact)

    $auditParameters = @{
        Cell = Get-NativeValidationCellId -Artifact $Artifact
        BuildDirectory = $Artifact.Build
        ResultPath = Join-Path $Artifact.Reports 'validation-inventory.audit.json'
    }
    if ($Artifact.Definition.Compiler -eq 'msvc') {
        $auditParameters.Configuration = $Artifact.Definition.BuildProfile
    }
    & (Join-Path $PSScriptRoot 'Audit-ValidationMatrix.ps1') @auditParameters
    if ($LASTEXITCODE -ne 0) {
        throw "Validation inventory audit failed for $($Artifact.Id)"
    }
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
Writes the aggregate generated-code record index from CMake-owned validation indexes.
.PARAMETER BuildDirectory
Configured build tree containing the owner indexes.
.PARAMETER OutputPath
Pipeline record index to write.
.PARAMETER AllowEmpty
Allows profiles that own no generated-code work to emit an empty index.
#>
function Write-CodegenRecordIndex {
    param(
        [Parameter(Mandatory)][string]$BuildDirectory,
        [Parameter(Mandatory)][string]$OutputPath,
        [switch]$AllowEmpty
    )
    $ownerIndexes = @(
        (Join-Path $BuildDirectory 'method-flags-codegen/all-records.txt'),
        (Join-Path $BuildDirectory 'register-codegen/sse42/128/all-records.txt'),
        (Join-Path $BuildDirectory 'register-codegen/avx2/128/all-records.txt'),
        (Join-Path $BuildDirectory 'register-codegen/avx2/256/all-records.txt')
    )
    $records = @(
        foreach ($ownerIndex in $ownerIndexes) {
            if (Test-Path -LiteralPath $ownerIndex -PathType Leaf) {
                Get-Content -LiteralPath $ownerIndex | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
            }
        }
    )
    $records = @($records | Sort-Object -Unique)
    if (-not $records.Count -and -not $AllowEmpty) {
        throw "No CMake-owned generated-code records were found under $BuildDirectory"
    }
    $content = if ($records.Count) {
        ($records -join [Environment]::NewLine) + [Environment]::NewLine
    } else {
        ''
    }
    Set-PipelineTextFile -Path $OutputPath -Content $content
}

<#
.SYNOPSIS
Returns and validates the external-consumer scope owned by one native cell.
.PARAMETER Artifact
Resolved cell whose configured capability inventory is inspected.
#>
function Get-NativeConsumerScope {
    param([Parameter(Mandatory)]$Artifact)
    if (-not $Artifact.Definition.Consumer) { return 'none' }

    $capabilityPath = Join-Path $Artifact.Build 'external-consumer-targets.txt'
    if (-not (Test-Path -LiteralPath $capabilityPath -PathType Leaf)) {
        throw "External-consumer capability inventory is missing: $capabilityPath"
    }
    $targets = @(
        Get-Content -LiteralPath $capabilityPath |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            Sort-Object -Unique
    )
    $targetSequence = $targets -join '|'
    if ($targetSequence -eq 'CoreConsumerSmoke') { return 'core' }
    if ($targetSequence -eq 'CoreConsumerSmoke|RegisterConsumerSmoke') {
        return 'core-register'
    }
    throw "Unsupported external-consumer capability inventory: $targetSequence"
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
    $targetInventory = Join-Path $Artifact.Build 'development-profile-targets.txt'
    $ownershipAudit = Join-Path $Artifact.Reports 'validation-inventory.audit.json'
    $matrixContract = Join-Path $PSScriptRoot 'validation-matrix.json'
    $consumerScope = Get-NativeConsumerScope -Artifact $Artifact
    $aggregate = if ($Operation -eq 'build-benchmarks') { 'BenchmarkArtifacts' } else { $Artifact.Definition.Aggregate }
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
        "preset=$($Artifact.Definition.Preset)", "build_profile=$($Artifact.Definition.BuildProfile)",
        "sanitizer=$($Artifact.Definition.Sanitizer)", "instrumentation=$($Artifact.Definition.Instrumentation)",
        "codegen_mode=$($Artifact.Definition.CodegenMode)",
        "aggregate=$aggregate", "matrix_cell=$(Get-NativeValidationCellId -Artifact $Artifact)", "consumer_scope=$consumerScope",
        "target_inventory=$targetInventory", "target_inventory_sha256=$(Get-OptionalFileHash -Path $targetInventory)",
        "matrix_contract_sha256=$(Get-OptionalFileHash -Path $matrixContract)",
        "validation_inventory_audit=$ownershipAudit",
        "validation_inventory_audit_sha256=$(Get-OptionalFileHash -Path $ownershipAudit)",
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
        build_profile = $Artifact.Definition.BuildProfile; sanitizer = $Artifact.Definition.Sanitizer
        instrumentation = $Artifact.Definition.Instrumentation; codegen_mode = $Artifact.Definition.CodegenMode
        aggregate = if ($Operation -eq 'build-benchmarks') { 'BenchmarkArtifacts' } else { $Artifact.Definition.Aggregate }
        matrix_cell = Get-NativeValidationCellId -Artifact $Artifact
        consumer_scope = Get-NativeConsumerScope -Artifact $Artifact
    }
    foreach ($key in $expected.Keys) {
        if ($manifest[$key] -ne $expected[$key]) { throw "Manifest $path has mismatched $key" }
    }
    $sourceDigest = Get-PipelineSourceDigest -RepositoryRoot $repositoryRoot
    if ($manifest.source_digest -ne $sourceDigest) { throw "Build manifest is stale for current source inputs: $path" }
    $cache = Join-Path $Artifact.Build 'CMakeCache.txt'
    if ($manifest.cmake_cache_sha256 -ne (Get-OptionalFileHash -Path $cache)) { throw "Build manifest is stale for CMake cache: $path" }
    if ($manifest.target_inventory_sha256 -ne (Get-OptionalFileHash -Path $manifest.target_inventory)) { throw "Configured target inventory is missing or stale: $($manifest.target_inventory)" }
    $matrixContract = Join-Path $PSScriptRoot 'validation-matrix.json'
    Invoke-NativeValidationInventoryAudit -Artifact $Artifact
    if ($manifest.matrix_contract_sha256 -ne (Get-OptionalFileHash -Path $matrixContract)) { throw "Validation matrix contract is stale for $($Artifact.Id)" }
    if ($manifest.validation_inventory_audit_sha256 -ne (Get-OptionalFileHash -Path $manifest.validation_inventory_audit)) { throw "Validation inventory audit is missing or stale: $($manifest.validation_inventory_audit)" }
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
    $buildArguments = @('--build', $Artifact.Build, '--parallel', '--target', $Artifact.Definition.Aggregate)
    if ($Artifact.Definition.Compiler -eq 'msvc') { $buildArguments += @('--config', $Artifact.Definition.BuildProfile) }
    Invoke-PipelineCommand -FilePath $cmake -ArgumentList $buildArguments -LogPath (Join-Path $Artifact.Reports 'main-build.log')

    if ($Artifact.Definition.Consumer) {
        $consumerScope = Get-NativeConsumerScope -Artifact $Artifact
        $registerConsumer = if ($consumerScope -eq 'core-register') { 'ON' } else { 'OFF' }
        $consumerArguments = @('-S', (Join-Path $repositoryRoot 'tests/consumer'), '-B', $Artifact.Consumer, "-DSIMDLIB_SOURCE_DIR=$repositoryRoot", "-DSIMDLIB_BUILD_REGISTER_CONSUMER=$registerConsumer")
        if ($Artifact.Definition.Compiler -eq 'msvc') {
            $consumerArguments += @('-G', 'Visual Studio 17 2022', '-A', 'x64', "-DCMAKE_CONFIGURATION_TYPES=$($Artifact.Definition.BuildProfile)")
        } else {
            $consumerArguments += @('-G', 'Ninja', "-DCMAKE_BUILD_TYPE=$($Artifact.Definition.BuildProfile)", "-DCMAKE_CXX_COMPILER=$((Get-Command clang-cl.exe).Source)", "-DCMAKE_MAKE_PROGRAM=$ninja")
        }
        Invoke-PipelineCommand -FilePath $cmake -ArgumentList $consumerArguments -LogPath (Join-Path $Artifact.Reports 'consumer-configure.log')
        $consumerBuildArguments = @('--build', $Artifact.Consumer, '--parallel')
        if ($Artifact.Definition.Compiler -eq 'msvc') { $consumerBuildArguments += @('--config', $Artifact.Definition.BuildProfile) }
        Invoke-PipelineCommand -FilePath $cmake -ArgumentList $consumerBuildArguments -LogPath (Join-Path $Artifact.Reports 'consumer-build.log')
    } elseif (Test-Path -LiteralPath $Artifact.Consumer) {
        throw "Consumer-free cell contains an external-consumer tree: $($Artifact.Consumer)"
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
    $allowEmptyCodegen = $Artifact.Definition.CodegenMode -eq 'OFF'
    Write-CodegenRecordIndex -BuildDirectory $Artifact.Build -OutputPath (Join-Path $Artifact.Provenance 'codegen-records.index') -AllowEmpty:$allowEmptyCodegen
    Invoke-NativeValidationInventoryAudit -Artifact $Artifact
    Write-NativeManifest -Artifact $Artifact -Operation 'build-validation'
}

<#
.SYNOPSIS
Runs the focused compiler-contract CTest inventory without building.
.PARAMETER Artifact
Resolved compiler-contract artifact.
#>
function Test-NativeCompilerContractCell {
    param([Parameter(Mandatory)]$Artifact)
    [void](Assert-NativeManifest -Artifact $Artifact -Operation 'build-validation')
    $arguments = @('--test-dir', $Artifact.Build, '--output-on-failure')
    if ($Artifact.Definition.Compiler -eq 'msvc') { $arguments += @('-C', $Artifact.Definition.BuildProfile) }
    if ($TestRegex) { $arguments += @('--tests-regex', $TestRegex) }
    if ($TestLabel) { $arguments += @('--label-regex', $TestLabel) }
    Invoke-PipelineCommand -FilePath $ctest -ArgumentList $arguments -LogPath (Join-Path $Artifact.Reports 'compiler-contract-tests.log')
}

<#
.SYNOPSIS
Writes dedicated provenance for one record-only native codegen diagnostic.
.PARAMETER Artifact
Resolved diagnostic fingerprint.
.PARAMETER InvocationCompilationSeconds
Elapsed fixture-object compilation time for the current invocation.
.PARAMETER InvocationComparisonSeconds
Elapsed disassembly and comparison time for the current invocation.
.PARAMETER MeasuredCompilationSeconds
Largest source-compatible compilation measurement retained across cached runs.
.PARAMETER MeasuredComparisonSeconds
Largest source-compatible comparison measurement retained across cached runs.
#>
function Write-NativeCodegenDiagnosticProvenance {
    param(
        [Parameter(Mandatory)]$Artifact,
        [Parameter(Mandatory)][double]$InvocationCompilationSeconds,
        [Parameter(Mandatory)][double]$InvocationComparisonSeconds,
        [Parameter(Mandatory)][double]$MeasuredCompilationSeconds,
        [Parameter(Mandatory)][double]$MeasuredComparisonSeconds
    )
    $recordIndex = Join-Path $Artifact.Provenance 'codegen-records.index'
    $compileCommands = Join-Path $Artifact.Build 'compile_commands.json'
    if (-not (Test-Path -LiteralPath $compileCommands -PathType Leaf)) {
        throw "Diagnostic compiler-flag inventory is missing: $compileCommands"
    }
    $recordPaths = @(Get-Content -LiteralPath $recordIndex | Where-Object { $_ })
    $recordTimings = @(
        foreach ($recordPath in $recordPaths) {
            $record = Get-Content -LiteralPath $recordPath -Raw | ConvertFrom-Json
            $profileProperty = $record.policy.PSObject.Properties['codegen_profile']
            [pscustomobject]@{
                path = $recordPath
                profile = if ($profileProperty) { $profileProperty.Value } else { 'default-abi' }
                result = $record.result
                seconds = [int]$record.timing.total_seconds
                stackProtectorMode = $record.stack_protector_mode
                disassemblyTool = [pscustomobject]@{
                    path = $record.tool.path
                    version = $record.tool.version
                    sha256 = $record.tool.sha256
                }
            }
        }
    )
    $slowestRecords = @($recordTimings | Sort-Object seconds -Descending | Select-Object -First 10)
    $stackProtectorModes = @(
        $recordTimings | Select-Object -ExpandProperty stackProtectorMode -Unique |
            Sort-Object
    )
    $disassemblyTools = @(
        $recordTimings | Group-Object {
            "$($_.disassemblyTool.path)|$($_.disassemblyTool.version)|$($_.disassemblyTool.sha256)"
        } | ForEach-Object { $_.Group[0].disassemblyTool }
    )
    $document = [ordered]@{
        schema = 'simdlib.codegen-diagnostic-provenance.v1'
        operation = 'record-codegen'
        status = 'complete'
        sourceRevision = Get-PipelineRevision -RepositoryRoot $repositoryRoot
        sourceDigest = Get-PipelineSourceDigest -RepositoryRoot $repositoryRoot
        fingerprint = $Artifact.Fingerprint
        compiler = $Artifact.CompilerIdentity
        configuration = [ordered]@{
            preset = $Artifact.Definition.Preset
            buildProfile = $Artifact.Definition.BuildProfile
            sanitizer = $Artifact.Definition.Sanitizer
            codegenMode = $Artifact.Definition.CodegenMode
        }
        compilerFlags = [ordered]@{
            path = $compileCommands
            sha256 = Get-OptionalFileHash -Path $compileCommands
        }
        records = [ordered]@{
            index = $recordIndex
            sha256 = Get-OptionalFileHash -Path $recordIndex
            count = $recordPaths.Count
            slowest = $slowestRecords
        }
        stackProtectorModes = $stackProtectorModes
        disassemblyTools = $disassemblyTools
        timing = [ordered]@{
            invocation = [ordered]@{
                compilationSeconds = [Math]::Round($InvocationCompilationSeconds, 3)
                comparisonSeconds = [Math]::Round($InvocationComparisonSeconds, 3)
                totalSeconds = [Math]::Round(
                    $InvocationCompilationSeconds + $InvocationComparisonSeconds, 3)
            }
            measured = [ordered]@{
                compilationSeconds = [Math]::Round($MeasuredCompilationSeconds, 3)
                comparisonSeconds = [Math]::Round($MeasuredComparisonSeconds, 3)
                totalSeconds = [Math]::Round(
                    $MeasuredCompilationSeconds + $MeasuredComparisonSeconds, 3)
            }
        }
    }
    $provenancePath = Join-Path $Artifact.Provenance 'codegen-diagnostic.json'
    Set-PipelineTextFile -Path $provenancePath -Content ($document | ConvertTo-Json -Depth 10)
    return $provenancePath
}

<#
.SYNOPSIS
Compiles only native Register fixtures, then records and validates diagnostics.
.PARAMETER Artifact
Resolved diagnostic fingerprint.
#>
function Record-NativeCodegenDiagnostic {
    param([Parameter(Mandatory)]$Artifact)
    if ($InjectFailure -contains 'All' -or $InjectFailure -contains $Artifact.Id) {
        throw "Intentional native failure: $($Artifact.Id)"
    }
    New-Item -ItemType Directory -Path $Artifact.Reports, $Artifact.Provenance -Force | Out-Null
    $provenancePath = Join-Path $Artifact.Provenance 'codegen-diagnostic.json'
    $priorProvenance = $null
    if (Test-Path -LiteralPath $provenancePath -PathType Leaf) {
        $priorProvenance = Get-Content -LiteralPath $provenancePath -Raw | ConvertFrom-Json
    }
    $priorCompilationSeconds = 0.0
    $priorComparisonSeconds = 0.0
    $env:SIMDLIB_BUILD_DIRECTORY = $Artifact.Build
    $configureArguments = @('--preset', $Artifact.Definition.Preset, '-S', $repositoryRoot)
    if (Test-CiEnvironment) { $configureArguments = @('--fresh') + $configureArguments }
    Invoke-PipelineCommand -FilePath $cmake -ArgumentList $configureArguments -LogPath (Join-Path $Artifact.Reports 'codegen-configure.log')

    $compilationWatch = [System.Diagnostics.Stopwatch]::StartNew()
    Invoke-PipelineCommand -FilePath $cmake -ArgumentList @(
        '--build', $Artifact.Build, '--parallel', '--target', 'RegisterCodegenFixtureObjects'
    ) -LogPath (Join-Path $Artifact.Reports 'codegen-compilation.log')
    $compilationWatch.Stop()

    $comparisonWatch = [System.Diagnostics.Stopwatch]::StartNew()
    Invoke-PipelineCommand -FilePath $cmake -ArgumentList @(
        '--build', $Artifact.Build, '--parallel', '--target', 'SimdLibDebugDiagnosticArtifacts'
    ) -LogPath (Join-Path $Artifact.Reports 'codegen-comparison.log')
    $comparisonWatch.Stop()

    $recordIndex = Join-Path $Artifact.Provenance 'codegen-records.index'
    Write-CodegenRecordIndex -BuildDirectory $Artifact.Build -OutputPath $recordIndex
    $compileCommands = Join-Path $Artifact.Build 'compile_commands.json'
    if ($priorProvenance) {
        $priorCompilerFlags = $priorProvenance.PSObject.Properties['compilerFlags']
        $priorRecords = $priorProvenance.PSObject.Properties['records']
        $sameDiagnosticInputs = $priorCompilerFlags -and $priorRecords -and
            $priorCompilerFlags.Value.sha256 -eq (Get-OptionalFileHash -Path $compileCommands) -and
            $priorRecords.Value.sha256 -eq (Get-OptionalFileHash -Path $recordIndex)
        if ($sameDiagnosticInputs) {
            $measuredTiming = $priorProvenance.timing.PSObject.Properties['measured']
            if ($measuredTiming) {
                $priorCompilationSeconds = [double]$measuredTiming.Value.compilationSeconds
                $priorComparisonSeconds = [double]$measuredTiming.Value.comparisonSeconds
            } else {
                $priorCompilationSeconds = [double]$priorProvenance.timing.compilationSeconds
                $priorComparisonSeconds = [double]$priorProvenance.timing.comparisonSeconds
            }
        }
    }
    & $cmake "-DRECORD_INDEX=$recordIndex" '-DEXPECTED_POLICY_MODE=RECORD' `
        '-DEXPECTED_CONFIGURATION=Debug' '-DREQUIRE_RECORDS=ON' `
        -P (Join-Path $repositoryRoot 'cmake/ValidateCodegenRecords.cmake')
    if ($LASTEXITCODE -ne 0) { throw "Diagnostic records are invalid for $($Artifact.Id)" }
    & $cmake "-DBINARY_DIRECTORY=$($Artifact.Build)" `
        "-DOWNERSHIP_FILE=$(Join-Path $Artifact.Build 'development-target-ownership.tsv')" `
        '-DPROFILE=CODEGEN_DIAGNOSTIC' '-DCODEGEN_MODE=RECORD' `
        -P (Join-Path $repositoryRoot 'cmake/VerifyCodegenProfileIsolation.cmake')
    if ($LASTEXITCODE -ne 0) { throw "Diagnostic profile isolation failed for $($Artifact.Id)" }
    $measuredCompilationSeconds = [Math]::Max(
        $compilationWatch.Elapsed.TotalSeconds, $priorCompilationSeconds)
    $measuredComparisonSeconds = [Math]::Max(
        $comparisonWatch.Elapsed.TotalSeconds, $priorComparisonSeconds)
    $provenance = Write-NativeCodegenDiagnosticProvenance -Artifact $Artifact `
        -InvocationCompilationSeconds $compilationWatch.Elapsed.TotalSeconds `
        -InvocationComparisonSeconds $comparisonWatch.Elapsed.TotalSeconds `
        -MeasuredCompilationSeconds $measuredCompilationSeconds `
        -MeasuredComparisonSeconds $measuredComparisonSeconds
    Write-Host "Native codegen diagnostic provenance: $provenance"
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
    Invoke-RuntimeTestInventoryAudit -Artifact $Artifact
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

if (($TestRegex -or $TestLabel) -and $Action -notin @('Test', 'TestCompilerContracts')) { throw '-TestRegex and -TestLabel are valid only for Test and TestCompilerContracts.' }
if ($Cell -eq 'Coverage' -and $Compiler -notin @('All', 'ClangCoverage')) { throw 'Coverage is owned by the native Clang coverage compiler.' }
if ($Action -in @('BuildBenchmarks', 'RunBenchmarks') -and $Cell -notin @('All', 'Release')) { throw 'Benchmark operations use Release cells only.' }
if ($Action -in @('BuildCompilerContracts', 'TestCompilerContracts') -and $Cell -notin @('All', 'Release')) {
    throw 'Focused compiler-contract operations use Release compiler identities.'
}
if ($Action -eq 'RecordCodegen') {
    if ($Cell -notin @('All', 'Debug')) { throw 'Native codegen diagnostics use Debug cells only.' }
    if ($Compiler -eq 'ClangCoverage') { throw 'Native coverage does not own a Register codegen diagnostic.' }
}

$cells = @(Resolve-NativeCells -CompilerName $Compiler -CellScope $Cell -Operation $Action)
if ($cells.Count -eq 0) { throw 'The native compiler and cell selections identify no operation cells.' }
$artifacts = @($cells | ForEach-Object { Initialize-NativeArtifact -BuildCell $_ })
$failures = [System.Collections.Generic.List[string]]::new()
foreach ($artifact in $artifacts) {
    try {
        Write-Host "Native operation: action=$Action cell=$($artifact.Id) root=$($artifact.Root)"
        switch ($Action) {
            'Build' { Build-NativeValidationCell -Artifact $artifact }
            'BuildCompilerContracts' { Build-NativeValidationCell -Artifact $artifact }
            'TestCompilerContracts' { Test-NativeCompilerContractCell -Artifact $artifact }
            'Test' { Test-NativeCell -Artifact $artifact }
            'RecordCodegen' { Record-NativeCodegenDiagnostic -Artifact $artifact }
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
