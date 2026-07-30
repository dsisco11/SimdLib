<#
.SYNOPSIS
Audits the canonical method-flags declaration surface.
.DESCRIPTION
Rejects retired declaration spellings, invalid `SIMD_FLAGS(...)` combinations,
unreviewed internal adapters, prohibited short flag macros, and public Doxygen
references to internal method-flags helpers.
#>
[CmdletBinding()]
param([string]$RepositoryRoot = '')

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryRoot = if ($RepositoryRoot) {
    [System.IO.Path]::GetFullPath($RepositoryRoot)
} else {
    Split-Path -Parent $PSScriptRoot
}
$retiredDeclarationPattern = '\b(VECTORCALL|SIMDLIB_REGISTER_ONLY|SIMDLIB_FORCE_INLINE|SIMDLIB_FLATTEN)\b'
$sourceExtensions = @('.h', '.hpp', '.cpp', '.cc', '.cxx')

<#
.SYNOPSIS
Removes C++ comments while preserving source length and line positions.
.PARAMETER Text
Original C++ source text.
#>
function Remove-CxxCommentsPreservePositions {
    param([Parameter(Mandatory)][string]$Text)

    $builder = [System.Text.StringBuilder]::new($Text.Length)
    $state = 'Code'
    for ($index = 0; $index -lt $Text.Length; ++$index) {
        $character = $Text[$index]
        $next = if ($index + 1 -lt $Text.Length) { $Text[$index + 1] } else { [char]0 }
        switch ($state) {
            'Code' {
                if ($character -eq '/' -and $next -eq '/') {
                    [void]$builder.Append('  ')
                    ++$index
                    $state = 'LineComment'
                } elseif ($character -eq '/' -and $next -eq '*') {
                    [void]$builder.Append('  ')
                    ++$index
                    $state = 'BlockComment'
                } elseif ($character -eq '"') {
                    [void]$builder.Append($character)
                    $state = 'String'
                } elseif ($character -eq "'") {
                    [void]$builder.Append($character)
                    $state = 'Character'
                } else {
                    [void]$builder.Append($character)
                }
            }
            'LineComment' {
                if ($character -eq "`n") {
                    [void]$builder.Append($character)
                    $state = 'Code'
                } else {
                    [void]$builder.Append(' ')
                }
            }
            'BlockComment' {
                if ($character -eq '*' -and $next -eq '/') {
                    [void]$builder.Append('  ')
                    ++$index
                    $state = 'Code'
                } elseif ($character -eq "`n") {
                    [void]$builder.Append($character)
                } else {
                    [void]$builder.Append(' ')
                }
            }
            'String' {
                [void]$builder.Append($character)
                if ($character -eq '\') {
                    if ($index + 1 -lt $Text.Length) {
                        [void]$builder.Append($Text[++$index])
                    }
                } elseif ($character -eq '"') {
                    $state = 'Code'
                }
            }
            'Character' {
                [void]$builder.Append($character)
                if ($character -eq '\') {
                    if ($index + 1 -lt $Text.Length) {
                        [void]$builder.Append($Text[++$index])
                    }
                } elseif ($character -eq "'") {
                    $state = 'Code'
                }
            }
        }
    }
    return $builder.ToString()
}

<#
.SYNOPSIS
Returns a one-based source line for a character position.
.PARAMETER Text
Source text whose newlines define the line map.
.PARAMETER Position
Zero-based character position.
#>
function Get-SourceLine {
    param(
        [Parameter(Mandatory)][string]$Text,
        [Parameter(Mandatory)][int]$Position
    )
    if ($Position -le 0) { return 1 }
    return 1 + ([regex]::Matches($Text.Substring(0, $Position), "`n")).Count
}

