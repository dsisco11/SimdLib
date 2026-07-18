# SimdLib

SimdLib is a C++20, header-only SIMD and bit-utility library designed for
direct Git consumption. It does not build a DLL or static library.

The core `Api`, `SimdVector`, and `SimdAlgo` facade is available under the
`SimdLib` namespace, with intrinsic implementation details under
`SimdLib::Detail`.

The core backend is validated with MSVC and Clang on x64. Compiler-native
register layout is isolated under `SimdLib::Detail`; SSE4.2, AVX2, and FMA code
is excluded at preprocessing time unless its capability is enabled. Use
`SimdLib::is_api_available_v<width, element>` to query whether a facade
specialization is available in the current translation unit.

`SimdVector<element_t, element_count>` owns one SIMD register and keeps every
hardware lane beyond its logical element count zero. `SimdAlgo` provides the
fixed-extent and dynamic-span bulk operations retained from the original
facade. `SimdResample` reduces eight byte masks into one packed byte or expands
one packed byte into eight `0x00`/`0xFF` bytes. Resampling selects its SIMD path
only when the configured 128-bit facade is available and otherwise uses the
same public functions with a scalar implementation.

## Consumption

### `add_subdirectory` or Git submodule

```cmake
add_subdirectory(external/SimdLib)
target_link_libraries(MyTarget PRIVATE SimdLib::SimdLib)
```

### FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(SimdLib
    GIT_REPOSITORY https://example.invalid/SimdLib.git
    GIT_TAG main)
FetchContent_MakeAvailable(SimdLib)
target_link_libraries(MyTarget PRIVATE SimdLib::SimdLib)
```

The only consumer include directory is `include/`; use focused headers such as
`<SimdLib/Api.h>` or `<SimdLib/Bmi.h>`, or `<SimdLib/SimdLib.h>` for the
complete non-formatting surface. Include `<SimdLib/Format.h>` only where
`std::format` support is required.

## Supported environments

SimdLib requires C++20. The validation matrix covers the following
compiler families and targets:

| Compiler family | Validated frontend | Targets |
| --- | --- | --- |
| MSVC | Visual Studio 2022 / MSVC 19.44 | Windows x86 and x64 |
| clang-cl | LLVM Clang 22 with the MSVC ABI | Windows x86 and x64 |
| Clang | LLVM Clang 22 | Linux x86 and x64 |
| GCC | GCC 13.2 or newer | Linux/MinGW x86 and x64 |

The SIMD backends require x86/x64 intrinsic headers. Portable configuration,
BMI fallback algorithms, and the two-word `uint128_t` representation do not
perform runtime CPU dispatch. A binary must run only on processors supporting
the instruction families enabled when that binary was compiled.

SSE4.2 enables the 128-bit facade. AVX2 plus SSE4.2 enables the 256-bit facade.
FMA, BMI1, and BMI2 are independently selected by their corresponding
`SIMDLIB_HAS_*` controls. The CI feature matrix executes AVX2 and scalar paths,
FMA-enabled and FMA-disabled paths, and the four BMI1/BMI2 combinations.

## Public headers

| Header | Public entry point |
| --- | --- |
| `<SimdLib/Config.h>` | Version, compiler, target, instruction, assertion, and ABI configuration |
| `<SimdLib/Api.h>` | `Api<register_width, element_t>` intrinsic facade and availability query |
| `<SimdLib/SimdApi.h>` | Deprecated compatibility forwarding header; use `Api.h` |
| `<SimdLib/SimdVector.h>` | Root `SimdVector<element_t, element_count>` value type |
| `<SimdLib/SimdAlgo.h>` | Fixed-extent and dynamic-span `SimdAlgo` operations |
| `<SimdLib/SimdResample.h>` | Byte-mask reduction and expansion functions |
| `<SimdLib/Bmi.h>` | Portable and intrinsic `SimdLib::Bmi` bit-manipulation helpers |
| `<SimdLib/UInt128.h>` | Root `uint128_t`, literals, bit utilities, hash, and numeric limits |
| `<SimdLib/Format.h>` | Opt-in `std::formatter` specializations |
| `<SimdLib/SimdLib.h>` | Complete non-formatting public surface |

`Detail` headers and declarations are implementation-only and must not be
named by consumer code.

SimdLib 0.2.0 adopts `Api` as the primary facade name. The deprecated
`SimdApi` spelling remains available through `<SimdLib/SimdApi.h>` until
1.0.0. Wide-integer BMI operations are available only under `SimdLib::Bmi`;
the redundant root forwarding functions were removed. See
[PublicNamespace.md](docs/PublicNamespace.md) for the complete namespace map
and compatibility policy.

## Configuration and ABI

All library-controlled macros are guarded by `#ifndef` and may be set before
the first SimdLib include. The primary controls are:

- `SIMDLIB_TARGET_X86` and `SIMDLIB_TARGET_X64` describe the compiler target;
  the corresponding `SimdLib::Config::target_x86` and `target_x64` constants
  expose the resolved values.
- `SIMDLIB_HAS_SSE*`, `SIMDLIB_HAS_AVX`, `SIMDLIB_HAS_AVX2`,
  `SIMDLIB_HAS_FMA`, `SIMDLIB_HAS_BMI1`, and `SIMDLIB_HAS_BMI2` describe
  compiler-enabled instruction families. They are not runtime CPU detection.
- `SIMDLIB_FORCE_INLINE` selects the supported compiler attribute plus
  `inline` and may be replaced with ordinary `inline`.
