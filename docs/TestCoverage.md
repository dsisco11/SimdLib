# Test coverage audit

This document records the standalone SimdLib coverage audit completed on
2026-07-17. Coverage percentages are supporting evidence; the behavioral map
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

The standard Clang coverage preset contributes 113 CTest entries: 106
individual Catch2 test cases discovered by `catch_discover_tests()` and seven
direct CTest integration/equivalence tests. Catch2 executables remain grouped
by these stable name prefixes:

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
| `SimdVector` | Construction, lane access, arithmetic, comparisons, masks, reductions, hashing, pair products, signed partial vectors, inactive-lane min/max behavior, and floating signed-zero equality/hash consistency | Representative integral and floating lane types, full and partial extents | Convenience overloads that delegate directly to `Api` are not all tested individually. Their underlying behavior is covered; add overload-specific tests when they acquire distinct contracts. |
| `SimdAlgo` | `AnyEqual` and `AllEqual` full-register/tail outcomes, bitwise transforms, conversions, comparison packing, scalar parity, non-register-multiple tails, and destination canaries | Read widths 8/16/32/64; empty, single, multi-element, exact-register, multi-register, and tail extents | General comparison currently supports `WriteWidth == 1`; unsupported widths are a compile-time precondition, not an untested runtime branch. |
| `SimdResample` | Scalar-reference parity for reductions and expansion, boundary dimensions, randomized inputs, and SIMD/scalar equivalence | SIMD enabled and scalar-only profiles | No material gap found. This remains the strongest standalone surface. |
| `Bmi` | BMI1/BMI2 operations, portable and intrinsic equivalence, signed bit-pattern preservation, boundary indices/counts, generic 128-bit support, constexpr evaluation, and deterministic randomized scalar oracles | BMI1 only, BMI2 only, both, and portable | Several derived helpers have names whose edge semantics are not independently specified. Existing production contracts remain covered, but new tests should follow an API-contract review rather than canonizing incidental behavior. |
| `uint128_t` | Construction, heterogeneous comparisons, arithmetic, carry/borrow, shifts, masks, bit helpers, register conversion, constexpr behavior, randomized native/scalar oracle parity, and optimized/portable/scalar digest equivalence | Optimized compiler carry, portable carry, and all SIMD/BMI/FMA disabled | Division and remainder are not public operations, so the audit item is not applicable. |
| Formatters | UInt128 decimal/binary/octal/hex output, signs, alternate forms, width/alignment/fill/zero padding, scalar `uint64_t` parity where the value fits, accepted/rejected grammar, integral and floating vectors, header isolation, and multi-TU ODR | Opt-in `Format.h` and umbrella include | Locale-specific formatting is deliberately rejected so output remains locale-independent; no formatter-contract gap remains. |
| `Config` and `TemplateTools` | Compiler/target detection, feature constants, caller overrides, disabled public headers, type availability, alias widths, concepts, constexpr loops, tuple iteration, and constant evaluation | Default, override, disabled, MSVC, clang-cl, and Clang | Unavailable `Api` instantiations are tested through availability concepts instead of intentional hard-error compile failures. |

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
- Partial signed `SimdVector` bitwise NOT and pair products used a signed
  numeric maximum instead of an all-bits mask. Signed bit-pattern tests cover
  the corrected behavior.
- Partial `SimdVector` min/max position could select an inactive zero lane.
  Tests now cover nonzero active lanes and inactive-lane sentinels.
- Floating `SimdVector` hashing distinguished `+0` and `-0` even though they
  compare equal. Float and double equality/hash regressions now cover this.
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
`0xC001D00D12345678`, `0x9E3779B97F4A7C15`,
`0xD1B54A32D192ED03`, and `0xA0761D6478BD642F`. UInt128 uses
`0xD1B54A32D192ED03`, `0x94D049BB133111EB`, and
`0xA0761D6478BD642F`. Resampling derives its `std::mt19937` seed from
the tested dimensions so a failing case can be reproduced directly. New
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
ctest --test-dir build-coverage -T Test --output-on-failure
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

