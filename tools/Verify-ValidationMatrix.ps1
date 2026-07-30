<#
.SYNOPSIS
Verifies the canonical default validation matrix and opt-in Debug selectors.
#>
[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Pipeline.Common.psm1') -Force

<#
.SYNOPSIS
Imports one function definition without executing its owning script.
.PARAMETER Path
PowerShell script containing the function.
.PARAMETER Name
Function name to import into this verifier's script scope.
#>
function Import-MatrixResolver {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Name
    )

    $tokens = $null
    $errors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile(
        $Path, [ref]$tokens, [ref]$errors)
    if ($errors.Count -ne 0) {
        throw "Unable to parse $Path`: $($errors.Message -join '; ')"
    }
    $definitions = @($ast.FindAll({
                param($node)
                $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and
                $node.Name -eq $Name
            }, $true))
    if ($definitions.Count -ne 1) {
        throw "Expected exactly one $Name definition in $Path"
    }
    Invoke-Expression "function script:$Name $($definitions[0].Body.Extent.Text)"
}

<#
.SYNOPSIS
Rejects a sequence that differs from its exact expected order.
.PARAMETER Name
Human-readable sequence name.
.PARAMETER Actual
Observed sequence.
.PARAMETER Expected
Required sequence.
#>
function Assert-MatrixSequence {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][AllowEmptyCollection()][string[]]$Actual,
        [Parameter(Mandatory)][AllowEmptyCollection()][string[]]$Expected
    )

    if (($Actual -join '|') -ne ($Expected -join '|')) {
        throw "$Name mismatch. Expected '$($Expected -join ', ')'; received '$($Actual -join ', ')'"
    }
}

$compilerOrder = @('Msvc', 'ClangCl', 'ClangCoverage', 'Gcc13', 'Gcc14', 'Clang22')
$matrixPath = Join-Path $PSScriptRoot 'validation-matrix.json'
$matrix = Get-Content -LiteralPath $matrixPath -Raw | ConvertFrom-Json
if ($matrix.schema -ne 'simdlib.validation-matrix.v1') {
    throw "Unsupported validation matrix schema in $matrixPath"
}

<#
.SYNOPSIS
Returns the canonical cell objects assigned to one matrix operation.
.PARAMETER Operation
Operation property from the machine-readable matrix.
#>
function Get-ExpectedMatrixCells {
    param([Parameter(Mandatory)][string]$Operation)

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
            Add-Member -InputObject $cellProperty.Value `
                -NotePropertyName MatrixCell -NotePropertyValue ([string]$cellId) `
                -Force -PassThru
        }
    )
}

$defaultContractCells = @(Get-ExpectedMatrixCells -Operation defaultBuild)
$defaultTestContractCells = @(Get-ExpectedMatrixCells -Operation defaultTests)
Assert-MatrixSequence -Name 'Default build/test ownership' `
    -Actual @($defaultTestContractCells.MatrixCell) `
    -Expected @($defaultContractCells.MatrixCell)
