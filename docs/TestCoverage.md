# Test coverage audit

This document records the standalone SimdLib coverage audit completed on
2026-07-19. Coverage percentages are supporting evidence; the behavioral map
and the feature-profile matrix are the acceptance criteria.

## Coverage layers

| Layer | Evidence |
| --- | --- |
| Runtime behavior | Catch2 suites for `Api`, `Bmi`, `uint128_t`, formatting, `SimdAlgo`, `SimdVector`, and `SimdResample` |
| Compile-time behavior | `static_assert` contracts in the Catch2 sources and the configuration, availability, and public-header probes |
| Header isolation | Every public header is compiled as the first and only SimdLib include; the umbrella header has a separate probe |
| Configuration | Default detection, caller overrides, all instruction families disabled, FMA enabled/disabled, BMI1/BMI2 independently enabled, and portable/optimized/scalar UInt128 profiles |
| Formatter and ODR | Scalar-formatter parity, vector and UInt128 formatting, umbrella/focused-header probes, and a two-translation-unit formatter executable |
| Oracle/property testing | Deterministic scalar oracles for comparisons, transfers, BMI operations, UInt128 arithmetic/bit operations, algorithms, and resampling |
| Compiler/runtime diagnostics | Strict Release builds on MSVC 19.44 and clang-cl 22.1.8; Clang 22.1.8 ASan/UBSan Debug run |
| External consumer | `tests/consumer` validates source-tree import, the interface-library target, public includes, and header-only linkage |

Benchmarks are intentionally excluded from correctness counts. They exercise
representative optimized operations but have their own performance purpose and
acceptance rules.

## Test inventory

The standard Clang coverage preset contributes 182 CTest entries: 172
individual Catch2 test cases discovered by `catch_discover_tests()` and 10
direct CTest audit, compile, example, and equivalence tests. The 13
terminating precondition cases are discovered Catch2 cases, not direct CTest
driver scenarios. Catch2 executables remain grouped by these stable name
prefixes:

| Entry | Coverage role |
| --- | --- |
| `SimdLib.HeaderOnlySmoke` | Multi-translation-unit umbrella-header use and header-only linkage |
| `SimdLib.Tests.BmiPortable.*` | Portable BMI behavior, constexpr checks, boundaries, signed bit patterns, and deterministic randomized oracles |
| `SimdLib.Tests.Format.*` | UInt128 and vector formatter behavior plus standard scalar parity |
| `SimdLib.FormatOdr` | Formatter specialization linkage across two translation units |
| `SimdLib.Tests.SSE42.*` | 128-bit `Api`, partial transfers, comparisons, conversion, movemask, and register metadata |
| `SimdLib.Tests.UInt128Optimized.*` | UInt128 with compiler carry primitives and available SIMD support |
| `SimdLib.Tests.UInt128Portable.*` | UInt128 with portable carry/borrow |
| `SimdLib.Tests.UInt128Scalar.*` | UInt128 with all SIMD, BMI, FMA, and compiler-carry features disabled |
| `SimdLib.Tests.UInt128ResultSetEquivalence` | Optimized-versus-portable deterministic result digest |
| `SimdLib.Tests.UInt128ScalarResultSetEquivalence` | Optimized-versus-scalar deterministic result digest |
| `SimdLib.Tests.AVX2.*` | 256-bit `Api`, partial transfers, comparisons, movemask, and register metadata |
| `SimdLib.Tests.FMA.Enabled.*` | FMA-enabled dispatch and expected result |
| `SimdLib.Tests.FMA.Disabled.*` | Non-FMA fallback dispatch and expected result |
| `SimdLib.Tests.Bmi.Bmi1Only.*` | BMI1 intrinsic profile |
| `SimdLib.Tests.Bmi.Bmi1Only.Equivalence` | BMI1-versus-portable deterministic result digest |
| `SimdLib.Tests.Bmi.Bmi2Only.*` | BMI2 intrinsic profile |
| `SimdLib.Tests.Bmi.Bmi2Only.Equivalence` | BMI2-versus-portable deterministic result digest |
| `SimdLib.Tests.Bmi.Bmi1AndBmi2.*` | Combined BMI1/BMI2 intrinsic profile |
| `SimdLib.Tests.Bmi.Bmi1AndBmi2.Equivalence` | Combined-profile-versus-portable deterministic result digest |
| `SimdLib.Tests.VectorAlgorithms.*` | `SimdVector`, `SimdAlgo`, and SIMD `SimdResample` behavior |
| `SimdLib.Tests.ResampleScalar.*` | Scalar-only `SimdResample` behavior and oracle parity |
| `SimdLib.Tests.Preconditions.*` | Individually discovered terminating caller-facing precondition contracts; marker-gated CTest success |
| `SimdLib.ApiExamples` | Public documented call sites compiled and run together |

