# Test coverage contract

This document defines SimdLib's enduring behavioral coverage and feature-profile
ownership. Run-specific percentages, counts, timings, and tool identities belong
in generated build receipts, reports, coverage artifacts, and CI results.

## Coverage layers

| Layer | Evidence |
| --- | --- |
| Runtime behavior | Catch2 suites for `Api`, `Bmi`, `uint128_t`, formatting, `SimdAlgo`, `SimdVector`, and `SimdResample` |
| Compile-time behavior | `static_assert` contracts in the Catch2 sources and the configuration, availability, and public-header probes |
| Header isolation | Every public header is compiled as the first and only SimdLib include; the umbrella header has a separate probe |
| Configuration | Default detection, caller overrides, all instruction families disabled, FMA enabled/disabled, BMI1/BMI2 independently enabled, and portable/optimized/scalar UInt128 profiles |
| Formatter and ODR | Scalar-formatter parity, vector and UInt128 formatting, umbrella/focused-header probes, and a two-translation-unit formatter executable |
| Oracle/property testing | Deterministic scalar oracles for comparisons, transfers, BMI operations, UInt128 arithmetic/bit operations, algorithms, and resampling |
| Compiler/runtime diagnostics | Every applicable supported Release compiler, representative MSVC Debug, and the independent Clang ASan/UBSan Debug cell |
| External consumer | `tests/consumer` validates source-tree import, the interface-library target, public includes, and header-only linkage |

Benchmarks are intentionally excluded from correctness counts. They exercise
representative optimized operations but have their own performance purpose and
acceptance rules.

## Test inventory

Every runtime profile discovers individual Catch2 cases with
`catch_discover_tests()`. The Clang coverage profile owns only execution-bearing
runtime and checks/precondition targets; applicable Release compiler cells own
header, compiler-contract, constexpr, example, smoke, and ODR validation.
Terminating precondition cases are discovered Catch2 cases, not direct CTest
driver scenarios. Catch2 executables remain grouped by these stable name
prefixes:

| Entry | Coverage role |
| --- | --- |
| `HeaderOnlySmoke` | Multi-translation-unit umbrella-header use and header-only linkage |
| `RegisterOdr` | Multi-translation-unit Register and RegisterMask use through the C++23 interface target |
| `BmiPortable.*` | Portable BMI behavior, constexpr checks, boundaries, signed bit patterns, and deterministic randomized oracles |
| `Format.*` | UInt128 and vector formatter behavior plus standard scalar parity |
| `FormatOdr` | Formatter specialization linkage across two translation units |
| `Api.SSE42.*` | 128-bit `Api`, partial transfers, comparisons, conversion, movemask, and register metadata |
| `Api.AVX2.*` | 256-bit `Api`, partial transfers, comparisons, movemask, and register metadata |
| `Register.SSE42.*` | 128-bit Register and RegisterMask behavior under the SSE4.2 availability profile |
| `Register.AVX2.*` | 128-bit and 256-bit Register and RegisterMask behavior under AVX2 |
| `Register.AVX2Preconditions.*` | Marker-gated Register alignment and runtime-shift precondition failures |
| `UInt128Optimized.*` | UInt128 with compiler carry primitives, BMI1 extraction, and available SIMD support |
| `UInt128Portable.*` | UInt128 with portable carry/borrow and BMI1 extraction disabled |
| `UInt128Scalar.*` | UInt128 with all SIMD, BMI, FMA, and compiler-carry features disabled |
| `UInt128ResultSetEquivalence` | Optimized-versus-portable deterministic result digest |
| `UInt128ScalarResultSetEquivalence` | Optimized-versus-scalar deterministic result digest |
| `FMA.Enabled.*` | FMA-enabled dispatch and expected result |
| `FMA.Disabled.*` | Non-FMA fallback dispatch and expected result |
| `Bmi.Bmi1.*` | BMI1 intrinsic profile |
| `Bmi.Bmi1.Equivalence` | BMI1-versus-portable deterministic result digest |
| `Bmi.Bmi2.*` | BMI2 intrinsic profile |
| `Bmi.Bmi2.Equivalence` | BMI2-versus-portable deterministic result digest |
| `Bmi.Bmi1Bmi2.*` | Combined BMI1/BMI2 intrinsic profile |
| `Bmi.Bmi1Bmi2.Equivalence` | Combined-profile-versus-portable deterministic result digest |
| `VectorAlgorithms.*` | `SimdVector`, `SimdAlgo`, and SIMD `SimdResample` behavior |
| `VectorChecks.*` | Checks-enabled partial and full-vector result validation |
| `ResampleScalar.*` | Scalar-only `SimdResample` behavior and oracle parity |
| `Preconditions.*` | Individually discovered terminating caller-facing precondition contracts; marker-gated CTest success |
| `ApiExamples` | Public C++20 call sites compiled and run together |
| `RegisterExamples` | Public C++23 Register call sites compiled and run together |