$ordinaryDebugCells = @('clangcl-debug', 'gcc13-debug', 'gcc14-debug', 'clang22-debug')
foreach ($ordinaryDebugCell in $ordinaryDebugCells) {
    if ($ordinaryDebugCell -in @($defaultContractCells.MatrixCell)) {
        throw "Ordinary Debug cell re-entered the default matrix: $ordinaryDebugCell"
    }
}
foreach ($profileName in @('SANITIZER', 'COVERAGE')) {
    $profile = $matrix.profiles.$profileName
    $forbidden = @(@($profile.allowedTargetCategories) |
        Where-Object { $_ -in @('OPTIMIZED_CODEGEN', 'DEBUG_DIAGNOSTIC', 'CONSTEXPR_CONTRACT', 'SMOKE_VALIDATION') })
    if ($forbidden.Count) {
        throw "$profileName profile permits forbidden categories: $($forbidden -join ', ')"
    }
}
$contractCells = @(Get-ExpectedMatrixCells -Operation compilerContracts)
$contractCompilerIdentities = @($contractCells.compilerIdentity)
if (@($contractCompilerIdentities | Select-Object -Unique).Count -ne $contractCompilerIdentities.Count) {
    throw 'Compiler-front-end contracts are assigned more than once per compiler identity'
}
foreach ($cell in $defaultContractCells | Where-Object {
        $_.profile -eq 'RELEASE' -and $_.registerCapable }) {
    if ($cell.codegenMode -ne 'ENFORCE') {
        throw "Register-capable Release cell does not enforce codegen: $($cell.MatrixCell)"
    }
}
foreach ($cell in @(Get-ExpectedMatrixCells -Operation optionalDiagnostics)) {
    if ($cell.codegenMode -ne 'RECORD' -or
        $cell.MatrixCell -in @($defaultContractCells.MatrixCell)) {
        throw "Optional diagnostic is not isolated record-only evidence: $($cell.MatrixCell)"
    }
}
$defaultPresets = @(
    'msvc-release-exhaustive',
    'msvc-debug-diagnostics',
    'clangcl-release-exhaustive',
    'clang-debug-coverage',
    'gcc13-core-release-exhaustive',
    'gcc14-release-exhaustive',
    'clang22-release-exhaustive',
    'clang22-debug-asan-ubsan'
)
Assert-MatrixSequence -Name 'Machine-readable default presets' `
    -Actual @($defaultContractCells.preset) `
    -Expected $defaultPresets
Assert-MatrixSequence -Name 'Canonical default presets' `
    -Actual @(Get-PipelineDefaultValidationPresets -SelectedCompilers $compilerOrder) `
    -Expected $defaultPresets

Import-MatrixResolver -Path (Join-Path $PSScriptRoot 'Run-NativeMatrix.ps1') `
    -Name 'Resolve-NativeCells'
Import-MatrixResolver -Path (Join-Path $PSScriptRoot 'Run-ContainerMatrix.ps1') `
    -Name 'Resolve-Cells'

$nativeDefaultCells = @(Resolve-NativeCells -CompilerName All -CellScope All -Operation Build)
Assert-MatrixSequence -Name 'Native default cells' `
    -Actual @($nativeDefaultCells.Preset) `
    -Expected @(
        'msvc-release-exhaustive',
        'msvc-debug-diagnostics',
        'clangcl-release-exhaustive',
        'clang-debug-coverage')
$containerDefaultCells = @(Resolve-Cells -Services @('gcc13', 'gcc14', 'clang22') -CellScope All -Operation Build)
Assert-MatrixSequence -Name 'Container default cells' `
    -Actual @($containerDefaultCells.Preset) `
    -Expected @(
        'gcc13-core-release-exhaustive',
        'gcc14-release-exhaustive',
        'clang22-release-exhaustive',
        'clang22-debug-asan-ubsan')
$nativeBenchmarkCells = @(
    Resolve-NativeCells -CompilerName All -CellScope Release -Operation BuildBenchmarks)
$containerBenchmarkCells = @(
    Resolve-Cells -Services @('gcc13', 'gcc14', 'clang22') -CellScope Release -Operation BuildBenchmarks)
$benchmarkContractCells = @(Get-ExpectedMatrixCells -Operation benchmarks)
Assert-MatrixSequence -Name 'Native benchmark Release-tree reuse' `
    -Actual @($nativeBenchmarkCells.Preset) `
    -Expected @($benchmarkContractCells | Where-Object platform -eq native | ForEach-Object preset)
Assert-MatrixSequence -Name 'Container benchmark Release-tree reuse' `
    -Actual @($containerBenchmarkCells.Preset) `
    -Expected @($benchmarkContractCells | Where-Object platform -eq container | ForEach-Object preset)