Compile-only targets cover:

- `ApiDisabledProbe` and `ApiEnabledProbe` for API availability, supported lane
  types, register widths, and conversion constraints;
- `ConfigDefaultProbe`, `ConfigDisabledInstructionsProbe`,
  `ConfigDisabledPublicHeadersProbe`, `ConfigOverrideForceInlineProbe`,
  `ConfigOverridePreconditionProbe`, `ConfigOverrideVectorcallProbe`,
  `ConfigVendorAttributeProbe`, `ConfigClangUnsupportedTargetProbe`, and
  `ConstexprProbe` for detection, override, disabled, attribute, target, and
  constant-evaluation paths;
- first-and-only include probes for `Api.h`, `Bmi.h`, `Config.h`, `Format.h`,
  `SimdAlgo.h`, the deprecated `SimdApi.h` compatibility include, `SimdLib.h`,
  `SimdResample.h`, `SimdVector.h`, `TemplateTools.h`, and `UInt128.h`; and
- `PublicSurfaceHeaderProbe` for the supported umbrella/focused-header boundary
  and the guard against public `Detail` dependencies; and
- dedicated BMI, UInt128, 128/256-bit API/vector, and disabled-feature constexpr
  targets aggregated by `SimdLibConstexprProbes`.

The retained-assertion classifications and mechanical allowlist are recorded in
[`StaticAssertionInventory.md`](StaticAssertionInventory.md). The complete
constexpr/compiler matrix, runtime-path evidence, and consumer compile-time
measurements are recorded in
[`ConstexprCompilerEvidence.md`](ConstexprCompilerEvidence.md).

`tests/consumer` separately imports the source tree through
`add_subdirectory`, verifies that `SimdLib::SimdLib` is an interface target,
and runs an external header-only consumer. `SimdLib.benchmarks.cpp` is the sole
benchmark executable and samples 128/256-bit API addition, BMI extraction,
UInt128 addition, and resampling; each operation also has a correctness test.

## Public surface map

| Surface | Directly covered contracts | Profiles | Remaining gap or justification |
| --- | --- | --- | --- |
| `Api` | Arithmetic, signed and unsigned comparisons, equality masks, movemasks, loads/stores, unaligned and partial transfers, same-shape transforms, packed transforms with full batches and tails, conversion between signed 32-bit lanes and float, shifts, shuffles, blends, reductions, casts, extraction, and register metadata | 128-bit SSE and 256-bit AVX2; FMA on/off; availability-disabled probes | Some inherited backend helper names are implementation exposure rather than a promised public family. Exhaustively testing them would freeze an accidental contract; the inheritance boundary should be clarified before such tests are added. More conversion rounding/overflow cases are medium-risk follow-up work. |
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

`SimdLibPreconditionTests` overrides `SIMDLIB_PRECONDITION`, writes the
private `SIMDLIB_PRECONDITION_FAILURE_EXPECTED_18A7E3` marker to stderr,
flushes it, and exits with diagnostic status 73 on failure. CTest discovers
each Catch2 case as a separate process and requires that marker for success;
a missing marker, access violation, unrelated crash, or timeout fails the
case. The executable's target-aware coverage prefix is
`SimdLib.Tests.Preconditions`, so its terminating profiles map only to
that executable in the LCOV report. The override remains active in Release,
where the default `assert` policy is compiled out by `NDEBUG`.

The failure matrix directly covers undersized `load_partial` and raw-byte
`Api::store` spans, misaligned aligned load/store addresses, every dynamic
bitwise span mismatch, and every resampling size-ratio mismatch. The audit found
no runtime `SIMDLIB_PRECONDITION` governing an index, divisor, or overlap;
compile-time constraints and explicitly unsafe entry points retain their
existing classifications.

Focused MSVC Release, Clang coverage, and Clang ASan/UBSan runs each pass all
13 isolated failure scenarios. The valid-boundary selection passes 19
assertions across three cases and covers exact aligned/raw capacities, empty
and one-element partial loads, matching empty/one-element algorithm spans, and
empty/minimum resampling shapes. The complete strict suites pass 179/179 with
MSVC Release and 182/182 with Clang coverage. Clang 22.1.8 ASan/UBSan Debug
passes 151/151 with no diagnostics. The target-aware coverage report maps 190
profiles to 19 executables, including all 13 failure-probe profiles and the
public API example executable.

