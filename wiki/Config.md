# Configuration

`SimdLib::Config` exposes the library version and the compiler features enabled for the current translation unit. These are compile-time constants, not runtime CPU detection.

## Contents

- [Version constants](#version-constants)
- [Compiler and target constants](#compiler-and-target-constants)
- [Instruction constants](#instruction-constants)
- [Register interface availability](#register-interface-availability)
- [Customization macros](#customization-macros)

## Version constants

`version_major`, `version_minor`, and `version_patch` form the library version. The same three names are also available directly in `SimdLib`.

```cpp
SimdLib::Config::version_major; // => 0 for version 0.2.0
```

## Compiler and target constants

`compiler_clang`, `compiler_msvc`, `compiler_gcc`, `target_x86`, `target_x64`, and `vectorcall_enabled` describe the active compiler and ABI target.

```cpp
SimdLib::Config::target_x64; // => true when compiling for x64
```

## Instruction constants

`has_sse`, `has_sse2`, `has_sse3`, `has_ssse3`, `has_sse41`, `has_sse42`, `has_avx`, `has_avx2`, `has_fma`, `has_bmi1`, and `has_bmi2` mirror the instruction families enabled by compiler flags.

```cpp
SimdLib::Config::has_avx2; // => true when AVX2 code generation is enabled
```

## Register interface availability

`SIMDLIB_REGISTER_INTERFACE_AVAILABLE` is `1` when the current translation unit supports the C++23 explicit-object syntax required by `<SimdLib/Register.h>`. SimdLib computes this macro from `__cpp_explicit_this_parameter >= 202110L`, or from the documented Microsoft C++ 19.44 fallback when `_MSVC_LANG` selects a post-C++20 mode. The Microsoft fallback intentionally excludes clang-cl.

Unlike the customization macros below, this availability result is not caller-overridable. `SIMDLIB_REQUIRE_REGISTER_INTERFACE=1` can require the capability and produce a focused diagnostic when it is unavailable, but it cannot enable the interface. Linking the opt-in `SimdLib::Register` CMake target publishes this requirement and requests C++23; `SimdLib::SimdLib` remains C++20.

## Customization macros

Except for the computed `SIMDLIB_REGISTER_INTERFACE_AVAILABLE` result, `SIMDLIB_*` configuration macros are caller-overridable before including SimdLib. `SIMDLIB_PRECONDITION`, `SIMDLIB_ENABLE_CHECKS`, `SIMDLIB_FORCE_INLINE`, and `VECTORCALL` control contracts, diagnostics, inlining, and the public calling convention.

```cpp
#define SIMDLIB_ENABLE_CHECKS 1
#include <SimdLib/SimdLib.h>

static_assert(
    SIMDLIB_ENABLE_CHECKS == 1); // => checks are enabled in this translation unit
```