foreach ($benchmarkCell in @($nativeBenchmarkCells) + @($containerBenchmarkCells)) {
    if ($benchmarkCell.BuildProfile -ne 'Release' -or
        $benchmarkCell.Aggregate -ne 'ExhaustiveArtifacts') {
        throw "Benchmark operation does not reuse its owning Release tree: $($benchmarkCell.Preset)"
    }
}
Assert-MatrixSequence -Name 'Native consumer owners' `
    -Actual @($nativeDefaultCells | ForEach-Object { "$($_.Preset):$($_.Consumer)" }) `
    -Expected @(
        'msvc-release-exhaustive:True',
        'msvc-debug-diagnostics:False',
        'clangcl-release-exhaustive:True',
        'clang-debug-coverage:False')
Assert-MatrixSequence -Name 'Container consumer owners' `
    -Actual @($containerDefaultCells | ForEach-Object { "$($_.Preset):$($_.Consumer)" }) `
    -Expected @(
        'gcc13-core-release-exhaustive:True',
        'gcc14-release-exhaustive:True',
        'clang22-release-exhaustive:True',
        'clang22-debug-asan-ubsan:False')

Assert-MatrixSequence -Name 'clang-cl opt-in Debug cell' `
    -Actual @((Resolve-NativeCells -CompilerName ClangCl -CellScope Debug -Operation Build).Preset) `
    -Expected @('clangcl-debug-diagnostics')
if ((Resolve-NativeCells -CompilerName ClangCl -CellScope Debug -Operation Build)[0].Consumer) {
    throw 'clang-cl opt-in Debug cell unexpectedly owns an external consumer'
}

foreach ($debugSelection in @(
        @('gcc13', 'gcc13-core-debug-diagnostics'),
        @('gcc14', 'gcc14-debug-diagnostics'),
        @('clang22', 'clang22-debug-diagnostics'))) {
    Assert-MatrixSequence -Name "$($debugSelection[0]) opt-in Debug cell" `
        -Actual @((Resolve-Cells -Services @($debugSelection[0]) -CellScope Debug -Operation Build).Preset) `
        -Expected @($debugSelection[1])
    if ((Resolve-Cells -Services @($debugSelection[0]) -CellScope Debug -Operation Build)[0].Consumer) {
        throw "$($debugSelection[0]) opt-in Debug cell unexpectedly owns an external consumer"
    }
}

Assert-MatrixSequence -Name 'Native scoped aggregates' `
    -Actual @($nativeDefaultCells.Aggregate) `
    -Expected @('ExhaustiveArtifacts', 'ExhaustiveArtifacts', 'ExhaustiveArtifacts', 'ExhaustiveArtifacts')
Assert-MatrixSequence -Name 'Container scoped aggregates' `
    -Actual @($containerDefaultCells.Aggregate) `
    -Expected @('ExhaustiveArtifacts', 'ExhaustiveArtifacts', 'ExhaustiveArtifacts', 'ExhaustiveArtifacts')

$nativeContractCells = @(Resolve-NativeCells -CompilerName All -CellScope Release -Operation BuildCompilerContracts)
Assert-MatrixSequence -Name 'Native compiler-contract cells' `
    -Actual @($nativeContractCells.Preset) `
    -Expected @('msvc-compiler-contracts', 'clangcl-compiler-contracts')
Assert-MatrixSequence -Name 'Native compiler-contract aggregates' `
    -Actual @($nativeContractCells.Aggregate) `
    -Expected @('SimdLibCompilerContractArtifacts', 'SimdLibCompilerContractArtifacts')
Assert-MatrixSequence -Name 'Machine-readable native compiler contracts' `
    -Actual @($nativeContractCells.Preset) `
    -Expected @($contractCells | Where-Object platform -eq native | ForEach-Object preset)
$containerContractCells = @(Resolve-Cells -Services @('gcc13', 'gcc14', 'clang22') -CellScope Release -Operation BuildCompilerContracts)
Assert-MatrixSequence -Name 'Machine-readable container compiler contracts' `
    -Actual @($containerContractCells.Preset) `
    -Expected @($contractCells | Where-Object platform -eq container | ForEach-Object preset)
Assert-MatrixSequence -Name 'Container compiler-contract cells' `
    -Actual @($containerContractCells.Preset) `
    -Expected @('container-release-contracts', 'container-release-contracts', 'container-release-contracts')