## SimdVector full, partial, and wide-vector matrix

| Contract | Direct proof |
| --- | --- |
| Divide and modulus | Three-lane `int32_t` vectors pass a raw divisor register whose inactive lane is zero. Both value-returning and compound operators produce exact active quotients/remainders and restore the inactive result lane to zero, proving the divisor is filled with multiplicative identity before evaluation. Matching full four-lane cases prove the non-partial route. |
| Clamp | A partial `int32_t` vector uses per-lane lower/upper registers with adversarial inactive bounds (`100` and `-100`); active results match their individual bounds and the inactive result is zero. A full four-lane scalar-bound case covers the direct route. |
| `area` | Signed `int8_t[5]`, cross-128-bit-lane `int16_t[9]` and `int64_t[3]`, unsigned `uint16_t[5]`, cross-128-bit-lane `uint8_t[17]` and `uint32_t[5]`, odd signed `int32_t[3]`, full `int32_t[4]`, and `int64_t[2]` cases exercise narrow/wide types, odd counts, full/partial reductions, both register halves, and modular signed overflow. |
| Integer magnitude | Partial 256-bit `int16_t[9]` and `uint8_t[17]` inputs produce exact lane-local magnitudes in both 128-bit halves. The first high-lane active value is isolated so omission or cross-lane mixing is observable. |
| Min/max position | Partial `uint16_t[3]` proves inactive zero lanes cannot win; full `uint16_t[8]` proves the no-fill route and exact positions. |
| Float dot product | A partial 128-bit three-float case remains covered. Counts four through eight cover full 128-bit, partial 256-bit, and full 256-bit vectors; counts five through eight require the high 128-bit lane to contribute to the scalar result. |
| Double dot product | Counts one through four cover partial/full 128-bit and partial/full 256-bit vectors. The three- and four-element cases require the high 128-bit lane to contribute. |
| Floating hash | Nonzero float and double vectors assert nonzero hashes, copy/equal-value consistency, and selected distinct logical-lane results. Infinity and two representative NaN encodings per type are evaluated with copy consistency; no assertion requires unequal NaNs to hash differently. Existing float and double `+0`/`-0` equality and equal-hash regressions remain direct. |
| Debug result validation | `SimdLibTestsVectorChecks` forces `SIMDLIB_ENABLE_CHECKS=1` and installs an observing precondition hook. Divide, modulus, and clamp on a partial vector invoke the inactive-lane result check three times with true conditions; the same operations on a full vector invoke it zero times. |

The cross-lane `area` case exposed a register-shape defect: recursive pair
reduction could infer a narrower `SimdVector` even though its pair-product
register retained the original 256-bit width. `area` now converts the original
register to an array and multiplies only the compile-time-bounded active lanes.
This removes inactive-lane reconstruction, partner shuffling, adjacent-product
emulation, promoted-register extraction, and the width mismatch. Accumulation
still uses the unsigned object representation so signed overflow remains
modular. Narrow cross-lane vectors no longer require unsupported 512- or
1024-bit widened intermediates.

Focused validation on 2026-07-19 passes 266 assertions across 14 public
`SimdVector` cases and 10 assertions in the checks-enabled case with both MSVC
Release and Clang coverage builds. Separate Clang profiles report 100.00% branch
coverage for both the public-vector and checks-enabled instantiations;
counters show three partial-result checks,
zero checks for the full-vector specializations, direct area reduction across
8-, 16-, 32-, and 64-bit lanes, four high-lane float dot additions, and two
high-lane double dot additions.
The complete strict suites pass 162/162 with MSVC Release and 165/165 with
Clang coverage.

## uint128_t boundary and compatibility matrix

| Contract | Direct boundary proof |
| --- | --- |
| Heterogeneous comparison | Signed integral cases cover a negative right operand, zero, matching and mismatching positive low words, a nonzero high word, equality true/false, and less/equal/greater ordering. Unsigned cases separately retain matching/mismatching equality and all three ordering results. |
| Deprecated dynamic `extract` | Volatile-derived runtime calls cover zero length, bit 127, starts 128 and 200, a 12-bit range crossing bit 64, and a 16-bit request truncated at bit 127. Every result is checked against an exact value and the preferred `Bmi::bextr` call. |
| `Bmi::bextr(uint128_t)` | The same table directly covers zero length, out-of-range starts, cross-word extraction, ordinary extraction, and truncation at the upper object boundary. |
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

