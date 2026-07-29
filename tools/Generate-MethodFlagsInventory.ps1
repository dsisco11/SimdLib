<#
.SYNOPSIS
Generates or verifies the exhaustive legacy method-flags migration inventory.
.DESCRIPTION
Scans active C++ source rather than comments, associates every direct legacy
attribute occurrence with one declaration or reviewed exception, and records
the intended SIMD boundary and optimization disposition.
#>
[CmdletBinding()]
param(
    [string]$OutputPath = '',
    [switch]$Verify
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
if (-not $OutputPath) {
    $OutputPath = Join-Path $repositoryRoot 'docs/MethodFlagsInventory.csv'
} elseif (-not [System.IO.Path]::IsPathRooted($OutputPath)) {
    $OutputPath = Join-Path $repositoryRoot $OutputPath
}
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
$legacyTokenPattern = '\b(VECTORCALL|SIMDLIB_REGISTER_ONLY|SIMDLIB_FORCE_INLINE|SIMDLIB_FLATTEN)\b'
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
Character position of the first legacy token.
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
Extracts the declared function name from a legacy declaration header.
.PARAMETER Header
Declaration header containing one or more legacy tokens.
#>
function Get-DeclarationSymbol {
    param([Parameter(Mandatory)][string]$Header)

    if ($Header -match '^\s*#') { return '' }
    $withoutLegacy = [regex]::Replace($Header, $legacyTokenPattern, ' ')
    $operatorMatch = [regex]::Match(
        $withoutLegacy,
        'operator\s*(?:\[\]|[+\-*/%&|^~!=<>]+|[A-Za-z_][A-Za-z0-9_:<>,\s]*)\s*\(')
    if ($operatorMatch.Success) {
        return ($operatorMatch.Value -replace '\s*\($', '').Trim()
    }

    $excluded = @(
        'alignas', 'decltype', 'for', 'if', 'noexcept', 'requires',
        'sizeof', 'static_assert', 'switch', 'while')
    $matches = [regex]::Matches(
        $withoutLegacy,
        '(~?[A-Za-z_][A-Za-z0-9_]*)(?:\s*<[^<>]*(?:<[^<>]*>[^<>]*)*>)?\s*\(')
    foreach ($match in $matches) {
        $candidate = $match.Groups[1].Value
        if ($candidate -notin $excluded) { return $candidate }
    }
    return ''
}

<#
.SYNOPSIS
Returns the parameter-list text for a named declaration.
.PARAMETER Header
Function declaration header.
.PARAMETER Symbol
Extracted function symbol.
#>
function Get-ParameterText {
    param(
        [Parameter(Mandatory)][string]$Header,
        [Parameter(Mandatory)][string]$Symbol
    )
    if (-not $Symbol) { return '' }
    $symbolIndex = if ($Symbol.StartsWith('operator')) {
        $Header.IndexOf('operator', [StringComparison]::Ordinal)
    } else {
        $matches = [regex]::Matches(
            $Header,
            "(?<![A-Za-z0-9_])$([regex]::Escape($Symbol))(?:\s*<[^<>]*(?:<[^<>]*>[^<>]*)*>)?\s*\(")
        if ($matches.Count -eq 0) { -1 } else { $matches[0].Index }
    }
    if ($symbolIndex -lt 0) { return '' }
    $open = $Header.IndexOf('(', $symbolIndex)
    if ($open -lt 0) { return '' }
    $depth = 0
    for ($index = $open; $index -lt $Header.Length; ++$index) {
        if ($Header[$index] -eq '(') {
            ++$depth
        } elseif ($Header[$index] -eq ')') {
            --$depth
            if ($depth -eq 0) {
                return $Header.Substring($open + 1, $index - $open - 1)
            }
        }
    }
    return ''
}

<#
.SYNOPSIS
Returns only the independently specified return-type portion of a declaration.
.DESCRIPTION
Legacy attributes can appear before or after the return type. The unified macro
does not own that type, so boundary classification must ignore template heads,
requires clauses, and other declaration text that precedes the final legacy
attribute token.
.PARAMETER Header
Function declaration header.
.PARAMETER Symbol
Extracted function symbol.
#>
function Get-ReturnText {
    param(
        [Parameter(Mandatory)][string]$Header,
        [Parameter(Mandatory)][string]$Symbol
    )

    $symbolOffset = if ($Symbol.StartsWith('operator')) {
        $Header.IndexOf('operator', [StringComparison]::Ordinal)
    } else {
        $match = [regex]::Match(
            $Header,
            "(?<![A-Za-z0-9_])$([regex]::Escape($Symbol))(?:\s*<[^<>]*(?:<[^<>]*>[^<>]*)*>)?\s*\(")
        if ($match.Success) { $match.Index } else { -1 }
    }
    if ($symbolOffset -lt 0) { return '' }

    $prefix = $Header.Substring(0, $symbolOffset)
    $prefix = [regex]::Replace($prefix, $legacyTokenPattern, ' ')
    return ($prefix -replace '\s+', ' ').Trim()
}

<#
.SYNOPSIS
Reports whether a parameter list carries a SIMD value by value.
.PARAMETER Parameters
Comma-separated declaration parameter text.
.PARAMETER Path
Repository-relative source path used for generic implementation parameters.
.PARAMETER Symbol
Function symbol used to distinguish scalar constructors and loads.
.PARAMETER HasVectorcall
Whether the legacy declaration requests vectorcall.
#>
function Test-SimdInput {
    param(
        [Parameter(Mandatory)][AllowEmptyString()][string]$Parameters,
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Symbol,
        [Parameter(Mandatory)][bool]$HasVectorcall
    )
    if (-not $Parameters.Trim()) { return $false }

    $resultOnlySymbols = @(
        'broadcast', 'construct', 'from_array', 'from_lanes', 'load',
        'load_aligned', 'load_bytes', 'load_partial', 'load_unaligned',
        'load_unsafe', 'register_from_array', 'register_from_repeated_value',
        'register_from_values', 'set', 'set1', 'set_partial', 'setr',
        'setr_partial', 'setzero', 'zero')
    $simdTypePattern =
        '\b(__m(?:128|256)[a-z0-9_]*|AbiMask|AbiRegister|double_vector_t|' +
        'float_vector_t|int_vector_t|integer_native_type|native_t|native_type|' +
        'predicate_type|raw_t|register_t|register_type|RegisterMask|Register|' +
        'result_t|SimdVector|StableRegister|uint_native_type|vector_t|' +
        'vector_type|Wrapper)\b'
    foreach ($parameter in $Parameters -split ',') {
        if ($parameter -notmatch $simdTypePattern) { continue }
        if ($parameter -match '\b(span|array)\s*<' -or $parameter -match '[*&]') { continue }
        return $true
    }

    if ($HasVectorcall -and
        $Path -match '^include/SimdLib/Detail/(Implementations|Extensions)\.h$' -and
        $Symbol -notin $resultOnlySymbols -and
        $Parameters -match '\bauto\s+(lhs|value|vector|condition|mask)\b') {
        return $true
    }
    return $false
}

<#
.SYNOPSIS
Reports whether a declaration returns a SIMD value by value.
.PARAMETER Header
Function declaration header.
.PARAMETER Symbol
Function symbol.
.PARAMETER HasVectorcall
Whether the legacy declaration requests vectorcall.
#>
function Test-SimdOutput {
    param(
        [Parameter(Mandatory)][string]$Header,
        [Parameter(Mandatory)][string]$Symbol,
        [Parameter(Mandatory)][bool]$HasVectorcall
    )

    $prefix = Get-ReturnText -Header $Header -Symbol $Symbol
    if (-not $prefix) { return $false }
    if ($prefix -match '[*&]\s*$') {
        return $false
    }
    if ($prefix -match '\b(std::)?(array|span|tuple)\s*<[^;{}]*>\s*$') {
        return $false
    }
    if ($prefix -match '\b(__m(?:128|256)[a-z0-9_]*|AbiMask|AbiRegister|' +
        'double_vector_t|float_vector_t|int_vector_t|integer_native_type|' +
        'native_t|native_type|predicate_type|raw_t|register_t|register_type|' +
        'RegisterMask|Register|result_t|SimdVector|StableRegister|' +
        'uint_native_type|vector_t|vector_type|Wrapper)(?:\s*<[^;{}]*>)?\s*$') {
        return $true
    }

    $scalarAutoSymbols = @(
        'all', 'any', 'area', 'bits', 'dot_product', 'extract', 'getTuple',
        'lane', 'max_position', 'min_position', 'movemask', 'movemask_slim',
        'none', 'register_data', 'register_get_constexpr', 'register_to_array',
        'scalar_result', 'toArray', 'to_array')
    if ($Symbol -match '^(all|any)_' -or $Symbol -match '^cmp_') { return $false }
    if ($prefix -match '\bauto\s*$') {
        return $Symbol -notin $scalarAutoSymbols -and $HasVectorcall
    }
    return $false
}

<#
.SYNOPSIS
Returns the canonical boundary mode for one supported function declaration.
.PARAMETER Header
Function declaration header.
.PARAMETER Path
Repository-relative source path.
.PARAMETER Symbol
Function symbol.
.PARAMETER HasVectorcall
Whether vectorcall is present today.
#>
function Get-BoundaryMode {
    param(
        [Parameter(Mandatory)][string]$Header,
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Symbol,
        [Parameter(Mandatory)][bool]$HasVectorcall
    )
    $parameters = Get-ParameterText -Header $Header -Symbol $Symbol
    $hasInput = Test-SimdInput -Parameters $parameters -Path $Path -Symbol $Symbol -HasVectorcall $HasVectorcall
    $hasOutput = Test-SimdOutput -Header $Header -Symbol $Symbol -HasVectorcall $HasVectorcall
    if ($hasInput -and $hasOutput) { return 'InOut' }
    if ($hasInput) { return 'In' }
    if ($hasOutput) { return 'Out' }
    return 'Neither'
}

<#
.SYNOPSIS
Returns non-intrinsic call names made by a function body.
.PARAMETER Body
Comment-free function body.
#>
function Get-BodyCalls {
    param([Parameter(Mandatory)][AllowEmptyString()][string]$Body)
    if (-not $Body) { return @() }
    $excluded = @(
        'alignas', 'bit_cast', 'constexpr', 'decltype', 'defined', 'fill',
        'for', 'forward', 'if', 'is_constant_evaluated', 'noexcept',
        'reinterpret_cast', 'requires', 'return', 'size', 'sizeof',
        'static_assert', 'static_cast', 'switch', 'while')
    $calls = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($match in [regex]::Matches($Body, '(?:template\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*(?:<[^;{}()]*>)?\s*\(')) {
        $name = $match.Groups[1].Value
        if ($name -in $excluded -or $name -match '^_mm' -or $name -match '^__builtin') { continue }
        [void]$calls.Add($name)
    }
    return @($calls | Sort-Object)
}

<#
.SYNOPSIS
Classifies authored memory effects conservatively.
.PARAMETER Header
Function declaration header.
.PARAMETER Body
Comment-free function body.
.PARAMETER HasRegisterOnly
Whether the declaration already carries the audited promise.
#>
function Get-MemoryClassification {
    param(
        [Parameter(Mandatory)][string]$Header,
        [Parameter(Mandatory)][string]$Symbol,
        [Parameter(Mandatory)][AllowEmptyString()][string]$Context,
        [Parameter(Mandatory)][AllowEmptyString()][string]$Parameters,
        [Parameter(Mandatory)][AllowEmptyString()][string]$Body,
        [Parameter(Mandatory)][bool]$HasRegisterOnly,
        [Parameter(Mandatory)][AllowEmptyCollection()][string[]]$Calls
    )

    $constexprIsolation = $Body -match '\b(if\s+consteval|is_constant_evaluated\s*\()'
    $prohibitedRuntimePattern =
        '\b(memcpy|memmove|register_set_constexpr)\s*\(|_mm(?:128|256)?_[A-Za-z0-9_]*store|' +
        '\b(destination|write)\b|\bstd::span\s*<\s*(?!const\b)|\b[A-Za-z_][A-Za-z0-9_:<>]*\s*&\s*(hi|out_[A-Za-z0-9_]*)\b'
    $addressableStoragePattern = '\b(std::array|register_to_array|to_array)\b'
    $hasRuntimeWrite = $Header -match $prohibitedRuntimePattern -or $Body -match $prohibitedRuntimePattern
    $hasAddressableStorage = $Body -match $addressableStoragePattern
    $hasByValueArrayParameter =
        $Parameters -match '(?:const\s+)?std::array\s*<[^;{}()]*>\s+(?![&*])'
    $dependentWriterPath =
        $Body -match '\bimpl::(?:blend|shuffle|shuffle_lo|shuffle_hi)(?:_slow)?\s*\('
    $runtimeBody = [regex]::Replace(
        $Body,
        '\bconstexpr\b[^;{}]*\bregister_from_values\b[^;{}]*;',
        '')
    $runtimeStorageHelpers = @($Calls | Where-Object {
            $_ -match '^register_(?:get|set|from_array|from_values|' +
                'from_repeated_value|to_array|data|insert|blend|blend_slow|blend_bytes|' +
                'insert_float|shuffle_float|shuffle_float_slow|shuffle_double|shuffle_double_slow|shuffle_32|shuffle_32_slow|' +
                'shuffle_half_16|shuffle_half_16_slow|byte_shift_left|byte_shift_right|' +
                'transform_binary)$' -and
            $runtimeBody -match "\b$([regex]::Escape($_))\b"
        })
    if ($constexprIsolation -and
        $Symbol -match '^_ext128_shift_(?:left|right)_bits_slow$') {
        $runtimeStorageHelpers = @()
    }
    $compileTimeArrayOnly =
        $Body -match '(<\s*std::array\s*\{|constexpr[^;{}]*\bstd::array\b)' -or
        $constexprIsolation

    if ($HasRegisterOnly) {
        if ($hasRuntimeWrite) { return 'Conflict:ExistingRegisterOnlyWrites' }
        if ($hasByValueArrayParameter) {
            return 'Conflict:ExistingRegisterOnlyByValueArray'
        }
        if ($dependentWriterPath) {
            return 'ReviewRequired:ExistingRegisterOnlyDependentWriterPath'
        }
        if ($runtimeStorageHelpers.Count -gt 0) {
            return 'ReviewRequired:ExistingRegisterOnlyTransitiveStorage'
        }
        if ($hasAddressableStorage -and -not $compileTimeArrayOnly) {
            return 'Conflict:ExistingRegisterOnlyAddressableStorage'
        }
        if ($hasAddressableStorage) { return 'NoWrite:ConstexprStorageIsolated' }
        return 'NoWrite:ExistingAuditPreserved'
    }
    if ($hasRuntimeWrite -or $hasAddressableStorage -or $hasByValueArrayParameter -or
        $dependentWriterPath -or $runtimeStorageHelpers.Count -gt 0) {
        return 'WritesOrMaterializesMemory'
    }
    return 'NoWrite:ReviewCandidate'
}

<#
.SYNOPSIS
Returns the declaration kind and migration disposition.
.PARAMETER Path
Repository-relative source path.
.PARAMETER Header
Function declaration header.
.PARAMETER Symbol
Extracted function symbol.
#>
function Get-DeclarationDisposition {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Header,
        [Parameter(Mandatory)][AllowEmptyString()][string]$Symbol
    )

    if ($Path -eq 'include/SimdLib/Config.h' -and $Header -match '^\s*#') {
        return @('AdapterDefinition', 'KeepLegacyAdapter', 'Compiler adapter definition or forwarding mapping')
    }
    if ($Path -match '^tests/config/') {
        return @('ConfigurationProbe', 'KeepLegacyProbe', 'Focused low-level adapter configuration probe')
    }
    if ($Path -match '^tests/method_flags/') {
        return @('LegacyComparisonFixture', 'KeepLegacyBaseline', 'Intentional legacy side of method-flags syntax, ABI, or codegen comparison')
    }
    if (-not $Symbol) {
        return @('Unclassified', 'Error', 'Active legacy occurrence has no declaration or reviewed adapter role')
    }
    if ($Symbol -eq 'SimdVector' -or $Symbol.StartsWith('~')) {
        return @('ConstructorOrDestructor', 'KeepLegacyGrammarException', 'No independent return type exists before the function name')
    }
    if ($Symbol.StartsWith('operator ') -and
        $Symbol -notmatch '^operator\s*(\[\]|[+\-*/%&|^~!=<>]+)$') {
        return @('ConversionOperator', 'KeepLegacyGrammarException', 'Conversion operators have no independent return type')
    }
    return @('Function', 'Migrate', 'Supported ordinary function declaration')
}

