<#
.SYNOPSIS
Verifies validation-matrix topology and its pipeline integrations.
#>
[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Pipeline.Common.psm1') -Force
$repositoryRoot = Get-PipelineRepositoryRoot
$matrix = Get-PipelineValidationMatrix -RepositoryRoot $repositoryRoot

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
Rejects duplicate values and returns an ordinal set.
.PARAMETER Name
Human-readable collection name.
.PARAMETER Values
Values required to be unique.
#>
function New-UniqueSet {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][AllowEmptyCollection()][object[]]$Values
    )

    $set = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::Ordinal)
    foreach ($value in $Values) {
        if (-not $set.Add([string]$value)) {
            throw "$Name duplicates $value"
        }
    }
    return ,$set
}

<#
.SYNOPSIS
Rejects two ownership collections that differ as ordinal sets.
.PARAMETER Name
Human-readable ownership name.
.PARAMETER Actual
Observed values.
.PARAMETER Expected
Required values.
#>
function Assert-SetEqual {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][AllowEmptyCollection()][object[]]$Actual,
        [Parameter(Mandatory)][AllowEmptyCollection()][object[]]$Expected
    )

    $actualSet = New-UniqueSet -Name "$Name actual" -Values $Actual
    $expectedSet = New-UniqueSet -Name "$Name expected" -Values $Expected
    if (-not $actualSet.SetEquals($expectedSet)) {
        throw "$Name mismatch. Expected '$(@($expectedSet) -join ', ')'; received '$(@($actualSet) -join ', ')'"
    }
}

<#
.SYNOPSIS
Rejects a sequence that differs from its matrix-owned execution order.
.PARAMETER Name
Human-readable sequence name.
.PARAMETER Actual
Observed sequence.
.PARAMETER Expected
Required matrix sequence.
#>
function Assert-SequenceEqual {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][AllowEmptyCollection()][object[]]$Actual,
        [Parameter(Mandatory)][AllowEmptyCollection()][object[]]$Expected
    )

    if ((@($Actual) -join '|') -ne (@($Expected) -join '|')) {
        throw "$Name execution order differs from validation-matrix.json"
    }
}

<#
.SYNOPSIS
Resolves one inherited configure-preset cache value.
.PARAMETER Name
Configure preset name.
.PARAMETER Variable
CMake cache variable to resolve.
.PARAMETER Presets
Configure-preset dictionary.
#>
function Get-ResolvedPresetValue {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Variable,
        [Parameter(Mandatory)][hashtable]$Presets
    )

    $visited = [System.Collections.Generic.HashSet[string]]::new()
    <#
    .SYNOPSIS
    Resolves the requested value from one preset and its inherited parents.
    .PARAMETER PresetName
    Configure preset currently being inspected.
    #>
    function Resolve-OnePresetValue {
        param([Parameter(Mandatory)][string]$PresetName)
        if (-not $visited.Add($PresetName)) { return $null }
        $preset = $Presets[$PresetName]
        if (-not $preset) { throw "Configure preset inheritance references missing preset $PresetName" }
        $cache = $preset.PSObject.Properties['cacheVariables']
        if ($cache -and $cache.Value.PSObject.Properties[$Variable]) {
            return [string]$cache.Value.$Variable
        }
        $inherits = $preset.PSObject.Properties['inherits']
        if ($inherits) {
            foreach ($parent in @($inherits.Value)) {
                $value = Resolve-OnePresetValue -PresetName ([string]$parent)
                if ($null -ne $value) { return $value }
            }
        }
        return $null
    }
    return Resolve-OnePresetValue -PresetName $Name
}