The optimized, portable-carry, and scalar-only executables each run the same
six focused boundary cases with 131 assertions. Separate Clang profiles preserve
object/profile provenance: the scalar profile records both outcomes for
comparison, extraction/truncation, five-bit mask offsets, boolean normalization,
zero/oversized shifts, and `bit_ceil`; the optimized profile records runtime SIMD
shift dispatch. Randomized two-word and compiler-native oracles plus optimized-
versus-portable and optimized-versus-scalar result-set comparisons remain intact.

Validation on 2026-07-19 passes 35/35 focused `UINT128` tests with MSVC
Release and 38/38 with Clang Debug coverage. Each of the three Clang runtime
profiles passes 131 assertions across the six focused boundary cases. The
complete strict suites pass 154/154 and 157/157 respectively.

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
from two translation units by `SimdLib.FormatOdr`.

Validation on 2026-07-19 runs 267 assertions across the seven `[format]`
cases. The focused formatter and ODR matrix passes 8/8 with MSVC Release and
Clang Debug coverage, the `Format.h` first-include probe compiles with both
compilers, and the complete suites pass 148/148 and 151/151 respectively. A
dedicated Clang profile records the checked width-overflow throw once, both
trailing-input outcomes, alternate-octal zero and nonzero outcomes, explicit
and default alignment, and both outcomes of insufficient-width zero padding.

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

Validation on 2026-07-19 runs 614 assertions across the seven `[algo]` cases.
The focused matrix passes 7/7 with MSVC Release and Clang Debug coverage; the
complete suites pass 147/147 and 150/150 respectively. A dedicated Clang
profile records the zero-count `LowBits` return four times, both outcomes of
the full-register search conditions, exact-traversal returns, and both tail
results. The `count >= 32` return remains at zero as justified above.

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
the tested dimensions so a failing case can be reproduced directly. The final
manual Catch2 assertion inventory used decimal seed `1592594996` for both
release compiler matrices. New
table-driven API comparison and partial-transfer checks report the lane type,
register width, active count, and failing values through Catch2 captures.

## Source-based coverage

Clang's LLVM instrumentation is available through
`SIMDLIB_ENABLE_COVERAGE`. CMake 4.4 or newer is required because CTest 4.4 is
the first release with native `LLVM-COV` dashboard coverage support. Coverage
configuration intentionally fails for unsupported compiler drivers rather
than silently producing misleading data.

The checked-in presets make CTest the authoritative runner. From the SimdLib
repository root:

```powershell
cmake --preset clang-coverage
cmake --build --preset coverage
cmake --build build-coverage --target SimdLibCoverageReset
ctest --preset coverage --output-on-failure
cmake --build build-coverage --target SimdLibCoverageReport
```

The CMake Tools extension is the workspace's VS Code test and coverage
provider. Select the `clang-coverage` configure preset and `coverage` build and
test presets, then use **Run with Coverage** in VS Code's Testing view. CMake
Tools runs the configured reset target, invokes CTest, runs the report target,
and imports `build-coverage/coverage.info` into VS Code's native Test Coverage
view. Restart VS Code after installing CMake or adding LLVM's `bin` directory
to `PATH` so the extension sees the tools.

Coverage report generation does not merge differently configured executables
into one `llvm-profdata` database. CMake generates
`build-coverage/coverage-targets-Debug.txt`, which records each instrumented
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

### Corrected trustworthy baseline

Before the coverage-pipeline correction, VS Code displayed 2,293/3,348 lines (68.5%), 361/433
branches (83.4%), and 442/558 functions (79.2%). That report also emitted
`621 functions have mismatched data` after combining 16 differently
configured executables into one incompatible profile database. Those values
are preserved only as the pre-correction baseline.

The trustworthy baseline below was reproduced on 2026-07-18 with CMake/CTest
4.4.0 and Clang/LLVM 22.1.8. A clean reset followed by all 113 CTest entries
produced 111 per-test `.profdata` files and two CTest-retained `.profraw`
files. The report mapped 108 single-executable profiles to 16 instrumented
executables and excluded five multi-executable equivalence profiles. LLVM
emitted no mismatched-function warning or other export diagnostic.