Compile-only targets cover:

- `ApiDisabledProbe` and `ApiEnabledProbe` for API availability, supported lane
  types, register widths, and conversion constraints;
- `ConfigDefaultProbe`, `ConfigDisabledInstructionsProbe`,
  `ConfigDisabledPublicHeadersProbe`, `MethodFlagsConfigOverrideProbe`,
  `ConfigOverridePreconditionProbe`, `ConfigVendorAttributeProbe`,
  `ConfigClangUnsupportedTargetProbe`, and `ConstexprProbe` for detection,
  override, disabled, attribute, target, and constant-evaluation paths;
- first-and-only include probes for `Aliases.h`, `Api.h`, `Bmi.h`, `Config.h`,
  `Format.h`, `SimdAlgo.h`, the deprecated `SimdApi.h` compatibility include,
  `SimdLib.h`, `SimdResample.h`, `SimdVector.h`, `TemplateTools.h`, and `UInt128.h`; and
- `PublicSurfaceHeaderProbe` for the supported umbrella/focused-header boundary
  and the guard against public `Detail` dependencies; and
- dedicated BMI, UInt128, 128/256-bit API/vector, and disabled-feature constexpr
  targets aggregated by `ConstexprProbes`.

The constexpr sources are ordinary object-library probes aggregated by
`ConstexprProbes`, which is owned by `ExhaustiveArtifacts`. The
`ConstexprProbes.Artifacts` CTest entry validates their recorded object hashes
without recompiling them. Profile ownership is:

| Contract source | Compile profiles |
| --- | --- |
| `BmiConstexpr.tests.cpp` | Portable, BMI1 only, BMI2 only, and BMI1 with BMI2 |
| `UInt128Constexpr.tests.cpp` | Compiler carry, portable carry, and scalar with SIMD, BMI, and FMA disabled |
| `Api128Constexpr.tests.cpp` | SSE4.2 public API and four-lane `SimdVector` |
| `Api256Constexpr.tests.cpp` | AVX2 public API and eight-lane `SimdVector` |
| `ApiDisabledConstexpr.tests.cpp` | All instruction families disabled |

Runtime parity targets rebuild deterministic inputs through volatile scalars
before exercising comparisons, extrema, lane shifts, addition, and subtraction.
MSVC x64 owns the `_addcarry_u64` and `_subborrow_u64` UInt128 path; Clang and
GCC own the `__builtin_add_overflow` and `__builtin_sub_overflow` path. Portable
and scalar profiles disable compiler carry intrinsics.

Production `static_assert` declarations remain local constraints and diagnostics
in their owning headers. Public-header probes and dedicated constexpr targets
compile those declarations under the applicable compiler profiles; no
source-text occurrence count is treated as correctness or compile-time evidence.

