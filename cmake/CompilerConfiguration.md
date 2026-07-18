# Compiler configuration probes

`Config.h` owns the standalone compiler configuration surface. Every
library-controlled macro uses `#ifndef`, so a downstream project may override
it before including any SimdLib header.

- `VECTORCALL` is ABI-affecting. It defaults to the shared `__vectorcall`
  keyword for MSVC and Clang x86/x64 targets and is empty elsewhere. A caller
  that supplies an empty `VECTORCALL` also sets
  `SIMDLIB_VECTORCALL_ENABLED=0`. The shared keyword preserves one declaration
  shape for free functions, members, templates, and function pointers.
  An empty fallback changes only the calling convention; it does not affect
  `SIMDLIB_HAS_*` instruction availability. Every linked translation unit must
  use the same definition to avoid an ABI mismatch.
- `SIMDLIB_FORCE_INLINE` defaults to the supported C++11 vendor attribute plus
  `inline`; callers may set it to ordinary `inline`.
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

Standalone tests are split and labelled `SSE42`, `AVX2`, `FMA`, and
`OPTIONAL`. Their matching `SIMDLIB_BUILD_TESTS_*` switches let CI omit runtime
families that the host CPU cannot execute.

`SIMDLIB_STRICT_WARNINGS=ON` selects `/W4 /WX /permissive-` for MSVC and
clang-cl, and `-Wall -Wextra -Wpedantic -Werror` for native Clang/GCC. The policy intentionally
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
ordinary/static/template functions and a function pointer; caller overrides;
disabled instruction families; vendor attributes; and an explicitly forced
Clang non-x86 configuration. The default probe compiles the same
`__vectorcall` declaration shapes with MSVC and Clang. See the
[MSVC `__vectorcall` reference](https://learn.microsoft.com/en-us/cpp/cpp/vectorcall?view=msvc-170)
and [Clang vectorcall reference](https://clang.llvm.org/docs/AttributeReference.html#vectorcall).
