# Validation evidence

Validation was completed on 2026-07-17 with strict warnings enabled for every
SimdLib-owned target. Each full configuration built the configuration and
constexpr probes, first-and-only header probes, multi-translation-unit ODR
smoke test, API example, portable and optimized UInt128 variants, scalar and
SIMD resampling paths, FMA enabled/disabled paths, and all BMI1/BMI2 profiles.

## Local compiler matrix

| Compiler | Target | Configuration | Result |
| --- | --- | --- | --- |
| MSVC 19.44 | x64 | Debug, Release | 19/19 tests passed in each configuration |
| MSVC 19.44 | x86 | Debug, Release | 19/19 tests passed in each configuration |
| clang-cl 22.1.8 | x64 | Debug, Release | 19/19 tests passed in each configuration |
| clang-cl 22.1.8 | x86 | Debug, Release | 19/19 tests passed in each configuration |
| Clang 22.1.8 | x64 | Release | 19/19 tests passed |
| Clang 22.1.8 | x86 | Debug, Release | 19/19 tests passed in each configuration |
| GCC 13.2 | x64 | Debug, Release | 19/19 tests passed in each configuration |

The local MinGW GCC installation is x64-only and cannot link `-m32` because it
has no 32-bit UCRT/import libraries or multilib. The Linux CI x86 jobs install
`g++-multilib` explicitly, so x86 GCC and Clang remain part of the committed CI
contract rather than being silently omitted.

Clang ASan and UBSan validation used Debug symbols, `-O1`, frame pointers, and
strict warnings. All 13 runtime tests passed with no sanitizer diagnostics.
On Windows, the release CRT was selected for this run because Clang ASan and
the MSVC Debug CRT allocator instrumentation are incompatible.

The only intentionally suppressed diagnostics are unsupported/ignored vendor
attributes, Clang's diagnostic for Catch2's `__COUNTER__` extension use, and
compiler SIMD-register template attributes. The exact warning switches are
documented in [CompilerConfiguration.md](../cmake/CompilerConfiguration.md).

## Header-only consumer gate

`tests/consumer` imports the source tree with `add_subdirectory`, asserts that
the `SimdLib` CMake target is an `INTERFACE_LIBRARY`, and builds only its own
executable. The MSVC, Clang, and GCC Release consumer configurations each pass
their 1/1 CTest smoke test and produce no SimdLib library binary.

## Namespace stabilization gate

SimdLib 0.2.0 was revalidated after adopting root `Api`, keeping root
`SimdVector` and `uint128_t`, and consolidating wide-integer operations under
`Bmi`. The final MSVC 19.44 strict Release matrix under
`build-m12-msvc` passes 19/19 CTest entries. The external clang-cl 22.1.8
strict Release matrix under `build-m12-clang-ninja` also passes 19/19 entries;
its direct Catch executables cover 74 cases and 4,228,913 assertions. Both
matrices compile 12 first-and-only public-header probes, the availability and
configuration probes, the multi-translation-unit smoke executable, all
optional BMI profiles and equivalence checks, and the API example.

The strengthened external consumer instantiates `Api`, `SimdVector`, `Bmi`,
and `uint128_t` through `SimdLib::SimdLib`. Fresh MSVC and clang-cl consumer
builds each pass their 1/1 CTest entry under `build-m12-consumer-msvc` and
`build-m12-consumer-clang`. The library target remains an
`INTERFACE_LIBRARY`. A configure-time source guard also rejects any public
example, consumer, smoke source, or header probe that names `SimdLib::Detail`
or includes a `Detail` header.

The representative namespace-stabilization benchmarks passed 1/1 case on
both compilers:

| Operation | MSVC 19.44 | clang-cl 22.1.8 |
| --- | ---: | ---: |
| `Api` 128-bit add | 0.444812 ns | 0.293360 ns |
| `Api` 256-bit add | 0.444038 ns | 0.428736 ns |
| BMI2 `pext_u64` | 0.222357 ns | 0.203131 ns |
| `uint128_t` add | 0.405140 ns | 0.306990 ns |
| Reduce-by-8 resample | 9.54669 ns | 7.27583 ns |

The MSVC resample sample was noisy (6.43357 ns standard deviation), and the
compiler/harness difference makes cross-column comparison directional rather
than a regression measurement. Authoritative logs are
`build-m12-msvc/{configure-final,build-final,ctest-final,benchmark-final}.log`,
`build-m12-clang-ninja/{configure,build,ctest,benchmark}.log`, and the
corresponding consumer build directories.

## Pre-extraction benchmark comparison

The standalone GCC Release benchmark was sampled 100 times in three runs. The
last stable run is compared with the pre-extraction baseline below.

| Operation | Pre-extraction | Standalone | Change |
| --- | ---: | ---: | ---: |
| Api 128-bit add | 0.435963 ns | 0.296309 ns | -32.03% |
| Api 256-bit add | 0.470220 ns | 0.303181 ns | -35.52% |
| BMI2 `pext_u64` | 0.204546 ns | 0.280442 ns | +37.10% |
| `uint128_t` add | 0.267830 ns | 0.276382 ns | +3.19% |
| Reduce-by-8 resample | 7.694720 ns | 7.894700 ns | +2.60% |

The resample case was noisy in its first run, then stabilized at
7.89470-7.90786 ns, so it does not show a material regression. Disassembly
confirmed the expected `vpaddd` instructions in the SIMD cases and compare plus
movemask instructions in the resampler.

The apparent BMI2 percentage is only 0.075896 ns and is not an intrinsic
regression: disassembly contains no `pext` because the constant-input benchmark
was folded to an immediate result. The pre-extraction case used the same representative
constant-input shape, so this comparison measures sub-nanosecond loop/compiler
overhead. The constant-input UInt128 operation is likewise precomputed. Neither
result warrants an implementation change; future microarchitecture measurement
should use a runtime-generated input corpus.
