<#
.SYNOPSIS
Builds or consumes the dedicated Windows clang-cl 20 compatibility-floor container.
.DESCRIPTION
The source tree and pipeline output directory are bind-mounted into a pinned
Windows Server Core image containing Visual Studio Build Tools and the explicit
LLVM 20.1.8 compatibility-floor toolchain.
.PARAMETER Action
Pipeline operation to execute inside the container.
.PARAMETER SkipImageBuild
Reuses the existing local image instead of rebuilding it.
#>
[CmdletBinding()]
param(
    [ValidateSet('Build', 'Test', 'BuildBenchmarks')]
    [string]$Action = 'Build',
    [switch]$SkipImageBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$dockerfile = Join-Path $repositoryRoot 'containers/Dockerfile.windows-clang20'
$image = 'simdlib/windows-clang20:local'

if (-not $IsWindows) { throw 'The Windows clang-cl 20 container requires a Windows host.' }
if (-not (Get-Command docker -CommandType Application -ErrorAction SilentlyContinue)) {
    throw 'Docker is required to run the Windows clang-cl 20 container.'
}

$serverOs = (& docker version --format '{{.Server.Os}}' 2>&1).Trim()
if ($LASTEXITCODE -ne 0) { throw "Unable to query the Docker engine: $serverOs" }
if ($serverOs -ne 'windows') {
    throw "The Windows clang-cl 20 container requires a Windows Docker engine; selected engine: $serverOs"
}

if (-not $SkipImageBuild) {
    & docker build --isolation=process --memory 4GB --file $dockerfile --tag $image $repositoryRoot
    if ($LASTEXITCODE -ne 0) { throw "Unable to build $image" }
}

$script = switch ($Action) {
    'Build' { 'tools/Build.ps1' }
    'Test' { 'tools/Run-Tests.ps1' }
    'BuildBenchmarks' { 'tools/Build-Benchmarks.ps1' }
}
$revision = (& git -C $repositoryRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $revision -notmatch '^[0-9a-f]{40}$') {
    throw 'Unable to determine the source revision for container provenance.'
}

$mount = "$($repositoryRoot):C:\workspace"
& docker run --rm --isolation=process --memory 8GB `
    --volume $mount --workdir C:\workspace `
    --env CI=1 --env GITHUB_ACTIONS=$env:GITHUB_ACTIONS `
    --env SIMDLIB_BUILD_REVISION=$revision `
    $image -File $script -Scope Native -Compiler ClangCl
if ($LASTEXITCODE -ne 0) { throw "Windows clang-cl 20 container action failed: $Action" }