| Header | Lines | Branches | Functions |
| --- | ---: | ---: | ---: |
| `Api.h` | 324/425 (76.24%) | 63/160 (39.38%) | 583/618 (94.34%) |
| `Bmi.h` | 266/517 (51.45%) | 116/144 (80.56%) | 109/473 (23.04%) |
| `Config.h` | 1/1 (100.00%) | 0/0 | 0/0 |
| `Detail/Extensions.h` | 134/495 (27.07%) | 80/88 (90.91%) | 61/166 (36.75%) |
| `Detail/Implementations.h` | 618/812 (76.11%) | 39/60 (65.00%) | 387/432 (89.58%) |
| `Format.h` | 212/224 (94.64%) | 200/332 (60.24%) | 18/18 (100.00%) |
| `SimdAlgo.h` | 174/180 (96.67%) | 10/20 (50.00%) | 48/48 (100.00%) |
| `SimdResample.h` | 128/128 (100.00%) | 60/60 (100.00%) | 6/6 (100.00%) |
| `SimdVector.h` | 267/275 (97.09%) | 10/14 (71.43%) | 144/177 (81.36%) |
| `UInt128.h` | 342/409 (83.62%) | 187/264 (70.83%) | 82/95 (86.32%) |
| **Aggregate** | **2,466/3,466 (71.15%)** | **765/1,142 (66.99%)** | **1,438/2,033 (70.73%)** |

The larger corrected function and branch denominators are intentional. The
old incompatible database discarded or collided mutually exclusive template
and branch records. The corrected LCOV file preserves their union, so these
totals are not directly comparable with the legacy aggregate percentages.

Direct single-executable `llvm-cov report` checks provided an independent
comparison for the required headers:

| Header | Executable/profile | Regions | Functions | Lines | Branches |
| --- | --- | ---: | ---: | ---: | ---: |
| `Bmi.h` | `SimdLibTestsBmiPortable` | 67/111 (60.36%) | 23/67 (34.33%) | 173/362 (47.79%) | 28/28 (100.00%) |
| `Api.h` | `SimdLibTests128` | 89/127 (70.08%) | 40/41 (97.56%) | 256/349 (73.35%) | 19/39 (48.72%) |
| `UInt128.h` | `SimdLibTestsUInt128Optimized` | 162/197 (82.23%) | 62/74 (83.78%) | 300/378 (79.37%) | 61/84 (72.62%) |
| `Detail/Implementations.h` | `SimdLibTests128` | 100/104 (96.15%) | 69/70 (98.57%) | 234/248 (94.35%) | 7/7 (100.00%) |

### Final trustworthy close-out totals

The final clean-reset run passed 187/187 CTest entries and mapped 190 profiles
to 19 single-executable exports. No multi-executable or tool profile was
included in the final preset run. The LCOV merger now identifies a branch by
its source path, line, block, and branch number, and sums that identity across
executables. This prevents one covered header-template branch from being
reported again as an uncovered copy in every other executable. The final
accumulated report is:

| Header | Lines | Branches | Functions |
| --- | ---: | ---: | ---: |
| `Api.h` | 407/538 (75.65%) | 51/114 (44.74%) | 944/944 (100.00%) |
| `Bmi.h` | 474/499 (94.99%) | 39/50 (78.00%) | 182/232 (78.45%) |
| `Config.h` | 1/1 (100.00%) | 0/0 | 0/0 |
| `Detail/Extensions.h` | 349/507 (68.84%) | 46/50 (92.00%) | 162/196 (82.65%) |
| `Detail/Implementations.h` | 1,524/1,627 (93.67%) | 42/58 (72.41%) | 787/806 (97.64%) |
| `Format.h` | 224/224 (100.00%) | 152/166 (91.57%) | 20/20 (100.00%) |
| `SimdAlgo.h` | 192/195 (98.46%) | 20/30 (66.67%) | 121/126 (96.03%) |
| `SimdResample.h` | 128/137 (93.43%) | 46/46 (100.00%) | 6/6 (100.00%) |
| `SimdVector.h` | 282/283 (99.65%) | 13/16 (81.25%) | 208/210 (99.05%) |
| `UInt128.h` | 371/409 (90.71%) | 96/108 (88.89%) | 96/102 (94.12%) |
| **Aggregate** | **3,952/4,420 (89.41%)** | **505/638 (79.15%)** | **2,532/2,645 (95.73%)** |

