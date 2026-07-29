# Constexpr and Compiler-Path Evidence

## Compile-only matrix

All targets are ordinary CMake object-library probes. They are dependencies of
`ConstexprProbes`, which is owned by `ExhaustiveArtifacts`. The
`ConstexprProbes.Artifacts` CTest entry validates the recorded object hashes
without compiling, so assertion diagnostics retain their source file and
expression during the owning build operation.

| Contract source | Compile profiles | Result |
| --- | --- | --- |
| `BmiConstexpr.tests.cpp` | portable, BMI1 only, BMI2 only, BMI1 and BMI2 | MSVC Release and Clang coverage builds pass all four profiles. |
| `UInt128Constexpr.tests.cpp` | compiler carry, portable carry, scalar with SIMD/BMI/FMA disabled | MSVC Release and Clang coverage builds pass all three profiles. |
| `Api128Constexpr.tests.cpp` | SSE4.2 public API and four-lane `SimdVector` | MSVC Release and Clang coverage builds pass. |
| `Api256Constexpr.tests.cpp` | AVX2 public API and eight-lane `SimdVector` | MSVC Release and Clang coverage builds pass. |
| `ApiDisabledConstexpr.tests.cpp` | all instruction families disabled | MSVC Release and Clang coverage builds pass and confirm the SIMD facades are unavailable. |

The reusable contracts in `tests/constexpr/ApiConstexprContracts.h` cover construction, `setzero`, `setr`, `construct`, `set1`, `load_partial`, `to_array`, runtime-selected `extract` and `insert`, all six public comparison helpers, byte and slim movemasks for every signed, unsigned, float, and double lane family, integer extrema positions, lane-shift boundaries, 128-bit whole-register bit/byte-shift boundaries, and `SimdVector` default/array/broadcast construction. Public comparison contracts cover every operation choice reachable through the public helpers; the protected legacy `compare_each_element` dispatcher has no public caller and is not treated as a supported test seam.

A mechanical comparison with `HEAD` confirms that the first 121 BMI assertions and first six UInt128 assertions in the dedicated sources are text-identical to the removed production-header assertions. Expanded contracts follow those preserved blocks.

## Runtime/compiler parity

The 128- and 256-bit runtime parity tests rebuild deterministic inputs through volatile scalars before invoking comparisons, extrema, and lane shifts. This prevents compile-time folding and compares optimized dispatch with the same shared constexpr snapshot.

`UInt128.tests.cpp` also uses volatile operands for addition and subtraction. The optimized target has a compile-time selection check:

- MSVC x64 must select `_addcarry_u64` and `_subborrow_u64`;
- Clang/GCC must select `__builtin_add_overflow` and `__builtin_sub_overflow`;
- portable and scalar profiles must disable compiler carry intrinsics.

The complete MSVC Release suite passes 144/144 tests. The complete Clang coverage suite passes 147/147 tests; Clang has three additional native-`unsigned __int128` tests. Clang coverage cannot contain the preprocessor-excluded MSVC intrinsic lines, so the green MSVC optimized target and its volatile compiler-path test are the evidence for those lines rather than a Clang red-gutter defect.

The separate `tests/consumer` project configures with MSVC 19.44, builds against `SimdLib::SimdLib`, confirms that the target remains an interface library, and passes its 1/1 CTest entry. The public-header diff contains no declaration, `requires` clause, diagnostic-message, or representation change: it removes test examples, documents retained ABI assertions, and adds constant-evaluation-only bodies. The retained UInt128 size/alignment/layout assertions, strict full builds, volatile runtime parity tests, and external consumer build jointly cover ABI and runtime compatibility. CMake 4.4.0 drove both compiler matrices; Clang validation used Clang 22.1.8.

## Consumer compile-time and emitted-code comparison

Measurement date: 2026-07-19. The exact pre-extraction headers came from `HEAD`; post-extraction headers came from the working tree. Both were copied to equal-length sibling paths. Each minimal translation unit included one header and defined the same `extern "C"` anchor. Clang 22.1.8 used `-std=c++20 -O2 -msse4.2 -mavx2`. Runs alternated before/after order after a discarded warm-up. The table reports the median of 15 clean object compiles.

| Header | Before median | After median | Change | Preprocessed bytes before/after | Preprocessed lines before/after | Object bytes before/after |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `Bmi.h` | 286.10 ms | 271.48 ms | -5.11% | 2,682,189 / 2,674,248 | 45,980 / 45,860 | 974 / 974 |
| `UInt128.h` | 514.29 ms | 509.06 ms | -1.02% | 4,278,676 / 4,271,309 | 77,446 / 77,343 | 1,194 / 1,194 |
| `SimdLib.h` | 545.92 ms | 527.17 ms | -3.44% | 4,334,803 / 4,327,402 | 78,694 / 78,591 | 1,194 / 1,194 |

Clang `-ftime-report -fsyntax-only` front-end wall-clock medians also did not regress after stabilization: `Bmi.h` used 21 alternating runs and changed from 0.22 s to 0.21 s; seven alternating runs changed `UInt128.h` from 0.47 s to 0.44 s and `SimdLib.h` from 0.45 s to 0.44 s. The unchanged object sizes confirm that extracting compile-time assertions introduced no emitted code.
