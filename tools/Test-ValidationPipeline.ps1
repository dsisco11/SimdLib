<#
.SYNOPSIS
Runs focused validation-matrix, inventory, receipt, and no-rebuild regressions.
#>
[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Pipeline.Common.psm1') -Force

$repositoryRoot = Get-PipelineRepositoryRoot
$cmake = (Get-Command cmake -ErrorAction Stop).Source
$ctest = (Get-Command ctest -ErrorAction Stop).Source
$matrixPath = Join-Path $PSScriptRoot 'validation-matrix.json'
$auditScript = Join-Path $repositoryRoot 'cmake/AuditValidationInventory.cmake'
$regressionRoot = Join-Path $repositoryRoot "out/pipeline/regression-$PID"

<#
.SYNOPSIS
Imports one function definition without executing its owning script.
.PARAMETER Path
PowerShell script containing the function.
.PARAMETER Name
Function name to import into this script scope.
#>
function Import-ValidationFunction {
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
Writes one synthetic CTest JSON inventory.
.PARAMETER Path
Destination JSON path.
.PARAMETER Tests
Test objects containing name and owner.
#>
function Write-SyntheticTestInventory {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][AllowEmptyCollection()][object[]]$Tests
    )

    $testEntries = @(
        foreach ($test in $Tests) {
            $labels = if ($test.Owner) {
                @("SIMDLIB_OWNER_$($test.Owner)")
            } else {
                @('UNOWNED_TEST')
            }
            [ordered]@{
                name = [string]$test.Name
                properties = @([ordered]@{ name = 'LABELS'; value = [object[]]@($labels) })
            }
        }
    )
    $document = [ordered]@{
        version = [ordered]@{ major = 1; minor = 0 }
        tests = $testEntries
    }
    Set-PipelineTextFile -Path $Path -Content (
        $document | ConvertTo-Json -Depth 8)
}

<#
.SYNOPSIS
Invokes the production inventory audit against a synthetic fixture.
.PARAMETER Name
Fixture name.
.PARAMETER TargetRows
Ownership rows excluding the TSV header.
.PARAMETER Tests
Synthetic test entries.
.PARAMETER ExpectFailure
Requires the audit to reject the fixture.
#>
function Invoke-InventoryFixture {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][AllowEmptyCollection()][string[]]$TargetRows,
        [Parameter(Mandatory)][AllowEmptyCollection()][object[]]$Tests,
        [switch]$ExpectFailure
    )

    $fixtureRoot = Join-Path $regressionRoot "inventory-$Name"
    New-Item -ItemType Directory -Path $fixtureRoot -Force | Out-Null
    $ownership = "target`tcategory`towning_aggregate`tselected"
    if ($TargetRows.Count) {
        $ownership += "`n$($TargetRows -join "`n")"
    }
    Set-PipelineTextFile -Path (
        Join-Path $fixtureRoot 'development-target-ownership.tsv') `
        -Content "$ownership`n"
    $testJson = Join-Path $fixtureRoot 'tests.json'
    Write-SyntheticTestInventory -Path $testJson -Tests $Tests
    $result = Join-Path $fixtureRoot 'result.json'
    $arguments = @(
        "-DMATRIX_FILE=$matrixPath",
        '-DCELL_ID=msvc-release',
        "-DBUILD_DIRECTORY=$fixtureRoot",
        "-DCMAKE_CTEST_COMMAND=$ctest",
        "-DTEST_JSON_FILE=$testJson",
        "-DRESULT_FILE=$result",
        '-P', $auditScript
    )
    $logPath = Join-Path $fixtureRoot 'audit.log'
    & $cmake @arguments *> $logPath
    $failed = $LASTEXITCODE -ne 0
    if ($ExpectFailure -and -not $failed) {
        throw "Inventory regression $Name was accepted unexpectedly"
    }
    if (-not $ExpectFailure -and $failed) {
        throw "Inventory regression $Name failed unexpectedly: $((Get-Content -LiteralPath $logPath -Raw).Trim())"
    }
}