`tests/consumer` separately imports the source tree through
`add_subdirectory`, verifies that `SimdLib::SimdLib` is an interface target,
and runs an external header-only consumer. `Core.benchmarks.cpp` is the core
benchmark executable and samples 128/256-bit API addition, BMI extraction,
UInt128 addition, and resampling; each operation also has a correctness test.

## Public surface map

| Surface | Directly covered contracts | Profiles | Remaining gap or justification |
| --- | --- | --- | --- |
| `Api` | Arithmetic, signed and unsigned comparisons, equality masks, movemasks, loads/stores, unaligned and partial transfers, same-shape transforms, packed transforms with full batches and tails, conversion between signed 32-bit lanes and float, shifts, logical shuffles for all arithmetic element types, blends, reductions, casts, extraction, and register metadata | 128-bit SSE and 256-bit AVX2; FMA on/off; availability-disabled probes | Some inherited backend helper names are implementation exposure rather than a promised public family. Exhaustively testing them would freeze an accidental contract; the inheritance boundary should be clarified before such tests are added. More conversion rounding/overflow cases are medium-risk follow-up work. |
| `SimdVector` | Construction, lane access, arithmetic, comparisons, masks, partial divide/modulus/clamp identity handling, direct active-lane area reduction, lane-local magnitudes, 128/256-bit float/double dot products, floating hashing, inactive-lane min/max behavior, and checks-enabled result validation | Representative signed, unsigned, float, and double lane types; full and partial 128/256-bit extents; Release and checks-enabled profiles | Convenience overloads that delegate directly to `Api` are not all tested individually. Their underlying behavior is covered; add overload-specific tests when they acquire distinct contracts. |
| `SimdAlgo` | `AnyEqual` and `AllEqual` full-register/tail outcomes, bitwise transforms, conversions, comparison packing, scalar parity, non-register-multiple tails, and destination canaries | Read widths 8/16/32/64; empty, single, multi-element, exact-register, multi-register, and tail extents | General comparison currently supports `WriteWidth == 1`; unsupported widths are a compile-time precondition, not an untested runtime branch. |
| `SimdResample` | Scalar-reference parity for reductions and expansion, boundary dimensions, randomized inputs, and SIMD/scalar equivalence | SIMD enabled and scalar-only profiles | No material gap found. This remains the strongest standalone surface. |
| `Bmi` | BMI1/BMI2 operations, portable and intrinsic equivalence, signed bit-pattern preservation, boundary indices/counts, generic 128-bit support, constexpr evaluation, and deterministic randomized scalar oracles | BMI1 only, BMI2 only, both, and portable | Several derived helpers have names whose edge semantics are not independently specified. Existing production contracts remain covered, but new tests should follow an API-contract review rather than canonizing incidental behavior. |
| `uint128_t` | Construction, signed/unsigned heterogeneous comparisons, arithmetic, carry/borrow, boolean and integral shifts, dynamic and fixed-width masks, deprecated extraction compatibility, `Bmi::bextr`, numeric-limit sentinels, bit helpers including `bit_ceil` overflow, register conversion, constexpr behavior, randomized native/scalar oracle parity, and optimized/portable/scalar digest equivalence | Optimized compiler carry, portable carry, and all SIMD/BMI/FMA disabled | Division and remainder are not public operations. Deprecated `extract` is compatibility-only; `Bmi::bextr` remains the preferred API. |
| Formatters | UInt128 decimal/binary/octal/hex output, signs, alternate forms, width/alignment/fill/zero padding, scalar `uint64_t` parity where the value fits, accepted/rejected grammar, integral and floating vectors, header isolation, and multi-TU ODR | Opt-in `Format.h` and umbrella include | Locale-specific formatting is deliberately rejected so output remains locale-independent; no formatter-contract gap remains. |
| `Config` and `TemplateTools` | Compiler/target detection, feature constants, caller overrides, disabled public headers, type availability, automatic `NativeApi` width selection, fixed alias widths, concepts, constexpr loops, tuple iteration, and constant evaluation | Default, override, disabled, MSVC, clang-cl, and Clang | Unavailable `Api` instantiations are tested through availability concepts instead of intentional hard-error compile failures. |

