<#
.SYNOPSIS
Builds the requested complete SimdLib validation artifact matrix.
.DESCRIPTION
Scope must be explicit so a host cannot silently omit required native or
container cells. The command builds validation artifacts and records an exact
manifest receipt consumed by Run-Tests.ps1. Benchmark compilation is owned
exclusively by Build-Benchmarks.ps1.
#>
[CmdletBinding()]
param(
    [ValidateSet('', 'All', 'Native', 'Containers')]
    [string]$Scope = '',
    [ValidateSet('All', 'Msvc', 'ClangCl', 'ClangCoverage', 'Gcc13', 'Gcc14', 'Clang22')]
    [string[]]$Compiler = @('All')
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
function Resolve-BuildSelection {
    $nativeNames = @('Msvc', 'ClangCl', 'ClangCoverage')
    $containerNames = @('Gcc13', 'Gcc14', 'Clang22')
    if (-not $Scope) { throw 'Build scope is required. Use -Scope All, -Scope Native, or -Scope Containers.' }
    if ('All' -in $Compiler -and $Compiler.Count -ne 1) { throw 'Compiler All cannot be combined with another compiler filter.' }
    if ($Compiler -contains 'All') {
        $selected = switch ($Scope) {
            'Native' { $nativeNames }
            'Containers' { $containerNames }
            default { $nativeNames + $containerNames }
        }
    } else {
        $selected = @($Compiler | Select-Object -Unique)
    }
    if ($Scope -eq 'Native' -and @($selected | Where-Object { $_ -in $containerNames }).Count) { throw 'Container compiler filters are invalid for Native scope.' }
    if ($Scope -eq 'Containers' -and @($selected | Where-Object { $_ -in $nativeNames }).Count) { throw 'Native compiler filters are invalid for Containers scope.' }
    return @($selected)
}

<#
.SYNOPSIS
Records the exact completed validation manifests produced by this build.
.PARAMETER SelectedCompilers
Canonical compiler selection.
.PARAMETER RepositoryAuditPath
Machine-readable repository audit result for the current source digest.
#>
function Write-BuildReceipt {
    param(
        [Parameter(Mandatory)][string[]]$SelectedCompilers,
        [Parameter(Mandatory)][string]$RepositoryAuditPath
    )
    $currentSourceDigest = Get-PipelineSourceDigest -RepositoryRoot $repositoryRoot
    $repositoryAuditEntry = New-PipelineRepositoryAuditEntry `
        -RepositoryRoot $repositoryRoot `
        -AuditPath $RepositoryAuditPath `
        -ExpectedSourceDigest $currentSourceDigest
    $expectedPresets = @(Get-PipelineDefaultValidationPresets -SelectedCompilers $SelectedCompilers)
    $manifestFiles = @(Get-ChildItem -LiteralPath $pipelineRoot -Filter 'validation-build.manifest' -File -Recurse -ErrorAction SilentlyContinue)
    $entries = [System.Collections.Generic.List[object]]::new()
    foreach ($preset in $expectedPresets) {
        $matches = @($manifestFiles | Where-Object {
                try { (Read-PipelineManifest -Path $_.FullName).preset -eq $preset } catch { $false }
            } | Sort-Object LastWriteTimeUtc -Descending)
        if ($matches.Count -eq 0) { throw "Build completed without the required manifest for preset $preset" }
        $manifest = Read-PipelineManifest -Path $matches[0].FullName
        if ($manifest.operation -ne 'build-validation' -or $manifest.status -ne 'complete') { throw "Incomplete validation manifest for preset $preset" }
        if ($manifest.source_digest -ne $currentSourceDigest) { throw "Validation manifest has a stale source digest for preset $preset" }
        if ($manifest.aggregate -ne 'ExhaustiveArtifacts') { throw "Default validation manifest has an unexpected scoped aggregate for preset $preset" }
        foreach ($requiredManifestField in @(
                'target_inventory_sha256',
                'main_test_inventory_sha256',
                'matrix_cell',
                'matrix_contract_sha256',
                'validation_inventory_audit_sha256',
                'build_profile',
                'sanitizer',
                'codegen_mode',
                'consumer_scope'
            )) {
            if (-not $manifest.ContainsKey($requiredManifestField) -or [string]::IsNullOrWhiteSpace($manifest[$requiredManifestField])) {
                throw "Validation manifest omits required provenance $requiredManifestField for preset $preset"
            }
        }
        foreach ($requiredInventoryField in @(
                'target_inventory_sha256',
                'main_test_inventory_sha256',
                'matrix_cell',
                'matrix_contract_sha256',
                'validation_inventory_audit_sha256'
            )) {
            if ($manifest[$requiredInventoryField] -eq 'none') {
                throw "Validation manifest has no required $requiredInventoryField for preset $preset"
            }
        }
        $inventoryAuditPath = Resolve-PipelineArtifactPath `
            -RepositoryRoot $repositoryRoot `
            -Path ([string]$manifest.validation_inventory_audit)
        if (-not (Test-Path -LiteralPath $inventoryAuditPath -PathType Leaf)) {
            throw "Validation manifest inventory audit is missing for preset $preset`: $inventoryAuditPath"
        }
        $inventoryAuditHash = (
            Get-FileHash -LiteralPath $inventoryAuditPath -Algorithm SHA256
        ).Hash.ToLowerInvariant()
        if ($inventoryAuditHash -ne $manifest.validation_inventory_audit_sha256) {
            throw "Validation manifest inventory audit changed for preset $preset`: $inventoryAuditPath"
        }
        $entries.Add([ordered]@{
                preset = $preset
                path = [System.IO.Path]::GetRelativePath($repositoryRoot, $matches[0].FullName).Replace('\', '/')
                sha256 = (Get-FileHash -LiteralPath $matches[0].FullName -Algorithm SHA256).Hash.ToLowerInvariant()
                fingerprint = $manifest.fingerprint_sha256
                sourceDigest = $manifest.source_digest
                aggregate = $manifest.aggregate
                matrixCell = $manifest.matrix_cell
                targetInventorySha256 = $manifest.target_inventory_sha256
                testInventorySha256 = $manifest.main_test_inventory_sha256
                matrixContractSha256 = $manifest.matrix_contract_sha256
                inventoryAuditSha256 = $manifest.validation_inventory_audit_sha256
                configuration = $manifest.build_profile
                instrumentation = $manifest.sanitizer
                generatedCodeMode = $manifest.codegen_mode
                consumerScope = $manifest.consumer_scope
            })
    }
    $selectionText = "$Scope|$($SelectedCompilers -join ',')"
    $selectionId = (Get-PipelineTextDigest -Text $selectionText).Substring(0, 16)
    $receiptPath = Join-Path $pipelineRoot "provenance/build-$selectionId.json"
    $document = [ordered]@{
        schema = 'simdlib.unified-build-receipt.v4'; status = 'complete'; scope = $Scope
        compilers = @($SelectedCompilers); sourceDigest = $currentSourceDigest
        sourceRevision = Get-PipelineRevision -RepositoryRoot $repositoryRoot
        repositoryAudit = $repositoryAuditEntry
        manifests = $entries.ToArray()
    }
    Set-PipelineTextFile -Path $receiptPath -Content ($document | ConvertTo-Json -Depth 6)
    Set-PipelineTextFile -Path (Join-Path $pipelineRoot 'provenance/latest-build-receipt.txt') -Content ([System.IO.Path]::GetRelativePath($repositoryRoot, $receiptPath).Replace('\', '/'))
    return $receiptPath
}

$selectedCompilers = @(Resolve-BuildSelection)
if ($Scope -in @('All', 'Native') -and -not $IsWindows) { throw 'Native scope requires a Windows x64 host with Visual Studio C++ tools and LLVM 22.' }
$auditSourceDigest = Get-PipelineSourceDigest -RepositoryRoot $repositoryRoot
$repositoryAuditPath = Join-Path $pipelineRoot "provenance/repository-audit-$($auditSourceDigest.Substring(0, 16)).json"
& (Join-Path $PSScriptRoot 'Run-RepositoryAudit.ps1') -ResultPath $repositoryAuditPath
if ($LASTEXITCODE -ne 0) { throw 'Repository audit operation failed.' }

$operations = [System.Collections.Generic.List[object]]::new()
foreach ($name in @($selectedCompilers | Where-Object { $_ -in @('Msvc', 'ClangCl', 'ClangCoverage') })) {
    $operations.Add([pscustomobject]@{
            Id = "native-$($name.ToLowerInvariant())"; Script = Join-Path $PSScriptRoot 'Run-NativeMatrix.ps1'
            Arguments = @('-Action', 'Build', '-Compiler', $name, '-Cell', 'All')
        })
}
$containerCompilers = @($selectedCompilers | Where-Object { $_ -in @('Gcc13', 'Gcc14', 'Clang22') })
if ($containerCompilers.Count -eq 3) {
    $operations.Add([pscustomobject]@{ Id = 'containers'; Script = Join-Path $PSScriptRoot 'Run-ContainerMatrix.ps1'; Arguments = @('-Action', 'Build', '-Compiler', 'All', '-Cell', 'All') })
} else {
    foreach ($name in $containerCompilers) {
        $operations.Add([pscustomobject]@{ Id = "container-$($name.ToLowerInvariant())"; Script = Join-Path $PSScriptRoot 'Run-ContainerMatrix.ps1'; Arguments = @('-Action', 'Build', '-Compiler', $name, '-Cell', 'All') })
    }
}
$logDirectory = Join-Path $pipelineRoot "logs/$(Get-Date -Format 'yyyyMMdd-HHmmssfff')-build-$PID"
Invoke-PipelineChildOperations -Operations $operations.ToArray() -LogDirectory $logDirectory

$receipt = Write-BuildReceipt -SelectedCompilers $selectedCompilers -RepositoryAuditPath $repositoryAuditPath
Write-Host "Unified build passed. Receipt: $receipt"
