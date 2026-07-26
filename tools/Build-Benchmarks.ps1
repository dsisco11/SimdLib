<#
.SYNOPSIS
Builds benchmark artifacts in validated Release trees.
.DESCRIPTION
This operation never creates a benchmark-specific configure tree. Native and
container benchmark targets reuse the matching Release validation fingerprints.
#>
[CmdletBinding()]
param(
    [ValidateSet('All', 'Native', 'Containers')]
    [string]$Scope = 'All',
    [ValidateSet('All', 'Msvc', 'ClangCl', 'ClangCoverage', 'Gcc13', 'Gcc14', 'Clang22')]
    [string[]]$Compiler = @('All')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Pipeline.Common.psm1') -Force

<#
.SYNOPSIS
Expands and validates compiler filters for the requested platform scope.
#>
function Resolve-BenchmarkCompilerSelection {
    $nativeNames = @('Msvc', 'ClangCl', 'ClangCoverage')
    $containerNames = @('Gcc13', 'Gcc14', 'Clang22')
    if ('All' -in $Compiler -and $Compiler.Count -ne 1) { throw 'Compiler All cannot be combined with another compiler filter.' }
    $selected = if ($Compiler -contains 'All') {
        switch ($Scope) {
            'Native' { $nativeNames }
            'Containers' { $containerNames }
            default { $nativeNames + $containerNames }
        }
    } else { @($Compiler | Select-Object -Unique) }
    if ($Scope -eq 'Native' -and @($selected | Where-Object { $_ -in $containerNames }).Count) { throw 'Container compiler filters are invalid for Native scope.' }
    if ($Scope -eq 'Containers' -and @($selected | Where-Object { $_ -in $nativeNames }).Count) { throw 'Native compiler filters are invalid for Containers scope.' }
    [pscustomobject]@{
        Native = if ($Scope -in @('All', 'Native')) { @($selected | Where-Object { $_ -in @('Msvc', 'ClangCl') }) } else { @() }
        Containers = if ($Scope -in @('All', 'Containers')) { @($selected | Where-Object { $_ -in $containerNames }) } else { @() }
    }
}

$selection = Resolve-BenchmarkCompilerSelection
$operations = [System.Collections.Generic.List[object]]::new()
$nativeSelection = @($selection.Native)
$containerSelection = @($selection.Containers)
if ($nativeSelection.Count) {
    $nativeFilter = if ($nativeSelection.Count -eq 2) { 'All' } else { $nativeSelection[0] }
    $operations.Add([pscustomobject]@{
            Id = 'native-benchmarks'; Script = Join-Path $PSScriptRoot 'Run-NativeMatrix.ps1'
            Arguments = @('-Action', 'BuildBenchmarks', '-Compiler', $nativeFilter, '-Cell', 'Release')
        })
}
if ($containerSelection.Count) {
    $containerFilter = if ($containerSelection.Count -eq 3) { 'All' } else { $null }
    if ($containerFilter) {
        $operations.Add([pscustomobject]@{
                Id = 'container-benchmarks'; Script = Join-Path $PSScriptRoot 'Run-ContainerMatrix.ps1'
                Arguments = @('-Action', 'BuildBenchmarks', '-Compiler', 'All', '-Cell', 'Release')
            })
    } else {
        foreach ($name in $containerSelection) {
            $operations.Add([pscustomobject]@{
                    Id = "container-$($name.ToLowerInvariant())-benchmarks"; Script = Join-Path $PSScriptRoot 'Run-ContainerMatrix.ps1'
                    Arguments = @('-Action', 'BuildBenchmarks', '-Compiler', $name, '-Cell', 'Release')
                })
        }
    }
}
if (-not $operations.Count) {
    Write-Host 'No selected compiler owns benchmark artifacts.'
    exit 0
}
$logDirectory = Join-Path (Get-PipelineRepositoryRoot) "out/pipeline/logs/$(Get-Date -Format 'yyyyMMdd-HHmmssfff')-build-benchmarks-$PID"
Invoke-PipelineChildOperations -Operations $operations.ToArray() -LogDirectory $logDirectory
Write-Host "Benchmark artifacts built. Logs: $logDirectory"
