<#
.SYNOPSIS
Runs the requested SimdLib validation matrix from an exact build receipt.
.DESCRIPTION
The command validates the exact receipt produced by Build.ps1 and then runs only
test operations. A missing, stale, incomplete, or mismatched receipt is rejected
without configuring or rebuilding any target.
#>
[CmdletBinding()]
param(
    [ValidateSet('All', 'Native', 'Containers')]
    [string]$Scope = 'All',
    [ValidateSet('All', 'Msvc', 'ClangCl', 'ClangCoverage', 'Gcc13', 'Gcc14', 'Clang22')]
    [string[]]$Compiler = @('All'),
    [string]$TestRegex = '',
    [string]$TestLabel = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Pipeline.Common.psm1') -Force
$repositoryRoot = Get-PipelineRepositoryRoot
$pipelineRoot = Join-Path $repositoryRoot 'out/pipeline'

<#
.SYNOPSIS
Expands compiler filters and enforces their platform scope.
#>
function Resolve-TestSelection {
    $nativeNames = @('Msvc', 'ClangCl', 'ClangCoverage')
    $containerNames = @('Gcc13', 'Gcc14', 'Clang22')
    if ('All' -in $Compiler -and $Compiler.Count -ne 1) { throw 'Compiler All cannot be combined with another compiler filter.' }
    if ($Compiler -contains 'All') {
        $selected = switch ($Scope) {
            'Native' { $nativeNames }
            'Containers' { $containerNames }
            default { $nativeNames + $containerNames }
        }
    } else { $selected = @($Compiler | Select-Object -Unique) }
    if ($Scope -eq 'Native' -and @($selected | Where-Object { $_ -in $containerNames }).Count) { throw 'Container compiler filters are invalid for Native scope.' }
    if ($Scope -eq 'Containers' -and @($selected | Where-Object { $_ -in $nativeNames }).Count) { throw 'Native compiler filters are invalid for Containers scope.' }
    return @($selected)
}

<#
.SYNOPSIS
Validates the exact build receipt required by this test selection.
.PARAMETER SelectedCompilers
Canonical compiler selection.
#>
function Assert-BuildReceipt {
    param([Parameter(Mandatory)][string[]]$SelectedCompilers)
    $selectionText = "$Scope|$($SelectedCompilers -join ',')"
    $selectionId = (Get-PipelineTextDigest -Text $selectionText).Substring(0, 16)
    $receiptPath = Join-Path $pipelineRoot "provenance/build-$selectionId.json"
    if (-not (Test-Path -LiteralPath $receiptPath -PathType Leaf)) { throw "Required unified build receipt is missing: $receiptPath" }
    $receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json
    if ($receipt.schema -ne 'simdlib.unified-build-receipt.v4' -or $receipt.status -ne 'complete' -or $receipt.scope -ne $Scope) {
        throw "Unified build receipt is incomplete or incompatible: $receiptPath"
    }
    $receiptCompilers = @($receipt.compilers)
    if (($receiptCompilers -join ',') -ne ($SelectedCompilers -join ',')) { throw "Unified build receipt compiler set does not match the requested tests: $receiptPath" }
    $currentDigest = Get-PipelineSourceDigest -RepositoryRoot $repositoryRoot
    $matrixPath = Join-Path $repositoryRoot 'tools/validation-matrix.json'
    $matrixHash = (Get-FileHash -LiteralPath $matrixPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $matrix = Get-Content -LiteralPath $matrixPath -Raw | ConvertFrom-Json
    if ($receipt.sourceDigest -ne $currentDigest) { throw "Unified build receipt is stale for current source inputs: $receiptPath" }
    [void](Assert-PipelineRepositoryAuditEntry `
        -RepositoryRoot $repositoryRoot `
        -Entry $receipt.repositoryAudit `
        -ExpectedSourceDigest $currentDigest)
    $expectedPresets = @(Get-PipelineDefaultValidationPresets -SelectedCompilers $SelectedCompilers | Sort-Object)
    $receiptPresets = @($receipt.manifests | ForEach-Object { $_.preset } | Sort-Object)
    if (($receiptPresets -join ',') -ne ($expectedPresets -join ',')) { throw "Unified build receipt manifest set does not exactly match requested test cells: $receiptPath" }
    foreach ($entry in $receipt.manifests) {
        $manifestPath = Join-Path $repositoryRoot ([string]$entry.path)
        if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw "Receipt manifest is missing: $manifestPath" }
        $hash = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($hash -ne $entry.sha256) { throw "Receipt manifest changed after the unified build: $manifestPath" }
        $manifest = Read-PipelineManifest -Path $manifestPath
        if ($manifest.source_digest -ne $currentDigest -or $manifest.source_digest -ne $entry.sourceDigest) {
            throw "Receipt manifest source digest does not match the unified receipt and current sources: $manifestPath"
        }
        $provenancePairs = @{
            matrix_cell = 'matrixCell'
            aggregate = 'aggregate'; target_inventory_sha256 = 'targetInventorySha256'
            main_test_inventory_sha256 = 'testInventorySha256'; build_profile = 'configuration'
            sanitizer = 'instrumentation'; codegen_mode = 'generatedCodeMode'; consumer_scope = 'consumerScope'
            matrix_contract_sha256 = 'matrixContractSha256'
            validation_inventory_audit_sha256 = 'inventoryAuditSha256'
        }
        foreach ($manifestKey in $provenancePairs.Keys) {
            $receiptValue = [string]$entry.($provenancePairs[$manifestKey])
            if ($manifest[$manifestKey] -ne $receiptValue) {
                throw "Receipt manifest provenance $manifestKey does not match the unified receipt: $manifestPath"
            }
        }
        if ($manifest.matrix_contract_sha256 -ne $matrixHash) {
            throw "Receipt manifest uses a stale validation matrix contract: $manifestPath"
        }
        $inventoryAuditPath = Resolve-PipelineArtifactPath `
            -RepositoryRoot $repositoryRoot `
            -Path ([string]$manifest.validation_inventory_audit)
        if (-not (Test-Path -LiteralPath $inventoryAuditPath -PathType Leaf)) {
            throw "Receipt validation inventory audit is missing: $inventoryAuditPath"
        }
        $inventoryAuditHash = (Get-FileHash -LiteralPath $inventoryAuditPath -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($inventoryAuditHash -ne $manifest.validation_inventory_audit_sha256) {
            throw "Receipt validation inventory audit changed after the build: $inventoryAuditPath"
        }
        $inventoryAudit = Get-Content -LiteralPath $inventoryAuditPath -Raw | ConvertFrom-Json
        $matrixCell = $matrix.cells.PSObject.Properties[[string]$manifest.matrix_cell]
        if (-not $matrixCell -or
            $inventoryAudit.schema -ne 'simdlib.validation-inventory-audit.v1' -or
            $inventoryAudit.status -ne 'complete' -or
            $inventoryAudit.cell -ne $manifest.matrix_cell -or
            $inventoryAudit.profile -ne $matrixCell.Value.profile) {
            throw "Receipt validation inventory audit is category-incompatible: $inventoryAuditPath"
        }
        if ($manifest.aggregate -ne 'ExhaustiveArtifacts' -or
            $manifest.target_inventory_sha256 -eq 'none' -or
            $manifest.main_test_inventory_sha256 -eq 'none') {
            throw "Receipt manifest does not cover the required default target and test inventories: $manifestPath"
        }
    }
    return $receiptPath
}

$selectedCompilers = @(Resolve-TestSelection)
$receiptPath = Assert-BuildReceipt -SelectedCompilers $selectedCompilers

$operations = [System.Collections.Generic.List[object]]::new()
foreach ($name in @($selectedCompilers | Where-Object { $_ -in @('Msvc', 'ClangCl', 'ClangCoverage') })) {
    $arguments = @('-Action', 'Test', '-Compiler', $name, '-Cell', 'All')
    if ($TestRegex) { $arguments += @('-TestRegex', $TestRegex) }
    if ($TestLabel) { $arguments += @('-TestLabel', $TestLabel) }
    $operations.Add([pscustomobject]@{ Id = "native-$($name.ToLowerInvariant())"; Script = Join-Path $PSScriptRoot 'Run-NativeMatrix.ps1'; Arguments = $arguments })
}
$containerCompilers = @($selectedCompilers | Where-Object { $_ -in @('Gcc13', 'Gcc14', 'Clang22') })
if ($containerCompilers.Count -eq 3) {
    $arguments = @('-Action', 'Test', '-Compiler', 'All', '-Cell', 'All')
    if ($TestRegex) { $arguments += @('-TestRegex', $TestRegex) }
    if ($TestLabel) { $arguments += @('-TestLabel', $TestLabel) }
    $operations.Add([pscustomobject]@{ Id = 'containers'; Script = Join-Path $PSScriptRoot 'Run-ContainerMatrix.ps1'; Arguments = $arguments })
} else {
    foreach ($name in $containerCompilers) {
        $arguments = @('-Action', 'Test', '-Compiler', $name, '-Cell', 'All')
        if ($TestRegex) { $arguments += @('-TestRegex', $TestRegex) }
        if ($TestLabel) { $arguments += @('-TestLabel', $TestLabel) }
        $operations.Add([pscustomobject]@{ Id = "container-$($name.ToLowerInvariant())"; Script = Join-Path $PSScriptRoot 'Run-ContainerMatrix.ps1'; Arguments = $arguments })
    }
}
$logDirectory = Join-Path $pipelineRoot "logs/$(Get-Date -Format 'yyyyMMdd-HHmmssfff')-run-tests-$PID"
Invoke-PipelineChildOperations -Operations $operations.ToArray() -LogDirectory $logDirectory
Write-Host "Unified tests passed. Build receipt: $receiptPath"
