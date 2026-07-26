<#
.SYNOPSIS
Runs benchmarks from completed benchmark-build manifests.
.DESCRIPTION
The command delegates only to benchmark execution operations. Those operations
reject missing, stale, or incompatible manifests and never configure or build.
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
Expands benchmark execution filters into native and container owners.
#>
function Resolve-BenchmarkExecutionSelection {
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

$selection = Resolve-BenchmarkExecutionSelection
$operations = [System.Collections.Generic.List[object]]::new()
$nativeSelection = @($selection.Native)
$containerSelection = @($selection.Containers)
if ($nativeSelection.Count) {
    $nativeFilter = if ($nativeSelection.Count -eq 2) { 'All' } else { $nativeSelection[0] }
    $operations.Add([pscustomobject]@{ Id = 'native-benchmarks'; Script = Join-Path $PSScriptRoot 'Run-NativeMatrix.ps1'; Arguments = @('-Action', 'RunBenchmarks', '-Compiler', $nativeFilter, '-Cell', 'Release') })
}
if ($containerSelection.Count -eq 3) {
    $operations.Add([pscustomobject]@{ Id = 'container-benchmarks'; Script = Join-Path $PSScriptRoot 'Run-ContainerMatrix.ps1'; Arguments = @('-Action', 'RunBenchmarks', '-Compiler', 'All', '-Cell', 'Release') })
} else {
    foreach ($name in $containerSelection) {
        $operations.Add([pscustomobject]@{ Id = "container-$($name.ToLowerInvariant())-benchmarks"; Script = Join-Path $PSScriptRoot 'Run-ContainerMatrix.ps1'; Arguments = @('-Action', 'RunBenchmarks', '-Compiler', $name, '-Cell', 'Release') })
    }
}
if (-not $operations.Count) { Write-Host 'No selected compiler owns benchmark execution.'; exit 0 }
$logDirectory = Join-Path (Get-PipelineRepositoryRoot) "out/pipeline/logs/$(Get-Date -Format 'yyyyMMdd-HHmmssfff')-run-benchmarks-$PID"
Invoke-PipelineChildOperations -Operations $operations.ToArray() -LogDirectory $logDirectory
Write-Host "Benchmarks passed. Logs: $logDirectory"