Assert-MatrixSequence -Name 'Container compiler-contract aggregates' `
    -Actual @($containerContractCells.Aggregate) `
    -Expected @('SimdLibCompilerContractArtifacts', 'SimdLibCompilerContractArtifacts', 'SimdLibCompilerContractArtifacts')

Assert-MatrixSequence -Name 'Native compiler-contract test cells' `
    -Actual @((Resolve-NativeCells -CompilerName All -CellScope Release -Operation TestCompilerContracts).Preset) `
    -Expected @($nativeContractCells.Preset)
Assert-MatrixSequence -Name 'Container compiler-contract test cells' `
    -Actual @((Resolve-Cells -Services @('gcc13', 'gcc14', 'clang22') -CellScope Release -Operation TestCompilerContracts).Preset) `
    -Expected @($containerContractCells.Preset)

$nativeDiagnosticCells = @(Resolve-NativeCells -CompilerName All -CellScope Debug -Operation RecordCodegen)
Assert-MatrixSequence -Name 'Native optional codegen diagnostics' `
    -Actual @($nativeDiagnosticCells.Preset) `
    -Expected @('msvc-debug-codegen-diagnostic', 'clangcl-debug-codegen-diagnostic')
$diagnosticContractCells = @(Get-ExpectedMatrixCells -Operation optionalDiagnostics)
Assert-MatrixSequence -Name 'Machine-readable native diagnostics' `
    -Actual @($nativeDiagnosticCells.Preset) `
    -Expected @($diagnosticContractCells | Where-Object platform -eq native | ForEach-Object preset)
$containerDiagnosticCells = @(Resolve-Cells -Services @('gcc13', 'gcc14', 'clang22') -CellScope All -Operation RecordCodegen)
Assert-MatrixSequence -Name 'Container optional codegen diagnostics' `
    -Actual @($containerDiagnosticCells.Preset) `
    -Expected @('gcc14-debug-codegen-diagnostic', 'clang22-debug-codegen-diagnostic', 'clang22-asan-ubsan-codegen-diagnostic')
Assert-MatrixSequence -Name 'Machine-readable container diagnostics' `
    -Actual @($containerDiagnosticCells.Preset) `
    -Expected @($diagnosticContractCells | Where-Object platform -eq container | ForEach-Object preset)
foreach ($diagnosticCell in @($nativeDiagnosticCells) + @($containerDiagnosticCells)) {
    if ($diagnosticCell.Preset -in $defaultPresets -or $diagnosticCell.Aggregate -ne 'SimdLibDebugDiagnosticArtifacts') {
        throw "Optional codegen diagnostic contaminates the default matrix: $($diagnosticCell.Preset)"
    }
}

