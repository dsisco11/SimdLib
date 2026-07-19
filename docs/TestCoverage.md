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

The full configuration contributes these 22 CTest entries:

| Entry | Coverage role |
| --- | --- |
| `SimdLib.HeaderOnlySmoke` | Multi-translation-unit umbrella-header use and header-only linkage |
| `SimdLib.Tests.BmiPortable` | Portable BMI behavior, constexpr checks, boundaries, signed bit patterns, and deterministic randomized oracles |
| `SimdLib.Tests.Format` | UInt128 and vector formatter behavior plus standard scalar parity |
| `SimdLib.FormatOdr` | Formatter specialization linkage across two translation units |
| `SimdLib.Tests.SSE42` | 128-bit `Api`, partial transfers, comparisons, conversion, movemask, and register metadata |
| `SimdLib.Tests.UInt128Optimized` | UInt128 with compiler carry primitives and available SIMD support |
| `SimdLib.Tests.UInt128Portable` | UInt128 with portable carry/borrow |
| `SimdLib.Tests.UInt128Scalar` | UInt128 with all SIMD, BMI, FMA, and compiler-carry features disabled |
| `SimdLib.Tests.UInt128ResultSetEquivalence` | Optimized-versus-portable deterministic result digest |
| `SimdLib.Tests.UInt128ScalarResultSetEquivalence` | Optimized-versus-scalar deterministic result digest |
| `SimdLib.Tests.AVX2` | 256-bit `Api`, partial transfers, comparisons, movemask, and register metadata |
| `SimdLib.Tests.FMA.Enabled` | FMA-enabled dispatch and expected result |
| `SimdLib.Tests.FMA.Disabled` | Non-FMA fallback dispatch and expected result |
| `SimdLib.Tests.Bmi.Bmi1Only` | BMI1 intrinsic profile |
| `SimdLib.Tests.Bmi.Bmi1Only.Equivalence` | BMI1-versus-portable deterministic result digest |
| `SimdLib.Tests.Bmi.Bmi2Only` | BMI2 intrinsic profile |
| `SimdLib.Tests.Bmi.Bmi2Only.Equivalence` | BMI2-versus-portable deterministic result digest |
| `SimdLib.Tests.Bmi.Bmi1AndBmi2` | Combined BMI1/BMI2 intrinsic profile |
| `SimdLib.Tests.Bmi.Bmi1AndBmi2.Equivalence` | Combined-profile-versus-portable deterministic result digest |
| `SimdLib.Tests.VectorAlgorithms` | `SimdVector`, `SimdAlgo`, and SIMD `SimdResample` behavior |
| `SimdLib.Tests.ResampleScalar` | Scalar-only `SimdResample` behavior and oracle parity |
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
  and the guard against public `Detail` dependencies.

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
| `SimdAlgo` | `AnyEqual`, `AllEqual`, bitwise transforms, conversions, comparison packing, prefilled outputs, non-register-multiple tails, and destination canaries | Read widths 8/16/32/64; 24- and 40-element tail cases | General comparison currently supports `WriteWidth == 1`; unsupported widths are a compile-time precondition, not an untested runtime branch. |
| `SimdResample` | Scalar-reference parity for reductions and expansion, boundary dimensions, randomized inputs, and SIMD/scalar equivalence | SIMD enabled and scalar-only profiles | No material gap found. This remains the strongest standalone surface. |
| `Bmi` | BMI1/BMI2 operations, portable and intrinsic equivalence, signed bit-pattern preservation, boundary indices/counts, generic 128-bit support, constexpr evaluation, and deterministic randomized scalar oracles | BMI1 only, BMI2 only, both, and portable | Several derived helpers have names whose edge semantics are not independently specified. Existing production contracts remain covered, but new tests should follow an API-contract review rather than canonizing incidental behavior. |
| `uint128_t` | Construction, heterogeneous comparisons, arithmetic, carry/borrow, shifts, masks, bit helpers, register conversion, constexpr behavior, randomized native/scalar oracle parity, and optimized/portable/scalar digest equivalence | Optimized compiler carry, portable carry, and all SIMD/BMI/FMA disabled | Division and remainder are not public operations, so the audit item is not applicable. |
| Formatters | UInt128 decimal/binary/octal/hex output, signs, alternate forms, width/alignment/fill/zero padding, scalar `uint64_t` parity where the value fits, integral and floating vectors, header isolation, and multi-TU ODR | Opt-in `Format.h` and umbrella include | Locale-specific output and a larger invalid-specification matrix are medium-risk follow-ups because parsing delegates to the corresponding standard scalar formatter. |
| `Config` and `TemplateTools` | Compiler/target detection, feature constants, caller overrides, disabled public headers, type availability, alias widths, concepts, constexpr loops, tuple iteration, and constant evaluation | Default, override, disabled, MSVC, clang-cl, and Clang | Unavailable `Api` instantiations are tested through availability concepts instead of intentional hard-error compile failures. |

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
`SIMDLIB_ENABLE_COVERAGE`. It intentionally fails configuration for unsupported
compiler drivers rather than silently producing misleading data.

Representative commands from the repository root are:

```powershell
cmake -S SimdLib -B SimdLib/build-coverage -G Ninja `
  -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug `
  -DSIMDLIB_ENABLE_COVERAGE=ON -DSIMDLIB_STRICT_WARNINGS=ON `
  -DSIMDLIB_BUILD_BENCHMARKS=OFF
cmake --build SimdLib/build-coverage --parallel
$env:LLVM_PROFILE_FILE='SimdLib/build-coverage/profiles/%p-%m.profraw'
ctest --test-dir SimdLib/build-coverage --output-on-failure
llvm-profdata merge -sparse SimdLib/build-coverage/profiles/*.profraw `
  -o SimdLib/build-coverage/coverage.profdata
```

The workspace TestMate configuration keeps the normal MSVC executables and the
instrumented Clang executables in separate tagged groups. Its `llvm-cov`
coverage profile runs only the `coverage` group and publishes the resulting
line, branch, and function data to VS Code's native Test Coverage view. The VS
Code process must be restarted after adding `C:\Program Files\LLVM\bin` to the
user `PATH` so TestMate can invoke `llvm-profdata` and `llvm-cov`.

TestMate's experimental adapter deletes its temporary raw and merged profiles
after publishing them to VS Code. Use the commands above when a persistent
`coverage.profdata` artifact is required.

`llvm-cov report` was run over all CTest executables with the merged profile.
Because the same header templates are compiled under mutually exclusive
feature definitions, LLVM reports some mismatched-function warnings when all
profiles are merged. The aggregate numbers are therefore directional. The
per-profile behavior/equivalence tests above are authoritative.

### Before and after

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

The raw profiles and merged `coverage.profdata` are generated artifacts under
`build-coverage` and are intentionally not source-controlled. The historical
`baseline.profdata` and `final.profdata` used for the table above were likewise
generated artifacts rather than source-controlled inputs.

## Validation record

The final standalone suite contains 22 CTest entries. Direct MSVC Catch2 runs
contain 103 test cases and 4,254,178 assertions across the API, BMI profiles,
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
- Clang 22.1.8 source-coverage Debug: 22/22 passed.

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