## Runtime preconditions and failure contracts

The complete call-site classification is recorded in
[`PreconditionInventory.md`](PreconditionInventory.md). There are 13
caller-facing runtime contracts: four `Api` transfer checks, five dynamic
`SimdAlgo` span-shape checks, and four `SimdResample` extent checks. Each is
an independently discovered negative Catch2 case. The remaining public-header
call site is the checks-enabled `SimdVector` inactive-lane result invariant; it
is not caller-triggerable through a supported operation, so its direct proof
observes successful partial-vector checks and the full-vector bypass.

`PreconditionTests` overrides `SIMDLIB_PRECONDITION`, writes the
private `SIMDLIB_PRECONDITION_FAILURE_EXPECTED_18A7E3` marker to stderr,
flushes it, and exits with diagnostic status 73 on failure. CTest discovers
each Catch2 case as a separate process and requires that marker for success;
a missing marker, access violation, unrelated crash, or timeout fails the
case. The executable's target-aware coverage prefix is
`Preconditions`, so its terminating profiles map only to
that executable in the LCOV report. The override remains active in Release,
where the default `assert` policy is compiled out by `NDEBUG`.

The failure matrix directly covers undersized `load_partial` and raw-byte
`Api::store` spans, misaligned aligned load/store addresses, every dynamic
bitwise span mismatch, and every resampling size-ratio mismatch. The audit found
no runtime `SIMDLIB_PRECONDITION` governing an index, divisor, or overlap;
compile-time constraints and explicitly unsafe entry points retain their
existing classifications.

The precondition inventory assigns every isolated failure scenario to the
MSVC Release, Clang coverage, and Clang ASan/UBSan cells. Its valid-boundary
cases cover exact aligned/raw capacities, empty and one-element partial loads,
matching empty/one-element algorithm spans, and empty/minimum resampling
shapes. The target-aware coverage configuration includes the failure probes
and public API example as independently owned executable profiles.

## SimdVector full, partial, and wide-vector matrix

| Contract | Direct proof |
| --- | --- |
| Divide and modulus | Three-lane `int32_t` vectors pass a raw divisor register whose inactive lane is zero. Both value-returning and compound operators produce exact active quotients/remainders and restore the inactive result lane to zero, proving the divisor is filled with multiplicative identity before evaluation. Matching full four-lane cases prove the non-partial route. |
| Clamp | A partial `int32_t` vector uses per-lane lower/upper registers with adversarial inactive bounds (`100` and `-100`); active results match their individual bounds and the inactive result is zero. A full four-lane scalar-bound case covers the direct route. |
| `area` | Signed `int8_t[5]`, cross-128-bit-lane `int16_t[9]` and `int64_t[3]`, unsigned `uint16_t[5]`, cross-128-bit-lane `uint8_t[17]` and `uint32_t[5]`, odd signed `int32_t[3]`, full `int32_t[4]`, and `int64_t[2]` cases exercise narrow/wide types, odd counts, full/partial reductions, both register halves, and modular signed overflow. |
| Integer magnitude | Every signed and unsigned lane width at 128 and 256 bits covers unchecked representable inputs, checked representable inputs, exact maximum boundaries, multi-lane overflow, and signed-minimum overflow. Partial 256-bit `int16_t[9]` and `uint8_t[17]` vectors verify sparse group-leading magnitudes and adjacent checked overflow masks in both 128-bit halves; an isolated first high-half value makes omission or cross-group mixing observable. |
| Min/max position | Partial `uint16_t[3]` proves inactive zero lanes cannot win; full `uint16_t[8]` proves the no-fill route and exact positions. |
| Float dot product | A partial 128-bit three-float case remains covered. Counts four through eight cover full 128-bit, partial 256-bit, and full 256-bit vectors; counts five through eight require the high 128-bit lane to contribute to the scalar result. |
| Double dot product | Counts one through four cover partial/full 128-bit and partial/full 256-bit vectors. The three- and four-element cases require the high 128-bit lane to contribute. |
| Floating hash | Nonzero float and double vectors assert nonzero hashes, copy/equal-value consistency, and selected distinct logical-lane results. Infinity and two representative NaN encodings per type are evaluated with copy consistency; no assertion requires unequal NaNs to hash differently. Existing float and double `+0`/`-0` equality and equal-hash regressions remain direct. |
| Debug result validation | `VectorChecksTests` forces `SIMDLIB_ENABLE_CHECKS=1` and installs an observing precondition hook for partial and full-vector result checks. |

