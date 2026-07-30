# Compiler configuration probes

`Config.h` owns the standalone compiler configuration surface. Public function
declarations use `SIMD_FLAGS(...)`; downstream toolchains customize its
placement-safe compiler adapters before including any SimdLib header.

- `Neither`, `In`, `Out`, and `InOut` describe whether native or SimdLib SIMD
  values cross the function boundary by value. `In`, `Out`, and `InOut` emit
  the configured vector calling convention exactly once when the selected
  compiler supports it.
- `RegisterOnly`, `ForceInline`, and `Flatten` are independent modifiers.
  `RegisterOnly` maps to safe-buffer suppression only on supported Microsoft
  configurations. `ForceInline` requests that the annotated function be
  inlined into its caller; `Flatten` requests recursive inlining of eligible
  calls made by the annotated function.
- `SIMDLIB_METHOD_FLAGS_HAS_VECTORCALL`,
  `SIMDLIB_METHOD_FLAGS_HAS_SAFE_BUFFERS`,
  `SIMDLIB_METHOD_FLAGS_HAS_FORCE_INLINE`, and
  `SIMDLIB_METHOD_FLAGS_HAS_FLATTEN` report adapter capabilities. The matching
  `SIMDLIB_METHOD_FLAGS_VECTORCALL`, `SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS`,
  `SIMDLIB_METHOD_FLAGS_FORCE_INLINE`, and `SIMDLIB_METHOD_FLAGS_FLATTEN`
  adapters may be defined by a custom toolchain before the first SimdLib
  include.
- Vector calling-convention configuration is ABI-affecting. Every linked
  translation unit that exchanges flagged functions must use compatible
  capability and adapter definitions. Empty compiler mappings do not affect
  `SIMDLIB_HAS_*` instruction availability or erase the source-level promise.
- `SIMDLIB_PRECONDITION(condition, message)` defaults to `assert` and is the
  sole standalone replacement point for runtime preconditions.
- `SIMDLIB_TARGET_X86` and `SIMDLIB_TARGET_X64` report the selected compiler
  target and may be overridden only when a toolchain requires an explicit
  target declaration.
- `SIMDLIB_HAS_*` values report enabled compiler instruction families, rather
  than runtime processor detection. They may be overridden to zero for
  compile-only fallback coverage.

The current complete 128-bit facade is conservatively available when
`SIMDLIB_HAS_SSE42` is enabled. The 256-bit facade requires both
`SIMDLIB_HAS_SSE42` and `SIMDLIB_HAS_AVX2`. FMA operations use their intrinsic
only when `SIMDLIB_HAS_FMA` is enabled and otherwise retain multiply-plus-add
behavior. `SimdLib::is_api_available_v<width, element>` exposes this
compile-time availability without instantiating an unavailable backend.

Standalone tests are split and labelled `SSE42`, `AVX2`, `FMA`, `BMI`, and
`SCALAR`. The matching `SIMDLIB_BUILD_API_SSE42_TESTS`,
`SIMDLIB_BUILD_API_AVX2_TESTS`, `SIMDLIB_BUILD_FMA_TESTS`, and
`SIMDLIB_BUILD_BMI_TESTS` controls describe the owned artifact families.

`SIMDLIB_STRICT_WARNINGS=ON` selects `/W4 /WX /permissive-` for MSVC and
clang-cl on Windows, and `-Wall -Wextra -Wpedantic -Werror` for GNU-like Clang
and GCC on Linux. The policy intentionally
suppresses Clang `-Wunknown-attributes` and `-Wc2y-extensions`, GCC
`-Wattributes`, plus `-Wignored-attributes` on both, because public headers retain vendor attributes
such as `[[msvc::flatten]]` and compiler SIMD register types can trigger
non-actionable template-argument attribute diagnostics. Catch2's use of
`__COUNTER__` also triggers Clang's C2y-extension diagnostic at each test macro
expansion. No other warning class is globally suppressed.

The feature matrix runs FMA both enabled and forced off, AVX2 and scalar
paths, and BMI profiles `(0,0)`, `(1,0)`, `(0,1)`, and `(1,1)`. The feature
macros remain the source of truth even on MSVC, where `/arch:AVX2` is used to
make the intrinsic declarations available to the independently forced probes.

The configuration OBJECT probes cover default declaration placement for
ordinary, static, and template functions; callback types derived with
`decltype`; caller overrides; disabled instruction families; vendor
attributes; and an explicitly forced Clang non-x86 configuration. The method
flags probes compile the same `SIMD_FLAGS(...)` declaration shapes with MSVC
and Clang. See the
[MSVC `__vectorcall` reference](https://learn.microsoft.com/en-us/cpp/cpp/vectorcall?view=msvc-170)
and [Clang vectorcall reference](https://clang.llvm.org/docs/AttributeReference.html#vectorcall).
The compile-only constexpr matrix builds BMI under all four feature-macro
profiles, UInt128 under compiler-carry, portable-carry, and scalar profiles,
and the API/vector contracts under SSE4.2, AVX2, and fully disabled profiles.
`ConstexprProbes` aggregates these targets. The production-header
assertion audit is a build dependency and a CTest entry; any unallowlisted
assertion or stale justification fails with its header and assertion text.
See [`docs/ConstexprCompilerEvidence.md`](../docs/ConstexprCompilerEvidence.md)
for compiler-specific runtime-path evidence and measurement results.