For `Api.h`, every runtime-profiled alternative is covered: 51/51 (100.00%).
The remaining 63 raw alternatives consist of the constant-evaluation sides of
15 `std::is_constant_evaluated()` gates and 48 branches within their
constant-evaluation-only bodies. The dedicated 128-bit and 256-bit constexpr
targets prove those contracts at compile time, but LLVM runtime profiles cannot
increment their counters. The raw 51/114 total and the classified 51/51 runtime
total are therefore reported together; the latter is a project classification,
not a native LLVM percentage.

The final `coverage.info` has SHA-256
`9B07AFE889701BE3670504CFA28FE35CB0AA944C6697C4B952990EB77DC24A2C`.
A clean reset before CTest ensures the report cannot inherit stale profiles.

### Reviewed red-gutter exclusions

The table below exhaustively classifies every distinct `DA` line with a zero
count in the final LCOV file. `non-code` includes blank/comment/preprocessor
lines and counterless fully inlined wrapper or `if constexpr` selection sites
whose public callers are directly proved by `ApiOperationMatrix.md`. These are
line-gutter classifications; unhit LCOV branch alternatives remain visible in
the totals and are covered by the compiler/configuration matrix or the same
reviewed compile-time and availability constraints.

| Header | Zero-count line ranges | Category and reviewed reason |
| --- | --- | --- |
| `Api.h` | 222-227, 579-582, 598-601, 751-764, 778-787, 802-811, 826-835, 850-859, 884-893, 1079-1088, 1102-1112, 1125-1134, 1154-1164, 1183-1193 | constexpr-only: these are the constant-evaluation bodies; dedicated API constexpr targets prove the same contracts. |
| `Bmi.h` | 153-154, 160-161, 203, 232, 266, 289, 342, 387, 776, 816-817, 820, 823, 831, 841, 851, 878-879, 882, 885, 893, 903, 913 | non-code: blank/comment/preprocessor lines and counterless template-selection sites; the selected multiplication bodies and public BMI operations have exhaustive/runtime profiles. |
| `SimdResample.h` | 61, 78, 95, 106, 114, 125, 145, 163, 171 | non-code: blank and preprocessor-alternative lines. |
| `SimdAlgo.h` | 26 | unreachable: `LowBits` is called only for a count below the selected register's lane count, which cannot reach 32. |
| `SimdAlgo.h` | 82, 135 | non-code: LLVM assigns no separate line counter to the terminal return after the loop; exact-register no-match and all-match assertions directly prove both returns. |
| `SimdVector.h` | 112 | non-code: the fully inlined `to_array` assignment has no retained line counter; the signed/unsigned full, partial, odd, and cross-lane `area()` matrix directly executes the reduction. |
| `UInt128.h` | 162, 173, 184, 195, 427, 454, 497, 525 | non-code: preprocessor terminators. |
| `UInt128.h` | 354-356 | constexpr-only: the compatibility `getBlock` contract is asserted in the constexpr snapshot. |
| `UInt128.h` | 409-417, 436-444 | compiler-specific: MSVC carry intrinsics and Clang/GCC overflow builtins are separately selected and proved by the strict compiler profiles; the portable profile cannot execute them. |
| `UInt128.h` | 461, 464-465, 467, 551, 554-555, 558-559 | non-code: counterless `if constexpr` selection and brace lines; boolean/signed shift normalization and all three bitwise selections have direct assertions. |
| `Detail/Extensions.h` | 27-74, 83-130 | compiler-specific: MSVC intrinsic-register union access is preprocessor-excluded from the Clang LCOV build and is covered by the strict MSVC matrix. |
| `Detail/Extensions.h` | 324, 327, 366-368, 372, 377-380, 393, 399, 404-407, 411-413, 417-419, 442-444, 448, 477-479, 512-515, 519-522, 590, 611, 669, 672, 678-681, 717-719, 728, 738, 748, 805-808, 812-815, 915-917 | non-code: comments, blank lines, and counterless fully inlined backend wrappers/selection sites. Their supported public operation/type cells are directly tested at 128 and 256 bits. |
| `Detail/Implementations.h` | 543, 1998, 2000, 2002, 2186, 2188, 2190, 2202, 2204, 2206, 2218, 2220, 2222, 2233, 2235, 2237, 2249, 2251, 2253, 4310-4312, 4314-4315 | non-code: counterless inlined/template selection sites; the selected public extrema, construction, and bitwise cells are directly tested for every supported lane family. |
| `Detail/Implementations.h` | 1993-1995, 2012-2014, 2024-2026, 2036-2040, 4305-4307, 4324-4326, 4336-4338, 4348-4352 | constexpr-only: 128/256-bit construction bodies are proved by the dedicated constexpr targets. |
| `Detail/Implementations.h` | 1389-1400, 2694-2717, 2887-2901 | intentionally unsupported: inherited signed-64 adjacent multiplication and integer square-root backend helpers are not supported public operation/type cells. They remain subject to the post-plan unavailable-area API review rather than being promoted through tests. |