<#
.SYNOPSIS
Returns the nearest implementation or mapping type containing a declaration.
.PARAMETER Text
Comment-free source text.
.PARAMETER Position
Character position where the declaration begins.
#>
function Get-ContainingImplementationType {
    param(
        [Parameter(Mandatory)][string]$Text,
        [Parameter(Mandatory)][int]$Position
    )

    $prefix = $Text.Substring(0, $Position)
    $matches = [regex]::Matches(
        $prefix,
        'struct\s+(Simd(?:Impl128|Impl256|Mappings)(?:\s*<[^>{}\r\n]+>)?)')
    if ($matches.Count -eq 0) { return '' }
    return ($matches[$matches.Count - 1].Groups[1].Value -replace '\s+', ' ').Trim()
}

<#
.SYNOPSIS
Creates one exhaustive inventory record.
.PARAMETER Path
Repository-relative source path.
.PARAMETER CleanText
Comment-free source text.
.PARAMETER Extent
Declaration character extent.
#>
function New-InventoryRecord {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$CleanText,
        [Parameter(Mandatory)]$Extent
    )

    $header = $CleanText.Substring($Extent.Start, $Extent.HeaderEnd - $Extent.Start)
    $body = if ($Extent.HasBody) {
        $CleanText.Substring($Extent.HeaderEnd, $Extent.End - $Extent.HeaderEnd)
    } else {
        ''
    }
    $header = ($header -replace '\s+', ' ').Trim()
    $symbol = Get-DeclarationSymbol -Header $header
    $context = Get-ContainingImplementationType -Text $CleanText -Position $Extent.Start
    $disposition = Get-DeclarationDisposition -Path $Path -Header $header -Symbol $symbol
    $kind, $target, $reason = $disposition
    $hasVectorcall = $header -match '\bVECTORCALL\b'
    $hasRegisterOnly = $header -match '\bSIMDLIB_REGISTER_ONLY\b'
    $hasForceInline = $header -match '\bSIMDLIB_FORCE_INLINE\b'
    $hasFlatten = $header -match '\bSIMDLIB_FLATTEN\b'
    $parameters = if ($target -eq 'Migrate') {
        Get-ParameterText -Header $header -Symbol $symbol
    } else {
        ''
    }
    $simdInput = if ($target -eq 'Migrate') {
        Test-SimdInput -Parameters $parameters -Path $Path -Symbol $symbol -HasVectorcall $hasVectorcall
    } else {
        $false
    }
    $simdOutput = if ($target -eq 'Migrate') {
        Test-SimdOutput -Header $header -Symbol $symbol -HasVectorcall $hasVectorcall
    } else {
        $false
    }
    $boundary = if ($target -ne 'Migrate') {
        'Exception'
    } elseif ($simdInput -and $simdOutput) {
        'InOut'
    } elseif ($simdInput) {
        'In'
    } elseif ($simdOutput) {
        'Out'
    } else {
        'Neither'
    }
    $calls = @(Get-BodyCalls -Body $body)
    $memory = if ($target -eq 'Migrate') {
        Get-MemoryClassification -Header $header -Symbol $symbol -Context $context -Body $body `
            -Parameters $parameters -HasRegisterOnly $hasRegisterOnly -Calls $calls
    } else {
        'Exception'
    }
    $registerOnlyTarget = if ($target -ne 'Migrate') {
        'Exception'
    } elseif ($hasRegisterOnly) {
        if ($memory -like 'ReviewRequired:*') {
            'KeepPendingSourceRepair'
        } else {
            'Keep'
        }
    } elseif ($memory -eq 'NoWrite:ReviewCandidate') {
        'ReviewCandidate'
    } else {
        'Omit'
    }
    $forceInlineTarget = if ($target -ne 'Migrate') {
        'Exception'
    } elseif ($hasForceInline) {
        'Keep'
    } else {
        'Omit'
    }
    $flattenTarget = if ($target -ne 'Migrate') {
        'Exception'
    } elseif ($hasFlatten) {
        'Keep'
    } else {
        'Omit'
    }
    $forceInlineAudit = if ($target -ne 'Migrate') {
        'Exception'
    } elseif ($hasForceInline) {
        'RequiredOptimizedCodeShape'
    } else {
        'NoSelfInliningPromise'
    }
    $flattenAudit = if ($target -ne 'Migrate') {
        'Exception'
    } elseif ($hasFlatten) {
        'RequiredRecursiveInliningContract'
    } elseif ($calls.Count -gt 0) {
        'NoIndependentRequirementForRecursiveInlining'
    } else {
        'LeafHasNoRecursiveCalls'
    }
    $constexprAudit = if ($body -match '\bif\s+consteval\b') {
        'SeparateIfConstevalBranch'
    } elseif ($body -match '\bis_constant_evaluated\s*\(') {
        'SeparateConstantEvaluationBranch'
    } elseif ($header -match '\bconstexpr\b') {
        'SharedBodyNoExplicitBranch'
    } else {
        'RuntimeOnly'
    }

    $existing = @()
    if ($hasVectorcall) { $existing += 'Vectorcall' }
    if ($hasRegisterOnly) { $existing += 'RegisterOnly' }
    if ($hasForceInline) { $existing += 'ForceInline' }
    if ($hasFlatten) { $existing += 'Flatten' }
    $legacyOccurrences = [regex]::Matches($header, $legacyTokenPattern).Count
    $targetFlags = if ($target -eq 'Migrate') {
        $flags = @($boundary)
        if ($registerOnlyTarget -like 'Keep*') { $flags += 'RegisterOnly' }
        if ($forceInlineTarget -eq 'Keep') { $flags += 'ForceInline' }
        if ($flattenTarget -eq 'Keep') { $flags += 'Flatten' }
        'SIMD_FLAGS(' + ($flags -join ', ') + ')'
    } else {
        'LegacyException'
    }
    return [pscustomobject][ordered]@{
        Path = $Path
        Line = Get-SourceLine -Text $CleanText -Position $Extent.Start
        Symbol = $symbol
        Context = $context
        Kind = $kind
        Existing = $existing -join '+'
        LegacyOccurrenceCount = $legacyOccurrences
        SimdInput = $simdInput
        SimdOutput = $simdOutput
        Boundary = $boundary
        Memory = $memory
        RegisterOnlyTarget = $registerOnlyTarget
        ForceInlineTarget = $forceInlineTarget
        ForceInlineAudit = $forceInlineAudit
        FlattenTarget = $flattenTarget
        FlattenAudit = $flattenAudit
        TargetFlags = $targetFlags
        ConstexprAudit = $constexprAudit
        DirectCalls = $calls -join '+'
        TransitiveAudit = 'Pending'
        Disposition = $target
        Reason = $reason
    }
}

<#
.SYNOPSIS
Returns every active legacy declaration or reviewed exception.
.PARAMETER RepositoryRoot
Absolute repository root.
#>
function Get-MethodFlagsInventory {
    param([Parameter(Mandatory)][string]$RepositoryRoot)

    $records = [System.Collections.Generic.List[object]]::new()
    $sourceFiles = foreach ($directory in @('include', 'tests', 'examples')) {
        Get-ChildItem -LiteralPath (Join-Path $RepositoryRoot $directory) -Recurse -File |
            Where-Object Extension -in $sourceExtensions
    }
    foreach ($sourceFile in $sourceFiles | Sort-Object FullName) {
        $path = [System.IO.Path]::GetRelativePath($RepositoryRoot, $sourceFile.FullName).Replace('\', '/')
        $cleanText = Remove-CxxCommentsPreservePositions -Text (
            [System.IO.File]::ReadAllText($sourceFile.FullName))
        $matches = [regex]::Matches($cleanText, $legacyTokenPattern)
        $consumedThrough = -1
        foreach ($match in $matches) {
            if ($match.Index -le $consumedThrough) { continue }
            $extent = Get-DeclarationExtent -Text $cleanText -Start $match.Index
            $records.Add((New-InventoryRecord -Path $path -CleanText $cleanText -Extent $extent))
            $consumedThrough = $extent.End - 1
        }
    }
    return $records.ToArray()
}

$inventory = @(Get-MethodFlagsInventory -RepositoryRoot $repositoryRoot)
$recordedOccurrenceCount = ($inventory | Measure-Object LegacyOccurrenceCount -Sum).Sum
$activeOccurrenceCount = 0
foreach ($directory in @('include', 'tests', 'examples')) {
    foreach ($sourceFile in Get-ChildItem -LiteralPath (Join-Path $repositoryRoot $directory) -Recurse -File |
        Where-Object Extension -in $sourceExtensions) {
        $cleanText = Remove-CxxCommentsPreservePositions -Text (
            [System.IO.File]::ReadAllText($sourceFile.FullName))
        $activeOccurrenceCount += [regex]::Matches($cleanText, $legacyTokenPattern).Count
    }
}
if ($recordedOccurrenceCount -ne $activeOccurrenceCount) {
    throw "Inventory accounts for $recordedOccurrenceCount of $activeOccurrenceCount active legacy occurrences"
}
$symbolRecords = @{}
foreach ($record in $inventory) {
    if (-not $record.Symbol) { continue }
    if (-not $symbolRecords.ContainsKey($record.Symbol)) {
        $symbolRecords[$record.Symbol] = [System.Collections.Generic.List[object]]::new()
    }
    $symbolRecords[$record.Symbol].Add($record)
}
$writerSymbols = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($record in $inventory) {
    if ($record.Memory -eq 'WritesOrMaterializesMemory' -or
        $record.Memory -like 'Conflict:*' -or
        $record.Memory -like 'ReviewRequired:*') {
        [void]$writerSymbols.Add($record.Symbol)
    }
}
$reviewedExternalCalls = @(
    'all_lane_bits', 'bit_floor', 'bit_width', 'byte_shift_left_constexpr',
    'byte_shift_right_constexpr', 'lowest', 'popcount', 'scalarRhs',
    'SIMDLIB_PRECONDITION')
foreach ($record in $inventory) {
    if ($record.Disposition -ne 'Migrate') {
        $record.TransitiveAudit = 'Exception'
        continue
    }
    $calls = @($record.DirectCalls -split '\+' | Where-Object { $_ })
    if ($calls.Count -eq 0) {
        $record.TransitiveAudit = 'Leaf'
        continue
    }
    if ($record.Memory -like 'ReviewRequired:*') {
        $hazards = @($calls | Where-Object {
                $_ -match '^register_(?:get|set|from_array|from_values|' +
                    'from_repeated_value|to_array|data|insert|blend|blend_slow|blend_bytes|' +
                    'insert_float|shuffle_float|shuffle_float_slow|shuffle_double|shuffle_double_slow|shuffle_32|shuffle_32_slow|' +
                    'shuffle_half_16|shuffle_half_16_slow|byte_shift_left|byte_shift_right|' +
                    'transform_binary)$'
            })
        $record.TransitiveAudit = if ($hazards.Count -gt 0) {
            'ReviewRequired:' + ($hazards -join '+')
        } else {
            'ReviewRequired:DependentWriterPath'
        }
        continue
    }
    $knownWriters = @($calls | Where-Object { $writerSymbols.Contains($_) })
    $unknownCalls = @($calls | Where-Object {
            -not $symbolRecords.ContainsKey($_) -and
            $_ -notin $reviewedExternalCalls -and
            $_ -notmatch '^_'
        })
    if ($record.RegisterOnlyTarget -eq 'ReviewCandidate' -and
        ($knownWriters.Count -gt 0 -or $unknownCalls.Count -gt 0)) {
        $record.RegisterOnlyTarget = 'Omit'
        if ($knownWriters.Count -gt 0) {
            $record.Memory = 'WritesOrMaterializesMemory:Transitive'
        } else {
            $record.Memory = 'UnprovenTransitiveCallee'
        }
    }
    if ($knownWriters.Count -gt 0) {
        $record.TransitiveAudit = 'KnownWriterFamily:' + ($knownWriters -join '+')
    } elseif ($unknownCalls.Count -gt 0) {
        $record.TransitiveAudit = 'UnprovenCallee:' + ($unknownCalls -join '+')
    } else {
        $record.TransitiveAudit = 'ReviewedNoKnownWriter'
    }
}
$errors = @($inventory | Where-Object {
        $_.Disposition -eq 'Error' -or $_.Memory -like 'Conflict:*'
    })
if ($errors.Count -gt 0) {
    $errors | Format-Table Path, Line, Symbol, Memory, Reason -AutoSize | Out-String | Write-Error
    throw "Method-flags inventory contains $($errors.Count) unresolved or contradictory records"
}

$csv = (($inventory | ConvertTo-Csv -NoTypeInformation) -join "`n") + "`n"
if ($Verify) {
    if (-not (Test-Path -LiteralPath $OutputPath -PathType Leaf)) {
        throw "Method-flags inventory is missing: $OutputPath"
    }
    $existing = [System.IO.File]::ReadAllText($OutputPath)
    if ($existing -ne $csv) {
        throw "Method-flags inventory is stale; regenerate $OutputPath"
    }
} else {
    [System.IO.File]::WriteAllText($OutputPath, $csv, $utf8NoBom)
}

$migrateCount = @($inventory | Where-Object Disposition -eq 'Migrate').Count
$exceptionCount = $inventory.Count - $migrateCount
$registerOnlyCandidates = @($inventory | Where-Object RegisterOnlyTarget -eq 'ReviewCandidate').Count
$registerOnlyReviewRequired = @($inventory | Where-Object {
        $_.Memory -like 'ReviewRequired:*'
    }).Count
Write-Host (
    (
        "Method-flags inventory: {0} records, {1} migrations, {2} exceptions, " +
        "{3} RegisterOnly candidates, {4} existing RegisterOnly reviews"
    ) -f $inventory.Count, $migrateCount, $exceptionCount,
        $registerOnlyCandidates, $registerOnlyReviewRequired)
