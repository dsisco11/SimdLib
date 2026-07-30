<#
.SYNOPSIS
Audits source-revision-wide repository contracts once and records the result.
.DESCRIPTION
The result is keyed by the canonical pipeline source digest and can be reused
by every compiler and configuration cell represented by one unified build.
.PARAMETER ResultPath
Optional explicit machine-readable result path.
#>
[CmdletBinding()]
param([string]$ResultPath = '')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Pipeline.Common.psm1') -Force

$repositoryRoot = Get-PipelineRepositoryRoot
$sourceDigest = Get-PipelineSourceDigest -RepositoryRoot $repositoryRoot
$sourceRevision = Get-PipelineRevision -RepositoryRoot $repositoryRoot
if (-not $ResultPath) {
    $ResultPath = Join-Path $repositoryRoot "out/pipeline/provenance/repository-audit-$($sourceDigest.Substring(0, 16)).json"
}
$ResultPath = [System.IO.Path]::GetFullPath($ResultPath)

<#
.SYNOPSIS
Returns whether an existing result exactly owns the current source digest.
#>
function Test-CurrentRepositoryAudit {
    if (-not (Test-Path -LiteralPath $ResultPath -PathType Leaf)) { return $false }
    try {
        $result = Get-Content -LiteralPath $ResultPath -Raw | ConvertFrom-Json
        return $result.schema -eq 'simdlib.repository-audit.v1' -and
            $result.status -eq 'complete' -and
            $result.sourceDigest -eq $sourceDigest -and
            $result.sourceRevision -eq $sourceRevision
    } catch {
        return $false
    }
}

if (-not (Test-CurrentRepositoryAudit)) {
    & (Join-Path $PSScriptRoot 'Verify-ValidationMatrix.ps1')
    & (Join-Path $PSScriptRoot 'Test-ValidationPipeline.ps1')
    & (Join-Path $PSScriptRoot 'Test-MethodFlagsSourceAudit.ps1')
    & (Join-Path $PSScriptRoot 'Generate-MethodFlagsInventory.ps1') -Verify
    $legacyInventoryPath = Join-Path $repositoryRoot 'docs/MethodFlagsInventory.csv'
    $registerOnlyInventoryPath = Join-Path $repositoryRoot 'docs/MethodFlagsRegisterOnly.csv'
    $legacyInventoryHash = (Get-FileHash -LiteralPath $legacyInventoryPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $registerOnlyInventoryHash = (Get-FileHash -LiteralPath $registerOnlyInventoryPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $legacyInventoryCount = @(Import-Csv -LiteralPath $legacyInventoryPath).Count
    $registerOnlyInventoryCount = @(Import-Csv -LiteralPath $registerOnlyInventoryPath).Count
    $cmake = (Get-Command cmake -ErrorAction Stop).Source
    $arguments = @(
        "-DSOURCE_DIRECTORY=$repositoryRoot",
        "-DSOURCE_DIGEST=$sourceDigest",
        "-DSOURCE_REVISION=$sourceRevision",
        "-DMETHOD_FLAGS_LEGACY_COUNT=$legacyInventoryCount",
        "-DMETHOD_FLAGS_LEGACY_SHA256=$legacyInventoryHash",
        "-DMETHOD_FLAGS_REGISTER_ONLY_COUNT=$registerOnlyInventoryCount",
        "-DMETHOD_FLAGS_REGISTER_ONLY_SHA256=$registerOnlyInventoryHash",
        "-DRESULT_FILE=$ResultPath",
        '-P', (Join-Path $repositoryRoot 'cmake/AuditRepository.cmake')
    )
    & $cmake @arguments | Out-Host
    if ($LASTEXITCODE -ne 0) { throw 'Repository audit failed.' }
}

if (-not (Test-CurrentRepositoryAudit)) {
    throw "Repository audit did not produce a current result: $ResultPath"
}
Write-Host "Repository audit result: $ResultPath"