The cross-lane `area` case exposed a register-shape defect: recursive pair
reduction could infer a narrower `SimdVector` even though its pair-product
register retained the original 256-bit width. `area` now converts the original
register to an array and multiplies only the compile-time-bounded active lanes.
This removes inactive-lane reconstruction, partner shuffling, adjacent-product
emulation, promoted-register extraction, and the width mismatch. Accumulation
still uses the unsigned object representation so signed overflow remains
modular. Narrow cross-lane vectors no longer require unsupported 512- or
1024-bit widened intermediates.

Separate public-vector and checks-enabled profiles keep partial-result
validation distinct from full-vector specializations. The coverage contract
requires direct area reduction across 8-, 16-, 32-, and 64-bit lanes and
requires high-lane contributions in the floating-point dot-product cases.

## uint128_t boundary and compatibility matrix

| Contract | Direct boundary proof |
| --- | --- |
| Heterogeneous comparison | Signed integral cases cover a negative right operand, zero, matching and mismatching positive low words, a nonzero high word, equality true/false, and less/equal/greater ordering. Unsigned cases separately retain matching/mismatching equality and all three ordering results. |
| Deprecated dynamic `extract` | Volatile-derived runtime calls cover zero length, bit 127, starts 128 and 200, a 12-bit range crossing bit 64, and a 16-bit request truncated at bit 127. Every result is checked against an exact value and the preferred `Bmi::bextr` call. |
| `Bmi::bextr(uint128_t)` | The length/start and encoded-control overloads use the same boundary table. An independent two-word oracle exhaustively covers all 65,536 encoded start/length combinations across zero, all-ones, mixed-word, and sparse-boundary inputs. Profile digests additionally compare randomized controls across optimized, portable, and scalar configurations. |
| `create_mask<Width>(offset)` | Runtime offsets cover negative, zero, 63, 64, 127, 128, and 129 for a five-bit mask, proving unchanged, cross-word, truncated-final-bit, and empty out-of-range results. Existing cases retain representative widths 1, 64, 65, and 128. |
| Shift counts | Volatile-derived values cover `false`, `true`, negative, zero, 1, 63, 64, 65, 127, 128, 129, 191, 255, 256, and `UINT64_MAX` for both directions. The optimized profile executes SIMD shifts; the scalar-only profile executes the two-word branches. |
| `numeric_limits` sentinels | Runtime assertions cover `min`, `lowest`, `max`, `epsilon`, `round_error`, `infinity`, `quiet_NaN`, `signaling_NaN`, and `denorm_min`. All non-finite/fractional sentinels are zero because this is an exact bounded unsigned integer type. |
| `bit_ceil` | Volatile-derived inputs cover zero, one, an ordinary low-word value, an ordinary high-word value, the largest value that rounds to `2^127`, exact `2^127`, and overflow from both `2^127 + 1` and the all-ones value. |

The deprecated dynamic member `extract` remains a tested compatibility contract
because it is still part of the public class and has documented boundary
behavior. This coverage does not promote it for new callers: its deprecation and
the preferred `Bmi::bextr` replacement remain unchanged. If the deprecated API
is intentionally removed later, its compatibility tests should be removed with
the declaration rather than transferred into a new preferred surface.

