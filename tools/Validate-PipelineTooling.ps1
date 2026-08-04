<#
.SYNOPSIS
Validates pipeline tooling once per reviewed tooling/configuration digest.
.PARAMETER ResultPath
Optional explicit machine-readable result path.
#>
[CmdletBinding()]
param([string]$ResultPath = '')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Pipeline.Common.psm1') -Force
$repositoryRoot = Get-PipelineRepositoryRoot
$toolingDigest = Get-PipelineToolingDigest -RepositoryRoot $repositoryRoot
if (-not $ResultPath) {
    $ResultPath = Join-Path $repositoryRoot (
        "out/pipeline/provenance/pipeline-validation-$($toolingDigest.Substring(0, 16)).json")
}
$ResultPath = [System.IO.Path]::GetFullPath($ResultPath)

<#
.SYNOPSIS
Returns whether an existing result owns the current tooling digest.
#>
function Test-CurrentPipelineValidation {
    if (-not (Test-Path -LiteralPath $ResultPath -PathType Leaf)) { return $false }
    try {
        $result = Get-Content -LiteralPath $ResultPath -Raw | ConvertFrom-Json
        return $result.schema -eq 'simdlib.pipeline-tooling-validation.v1' -and
            $result.status -eq 'complete' -and
            $result.toolingDigest -eq $toolingDigest
    } catch {
        return $false
    }
}

if (-not (Test-CurrentPipelineValidation)) {
    & (Join-Path $PSScriptRoot 'Verify-ValidationMatrix.ps1')
    & (Join-Path $PSScriptRoot 'Test-ValidationPipeline.ps1')
    $matrixPath = Join-Path $PSScriptRoot 'validation-matrix.json'
    $document = [ordered]@{
        schema = 'simdlib.pipeline-tooling-validation.v1'
        status = 'complete'
        toolingDigest = $toolingDigest
        matrixSha256 = (Get-FileHash -LiteralPath $matrixPath -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    Set-PipelineTextFile -Path $ResultPath -Content (
        $document | ConvertTo-Json -Depth 4)
}
if (-not (Test-CurrentPipelineValidation)) {
    throw "Pipeline-tooling validation did not produce a current result: $ResultPath"
}
Write-Host "Pipeline-tooling validation result: $ResultPath"