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

Write-Host "Validated $($defaultPresets.Count) default validation presets and four opt-in ordinary Debug cells."