### Corrected Phase 0 baseline

Before Phase 0, VS Code displayed 2,293/3,348 lines (68.5%), 361/433
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

The final `coverage.info` contains one accumulated record per header and has
SHA-256
`B70877D263760CA5AF5602AA413A23C7D38D78669326748B334858241E869359`.
Regenerating it from unchanged profiles produced the same hash. A separate
clean-reset run containing only `SimdLib.HeaderOnlySmoke` made the report
fail on the missing `SimdLibTestsBmiPortable` profile, proving that partial
runs cannot inherit stale profiles.

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
min/max-position, pair-product, floating equality, and hashing branches are
directly exercised. UInt128's aggregate branch percentage is similarly
affected by merging mutually exclusive optimized and scalar profiles.

The raw profiles, merged `coverage.profdata`, and exported `coverage.info` are
generated artifacts under `build-coverage` and are intentionally not
source-controlled. The historical
`baseline.profdata` and `final.profdata` used for the table above were likewise
generated artifacts rather than source-controlled inputs.

## Validation record

The standard Clang coverage preset contains 113 CTest entries, including 106
individually addressable Catch2 cases. The earlier MSVC audit recorded 103
Catch2 test cases and 4,254,178 assertions across the API, BMI profiles,
UInt128 profiles, formatting, vector algorithms, and scalar resampling.

```powershell
cmake --build SimdLib/build-m12-msvc --config Release --parallel
ctest --test-dir SimdLib/build-m12-msvc -C Release --output-on-failure
cmake --build SimdLib/build-m12-clang-ninja --parallel
ctest --test-dir SimdLib/build-m12-clang-ninja --output-on-failure
cmake -S SimdLib -B SimdLib/build-m1-clang-sanitize -G Ninja `
  "-DCMAKE_CXX_COMPILER=C:/Program Files/LLVM/bin/clang++.exe" `
  "-DCMAKE_CXX_FLAGS_DEBUG=-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" `
  "-DCMAKE_EXE_LINKER_FLAGS_DEBUG=-fsanitize=address,undefined" `
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL `
  -DSIMDLIB_STRICT_WARNINGS=ON -DSIMDLIB_BUILD_BENCHMARKS=OFF `
  -DSIMDLIB_BUILD_TESTS_OPTIONAL=OFF
cmake --build SimdLib/build-m1-clang-sanitize --parallel
$env:PATH='C:\Program Files\LLVM\lib\clang\22\lib\windows;' + $env:PATH
ctest --test-dir SimdLib/build-m1-clang-sanitize --output-on-failure
```

- MSVC 19.44 strict Release: 22/22 passed.
- clang-cl 22.1.8 strict Release: 22/22 passed.
- Clang 22.1.8 ASan/UBSan Debug: 16/16 passed with no diagnostics. The LLVM
  sanitizer runtime directory must be on `PATH` on Windows. The release CRT is
  required because LLVM ASan is incompatible with the MSVC Debug CRT allocator
  instrumentation; without the override the process aborts in `ucrtbased.dll`
  before executing SimdLib code. The restricted validation environment reused
  the Catch2 source from an existing local build because network access was
  unavailable.
- Clang 22.1.8 source-coverage Debug audit: 22 aggregate executable entries
  passed before individual Catch2 discovery was enabled.

## Remaining work

No unresolved high-risk correctness gap remains from this audit. The following
items are intentionally retained as lower-risk follow-up work:

- clarify whether inherited backend helpers are part of the `Api` contract;
- specify conversion overflow/rounding and direct-transform overlap behavior;
- document edge semantics for the derived BMI helper family before expanding
  its tests;
- add broader invalid-format and locale cases; and
- add direct tests for convenience overloads when their behavior diverges from
  the already-covered core operations.
