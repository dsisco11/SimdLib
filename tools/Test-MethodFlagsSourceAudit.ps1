<#
.SYNOPSIS
Regression-tests the method-flags source audit against isolated source trees.
.DESCRIPTION
Creates disposable repositories containing valid and deliberately invalid
declarations, then verifies that the production source audit accepts
only the supported declaration surface.
#>
[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$sourceAudit = Join-Path $PSScriptRoot 'Audit-MethodFlagsSource.ps1'
$temporaryRoot = [System.IO.Path]::GetFullPath(
    (Join-Path ([System.IO.Path]::GetTempPath()) "SimdLib-MethodFlagsAudit-$([guid]::NewGuid().ToString('N'))"))
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)

<#
.SYNOPSIS
Writes one source fixture into the isolated repository.
.PARAMETER RelativePath
Repository-relative destination path.
.PARAMETER Content
Complete source-file contents.
#>
function Set-AuditFixture {
    param(
        [Parameter(Mandatory)][string]$RelativePath,
        [Parameter(Mandatory)][AllowEmptyString()][string]$Content
    )

    $path = Join-Path $temporaryRoot $RelativePath
    $directory = Split-Path -Parent $path
    [void](New-Item -ItemType Directory -Path $directory -Force)
    [System.IO.File]::WriteAllText($path, $Content, $utf8NoBom)
}

<#
.SYNOPSIS
Runs the production source audit against the isolated repository.
.OUTPUTS
An object containing the child process exit code and captured diagnostics.
#>
function Invoke-AuditFixture {

    $invocationId = [guid]::NewGuid().ToString('N')
    $standardOutputPath = Join-Path $temporaryRoot "audit-$invocationId.stdout"
    $standardErrorPath = Join-Path $temporaryRoot "audit-$invocationId.stderr"
    $arguments = @(
        '-NoProfile',
        '-File', $sourceAudit,
        '-RepositoryRoot', $temporaryRoot)
    $process = Start-Process -FilePath (Get-Process -Id $PID).Path `
        -ArgumentList $arguments -Wait -PassThru -NoNewWindow `
        -RedirectStandardOutput $standardOutputPath `
        -RedirectStandardError $standardErrorPath
    return [pscustomobject]@{
        ExitCode = $process.ExitCode
        Output = [System.IO.File]::ReadAllText($standardOutputPath)
        Error = [System.IO.File]::ReadAllText($standardErrorPath)
    }
}

<#
.SYNOPSIS
Requires one fixture invocation to succeed.
.PARAMETER Name
Readable regression-case name.
#>
function Assert-AuditSucceeds {
    param([Parameter(Mandatory)][string]$Name)

    $result = Invoke-AuditFixture
    if ($result.ExitCode -ne 0) {
        throw (
            "Method-flags source-audit regression '$Name' unexpectedly failed " +
            "with exit code $($result.ExitCode):`n$($result.Error)$($result.Output)")
    }
}

<#
.SYNOPSIS
Requires one fixture invocation to fail.
.PARAMETER Name
Readable regression-case name.
#>
function Assert-AuditFails {
    param([Parameter(Mandatory)][string]$Name)

    $result = Invoke-AuditFixture
    if ($result.ExitCode -eq 0) {
        throw "Method-flags source-audit regression '$Name' unexpectedly succeeded"
    }
}

try {
    foreach ($directory in @('include', 'tests', 'examples')) {
        [void](New-Item -ItemType Directory -Path (Join-Path $temporaryRoot $directory) -Force)
    }

    Set-AuditFixture -RelativePath 'include/Valid.h' -Content @'
int SIMD_FLAGS(Neither, RegisterOnly) valid_method() noexcept;
'@
    Assert-AuditSucceeds -Name 'canonical RegisterOnly declaration'

    Set-AuditFixture -RelativePath 'include/Valid.h' -Content @'
int SIMD_FLAGS(Neither, Unknown) invalid_method() noexcept;
'@
    Assert-AuditFails -Name 'unknown SIMD_FLAGS token'

    Set-AuditFixture -RelativePath 'include/Valid.h' -Content @'
#define In replacement
'@
    Assert-AuditFails -Name 'short object-like flag macro'

    Set-AuditFixture -RelativePath 'include/Valid.h' -Content @'
int SIMDLIB_METHOD_FLAGS_FORCE_INLINE leaked_adapter() noexcept;
'@
    Assert-AuditFails -Name 'internal adapter outside allowlist'

    Set-AuditFixture -RelativePath 'include/Valid.h' -Content @'
int valid_method() noexcept;
'@
    Assert-AuditSucceeds -Name 'legacy-free baseline'
    Set-AuditFixture -RelativePath 'include/Valid.h' -Content @'
SIMDLIB_FORCE_INLINE int legacy_method() noexcept;
'@
    Assert-AuditFails -Name 'direct legacy declaration'

    Set-AuditFixture -RelativePath 'include/Valid.h' -Content @'
/** Exposes SIMDLIB_METHOD_FLAGS_FORCE_INLINE as public documentation. */
int documented_method() noexcept;
'@
    Assert-AuditFails -Name 'internal adapter in Doxygen'

    Write-Host 'Method-flags source-audit regressions passed: 6 policy cases'
} finally {
    $resolvedTemporaryRoot = [System.IO.Path]::GetFullPath($temporaryRoot)
    $systemTemporaryRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
    if (-not $resolvedTemporaryRoot.StartsWith($systemTemporaryRoot, [StringComparison]::OrdinalIgnoreCase) -or
        [System.IO.Path]::GetFileName($resolvedTemporaryRoot) -notmatch '^SimdLib-MethodFlagsAudit-[0-9a-f]{32}$') {
        throw "Refusing to remove unexpected source-audit fixture path: $resolvedTemporaryRoot"
    }
    if (Test-Path -LiteralPath $resolvedTemporaryRoot) {
        Remove-Item -LiteralPath $resolvedTemporaryRoot -Recurse -Force
    }
}