$categories = New-UniqueSet -Name 'targetCategories' -Values @($matrix.targetCategories)
$testOnlyOwners = New-UniqueSet -Name 'testOnlyOwners' -Values @($matrix.testOnlyOwners)
[void](New-UniqueSet -Name 'compilerOrder' -Values @($matrix.compilerOrder))
foreach ($profileProperty in $matrix.profiles.PSObject.Properties) {
    $profileName = $profileProperty.Name
    $profile = $profileProperty.Value
    $allowed = New-UniqueSet -Name "$profileName allowedTargetCategories" `
        -Values @($profile.allowedTargetCategories)
    $selected = New-UniqueSet -Name "$profileName selectedTargetCategories" `
        -Values @($profile.selectedTargetCategories)
    if (-not $selected.IsSubsetOf($allowed)) {
        throw "$profileName selects a target category it does not allow"
    }
    foreach ($category in $allowed) {
        if (-not $categories.Contains($category)) {
            throw "$profileName references unknown target category $category"
        }
    }
    [void](New-UniqueSet -Name "$profileName allowedTestOwners" `
        -Values @($profile.allowedTestOwners))
    foreach ($owner in @($profile.allowedTestOwners)) {
        if (-not $categories.Contains([string]$owner) -and
                -not $testOnlyOwners.Contains([string]$owner)) {
            throw "$profileName references unknown test owner $owner"
        }
    }
}

$operationCells = @{}
foreach ($operation in $matrix.operations.PSObject.Properties) {
    $operationCells[$operation.Name] = @(
        Get-PipelineValidationOperationCells -Operation $operation.Name)
}
$defaultBuild = @($operationCells.defaultBuild)
$defaultTests = @($operationCells.defaultTests)
Assert-SetEqual -Name 'Default build and test ownership' `
    -Actual @($defaultTests.MatrixCell) -Expected @($defaultBuild.MatrixCell)
foreach ($cell in @($operationCells.optionalDebug)) {
    if ($cell.MatrixCell -in @($defaultBuild.MatrixCell)) {
        throw "Ordinary opt-in Debug cell enters the default operation: $($cell.MatrixCell)"
    }
}
$forbiddenInstrumentedCategories = @(
    'COMPILER_CONTRACT', 'CONSTEXPR_CONTRACT', 'OPTIMIZED_CODEGEN',
    'SMOKE_VALIDATION', 'DEBUG_DIAGNOSTIC')
foreach ($profileName in @('SANITIZER', 'COVERAGE')) {
    $profile = $matrix.profiles.$profileName
    $forbidden = @(@($profile.allowedTargetCategories) |
        Where-Object { $_ -in $forbiddenInstrumentedCategories })
    if ($forbidden.Count -ne 0) {
        throw "$profileName permits forbidden categories: $($forbidden -join ', ')"
    }
}

$releaseCompilerIdentities = @($matrix.cells.PSObject.Properties |
    Where-Object { $_.Value.profile -eq 'RELEASE' -and $_.Value.instrumentation -eq 'none' } |
    ForEach-Object { $_.Value.compilerIdentity } | Select-Object -Unique)
$contractIdentities = @($operationCells.compilerContracts.compilerIdentity)
Assert-SetEqual -Name 'Compiler-contract ownership' `
    -Actual $contractIdentities -Expected $releaseCompilerIdentities
foreach ($identity in $releaseCompilerIdentities) {
    if (@($contractIdentities | Where-Object { $_ -eq $identity }).Count -ne 1) {
        throw "Compiler identity $identity does not have exactly one compiler-contract owner"
    }
}
foreach ($cellProperty in $matrix.cells.PSObject.Properties) {
    $cellId = $cellProperty.Name
    $cell = $cellProperty.Value
    if (-not $matrix.profiles.PSObject.Properties[[string]$cell.profile]) {
        throw "Validation cell references unknown profile $($cell.profile)"
    }
    if ($cell.profile -eq 'RELEASE' -and $cell.registerCapable -and
            $cell.codegenMode -ne 'ENFORCE') {
        throw "Register-capable Release cell does not enforce generated code: $($cell.preset)"
    }
    if ($cell.consumer -and ($cell.profile -ne 'RELEASE' -or
            $cellId -notin @($defaultBuild.MatrixCell))) {
        throw "Consumer ownership is not isolated to a default Release cell: $($cell.preset)"
    }
}
foreach ($cell in @($operationCells.optionalDiagnostics)) {
    if ($cell.codegenMode -ne 'RECORD' -or
            $cell.MatrixCell -in @($defaultBuild.MatrixCell)) {
        throw "Optional diagnostic is not isolated record-only evidence: $($cell.MatrixCell)"
    }
}
foreach ($cell in @($operationCells.benchmarks)) {
    if ($cell.profile -ne 'RELEASE' -or $cell.configuration -ne 'Release' -or
            $cell.aggregate -ne 'ExhaustiveArtifacts') {
        throw "Benchmark operation does not reuse an owning Release configuration: $($cell.MatrixCell)"
    }
}

$presetPath = Join-Path $repositoryRoot 'CMakePresets.json'
$presetDocument = Get-Content -LiteralPath $presetPath -Raw | ConvertFrom-Json
$presetByName = @{}
foreach ($preset in $presetDocument.configurePresets) {
    if ($presetByName.ContainsKey($preset.name)) { throw "Duplicate configure preset: $($preset.name)" }
    $presetByName[$preset.name] = $preset
}
$buildPresetByName = @{}
foreach ($preset in $presetDocument.buildPresets) {
    if ($buildPresetByName.ContainsKey($preset.name)) { throw "Duplicate build preset: $($preset.name)" }
    $buildPresetByName[$preset.name] = $preset
}
foreach ($cellProperty in $matrix.cells.PSObject.Properties) {
    $cellId = $cellProperty.Name
    $cell = $cellProperty.Value
    if (-not $presetByName.ContainsKey([string]$cell.preset)) {
        throw "Validation cell $cellId references missing configure preset $($cell.preset)"
    }
    $profile = Get-ResolvedPresetValue -Name $cell.preset `
        -Variable SIMDLIB_VALIDATION_PROFILE -Presets $presetByName
    if ($profile -ne $cell.profile) {
        throw "Preset $($cell.preset) resolves profile $profile instead of $($cell.profile)"
    }
    $configuration = Get-ResolvedPresetValue -Name $cell.preset `
        -Variable CMAKE_BUILD_TYPE -Presets $presetByName
    if (-not $configuration) {
        $configuration = Get-ResolvedPresetValue -Name $cell.preset `
            -Variable CMAKE_CONFIGURATION_TYPES -Presets $presetByName
    }
    if ($configuration -ne $cell.configuration) {
        throw "Preset $($cell.preset) resolves configuration $configuration instead of $($cell.configuration)"
    }
    $codegenMode = Get-ResolvedPresetValue -Name $cell.preset `
        -Variable SIMDLIB_REGISTER_CODEGEN_MODE -Presets $presetByName
    if ($codegenMode -ne $cell.codegenMode) {
        throw "Preset $($cell.preset) resolves generated-code mode $codegenMode instead of $($cell.codegenMode)"
    }
    if ($cell.instrumentation -eq 'asan-ubsan') {
        $sanitizerFlags = Get-ResolvedPresetValue -Name $cell.preset `
            -Variable CMAKE_CXX_FLAGS_DEBUG -Presets $presetByName
        if ($sanitizerFlags -notmatch '-fsanitize=address,undefined') {
            throw "Preset $($cell.preset) does not resolve ASan and UBSan instrumentation"
        }
    } elseif ($cell.instrumentation -eq 'coverage') {
        if ((Get-ResolvedPresetValue -Name $cell.preset `
                -Variable SIMDLIB_ENABLE_COVERAGE -Presets $presetByName) -ne 'ON') {
            throw "Preset $($cell.preset) does not resolve coverage instrumentation"
        }
    }
    $buildPreset = $buildPresetByName[[string]$cell.preset]
    if ($buildPreset -and ($buildPreset.configurePreset -ne $cell.preset -or
            @($buildPreset.targets) -notcontains $cell.aggregate)) {
        throw "Build preset $($cell.preset) disagrees with cell aggregate $($cell.aggregate)"
    }
}
foreach ($cell in @($operationCells.benchmarks)) {
    $benchmarkPreset = @($presetDocument.buildPresets | Where-Object {
            $_.configurePreset -eq $cell.preset -and
            @($_.targets) -contains 'BenchmarkArtifacts'
        })
    if ($benchmarkPreset.Count -ne 1) {
        throw "Release cell $($cell.MatrixCell) does not have exactly one benchmark aggregate preset"
    }
}

$artifactAggregates = Get-Content -LiteralPath (
    Join-Path $repositoryRoot 'cmake/development/ArtifactAggregates.cmake') -Raw
if ($artifactAggregates -notmatch 'file\(READ "\$\{simdlib_validation_matrix\}"' -or
        $artifactAggregates -match 'simdlib_profile_allowed_RELEASE\s') {
    throw 'CMake development profiles do not consume validation-matrix.json directly'
}

Import-MatrixResolver -Path (Join-Path $PSScriptRoot 'Run-NativeMatrix.ps1') `
    -Name Resolve-NativeCells
Import-MatrixResolver -Path (Join-Path $PSScriptRoot 'Run-ContainerMatrix.ps1') `
    -Name Resolve-Cells
$services = @(Get-PipelineValidationCompilers -Platform container |
    ForEach-Object { $_.ToLowerInvariant() })
$resolverCases = @(
    [pscustomobject]@{ Name='native default'; Actual=@(Resolve-NativeCells -CompilerName All -CellScope All -Operation Build); Expected=@($defaultBuild | Where-Object platform -eq native) },
    [pscustomobject]@{ Name='container default'; Actual=@(Resolve-Cells -Services $services -CellScope All -Operation Build); Expected=@($defaultBuild | Where-Object platform -eq container) },
    [pscustomobject]@{ Name='native benchmarks'; Actual=@(Resolve-NativeCells -CompilerName All -CellScope Release -Operation BuildBenchmarks); Expected=@($operationCells.benchmarks | Where-Object platform -eq native) },
    [pscustomobject]@{ Name='container benchmarks'; Actual=@(Resolve-Cells -Services $services -CellScope Release -Operation BuildBenchmarks); Expected=@($operationCells.benchmarks | Where-Object platform -eq container) },
    [pscustomobject]@{ Name='native compiler contracts'; Actual=@(Resolve-NativeCells -CompilerName All -CellScope Release -Operation BuildCompilerContracts); Expected=@($operationCells.compilerContracts | Where-Object platform -eq native) },
    [pscustomobject]@{ Name='container compiler contracts'; Actual=@(Resolve-Cells -Services $services -CellScope Release -Operation BuildCompilerContracts); Expected=@($operationCells.compilerContracts | Where-Object platform -eq container) },
    [pscustomobject]@{ Name='native diagnostics'; Actual=@(Resolve-NativeCells -CompilerName All -CellScope Debug -Operation RecordCodegen); Expected=@($operationCells.optionalDiagnostics | Where-Object platform -eq native) },
    [pscustomobject]@{ Name='container diagnostics'; Actual=@(Resolve-Cells -Services $services -CellScope All -Operation RecordCodegen); Expected=@($operationCells.optionalDiagnostics | Where-Object platform -eq container) }
)
foreach ($case in $resolverCases) {
    Assert-SequenceEqual -Name $case.Name -Actual @($case.Actual.MatrixCell) `
        -Expected @($case.Expected.MatrixCell)
    foreach ($resolved in $case.Actual) {
        $canonical = $matrix.cells.PSObject.Properties[[string]$resolved.MatrixCell].Value
        if ($resolved.Preset -ne $canonical.preset -or
            $resolved.BuildProfile -ne $canonical.configuration -or
            $resolved.Instrumentation -ne $canonical.instrumentation -or
            $resolved.Aggregate -ne $canonical.aggregate -or
            $resolved.CodegenMode -ne $canonical.codegenMode -or
            $resolved.Consumer -ne $canonical.consumer) {
            throw "$($case.Name) resolver disagrees with cell $($resolved.MatrixCell)"
        }
    }
}

$compose = Get-Content -LiteralPath (Join-Path $repositoryRoot 'compose.yml') -Raw
$contractPresets = @($operationCells.compilerContracts |
    Where-Object platform -eq container | ForEach-Object preset | Select-Object -Unique)
if ($contractPresets.Count -ne 1 -or
        $compose -notmatch [regex]::Escape("SIMDLIB_CONTAINER_PRESET:-$($contractPresets[0])")) {
    throw 'Docker Compose does not select the matrix-owned container compiler-contract operation'
}
$runTestsSource = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'Run-Tests.ps1') -Raw
if ($runTestsSource -match '(?i)&\s*\(Join-Path[^\r\n]*Build\.ps1|--build|''-Action'',\s*''Build''|cmake\s+--preset') {
    throw 'Run-Tests contains a configure or build path'
}

[void](Get-PipelineToolingInputs -RepositoryRoot $repositoryRoot)
Write-Host "Validation matrix invariants passed for $(@($matrix.cells.PSObject.Properties).Count) cells and $(@($matrix.operations.PSObject.Properties).Count) operations."