- `SIMDLIB_PRECONDITION(condition, message)` is the assertion replacement
  point and defaults to standard `assert`.
- `VECTORCALL` is ABI-affecting. It is `__vectorcall` for supported MSVC and
  Clang x86/x64 targets and empty elsewhere. A caller overriding it with an
  empty definition must also set `SIMDLIB_VECTORCALL_ENABLED=0` consistently
  in every translation unit.

An empty `VECTORCALL` changes only function calling-convention selection. It
does not disable SSE, AVX, FMA, BMI, or any other target-specific instruction;
those remain controlled exclusively by `SIMDLIB_HAS_*` and compiler flags.
All linked translation units must use the same `VECTORCALL` definition for
SimdLib declarations to remain ABI-compatible.

See [CompilerConfiguration.md](cmake/CompilerConfiguration.md) for probe and
warning-policy details.

## API examples

The executable [ApiExamples.cpp](examples/ApiExamples.cpp) is compiled and run
by CI. It covers every major public facility in one consumer translation unit:

```cpp
#include <SimdLib/Format.h>
#include <SimdLib/SimdLib.h>

using Api = SimdLib::Api<128, std::uint32_t>;
auto sum = Api::add(Api::setr(1, 2, 3, 4), Api::set1(10));

SimdLib::SimdVector<std::uint32_t, 4> vector{3, 5, 7, 9};
auto vector_text = std::format("{}", vector);

std::array<std::uint8_t, 8> values{1, 2, 3, 4, 5, 6, 7, 8};
bool contains_six = SimdLib::SimdAlgo<8, 8>::AnyEqual(values, std::uint8_t{6});

auto extracted = SimdLib::Bmi::pext_u32(std::uint32_t{0xD2}, std::uint32_t{0xF0});
SimdLib::uint128_t wide = SimdLib::uint128_t{~std::uint64_t{0}} + 1;
auto wide_text = std::format("{}", wide);

std::array<std::uint8_t, 1> packed{};
SimdLib::SimdResample::ReduceBytesToBitsBy8_Any(values, packed);
```

The example target is enabled with `-DSIMDLIB_BUILD_EXAMPLES=ON`.

### Formatting

`SimdVector` formatting preserves the logical lane order and renders the
container as `{a, b, c}`. Its optional format specification is limited to
container-level `[[fill]align][width]`; elements use their default formatter,
and element format specifications are not forwarded.

`uint128_t` supports default decimal plus `d`, `x`, `X`, `b`, `B`, and `o`
presentations. Its supported specification is
`[[fill]align][sign][#][0][width][type]`. It follows unsigned-integer behavior
for `+`, space, alternate prefixes, uppercase, numeric zero padding, fill, and
left/right/center alignment. Precision, locale, dynamic width, and other
presentation types are rejected with `std::format_error`.

## Development switches

- `SIMDLIB_BUILD_SMOKE_TESTS=ON` builds the two-translation-unit ODR smoke
  executable (default).
- `SIMDLIB_BUILD_HEADER_TESTS=ON` compiles each public header as the first and
  only SimdLib header in its translation unit (default).
- `SIMDLIB_BUILD_TESTS=ON` builds the focused Catch2 parity tests. Catch2 v3 is
  fetched when it is not installed and `SIMDLIB_FETCH_TEST_DEPENDENCIES=ON`.
- `SIMDLIB_BUILD_TESTS_128`, `SIMDLIB_BUILD_TESTS_256`, and
  `SIMDLIB_BUILD_TESTS_FMA` independently control the SSE4.2, AVX2, and FMA
  executables. Disable families the test host cannot execute. CTest labels use
  the same family names so CI can include or exclude them explicitly.
- `SIMDLIB_BUILD_TESTS_OPTIONAL=ON` enables the BMI1/BMI2 intrinsic-path test
  executable and its deterministic result-set comparison with the always-built
  BMI-disabled portable executable; it is off by default so unsupported hosts
  never execute BMI instructions.
- `SIMDLIB_BUILD_VECTOR_ALGORITHM_TESTS=ON` builds the `SimdVector`, `SimdAlgo`, SIMD
  resampling, and forced-scalar resampling parity matrix. CTest labels these
  executions `VECTOR_ALGORITHMS`, with `AVX2` or `SCALAR` identifying the selected path.
- `SIMDLIB_BUILD_BENCHMARKS=ON` builds Catch2 benchmarks and requires a
  discoverable Catch2 v3 package.
- `SIMDLIB_BUILD_EXAMPLES=ON` builds and registers the complete API example.
- `SIMDLIB_STRICT_WARNINGS=ON` enables the compiler-specific strict warning
  policy and treats warnings as errors for SimdLib-owned targets.

## Continuous validation

`.github/workflows/ci.yml` defines Debug and Release jobs for MSVC, clang-cl,
Clang, and GCC on supported x86/x64 targets, a sanitizer job using Clang ASan
and UBSan, an independent instruction-family matrix, and explicit constexpr,
first-include header-hygiene, multi-translation-unit ODR, and consumer gates.

The consumer smoke project under `tests/consumer` imports SimdLib with
`add_subdirectory`, verifies that `SimdLib` is an `INTERFACE_LIBRARY`, and
links only the consumer executable—there is no SimdLib runtime binary.

The completed compiler, sanitizer, consumer, and pre-extraction benchmark
evidence is recorded in [Validation.md](docs/Validation.md).