$presetPath = Join-Path (Get-PipelineRepositoryRoot) 'CMakePresets.json'
$presetDocument = Get-Content -LiteralPath $presetPath -Raw | ConvertFrom-Json
$presetByName = @{}
foreach ($preset in $presetDocument.configurePresets) {
    if ($presetByName.ContainsKey($preset.name)) { throw "Duplicate configure preset: $($preset.name)" }
    $presetByName[$preset.name] = $preset
}
if ($presetByName.ContainsKey('development-common')) {
    throw 'Retired development-common option inheritance remains available'
}
foreach ($bundleName in @('release-exhaustive-options', 'debug-diagnostics-options', 'debug-asan-ubsan-options', 'coverage-options', 'compiler-contract-options', 'codegen-diagnostic-options')) {
    $bundle = $presetByName[$bundleName]
    if (-not $bundle -or @($bundle.inherits) -notcontains 'development-base-options') {
        throw "Validation option bundle does not inherit the neutral development base: $bundleName"
    }
}
$releaseContractOptions = @(
    'SIMDLIB_BUILD_CONFIGURATION_PROBES',
    'SIMDLIB_BUILD_CONSTEXPR_PROBES',
    'SIMDLIB_BUILD_HEADER_PROBES',
    'SIMDLIB_BUILD_METHOD_FLAGS_CODEGEN_GATES'
)
foreach ($releasePresetName in @(
        'msvc-release-exhaustive', 'clangcl-release-exhaustive',
        'gcc13-core-release-exhaustive', 'gcc14-release-exhaustive',
        'clang22-release-exhaustive')) {
    foreach ($optionName in $releaseContractOptions) {
        $resolvedValue = $null
        $visited = [System.Collections.Generic.HashSet[string]]::new()
        $pending = [System.Collections.Generic.Stack[string]]::new()
        $pending.Push($releasePresetName)
        while ($pending.Count -ne 0 -and $null -eq $resolvedValue) {
            $name = $pending.Pop()
            if (-not $visited.Add($name)) { continue }
            $preset = $presetByName[$name]
            if (-not $preset) { throw "Configure preset inheritance references missing preset $name" }
            $cacheProperty = $preset.PSObject.Properties['cacheVariables']
            if ($cacheProperty -and $cacheProperty.Value.PSObject.Properties[$optionName]) {
                $resolvedValue = [string]$cacheProperty.Value.$optionName
                break
            }
            $inheritsProperty = $preset.PSObject.Properties['inherits']
            if ($inheritsProperty) {
                $parents = @($inheritsProperty.Value)
                for ($index = $parents.Count - 1; $index -ge 0; --$index) {
                    $pending.Push([string]$parents[$index])
                }
            }
        }
        if ($resolvedValue -ne 'ON') {
            throw "Release preset $releasePresetName resolves $optionName=$resolvedValue instead of ON"
        }
    }
}

foreach ($profilePreset in @{
        'msvc-release-exhaustive' = 'RELEASE'; 'msvc-debug-diagnostics' = 'DEBUG'
        'clangcl-release-exhaustive' = 'RELEASE'; 'clang-debug-coverage' = 'COVERAGE'
        'gcc13-core-release-exhaustive' = 'RELEASE'; 'gcc14-release-exhaustive' = 'RELEASE'
        'clang22-release-exhaustive' = 'RELEASE'; 'clang22-debug-asan-ubsan' = 'SANITIZER'
    }.GetEnumerator()) {
    $visited = [System.Collections.Generic.HashSet[string]]::new()
    $pending = [System.Collections.Generic.Stack[string]]::new()
    $pending.Push($profilePreset.Key)
    $resolvedProfile = $null
    while ($pending.Count -ne 0) {
        $name = $pending.Pop()
        if (-not $visited.Add($name)) { continue }
        $preset = $presetByName[$name]
        if (-not $preset) { throw "Configure preset inheritance references missing preset $name" }
        $cacheProperty = $preset.PSObject.Properties['cacheVariables']
        if ($null -eq $resolvedProfile -and $cacheProperty -and
                $cacheProperty.Value.PSObject.Properties['SIMDLIB_VALIDATION_PROFILE']) {
            $resolvedProfile = [string]$cacheProperty.Value.SIMDLIB_VALIDATION_PROFILE
        }
        $inheritsProperty = $preset.PSObject.Properties['inherits']
        if ($inheritsProperty) {
            foreach ($parent in @($inheritsProperty.Value)) { $pending.Push([string]$parent) }
        }
    }
    if ($resolvedProfile -ne $profilePreset.Value) {
        throw "Default preset $($profilePreset.Key) resolves validation profile $resolvedProfile instead of $($profilePreset.Value)"
    }
}

$compose = Get-Content -LiteralPath (Join-Path (Get-PipelineRepositoryRoot) 'compose.yml') -Raw
if ($compose -notmatch 'SIMDLIB_CONTAINER_PRESET:-container-release-contracts') {
    throw 'Compose defaults do not select the owned compiler-contract profile'
}
$runTestsSource = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'Run-Tests.ps1') -Raw
if ($runTestsSource -match "&\s*\(Join-Path[^\r\n]*Build\.ps1|--build") {
    throw 'Run-Tests contains an automatic configure or build path'
}

Write-Host "Validated $($defaultPresets.Count) default presets, four opt-in Debug cells, five codegen diagnostics, and five focused compiler-contract cells."
