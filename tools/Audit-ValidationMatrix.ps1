<#
.SYNOPSIS
Audits one generated validation cell against the canonical matrix contract.
.DESCRIPTION
The command reports duplicate targets or tests, missing ownership, and profile
membership that differs from tools/validation-matrix.json.
.PARAMETER Cell
Canonical cell identifier from tools/validation-matrix.json.
.PARAMETER BuildDirectory
Configured CMake build tree containing generated ownership inventories.
.PARAMETER Configuration
Optional multi-config CTest configuration.
.PARAMETER ResultPath
Optional machine-readable audit result path.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Cell,
    [Parameter(Mandatory)][string]$BuildDirectory,
    [string]$Configuration = '',
    [string]$ResultPath = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Pipeline.Common.psm1') -Force

$repositoryRoot = Get-PipelineRepositoryRoot
$cmake = (Get-Command cmake -ErrorAction Stop).Source
$ctest = (Get-Command ctest -ErrorAction Stop).Source
$BuildDirectory = [System.IO.Path]::GetFullPath($BuildDirectory)
if (-not $ResultPath) {
    $ResultPath = Join-Path $BuildDirectory 'validation-inventory.audit.json'
}
$ResultPath = [System.IO.Path]::GetFullPath($ResultPath)
$arguments = @(
    "-DMATRIX_FILE=$(Join-Path $PSScriptRoot 'validation-matrix.json')",
    "-DCELL_ID=$Cell",
    "-DBUILD_DIRECTORY=$BuildDirectory",
    "-DCMAKE_CTEST_COMMAND=$ctest",
    "-DRESULT_FILE=$ResultPath"
)
if ($Configuration) {
    $arguments += "-DCONFIGURATION=$Configuration"
}
$arguments += @(
    '-P',
    (Join-Path $repositoryRoot 'cmake/AuditValidationInventory.cmake')
)

& $cmake @arguments | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Validation matrix inventory audit failed for $Cell"
}
if (-not (Test-Path -LiteralPath $ResultPath -PathType Leaf)) {
    throw "Validation matrix inventory audit did not produce $ResultPath"
}

$result = Get-Content -LiteralPath $ResultPath -Raw | ConvertFrom-Json
Write-Host (
    "Matrix audit passed: cell={0} profile={1} targets={2} selected={3} tests={4}" -f
    $result.cell,
    $result.profile,
    $result.targets,
    $result.selectedTargets,
    $result.tests)