The optimized, portable-carry, and scalar-only executables run the same focused
boundary cases. Separate Clang profiles preserve object/profile provenance:
the scalar profile owns comparison, extraction/truncation, five-bit mask
offsets, boolean normalization, zero/oversized shifts, and `bit_ceil`; the
optimized profile owns runtime SIMD shift dispatch. Randomized two-word and
compiler-native oracles plus optimized-versus-portable and
optimized-versus-scalar result-set comparisons remain part of the inventory.

## Formatter grammar matrix

| Formatter | Accepted grammar and direct proof |
| --- | --- |
| `SimdVector` | The empty specification and `[[fill]align][width]` are exercised with default, right, left, and center alignment, custom fill, sufficient width, and insufficient width. Formatting applies to the completed `{a, b, c}` container; each logical element retains its default scalar presentation. |
| `uint128_t` | The empty specification and `[[fill]align][sign][#][0][width][type]` are exercised. Direct cases cover `+`, space, and unsigned-default `-`; decimal, hexadecimal, uppercase hexadecimal, binary, uppercase binary, and octal; alternate prefixes; default and explicit alignment; custom fill; zero padding; sufficient and insufficient widths; and full-width 128-bit boundary values. |

| Rejected category | `SimdVector` proof | `uint128_t` proof |
| --- | --- | --- |
| Brace fill | Runtime parsing rejects `{` as a two-character fill/alignment specification; full `std::vformat` parsing rejects `}` because it terminates the replacement field before the following alignment text. | Same. |
| Precision | `.3` is rejected. | `.2` is rejected. |
| Dynamic width | `{}` is rejected directly and through `std::vformat`. | Same. |
| Nested replacement field | `>{}` is rejected directly and through `std::vformat`. | Same. |
| Locale | `L` is rejected. | `L` is rejected. |
| Unsupported presentation | `x` is rejected because vectors expose no container presentation type. | `q` is rejected. |
| Extra trailing input | `20x` is rejected after the otherwise valid width. | `dx` is rejected after the valid decimal presentation. |
| Width overflow | The vector formatter delegates width representation to the standard string formatter and has no SimdLib-specific arithmetic guard. | A decimal width greater than `SIZE_MAX` reaches the checked accumulation guard and throws `std::format_error` before formatting or allocation. |

Locale-specific formatting is intentionally unsupported. Both formatters reject
`L` at their own container/integer grammar boundary; vector elements are rendered
with the locale-independent default `{}` specification. This keeps SimdLib output
deterministic instead of inheriting locale behavior from scalar formatters.

Every accepted `uint128_t` format shared with the standard unsigned formatter is
checked against `uint64_t` over zero, small values, a mixed high-bit pattern, and
`UINT64_MAX`. Alternate-octal cases additionally assert exact scalar parity for
zero and nonzero values across default alignment, explicit alignment, zero
padding, and insufficient widths. `Format.h` remains the first include in its
standalone header probe, and the formatter specializations remain linked and run
from two translation units by `FormatOdr`.

The formatter runtime inventory executes in every applicable Release cell and
the representative Debug, sanitizer, and coverage profiles. Applicable Release
compiler cells alone own `FormatOdr` and the `Format.h` first-include probe. The
dedicated Clang coverage profile exercises checked width overflow, both
trailing-input outcomes, alternate-octal zero and nonzero outcomes, explicit
and default alignment, and both insufficient-width zero-padding outcomes.

## SimdAlgo outcome and boundary matrix

