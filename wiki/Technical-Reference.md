# SimdLib technical reference

This page collects the detailed integration, configuration, API, and
development information for SimdLib. For a shorter introduction and first
example, start with the [project README](../README.md).

## Contents

- [Library model](#library-model)
- [Integration](#integration)
- [Supported environments](#supported-environments)
- [SIMD availability and instruction families](#simd-availability-and-instruction-families)
- [Public headers](#public-headers)
- [Configuration and ABI](#configuration-and-abi)
- [API example](#api-example)
- [Formatting](#formatting)
- [Development workflow](#development-workflow)
- [Continuous validation](#continuous-validation)

## Library model

SimdLib is a header-only library with a C++20 core and an opt-in C++23
complete-register interface. Its CMake targets are `INTERFACE_LIBRARY`
targets; they do not produce a DLL or static library. The public API lives in
the `SimdLib` namespace, while `SimdLib::Detail` contains implementation
details that consumer code must not name.

The main API families are:

- `NativeRegister<element_t>`, the recommended C++23 complete-register value
  that selects the widest available register;
- `Register<element_t, register_width>` and `RegisterMask`, the explicit-width
  complete-register value and predicate types;
- `NativeApi<element_t>`, the C++20 backend facade that selects the widest
  available register;
- `Api<register_width, element_t>`, a typed intrinsic facade;
- `SimdVector<element_t, element_count>`, a fixed-size value type backed by one
  SIMD register;
- `SimdAlgo`, fixed-extent and dynamic-span bulk operations;
- `SimdResample`, byte-mask reduction and expansion;
- `Bmi`, portable and intrinsic bit-manipulation operations; and
- `uint128_t`, a two-word unsigned 128-bit integer.

`SimdVector` keeps hardware lanes beyond its logical element count zero.
`SimdResample` selects its SIMD implementation only when the configured
128-bit facade is available and otherwise uses a scalar implementation through
the same public functions.

## Integration

### `add_subdirectory` or a Git submodule

```cmake
add_subdirectory(external/SimdLib)
target_link_libraries(MyTarget PRIVATE SimdLib::SimdLib)
```

Targets that use `Register`, `RegisterMask`, or `NativeRegister` link the
C++23 interface target instead:

```cmake
target_link_libraries(MyRegisterTarget PRIVATE SimdLib::Register)
```

### `FetchContent`

Replace the repository URL and revision with the location used by your
project:

```cmake
include(FetchContent)

FetchContent_Declare(SimdLib
    GIT_REPOSITORY https://github.com/OWNER/SimdLib.git
    GIT_TAG <revision>)

FetchContent_MakeAvailable(SimdLib)
target_link_libraries(MyTarget PRIVATE SimdLib::SimdLib)
```

The only consumer include directory is `include/`. Include a focused public
header where practical, or use `<SimdLib/SimdLib.h>` for the complete
non-formatting surface. `<SimdLib/Format.h>` is intentionally separate so
translation units pay for formatting support only when they use it.

The repository's CMake project requires CMake 4.4 or newer. Consumers that
integrate the headers without the provided CMake project need a supported C++20
compiler for the core, a supported C++23 compiler for the Register interface,
and the appropriate target flags.

## Supported environments

The current validation matrix covers:

| Compiler family | Validated frontend              | Targets             |
| --------------- | ------------------------------- | ------------------- |
| MSVC            | Visual Studio 2022 / MSVC 19.44 | Windows x64         |
| clang-cl        | LLVM Clang 22 with the MSVC ABI | Windows x64         |
| Clang           | LLVM Clang 22                   | Linux x64           |
| GCC             | GCC 13.2 or newer               | Linux x64           |

The SIMD backends require x86-family intrinsic headers on an x64 target. The portable
configuration layer, BMI fallback algorithms, and two-word `uint128_t`
representation do not perform runtime CPU dispatch.

Instruction-family macros describe what the compiler may emit for the current
translation unit. They do not detect the processor at runtime. A binary must
run only on processors that support every instruction family enabled when the
binary was compiled.

## SIMD availability and instruction families

For C++23 complete-register work, use `SimdLib::NativeRegister<element_t>`. It
resolves to the widest available `Register` specialization. Use explicit
`Register<element_t, register_width>` when storage or ABI must not vary with
the target configuration. This is a compile-time choice based on compiler
flags; it is not runtime CPU detection.

Use `SimdLib::NativeApi<element_t>` for C++20, collection helpers, or direct
backend access. It resolves to `Api<256, element_t>` when the compile target
enables AVX2 and SSE4.2, and otherwise resolves to `Api<128, element_t>` when
SSE4.2 is enabled.

Use the explicit-width `Api<register_width, element_t>` form when a data
layout, ABI, or algorithm specifically requires 128-bit or 256-bit registers.
Use `SimdLib::is_api_available_v<width, element>` to ask whether an `Api`
specialization is available in the current translation unit.

- SSE4.2 enables the 128-bit facade.
- AVX2 together with SSE4.2 enables the 256-bit facade.
- FMA, BMI1, and BMI2 are selected independently.
- Code for disabled SSE4.2, AVX2, and FMA families is excluded during
  preprocessing.

The CI feature matrix executes AVX2 and scalar paths, FMA-enabled and
FMA-disabled paths, and all four BMI1/BMI2 combinations.

## Public headers

| Header                     | Public entry point                                                                                         |
| -------------------------- | ---------------------------------------------------------------------------------------------------------- |
| `<SimdLib/Config.h>`       | Version, compiler, target, instruction, assertion, and ABI configuration                                   |
| `<SimdLib/Api.h>`          | Auto-sized `NativeApi<element_t>`, explicit-width `Api<register_width, element_t>`, and availability query |
| `<SimdLib/Register.h>`     | C++23 `Register<element_t, register_width>` and `NativeRegister<element_t>` complete-register values       |
| `<SimdLib/RegisterMask.h>` | C++23 `RegisterMask<element_t, register_width>` predicate values                                           |
| `<SimdLib/SimdApi.h>`      | Deprecated compatibility forwarding header; use `Api.h`                                                    |
| `<SimdLib/Aliases.h>`      | C++23 named `Register` aliases exposed when their SSE4.2 or AVX2 width is available                         |
| `<SimdLib/SimdVector.h>`   | `SimdVector<element_t, element_count>` value type                                                          |
| `<SimdLib/SimdAlgo.h>`     | Fixed-extent and dynamic-span `SimdAlgo` operations                                                        |
| `<SimdLib/SimdResample.h>` | Byte-mask reduction and expansion functions                                                                |
| `<SimdLib/Bmi.h>`          | Portable and intrinsic `SimdLib::Bmi` bit helpers                                                          |
| `<SimdLib/UInt128.h>`      | `uint128_t`, literals, bit utilities, hash, and numeric limits                                             |
| `<SimdLib/Format.h>`       | Opt-in `std::formatter` specializations                                                                    |
| `<SimdLib/SimdLib.h>`      | Complete non-formatting public surface                                                                     |

Headers and declarations below `SimdLib::Detail` are implementation-only.

SimdLib 0.2.0 uses `Api` as the primary facade name. The deprecated `SimdApi`
spelling remains available through `<SimdLib/SimdApi.h>` until 1.0.0. Wide
integer BMI operations are available under `SimdLib::Bmi`; the former root
forwarding functions are not part of the 0.2.0 API. See
[PublicNamespace.md](../docs/PublicNamespace.md) for the complete namespace map
and compatibility policy.

## Configuration and ABI

Library-controlled macros are guarded by `#ifndef` and may be set before the
first SimdLib include.

- `SIMDLIB_TARGET_X86` and `SIMDLIB_TARGET_X64` describe the compiler target.
  `SimdLib::Config::target_x86` and `target_x64` expose the resolved values.
- `SIMDLIB_HAS_SSE*`, `SIMDLIB_HAS_AVX`, `SIMDLIB_HAS_AVX2`,
  `SIMDLIB_HAS_FMA`, `SIMDLIB_HAS_BMI1`, and `SIMDLIB_HAS_BMI2` describe
  compiler-enabled instruction families. They do not provide runtime CPU
  detection.
- `SIMDLIB_FORCE_INLINE` selects the supported compiler attribute together
  with `inline` and may be replaced with ordinary `inline`.
- `SIMDLIB_FLATTEN` selects the supported recursive-inlining attribute and
  may be replaced with an empty definition.
- `SIMDLIB_PRECONDITION(condition, message)` is the assertion replacement
  point and defaults to standard `assert`.
- `SIMDLIB_ENABLE_CHECKS` defaults to enabled without `NDEBUG` and disabled
  with `NDEBUG`.
- `VECTORCALL` affects the ABI. It is `__vectorcall` on supported MSVC and
  Clang Windows x64 targets and empty on non-Windows Clang and other
  unsupported targets.

A caller that overrides `VECTORCALL` with an empty definition must also set
`SIMDLIB_VECTORCALL_ENABLED=0` consistently in every translation unit. An
empty `VECTORCALL` changes only the calling convention; it does not disable
SSE, AVX, FMA, BMI, or any other target-specific instruction. Those remain
controlled by compiler flags and the corresponding `SIMDLIB_HAS_*` values.

All linked translation units must use the same ABI-affecting configuration.
See [CompilerConfiguration.md](../cmake/CompilerConfiguration.md) for compiler
probes and warning-policy details.

## API example

The executable [ApiExamples.cpp](../examples/ApiExamples.cpp) is compiled and
run by CI. It exercises every major public facility in one consumer
translation unit:

```cpp
#include <SimdLib/Format.h>
#include <SimdLib/SimdLib.h>

#include <array>
#include <cstdint>
#include <format>

using Api = SimdLib::Api<128, std::uint32_t>;
Api::add(
    Api::construct({2, 2, 2, 2}),
    Api::construct({10, 10, 10, 10})); // => {12, 12, 12, 12}

SimdLib::SimdVector<std::uint32_t, 4> vector{3, 5, 7, 9};
auto vector_text = std::format("{}", vector); // => "{3, 5, 7, 9}"

bool contains_six = SimdLib::SimdAlgo<8, 8>::AnyEqual(
    std::span<const std::uint8_t, 8>{
        std::array<std::uint8_t, 8>{1, 2, 3, 4, 5, 6, 7, 8}},
    std::uint8_t{6}); // => true

auto extracted =
    SimdLib::Bmi::pext_u32(std::uint32_t{0xD2}, std::uint32_t{0xF0}); // => 0x0D
SimdLib::uint128_t wide =
    SimdLib::uint128_t{~std::uint64_t{0}} + SimdLib::uint128_t{1}; // => 2^64
auto wide_text = std::format("{}", wide); // => "18446744073709551616"

std::array<std::uint8_t, 1> packed{};
SimdLib::SimdResample::ReduceBytesToBitsBy8_Any(
    std::array<std::uint8_t, 8>{1, 2, 3, 4, 5, 6, 7, 8},
    packed); // => packed[0] is 0b1111'1111
```

Enable the executable with `-DSIMDLIB_BUILD_EXAMPLES=ON`.

## Formatting

Formatting support requires `<SimdLib/Format.h>`.

`SimdVector` preserves logical lane order and renders the container as
`{a, b, c}`. Its optional format specification is limited to container-level
`[[fill]align][width]`. Elements use their default formatter; element format
specifications are not forwarded.

`uint128_t` supports default decimal output and the `d`, `x`, `X`, `b`, `B`,
and `o` presentations. Its supported specification is
`[[fill]align][sign][#][0][width][type]`. It follows unsigned-integer behavior
for signs, alternate prefixes, uppercase output, numeric zero padding, fill,
and left, right, or center alignment. Precision, locale, dynamic width, and
other presentation types throw `std::format_error`.

## Development workflow

The repository-owned commands require PowerShell 7+ and CMake 4.4. A complete
Windows-hosted run additionally requires Visual Studio 2022 with the x64 C++
tools, LLVM 22 on `PATH`, and Docker Desktop using Linux containers. Container-
only runs require Docker and do not require the native Windows compilers.

Build the complete native and Linux validation matrix, excluding benchmark
artifacts, with an explicit scope:

```powershell
tools/Build.ps1 -Scope All
```

Build once and run every assigned correctness, ABI, generated-code, sanitizer,
consumer, and coverage test cell with:

```powershell
tools/Run-Tests.ps1 -Scope All
```

Benchmark compilation and execution are supplemental and remain outside the
default build and correctness testing:

```powershell
tools/Build-Benchmarks.ps1 -Scope All
tools/Run-Benchmarks.ps1 -Scope All
```

The accepted scopes and compiler filters are:

| Scope        | Compiler filters                   | Owned cells                                         |
| ------------ | ---------------------------------- | --------------------------------------------------- |
| `All`        | `All` or any compatible subset     | Every native and container cell                     |
| `Native`     | `Msvc`, `ClangCl`, `ClangCoverage` | MSVC and clang-cl Release/Debug plus Clang coverage |
| `Containers` | `Gcc13`, `Gcc14`, `Clang22`        | Linux Release/Debug plus Clang ASan+UBSan           |

For example, a Linux-only CI worker uses `tools/Build.ps1 -Scope Containers`
followed by `tools/Run-Tests.ps1 -Scope Containers -SkipBuild`. A focused local
diagnostic can use `tools/Run-Tests.ps1 -Scope Native -Compiler Msvc` or
`tools/Run-Tests.ps1 -Scope Containers -Compiler Gcc14`.

Each compiler/configuration owns a fingerprinted tree below `out/pipeline`.
The fingerprint includes compiler and image identity, generator, configuration,
instrumentation, required flags, dependencies, and CPU requirements. Source
inputs have a separate digest in the completed manifest. Consequently,
test-only and benchmark-execution operations reject missing, stale, or
incompatible artifacts and never configure or compile. The explicit benchmark
build requires completed validation manifests and targets only
`BenchmarkArtifacts` in the owning Release trees. Objects are reusable only
when their complete compilation fingerprint matches. See [Unified build and
validation](../docs/BuildPipeline.md) for the complete identity and guarded
`-SkipBuild` contract.

Instrumentation boundaries are explicit. Release and Debug use separate trees;
Clang ASan+UBSan has its own instrumented Debug fingerprint; source coverage has
its own native Clang tree; and benchmark compilation reuses only an already
validated Release tree. Coverage is enabled only for top-level SimdLib
development builds and is never introduced into an `add_subdirectory`
consumer.

The following development CMake options exist only when SimdLib is the top-level
project. They are not declared for an `add_subdirectory` consumer:

- `SIMDLIB_BUILD_SMOKE_TESTS=ON` builds the two-translation-unit ODR smoke
  executable. It is enabled by default.
- `SIMDLIB_BUILD_HEADER_PROBES=ON` compiles every public header as the first and
  only SimdLib header in its translation unit. It is enabled by default.
- `SIMDLIB_BUILD_RUNTIME_TESTS=ON` builds the Catch2 test suite. Catch2 v3 is fetched
  when it is not installed and `SIMDLIB_FETCH_TEST_DEPENDENCIES=ON`.
- `SIMDLIB_FETCH_TEST_DEPENDENCIES=ON` permits a top-level development build to
  fetch Catch2 when no suitable package is already available.
- `SIMDLIB_BUILD_API_SSE42_TESTS`, `SIMDLIB_BUILD_API_AVX2_TESTS`, and
  `SIMDLIB_BUILD_FMA_TESTS` independently control the SSE4.2, AVX2, and FMA
  executables. Disable instruction families the test host cannot execute.
- `SIMDLIB_BUILD_BMI_TESTS=ON` enables BMI1/BMI2 intrinsic-path testing
  and deterministic comparison with the always-built portable path. It is off
  by default so unsupported hosts do not execute BMI instructions.
- `SIMDLIB_BUILD_VECTOR_ALGORITHM_TESTS=ON` builds the `SimdVector`,
  `SimdAlgo`, SIMD resampling, and forced-scalar resampling parity matrix.
- `SIMDLIB_BUILD_BENCHMARKS=ON` builds the Catch2 benchmarks and requires a
  discoverable Catch2 v3 package.
- `SIMDLIB_BUILD_EXAMPLES=ON` builds and registers the complete API example.
- `SIMDLIB_BUILD_CONFIGURATION_PROBES=ON` builds compile-only configuration
  probes. It is enabled by default.
- `SIMDLIB_BUILD_CONSTEXPR_PROBES=ON` builds compile-only constant-evaluation
  contracts. Exhaustive Release profiles own the compiler and feature matrix;
  Debug and sanitizer profiles disable duplicate evaluation, while native
  Clang coverage retains its distinct driver and platform contract.
- `SIMDLIB_BUILD_REGISTER_CODEGEN_GATES=ON` builds the Register wrapper/raw
  generated-code and ABI comparison corpus when the compiler supports the
  C++23 Register interface.
- `SIMDLIB_REGISTER_CODEGEN_MODE=ENFORCE|RECORD` selects whether generated-code
  differences fail the supported optimized gate or are retained as diagnostic
  records.
- `SIMDLIB_VALIDATE_EXHAUSTIVE_TARGETS=ON` makes configuration fail when an
  exhaustive profile does not define its required target inventory.
- `SIMDLIB_STRICT_WARNINGS=ON` enables the compiler-specific strict warning
  policy and treats warnings as errors for SimdLib-owned targets.
- `SIMDLIB_ENABLE_COVERAGE=ON` instruments supported Clang targets and
  configures LLVM source coverage.

CTest labels identify instruction families and test groups so automation can
include or exclude them explicitly. The `CoverageReset` and `CoverageReport`
targets produce `coverage.info` below the active fingerprint's build directory,
for example
`out/pipeline/windows-clang-coverage/debug-coverage-<fingerprint>/build/coverage.info`.

## Continuous validation

`.github/workflows/ci.yml` delegates to the same scoped `Build.ps1` and
`Run-Tests.ps1 -SkipBuild` commands used locally. Native MSVC, native clang-cl
plus coverage, and Linux container compilers each build their assigned
fingerprints once and then run test-only operations. Each benchmark-owning CI
job invokes `Build-Benchmarks.ps1` explicitly after correctness testing; the
default build remains benchmark-free. Clang ASan+UBSan remains an independent
instrumented fingerprint. Mandatory instruction-family labels,
constexpr probes, first-include header hygiene, ODR, examples, consumers,
generated-code comparisons, and ABI gates are members of those owned cells,
not separate rebuild scenarios.

The consumer smoke project under `tests/consumer` imports SimdLib with
`add_subdirectory`, verifies that `SimdLib` is an `INTERFACE_LIBRARY`, and
links only the consumer executable. No SimdLib runtime binary is produced.

The completed compiler, sanitizer, consumer, benchmark, and test evidence is
recorded in [Validation.md](../docs/Validation.md). Broader coverage details
and known gaps are recorded in [TestCoverage.md](../docs/TestCoverage.md).
