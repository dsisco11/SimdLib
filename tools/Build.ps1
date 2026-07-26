<#
.SYNOPSIS
Builds the requested complete SimdLib validation artifact matrix.
.DESCRIPTION
Scope must be explicit so a host cannot silently omit required native or
container cells. The command builds validation artifacts first, invokes the
benchmark build operation once for the same selection, and records an exact
manifest receipt consumed by Run-Tests.ps1.
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
Returns the exact validation presets owned by selected compiler filters.
.PARAMETER SelectedCompilers
Canonical compiler selection.
#>
function Get-ExpectedValidationPresets {
    param([Parameter(Mandatory)][string[]]$SelectedCompilers)
    $presets = [System.Collections.Generic.List[string]]::new()
    foreach ($name in $SelectedCompilers) {
        switch ($name) {
            'Msvc' { $presets.Add('msvc-release-exhaustive'); $presets.Add('msvc-debug-diagnostics') }
            'ClangCl' { $presets.Add('clangcl-release-exhaustive'); $presets.Add('clangcl-debug-diagnostics') }
            'ClangCoverage' { $presets.Add('clang-debug-coverage') }
            'Gcc13' { $presets.Add('gcc13-core-release-exhaustive'); $presets.Add('gcc13-core-debug-diagnostics') }
            'Gcc14' { $presets.Add('gcc14-release-exhaustive'); $presets.Add('gcc14-debug-diagnostics') }
            'Clang22' { $presets.Add('clang22-release-exhaustive'); $presets.Add('clang22-debug-diagnostics'); $presets.Add('clang22-debug-asan-ubsan') }
        }
    }
    return @($presets)
}

<#
.SYNOPSIS
Records the exact completed validation manifests produced by this build.
.PARAMETER SelectedCompilers
Canonical compiler selection.
#>
function Write-BuildReceipt {
    param([Parameter(Mandatory)][string[]]$SelectedCompilers)
    $expectedPresets = @(Get-ExpectedValidationPresets -SelectedCompilers $SelectedCompilers)
    $manifestFiles = @(Get-ChildItem -LiteralPath $pipelineRoot -Filter 'validation-build.manifest' -File -Recurse -ErrorAction SilentlyContinue)
    $entries = [System.Collections.Generic.List[object]]::new()
    foreach ($preset in $expectedPresets) {
        $matches = @($manifestFiles | Where-Object {
                try { (Read-PipelineManifest -Path $_.FullName).preset -eq $preset } catch { $false }
            } | Sort-Object LastWriteTimeUtc -Descending)
        if ($matches.Count -eq 0) { throw "Build completed without the required manifest for preset $preset" }
        $manifest = Read-PipelineManifest -Path $matches[0].FullName
        if ($manifest.operation -ne 'build-validation' -or $manifest.status -ne 'complete') { throw "Incomplete validation manifest for preset $preset" }
        $entries.Add([ordered]@{
                preset = $preset
                path = [System.IO.Path]::GetRelativePath($repositoryRoot, $matches[0].FullName).Replace('\', '/')
                sha256 = (Get-FileHash -LiteralPath $matches[0].FullName -Algorithm SHA256).Hash.ToLowerInvariant()
                fingerprint = $manifest.fingerprint_sha256
            })
    }
    $selectionText = "$Scope|$($SelectedCompilers -join ',')"
    $selectionId = (Get-PipelineTextDigest -Text $selectionText).Substring(0, 16)
    $receiptPath = Join-Path $pipelineRoot "provenance/build-$selectionId.json"
    $document = [ordered]@{
        schema = 'simdlib.unified-build-receipt.v1'; status = 'complete'; scope = $Scope
        compilers = @($SelectedCompilers); sourceDigest = Get-PipelineSourceDigest -RepositoryRoot $repositoryRoot
        sourceRevision = Get-PipelineRevision -RepositoryRoot $repositoryRoot; manifests = $entries.ToArray()
    }
    Set-PipelineTextFile -Path $receiptPath -Content ($document | ConvertTo-Json -Depth 6)
    Set-PipelineTextFile -Path (Join-Path $pipelineRoot 'provenance/latest-build-receipt.txt') -Content ([System.IO.Path]::GetRelativePath($repositoryRoot, $receiptPath).Replace('\', '/'))
    return $receiptPath
}

$selectedCompilers = @(Resolve-BuildSelection)
if ($Scope -in @('All', 'Native') -and -not $IsWindows) { throw 'Native scope requires a Windows x64 host with Visual Studio C++ tools and LLVM 22.' }
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

& (Join-Path $PSScriptRoot 'Build-Benchmarks.ps1') -Scope $Scope -Compiler $selectedCompilers
$receipt = Write-BuildReceipt -SelectedCompilers $selectedCompilers
Write-Host "Unified build passed. Receipt: $receipt"
