<#
.SYNOPSIS
Checks that public-consumer fixtures use only the supported public surface.
.PARAMETER SourceDirectory
Source tree whose public-consumer fixtures are checked.
#>
[CmdletBinding()]
param([string]$SourceDirectory = '')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $SourceDirectory) {
    $SourceDirectory = Split-Path -Parent $PSScriptRoot
}
$SourceDirectory = [System.IO.Path]::GetFullPath($SourceDirectory)
$cmake = (Get-Command cmake -ErrorAction Stop).Source
& $cmake "-DSOURCE_DIRECTORY=$SourceDirectory" -P (
    Join-Path (Split-Path -Parent $PSScriptRoot) 'cmake/CheckPublicConsumerBoundary.cmake')
if ($LASTEXITCODE -ne 0) {
    throw 'Public-consumer boundary validation failed.'
}