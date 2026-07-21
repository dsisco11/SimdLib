# Configuration

`SimdLib::Config` exposes the library version and the compiler features enabled for the current translation unit. These are compile-time constants, not runtime CPU detection.

## Contents

- [Version constants](#version-constants)
- [Compiler and target constants](#compiler-and-target-constants)
- [Instruction constants](#instruction-constants)
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

## Customization macros

All `SIMDLIB_*` configuration macros are caller-overridable before including SimdLib. `SIMDLIB_PRECONDITION`, `SIMDLIB_ENABLE_CHECKS`, `SIMDLIB_FORCE_INLINE`, and `VECTORCALL` control contracts, diagnostics, inlining, and the public calling convention.

```cpp
#define SIMDLIB_ENABLE_CHECKS 1
#include <SimdLib/SimdLib.h>

static_assert(
    SIMDLIB_ENABLE_CHECKS == 1); // => checks are enabled in this translation unit
```
