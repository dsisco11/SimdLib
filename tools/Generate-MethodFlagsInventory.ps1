<#
.SYNOPSIS
Generates or verifies the canonical RegisterOnly declaration ledger.
.DESCRIPTION
Audits the unified method-flags declaration surface, rejects retired declaration
spellings and invalid flag combinations, and records every RegisterOnly promise.
#>
[CmdletBinding()]
param(
    [string]$RepositoryRoot = '',
    [string]$RegisterOnlyOutputPath = '',
    [switch]$Verify
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryRoot = if ($RepositoryRoot) {
    [System.IO.Path]::GetFullPath($RepositoryRoot)
} else {
    Split-Path -Parent $PSScriptRoot
}
if (-not $RegisterOnlyOutputPath) {
    $RegisterOnlyOutputPath = Join-Path $repositoryRoot 'docs/MethodFlagsRegisterOnly.csv'
} elseif (-not [System.IO.Path]::IsPathRooted($RegisterOnlyOutputPath)) {
    $RegisterOnlyOutputPath = Join-Path $repositoryRoot $RegisterOnlyOutputPath
}
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
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
Finds the end of one preprocessor line or C++ declaration and definition.
.PARAMETER Text
Comment-free source text.
.PARAMETER Start
Character position of the `SIMD_FLAGS(...)` invocation.
#>
function Get-DeclarationExtent {
    param(
        [Parameter(Mandatory)][string]$Text,
        [Parameter(Mandatory)][int]$Start
    )

    $lineStart = $Text.LastIndexOf("`n", [Math]::Max(0, $Start - 1))
    $lineStart = if ($lineStart -lt 0) { 0 } else { $lineStart + 1 }
    $lineEnd = $Text.IndexOf("`n", $Start)
    if ($lineEnd -lt 0) { $lineEnd = $Text.Length }
    if ($Text.Substring($lineStart, $lineEnd - $lineStart) -match '^\s*#') {
        return [pscustomobject]@{
            Start = $lineStart
            HeaderEnd = $lineEnd
            End = $lineEnd
            HasBody = $false
        }
    }

    $parentheses = 0
    $brackets = 0
    $requiresBraces = 0
    $bodyStart = -1
    for ($index = $lineStart; $index -lt $Text.Length; ++$index) {
        $character = $Text[$index]
        switch ($character) {
            '(' { ++$parentheses }
            ')' { if ($parentheses -gt 0) { --$parentheses } }
            '[' { ++$brackets }
            ']' { if ($brackets -gt 0) { --$brackets } }
            '{' {
                if ($parentheses -eq 0 -and $brackets -eq 0) {
                    $prefixStart = [Math]::Max($lineStart, $index - 512)
                    $prefix = $Text.Substring($prefixStart, $index - $prefixStart)
                    if ($requiresBraces -gt 0 -or $prefix -match 'requires\s+requires\b[^{}]*$') {
                        ++$requiresBraces
                    } else {
                        $bodyStart = $index
                        break
                    }
                }
            }
            '}' {
                if ($requiresBraces -gt 0 -and $parentheses -eq 0 -and $brackets -eq 0) {
                    --$requiresBraces
                }
            }
            ';' {
                if ($parentheses -eq 0 -and $brackets -eq 0 -and $requiresBraces -eq 0) {
                    return [pscustomobject]@{
                        Start = $lineStart
                        HeaderEnd = $index + 1
                        End = $index + 1
                        HasBody = $false
                    }
                }
            }
        }
        if ($bodyStart -ge 0) { break }
    }

    if ($bodyStart -lt 0) {
        return [pscustomobject]@{
            Start = $lineStart
            HeaderEnd = $lineEnd
            End = $lineEnd
            HasBody = $false
        }
    }

    $depth = 0
    for ($index = $bodyStart; $index -lt $Text.Length; ++$index) {
        if ($Text[$index] -eq '{') {
            ++$depth
        } elseif ($Text[$index] -eq '}') {
            --$depth
            if ($depth -eq 0) {
                return [pscustomobject]@{
                    Start = $lineStart
                    HeaderEnd = $bodyStart
                    End = $index + 1
                    HasBody = $true
                }
            }
        }
    }
    throw "Unterminated function body beginning on line $(Get-SourceLine -Text $Text -Position $lineStart)"
}

<#
.SYNOPSIS
Extracts the declared function name from a method-flags declaration header.
.PARAMETER Header
Declaration header containing a canonical `SIMD_FLAGS(...)` invocation.
#>
function Get-DeclarationSymbol {
    param([Parameter(Mandatory)][string]$Header)

    if ($Header -match '^\s*#') { return '' }
    $withoutFlags = [regex]::Replace($Header, $retiredDeclarationPattern, ' ')
    $withoutFlags = [regex]::Replace($withoutFlags, '\bSIMD_FLAGS\s*\([^()]*\)', ' ')
    $operatorMatch = [regex]::Match(
        $withoutFlags,
        'operator\s*(?:\[\]|[+\-*/%&|^~!=<>]+|[A-Za-z_][A-Za-z0-9_:<>,\s]*)\s*\(')
    if ($operatorMatch.Success) {
        return ($operatorMatch.Value -replace '\s*\($', '').Trim()
    }

    $excluded = @(
        'alignas', 'decltype', 'for', 'if', 'noexcept', 'requires',
        'sizeof', 'static_assert', 'switch', 'while')
    $matches = [regex]::Matches(
        $withoutFlags,
        '(~?[A-Za-z_][A-Za-z0-9_]*)(?:\s*<[^<>]*(?:<[^<>]*>[^<>]*)*>)?\s*\(')
    foreach ($match in $matches) {
        $candidate = $match.Groups[1].Value
        if ($candidate -notin $excluded) { return $candidate }
    }
    return ''
}

<#
.SYNOPSIS
Audits unified method-flag usage and returns every RegisterOnly declaration.
.PARAMETER RepositoryRoot
Absolute repository root containing include, tests, and examples.
#>
function Get-RegisterOnlyInventory {
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

    $records = [System.Collections.Generic.List[object]]::new()
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
                if ('RegisterOnly' -notin $tokens) { continue }

                $extent = Get-DeclarationExtent -Text $cleanText -Start $match.Index
                $header = $cleanText.Substring($extent.Start, $extent.HeaderEnd - $extent.Start)
                $header = ($header -replace '\s+', ' ').Trim()
                $records.Add([pscustomobject][ordered]@{
                        Path = $relativePath
                        Line = $line
                        Symbol = Get-DeclarationSymbol -Header $header
                        Flags = 'SIMD_FLAGS(' + ($tokens -join ', ') + ')'
                    })
            }
        }
    }
    if ($errors.Count -gt 0) {
        throw "Method-flags source audit failed:`n$($errors -join "`n")"
    }
    return @($records | Sort-Object Path, @{ Expression = { [int]$_.Line } }, Symbol)
}
$registerOnlyInventory = @(Get-RegisterOnlyInventory -RepositoryRoot $repositoryRoot)
$registerOnlyHeader = '"Path","Line","Symbol","Flags"'
$registerOnlyCsv = if ($registerOnlyInventory.Count -eq 0) {
    $registerOnlyHeader + "`n"
} else {
    (($registerOnlyInventory | ConvertTo-Csv -NoTypeInformation) -join "`n") + "`n"
}
if ($Verify) {
    if (-not (Test-Path -LiteralPath $RegisterOnlyOutputPath -PathType Leaf)) {
        throw "RegisterOnly inventory is missing: $RegisterOnlyOutputPath"
    }
    $existingRegisterOnly = [System.IO.File]::ReadAllText($RegisterOnlyOutputPath)
    if ($existingRegisterOnly -ne $registerOnlyCsv) {
        throw "RegisterOnly inventory is stale; regenerate $RegisterOnlyOutputPath"
    }
} else {
    [System.IO.File]::WriteAllText($RegisterOnlyOutputPath, $registerOnlyCsv, $utf8NoBom)
}
Write-Host "RegisterOnly inventory: $($registerOnlyInventory.Count) declarations"