<#
.SYNOPSIS
Writes a receipt fixture and requires the production validator to reject it.
.PARAMETER Name
Fixture name.
.PARAMETER Receipt
Receipt document to validate.
.PARAMETER ExpectedPattern
Diagnostic pattern required from the rejection.
#>
function Assert-ReceiptRejected {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][object]$Receipt,
        [Parameter(Mandatory)][string]$ExpectedPattern
    )

    Set-PipelineTextFile -Path $script:receiptPath -Content (
        $Receipt | ConvertTo-Json -Depth 8)
    try {
        [void](Assert-BuildReceipt -SelectedCompilers @('ClangCl'))
        throw "Receipt regression $Name was accepted unexpectedly"
    } catch {
        if ($_.Exception.Message -notmatch $ExpectedPattern) {
            throw "Receipt regression $Name emitted an unexpected diagnostic: $($_.Exception.Message)"
        }
    }
}

try {
    New-Item -ItemType Directory -Path $regressionRoot -Force | Out-Null

    Invoke-InventoryFixture -Name valid `
        -TargetRows @(
            "RuntimeTarget`tRUNTIME_VALIDATION`tSimdLibRuntimeValidationArtifacts`tYES",
            "BenchmarkTarget`tBENCHMARK`tBenchmarkArtifacts`tNO") `
        -Tests @(
            [pscustomobject]@{ Name = 'Runtime.Case'; Owner = 'RUNTIME_VALIDATION' },
            [pscustomobject]@{ Name = 'Profile.Audit'; Owner = 'PROFILE_AUDIT' })
    Invoke-InventoryFixture -Name duplicate-target `
        -TargetRows @(
            "RuntimeTarget`tRUNTIME_VALIDATION`tSimdLibRuntimeValidationArtifacts`tYES",
            "RuntimeTarget`tRUNTIME_VALIDATION`tSimdLibRuntimeValidationArtifacts`tYES") `
        -Tests @() -ExpectFailure
    Invoke-InventoryFixture -Name unexpected-target `
        -TargetRows @(
            "DiagnosticTarget`tDEBUG_DIAGNOSTIC`tSimdLibDebugDiagnosticArtifacts`tYES") `
        -Tests @() -ExpectFailure
    Invoke-InventoryFixture -Name unowned-test `
        -TargetRows @() `
        -Tests @([pscustomobject]@{ Name = 'Unowned.Case'; Owner = '' }) `
        -ExpectFailure
    Invoke-InventoryFixture -Name duplicate-test `
        -TargetRows @() `
        -Tests @(
            [pscustomobject]@{ Name = 'Duplicate.Case'; Owner = 'PROFILE_AUDIT' },
            [pscustomobject]@{ Name = 'Duplicate.Case'; Owner = 'PROFILE_AUDIT' }) `
        -ExpectFailure
    Invoke-InventoryFixture -Name unexpected-test `
        -TargetRows @() `
        -Tests @([pscustomobject]@{
                Name = 'Diagnostic.Case'
                Owner = 'DEBUG_DIAGNOSTIC'
            }) `
        -ExpectFailure

    $runTestsPath = Join-Path $PSScriptRoot 'Run-Tests.ps1'
    Import-ValidationFunction -Path $runTestsPath -Name Assert-BuildReceipt
    $script:Scope = 'Native'
    $script:pipelineRoot = Join-Path $regressionRoot 'receipt-pipeline'
    New-Item -ItemType Directory -Path (
        Join-Path $script:pipelineRoot 'provenance') -Force | Out-Null
    $sourceDigest = Get-PipelineSourceDigest -RepositoryRoot $repositoryRoot
    $auditPath = Join-Path $regressionRoot 'repository-audit.json'
    $auditDocument = [ordered]@{
        schema = 'simdlib.repository-audit.v1'
        status = 'complete'
        sourceDigest = $sourceDigest
        sourceRevision = Get-PipelineRevision -RepositoryRoot $repositoryRoot
    }
    Set-PipelineTextFile -Path $auditPath -Content (
        $auditDocument | ConvertTo-Json -Depth 4)
    $inventoryAuditPath = Join-Path $regressionRoot 'inventory-audit.json'
    Set-PipelineTextFile -Path $inventoryAuditPath -Content (
        '{"schema":"simdlib.validation-inventory-audit.v1","status":"complete","cell":"clangcl-release","profile":"RELEASE"}')
    $manifestPath = Join-Path $regressionRoot 'validation-build.manifest'
    $matrixHash = (Get-FileHash -LiteralPath $matrixPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $inventoryAuditHash = (Get-FileHash -LiteralPath $inventoryAuditPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $manifestLines = @(
        'schema=simdlib.build-manifest.v1',
        'operation=build-validation',
        'status=complete',
        "source_digest=$sourceDigest",
        'preset=clangcl-release-exhaustive',
        'aggregate=ExhaustiveArtifacts',
        'matrix_cell=clangcl-release',
        'target_inventory_sha256=target-hash',
        'main_test_inventory_sha256=test-hash',
        "matrix_contract_sha256=$matrixHash",
        "validation_inventory_audit=$inventoryAuditPath",
        "validation_inventory_audit_sha256=$inventoryAuditHash",
        'build_profile=Release',
        'sanitizer=none',
        'codegen_mode=ENFORCE',
        'consumer_scope=core-register')
    Set-PipelineTextFile -Path $manifestPath -Content (
        ($manifestLines -join "`n") + "`n")
    $manifestHash = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $selectionId = (Get-PipelineTextDigest -Text 'Native|ClangCl').Substring(0, 16)
    $script:receiptPath = Join-Path $script:pipelineRoot "provenance/build-$selectionId.json"
    $receipt = [ordered]@{
        schema = 'simdlib.unified-build-receipt.v4'
        status = 'complete'
        scope = 'Native'
        compilers = @('ClangCl')
        sourceDigest = $sourceDigest
        repositoryAudit = [ordered]@{
            path = [System.IO.Path]::GetRelativePath(
                $repositoryRoot, $auditPath).Replace('\', '/')
            sha256 = (Get-FileHash -LiteralPath $auditPath -Algorithm SHA256).Hash.ToLowerInvariant()
            sourceDigest = $sourceDigest
        }
        manifests = @([ordered]@{
                preset = 'clangcl-release-exhaustive'
                path = [System.IO.Path]::GetRelativePath(
                    $repositoryRoot, $manifestPath).Replace('\', '/')
                sha256 = $manifestHash
                sourceDigest = $sourceDigest
                aggregate = 'ExhaustiveArtifacts'
                matrixCell = 'clangcl-release'
                targetInventorySha256 = 'target-hash'
                testInventorySha256 = 'test-hash'
                matrixContractSha256 = $matrixHash
                inventoryAuditSha256 = $inventoryAuditHash
                configuration = 'Release'
                instrumentation = 'none'
                generatedCodeMode = 'ENFORCE'
                consumerScope = 'core-register'
            })
    }
    Set-PipelineTextFile -Path $script:receiptPath -Content (
        $receipt | ConvertTo-Json -Depth 8)
    [void](Assert-BuildReceipt -SelectedCompilers @('ClangCl'))

    $case = $receipt | ConvertTo-Json -Depth 8 | ConvertFrom-Json
    $case.sourceDigest = 'stale'
    Assert-ReceiptRejected -Name stale -Receipt $case `
        -ExpectedPattern 'stale'
    $case = $receipt | ConvertTo-Json -Depth 8 | ConvertFrom-Json
    $case.status = 'building'
    Assert-ReceiptRejected -Name incomplete -Receipt $case `
        -ExpectedPattern 'incomplete|incompatible'
    $case = $receipt | ConvertTo-Json -Depth 8 | ConvertFrom-Json
    $case.compilers = @('Msvc')
    Assert-ReceiptRejected -Name mismatched-compiler -Receipt $case `
        -ExpectedPattern 'compiler set'
    $case = $receipt | ConvertTo-Json -Depth 8 | ConvertFrom-Json
    $case.manifests = @()
    Assert-ReceiptRejected -Name mismatched-cells -Receipt $case `
        -ExpectedPattern 'manifest set'
    $case = $receipt | ConvertTo-Json -Depth 8 | ConvertFrom-Json
    $case.manifests[0].aggregate = 'BenchmarkArtifacts'
    Assert-ReceiptRejected -Name category-incompatible -Receipt $case `
        -ExpectedPattern 'provenance aggregate'
    $case = $receipt | ConvertTo-Json -Depth 8 | ConvertFrom-Json
    $case.manifests[0].inventoryAuditSha256 = 'none'
    Assert-ReceiptRejected -Name missing-inventory-audit -Receipt $case `
        -ExpectedPattern 'validation_inventory_audit_sha256'

    $runTestsSource = Get-Content -LiteralPath $runTestsPath -Raw
    if ($runTestsSource -match "(?i)&\s*\(Join-Path[^\r\n]*Build\.ps1|--build|'-Action',\s*'Build'") {
        throw 'Run-Tests contains a configure or build dispatch'
    }
    if (@([regex]::Matches(
                $runTestsSource, "'-Action',\s*'Test'")).Count -lt 2) {
        throw 'Run-Tests does not dispatch both native and container test-only operations'
    }

    $nativeSource = Get-Content -LiteralPath (
        Join-Path $PSScriptRoot 'Run-NativeMatrix.ps1') -Raw
    if ($nativeSource -notmatch
            '(?s)function Build-NativeBenchmarks.+Assert-NativeManifest.+build-validation.+--target.+BenchmarkArtifacts') {
        throw 'Native benchmarks do not require and reuse the owning validation tree'
    }
    $containerSource = Get-Content -LiteralPath (
        Join-Path $repositoryRoot 'containers/container-entrypoint.sh') -Raw
    if ($containerSource -notmatch
            '(?s)build-benchmarks\).+can_reuse_validation_configuration.+Reusing validated Release configuration') {
        throw 'Container benchmarks do not require and reuse the owning validation tree'
    }

    $buildSource = Get-Content -LiteralPath (
        Join-Path $PSScriptRoot 'Build.ps1') -Raw
    if (@([regex]::Matches(
                $buildSource, 'Run-RepositoryAudit\.ps1')).Count -ne 1 -or
        $buildSource -notmatch 'repositoryAudit\s*=\s*\$repositoryAuditEntry') {
        throw 'Unified build does not execute one repository audit and bind it into provenance'
    }
    $auditSource = Get-Content -LiteralPath (
        Join-Path $PSScriptRoot 'Run-RepositoryAudit.ps1') -Raw
    if (@([regex]::Matches(
                $auditSource, 'if \(-not \(Test-CurrentRepositoryAudit\)\)')).Count -ne 2) {
        throw 'Repository audit no longer has one cache guard plus one completion guard'
    }

    Write-Host (
        'Validation pipeline regressions passed: six inventory cases, ' +
        'one valid receipt, six rejected receipts, and no-rebuild ownership checks.')
} finally {
    $resolvedRegressionRoot = [System.IO.Path]::GetFullPath($regressionRoot)
    $resolvedPipelineRoot = [System.IO.Path]::GetFullPath(
        (Join-Path $repositoryRoot 'out/pipeline'))
    if ($resolvedRegressionRoot.StartsWith(
            $resolvedPipelineRoot + [System.IO.Path]::DirectorySeparatorChar,
            [System.StringComparison]::OrdinalIgnoreCase) -and
        (Test-Path -LiteralPath $resolvedRegressionRoot)) {
        Remove-Item -LiteralPath $resolvedRegressionRoot -Recurse -Force
    }
}