| Operation and shape | Direct outcomes |
| --- | --- |
| `AnyEqual`, three exact 256-bit registers | No match returns false after the complete traversal; matches in the first, middle, and final register each return true. |
| `AnyEqual`, three registers plus a three-element tail | A tail with no match returns false; a match only in the final tail element returns true. |
| `AllEqual`, three exact 256-bit registers | Uniform input returns true; mismatches in the first, middle, and final register each return false. |
| `AllEqual`, three registers plus a three-element tail | A uniform tail returns true; a mismatch only in the final tail element returns false. |
| Static extents | Empty, one-element, three-element, exact 128-bit-register, and one-past-register extents run for every 8/16/32/64-bit read type. Empty `AnyEqual` directly exercises `LowBits` with a zero count; empty `AllEqual` verifies vacuous truth. |
| Packed and bitwise output | Packed comparisons retain scalar-reference parity and front/back canaries for 24- and 40-element inputs at every read width. The nine-element 32-bit bitwise tail retains scalar parity for all five transforms and front/back canaries. |

`LowBits` is called only when `count < SimdImpl<count>::element_count`.
The 128-bit selection therefore admits at most 15 elements (for 8-bit
reads), while selecting the 256-bit implementation requires at least its full
lane count and cannot enter that branch. Consequently, the `count >= 32`
safety case is unreachable through any supported public `AnyEqual`
instantiation; directly exposing the private helper solely for a test would
create an implementation test seam.

A dedicated Clang profile owns the zero-count `LowBits` return, both outcomes
of the full-register search conditions, exact-traversal returns, and both tail
results. The `count >= 32` branch remains structurally unreachable through the
public API for the reason above.

## High-risk findings resolved by the audit

- `Api::load_partial` used an aligned full-register load despite accepting a
  span with no alignment contract. Full partial-count loads are now unaligned.
- `Api::element_width` used `numeric_limits<T>::digits`, which excluded the sign
  bit and broke 32-bit conversion constraints. It now reports object width.
- `is_api_available_v` accepted arithmetic types without an implementation
  mapping. It now accepts only the canonical supported lane types.
- `SimdAlgo::Compare` could read beyond a partial tail, overwrite the packed
  destination, and depend on pre-zeroed output. Tail staging, exact packing,
  output initialization, and canary regressions now cover the contract.
- Partial signed `SimdVector` bitwise NOT and the former pair-product path
  used a signed
  numeric maximum instead of an all-bits mask. Signed bit-pattern tests cover
  the corrected behavior.
- Partial `SimdVector` min/max position could select an inactive zero lane.
  Tests now cover nonzero active lanes and inactive-lane sentinels.
- Floating `SimdVector` hashing distinguished `+0` and `-0` even though they
  compare equal. Float and double equality/hash regressions now cover this.
- Wide `SimdVector::area` recursively wrapped pair-product registers in
  reduced-count vectors that could select narrower or unsupported storage
  widths. Direct active-lane reduction removes the intermediate vector and
  covers narrow, wide, odd, and cross-128-bit-lane active counts.
- `Bmi::abs` used an incorrect sign shift and could invoke signed overflow.
  It now works on unsigned object representations with boundary tests,
  including signed minima.
- Integer-like concepts admitted `bool` and floating-point types. Compile-time
  probes now enforce integer, non-boolean types.
- `TemplateTools.h` relied on transitive tuple/concepts includes. Its isolated
  header probe now instantiates the public utilities.
- The configuration matrix lacked an all-features-disabled umbrella build, and
  formatter specializations lacked a multi-translation-unit ODR test. Both are
  now explicit CTest entries.

## Deterministic oracle inputs

The randomized/property suites are reproducible. BMI uses seeds
`0x4F1BBCDC6762FA9D`, `0xC001D00D12345678`, `0x9E3779B97F4A7C15`,
`0xD1B54A32D192ED03`, and `0xA0761D6478BD642F`. UInt128 uses
`0xD1B54A32D192ED03`, `0x94D049BB133111EB`, and
`0xA0761D6478BD642F`. Resampling derives its `std::mt19937` seed from
the tested dimensions so a failing case can be reproduced directly.
Table-driven API comparison and partial-transfer checks report the lane type,
register width, active count, and failing values through Catch2 captures.

## Source-based coverage