### Historical audit totals (legacy incompatible merge)

The following before/after table belongs to the original audit. It used the
single incompatible profile database that produced `621 functions have
mismatched data`; retain it as historical directional evidence only.

| Header | Regions before | Regions after | Functions before | Functions after | Lines before | Lines after | Branches before | Branches after |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `Api.h` | 68.75% | 71.83% | 97.92% | 98.18% | 69.21% | 71.79% | 48.72% | 50.00% |
| `Bmi.h` | 67.33% | 68.21% | 32.84% | 34.33% | 45.80% | 47.21% | 100.00% | 100.00% |
| `Format.h` | 98.06% | 98.06% | 100.00% | 100.00% | 94.64% | 94.64% | 86.75% | 87.35% |
| `SimdAlgo.h` | 91.53% | 92.96% | 100.00% | 100.00% | 97.06% | 97.42% | 64.29% | 77.27% |
| `SimdResample.h` | 100.00% | 100.00% | 100.00% | 100.00% | 100.00% | 100.00% | 100.00% | 100.00% |
| `SimdVector.h` | 98.44% | 92.55% | 100.00% | 98.31% | 100.00% | 97.14% | 75.00% | 90.00% |
| `UInt128.h` | 82.11% | 85.07% | 85.14% | 89.19% | 78.51% | 83.07% | 80.00% | 77.17% |
| `Detail/Extensions.h` | 52.94% | 60.50% | 24.32% | 36.49% | 21.14% | 27.42% | 100.00% | 100.00% |
| `Detail/Implementations.h` | 90.04% | 91.87% | 89.19% | 91.94% | 67.98% | 74.06% | 100.00% | 100.00% |
| Aggregate | 82.25% | 84.43% | 74.20% | 79.17% | 65.08% | 69.46% | 84.35% | 84.40% |

The lower percentage for `SimdVector` is caused by instantiating previously
unseen members, which increased the denominator; the new signed-tail,
min/max-position, area, floating equality, and hashing branches are
directly exercised. UInt128's aggregate branch percentage is similarly
affected by merging mutually exclusive optimized and scalar profiles.

The raw profiles, merged `coverage.profdata`, and exported `coverage.info` are
generated artifacts under `build-coverage` and are intentionally not
source-controlled. The historical
`baseline.profdata` and `final.profdata` used for the table above were likewise
generated artifacts rather than source-controlled inputs.

## Final validation record

All final runs used CMake/CTest 4.4.0. The MSVC tree used MSVC
19.44.35222.0 with the Visual Studio 17 2022 generator. The clang-cl Release,
Clang coverage Debug, and Clang ASan/UBSan Debug trees used LLVM 22.1.8 and
Ninja. Every tree enabled strict warnings and examples; benchmarks were
excluded from correctness runs. The sanitizer tree intentionally omitted the
optional compiler-feature profiles.

```powershell
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cmake --build build-phase9-clangcl-ninja --parallel
ctest --test-dir build-phase9-clangcl-ninja --output-on-failure
cmake --build build-coverage --parallel
cmake --build build-coverage --target SimdLibCoverageReset
ctest --preset coverage --output-on-failure
cmake --build build-coverage --target SimdLibCoverageReport
$env:PATH='C:\Program Files\LLVM\lib\clang\22\lib\windows;' + $env:PATH
cmake --build build-phase8-sanitize --parallel
ctest --test-dir build-phase8-sanitize --output-on-failure
```

| Matrix | Result | Catch2 cases/assertions | Measured CTest wall time | CTest log |
| --- | ---: | ---: | ---: | --- |
| strict MSVC Release | 179/179 | 156 / 4,324,488 | 4.175 s | `build/Testing/Temporary/LastTest.log` |
| strict clang-cl Release | 182/182 | 159 / 4,435,080 | 2.583 s | `build-phase9-clangcl-ninja/Testing/Temporary/LastTest.log` |
| Clang Debug coverage | 182/182 | same 159 discovered Catch2 cases | 1.321 s | `build-coverage/Testing/Temporary/LastTest.log` |
| Clang ASan/UBSan Debug | 146/146, no diagnostics | optional profiles intentionally omitted | 6.099 s | `build-phase8-sanitize/Testing/Temporary/LastTest.log` |