<#
.SYNOPSIS
Audits unified method-flag usage across production and consumer-facing sources.
.PARAMETER RepositoryRoot
Absolute repository root containing include, tests, and examples.
#>
function Invoke-MethodFlagsSourceAudit {
    param([Parameter(Mandatory)][string]$RepositoryRoot)

    $canonicalFlags = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    $boundaries = @('Neither', 'In', 'Out', 'InOut')
    $modifiers = @('RegisterOnly', 'ForceInline', 'Flatten')
    foreach ($boundary in $boundaries) {
        for ($mask = 0; $mask -lt 8; ++$mask) {
            $tokens = [System.Collections.Generic.List[string]]::new()
            $tokens.Add($boundary)
            for ($index = 0; $index -lt $modifiers.Count; ++$index) {
                if (($mask -band (1 -shl $index)) -ne 0) { $tokens.Add($modifiers[$index]) }
            }
            [void]$canonicalFlags.Add(($tokens -join ','))
        }
    }

    $negativeFixturePattern = '^tests/method_flags/(?:placement/)?Invalid[^/]*\.cpp$'
    $internalAdapterPaths = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($allowedPath in @(
            'include/SimdLib/Config.h',
            'include/SimdLib/SimdVector.h',
            'tests/config/MethodFlagsConfigOverrideProbe.cpp',
            'tests/method_flags/codegen/MethodFlagsRaw.cpp',
            'tests/method_flags/placement/MethodFlagsPlacementAbiDefinition.cpp',
            'tests/method_flags/placement/MethodFlagsPlacementFixture.h')) {
        [void]$internalAdapterPaths.Add($allowedPath)
    }

    $errors = [System.Collections.Generic.List[string]]::new()
    foreach ($directory in @('include', 'tests', 'examples')) {
        foreach ($sourceFile in Get-ChildItem -LiteralPath (Join-Path $RepositoryRoot $directory) -Recurse -File |
            Where-Object Extension -in $sourceExtensions) {
            $relativePath = [System.IO.Path]::GetRelativePath($RepositoryRoot, $sourceFile.FullName).Replace('\', '/')
            $sourceText = [System.IO.File]::ReadAllText($sourceFile.FullName)
            $cleanText = Remove-CxxCommentsPreservePositions -Text $sourceText
            $isNegativeFixture = $relativePath -match $negativeFixturePattern

            if (-not $isNegativeFixture) {
                foreach ($retiredDeclaration in [regex]::Matches($cleanText, $retiredDeclarationPattern)) {
                    $line = Get-SourceLine -Text $cleanText -Position $retiredDeclaration.Index
                    $errors.Add("$relativePath`:$line uses retired declaration attribute $($retiredDeclaration.Value)")
                }
                foreach ($shortMacro in [regex]::Matches(
                        $cleanText,
                        '(?m)^\s*#\s*define\s+(Neither|In|Out|InOut|RegisterOnly|ForceInline|Flatten)(?:\s|$)')) {
                    $line = Get-SourceLine -Text $cleanText -Position $shortMacro.Index
                    $errors.Add("$relativePath`:$line defines prohibited short object-like flag macro $($shortMacro.Groups[1].Value)")
                }
            }

            $internalAdapterMatches = [regex]::Matches(
                $cleanText,
                '\bSIMDLIB_METHOD_FLAGS_(VECTORCALL|SAFE_BUFFERS|FORCE_INLINE|FLATTEN)\b')
            if ($internalAdapterMatches.Count -gt 0 -and -not $internalAdapterPaths.Contains($relativePath)) {
                $line = Get-SourceLine -Text $cleanText -Position $internalAdapterMatches[0].Index
                $errors.Add("$relativePath`:$line uses an internal method-flags adapter outside the reviewed allowlist")
            }

            foreach ($doxygenComment in [regex]::Matches($sourceText, '(?s)/\*\*.*?\*/')) {
                if ($doxygenComment.Value -match '\bSIMDLIB_(?:DETAIL|METHOD)_FLAGS_') {
                    $line = Get-SourceLine -Text $sourceText -Position $doxygenComment.Index
                    $errors.Add("$relativePath`:$line exposes an internal method-flags macro through a Doxygen comment")
                }
            }

            if ($isNegativeFixture) { continue }
            foreach ($match in [regex]::Matches($cleanText, '\bSIMD_FLAGS\s*\(([^()]*)\)')) {
                $lineStart = $cleanText.LastIndexOf("`n", [Math]::Max(0, $match.Index - 1))
                $lineStart = if ($lineStart -lt 0) { 0 } else { $lineStart + 1 }
                $lineEnd = $cleanText.IndexOf("`n", $match.Index)
                if ($lineEnd -lt 0) { $lineEnd = $cleanText.Length }
                $sourceLine = $cleanText.Substring($lineStart, $lineEnd - $lineStart)
                if ($sourceLine -match '^\s*#\s*define\s+SIMD_FLAGS\b') { continue }

                $tokens = @($match.Groups[1].Value -split ',' | ForEach-Object Trim)
                $canonical = $tokens -join ','
                $line = Get-SourceLine -Text $cleanText -Position $match.Index
                if (-not $canonicalFlags.Contains($canonical)) {
                    $errors.Add("$relativePath`:$line uses noncanonical or unrecognized SIMD_FLAGS tokens: $canonical")
                    continue
                }
            }
        }
    }
    if ($errors.Count -gt 0) {
        throw "Method-flags source audit failed:`n$($errors -join "`n")"
    }
}
Invoke-MethodFlagsSourceAudit -RepositoryRoot $repositoryRoot
Write-Host 'Method-flags source audit passed.'