Clang's LLVM instrumentation is available through
`SIMDLIB_ENABLE_COVERAGE`. CMake 3.31 or newer drives the instrumented CTest
inventory, after which the pipeline invokes `llvm-profdata`, `llvm-cov`, and
`llvm-readobj` directly to generate the source-coverage report. Coverage
configuration intentionally fails for unsupported compiler drivers rather
than silently producing misleading data.

The unified native coverage fingerprint makes CTest the authoritative runner.
From the SimdLib repository root:

```powershell
tools/Build.ps1 -Scope Native -Compiler ClangCoverage
tools/Run-Tests.ps1 -Scope Native -Compiler ClangCoverage
```

The coverage operation resets profiles, runs the instrumented CTest inventory,
and generates `coverage.info` in the receipt-owned directory
`out/pipeline/windows-clang-coverage/debug-coverage-<fingerprint>/build`.
The workspace does not configure a static CMake Tools import path because a
literal “latest” alias could display coverage from an incompatible or stale
fingerprint. Open or import the `coverage.info` referenced by the current
receipt when inspecting coverage in an editor.

Coverage report generation does not merge differently configured executables
into one `llvm-profdata` database. CMake generates
`coverage-targets-Debug.txt` in that same fingerprint-owned build directory,
which records each instrumented
executable, its object path, and its CTest profile prefix. The report target
also reads the embedded platform binary identity (COFF/PDB on this baseline)
from every executable and profile. This identity maps CTest-created
`.profdata` files and retained generic `.profraw` files to their one producing
executable. Filename prefixes
are a second consistency check for CTest-named profiles.

Profiles are merged only within one executable. `llvm-cov export` then emits
one LCOV trace per executable, and `cmake/MergeLcov.cmake` deterministically
accumulates repeated source records. Line and function execution counts are
summed. Function identities are unioned by source line and symbol. Branch
records from mutually exclusive configurations remain distinct instead of
colliding by LLVM instrumentation index. Multi-executable equivalence-test
profiles are excluded because their constituent executables already have
single-object profiles. The report fails on an unknown binary identity, a
filename/identity disagreement, a missing executable profile, any LLVM export
diagnostic, or an export with no SimdLib source records.

### Execution evidence

Coverage percentages, test and profile counts, elapsed times, generated-file
hashes, compiler and tool versions, and line-number-specific exclusion reviews are
execution evidence. Keep them in the generated reports and artifacts below the
owning fingerprint rather than duplicating them as enduring claims in this
coverage contract.

LLVM runtime profiles cannot increment constant-evaluation-only branches.
Compile-time probes therefore own those contracts, while compiler-specific
runtime branches remain assigned to their corresponding compiler cells. The
generated LCOV report remains authoritative for the exact line, branch, and
function totals of a particular run.

Consumer-header compile-time measurements are also execution evidence rather
than correctness gates. Their method and results belong in the validation record
for the run that produced them.

Generated `.profraw`, `.profdata`, LCOV, binary, object, log, and temporary
analysis files remain ignored and untracked.

## VS Code coverage inspection

The workspace recommends VS Code CMake Tools through `.vscode/extensions.json`
and keeps CTest Test Explorer integration enabled. Coverage generation is owned
by the formal fingerprinted command rather than a static workspace path. After
that command completes, an LCOV-capable editor extension can open the current
receipt's `coverage.info`. Execution totals come from that generated LCOV file;
editor rendering is not validation evidence.

## Coverage expansion policy

Every supported operation/type cell in `ApiOperationMatrix.md` requires a
direct public test. Generated zero-count source lines must be classified in the
execution evidence for the run that produced them. Candidate expansion areas
include:

- inherited backend names that are not supported public operation/type cells;
- conversion rounding/overflow and direct-transform overlap behavior beyond
  the current documented cases;
- convenience overloads whose behavior delegates to directly tested
  core operations; and
- review of operation/type cells marked `unavailable` before deciding whether
  any should gain an
  implementation.
