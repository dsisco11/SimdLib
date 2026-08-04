<#
.SYNOPSIS
Records one explicitly selected Register generated-code diagnostic.
.DESCRIPTION
The command configures a diagnostic-only fingerprint, compiles only paired
Register fixtures, records wrapper/raw disassembly, and writes dedicated
provenance. Its record-only output cannot satisfy a Release generated-code gate.
#>
[CmdletBinding()]
param(
    [ValidateSet('', 'Native', 'Containers')]
    [string]$Scope = '',
    [ValidateSet('', 'Msvc', 'ClangCl', 'Gcc14', 'Clang22')]
    [string]$Compiler = '',
    [ValidateSet('', 'Debug', 'AsanUbsan')]
    [string]$Cell = '',
    [switch]$SkipImageBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $Scope -or -not $Compiler -or -not $Cell) {
    throw 'Record-Codegen requires explicit -Scope, -Compiler, and -Cell selections.'
}
if ($Scope -eq 'Native') {
    if ($Compiler -notin @('Msvc', 'ClangCl')) {
        throw 'Native codegen diagnostics support Msvc or ClangCl.'
    }
    if ($Cell -ne 'Debug') {
        throw 'Native codegen diagnostics support the Debug cell.'
    }
    if ($SkipImageBuild) {
        throw '-SkipImageBuild is available only for container diagnostics.'
    }
    & (Join-Path $PSScriptRoot 'Run-NativeMatrix.ps1') `
        -Action RecordCodegen -Compiler $Compiler -Cell $Cell
} else {
    if ($Compiler -notin @('Gcc14', 'Clang22')) {
        throw 'Container codegen diagnostics support Gcc14 or Clang22.'
    }
    if ($Cell -eq 'AsanUbsan' -and $Compiler -ne 'Clang22') {
        throw 'The sanitizer-instrumented codegen diagnostic is owned by Clang22.'
    }
    if ($SkipImageBuild) {
        & (Join-Path $PSScriptRoot 'Run-ContainerMatrix.ps1') `
            -Action RecordCodegen -Compiler $Compiler -Cell $Cell -SkipImageBuild
    } else {
        & (Join-Path $PSScriptRoot 'Run-ContainerMatrix.ps1') `
            -Action RecordCodegen -Compiler $Compiler -Cell $Cell
    }
}