The Catch2 totals are the sum of every `SimdLibTests*.exe` compact summary with
`--rng-seed 1592594996`. `SimdLibPreconditionTests.exe` is intentionally
excluded because it terminates after its selected contract case; its 13
independently discovered CTest entries remain part of the CTest totals. The
aggregate intentionally counts repeated portable, intrinsic, carry, scalar,
checks-enabled, SSE, and AVX2 profiles because those profiles are separate
behavioral evidence. The remaining CTest entries cover
header isolation, configuration/availability probes, formatter ODR, five
result-set equivalence runs, 13 isolated precondition failures, the public
example, the public-header assertion audit, and the constexpr target group.
All required portable, scalar-only, FMA on/off, BMI1-only, BMI2-only,
BMI1+BMI2, SSE4.2, and AVX2 profiles are present in the complete release and
coverage matrices.

Both freshly configured external consumers pass 1/1: MSVC in 0.084 s at
`build-phase9-consumer-msvc/Testing/Temporary/LastTest.log`, and clang-cl in
0.063 s at
`build-phase9-consumer-clangcl/Testing/Temporary/LastTest.log`. The clang-cl
consumer reports the expected ignored `[[msvc::flatten]]` vendor-attribute
diagnostics; SimdLib's strict clang-cl targets apply the documented private
suppression and are warning-clean.

Focused `clang-format --dry-run --Werror` passes for the two newly added
precondition sources after applying the checked-in style. `clang-tidy` 22.1.8
passes those sources; its only diagnostics are
`bugprone-throwing-static-initialization` reports originating from Catch2's
`TEST_CASE` registration macro. The configure/build assertion audit validates
48 production-header occurrences against 30 reviewed allowlist entries. A
source audit over `tests` and `examples` finds no `SimdLib::Detail`,
direct `Detail` include, or backend-routing reference. All dedicated constexpr
profiles build in both complete release matrices and in the Clang coverage
matrix.

## Consumer-header compile-time comparison

The final measurement repeats the method in `ConstexprCompilerEvidence.md`:
one header and the same empty `extern "C"` anchor, Clang 22.1.8,
`-std=c++20 -O2 -msse4.2 -mavx2`, a discarded warm-up, and the median of 15
clean object compiles. Generated fixtures and objects remain under the ignored
`build-phase9-compile-time` directory.

| Header | Extraction baseline | Final median | Change |
| --- | ---: | ---: | ---: |
| `Bmi.h` | 271.48 ms | 255.33 ms | -5.95% |
| `UInt128.h` | 509.06 ms | 441.86 ms | -13.20% |
| `SimdLib.h` | 527.17 ms | 515.44 ms | -2.23% |

No measured consumer header regressed against the extraction baseline.

## VS Code coverage integration

VS Code CMake Tools 1.23.52 is installed and recommended by
`.vscode/extensions.json`. The workspace enables CTest Test Explorer
integration, resets coverage before a run, generates the target-aware report
afterward, and imports exactly
`${workspaceFolder}/build-coverage/coverage.info`. The installed extension registers these exact settings; its LCOV handler reads
each configured file, constructs native scode.FileCoverage records for
lines, branches, and functions, and calls TestRun.addCoverage. Parsing the
same imported file produces the per-header and aggregate totals recorded above.
The command-line environment cannot inspect pixels in the native Test Coverage
view, so this check proves the provider/import contract and data agreement
without claiming a manual GUI observation.

## Reviewed remaining gaps

The earlier statement that no unresolved high-risk correctness gap remains is
consistent with the completed evidence: every supported operation/type cell in
`ApiOperationMatrix.md` has a direct public test, and every zero-count source
line is classified above. The remaining items are reviewed API-design or
lower-risk expansion work rather than known correctness defects:

- inherited backend names that are not supported public operation/type cells;
- conversion rounding/overflow and direct-transform overlap behavior beyond
  the current documented cases;
- convenience overloads whose behavior currently delegates to directly tested
  core operations; and
- the explicitly planned review of operation/type cells marked `unavailable`
  after the current coverage plan, before deciding whether any should gain an
  implementation.

Generated `.profraw`, `.profdata`, LCOV, binary, object, log, and temporary
analysis files remain ignored and untracked. The final source diff is limited
to formatter normalization of the two new precondition tests plus this
close-out documentation and planning evidence.
