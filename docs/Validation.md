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
| clang-cl 22.1.8 | x64 | Debug, Release | 19/19 tests passed in each configuration |
| Clang 22.1.8 | x64 | Release | 19/19 tests passed |
| GCC 13.2 | x64 | Debug, Release | 19/19 tests passed in each configuration |

SimdLib supports 64-bit targets only; 32-bit compiler configurations are outside
the validation contract.

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

## Register interface closeout (2026-07-24)

The C++23 complete-register interface was qualified with strict warnings on
native Windows and the pinned Alpine/musl containers. The C++20
`SimdLib::SimdLib` target remains unchanged: its umbrella, configuration,
header-isolation, constexpr, ODR, and external-consumer probes compile without
requiring the Register interface. `SimdLib::Register` remains the opt-in C++23
target and supplies the interface-availability requirement.

### Correctness and integration matrix

| Compiler | Configuration | Project tests | External consumer | Result |
| --- | --- | ---: | ---: | --- |
| MSVC 19.44.35222.0 | x64 Release | 237 | 2 | No failures |
| MSVC 19.44.35222.0 | x64 Debug | 237 | 2 Release consumer probes | No failures |
| clang-cl 22.1.8 | x64 Release | 249 | 2 | No failures |
| clang-cl 22.1.8 | x64 Debug | 210 | 2 Release consumer probes | No failures |
| GCC 14.2.0 | Alpine x86-64 Release | 240 | 2 | No failures |
| GCC 14.2.0 | Alpine x86-64 Debug | 210 | 2 | No failures |
| Clang 22.1.3 | Alpine x86-64 Release | 240 | 2 | No failures |
| Clang 22.1.3 | Alpine x86-64 Debug | 210 | 2 | No failures |
| Clang 22.1.3 | Alpine x86-64 Debug, ASan+UBSan | 210 | 2 | No failures or sanitizer diagnostics |

The Release MSVC and clang-cl Register executables were also run directly to
retain Catch assertion totals. The SSE4.2-only executable completed 15 test
cases and 3,048 assertions; the AVX2 executable completed 18 test cases and
8,095 assertions. Each compiler therefore completed 33 direct Register cases
and 11,143 assertions in addition to the CTest integration gates.

The different CTest totals are intentional. Release configurations include
the complete optional-feature and optimized code-generation matrix. Debug and
sanitizer configurations use the portable feature set and record, rather than
enforce, wrapper/raw instruction differences. All configurations include the
C++20 unavailable-interface probe, C++23 constexpr and constraint probes,
first-and-only public-header probes, the two-translation-unit Register ODR
executable, runtime scalar-oracle tests, and the C++23 example.

Windows JUnit records and copied comparison artifacts are under
`out/register-closeout`. Container JUnit, provenance, compiler identities, and
build output are under `out/container/{gcc14,clang22}/{full,debug,codegen}` and
`out/container/clang22/sanitizer`. The final per-run console logs are:

- Release: `out/container/logs/20260724-160527822-full-53304`;
- Debug: `out/container/logs/20260724-160640863-debug-35896`;
- sanitizer: `out/container/logs/20260724-160816616-sanitizer-47180`;
- generated code: `out/container/logs/20260724-160934259-codegen-53916`; and
- benchmarks: `out/container/logs/20260724-161026349-benchmark-52084`.

### Generated-code and ABI results

Each optimized profile compares separately compiled wrapper and raw objects,
including forced-inline expressions, no-inline ABI mirrors, downstream
consumer boundaries, register pressure, lane access, masks, transfers,
specialized operations, rearrangements, and conversions. The result counts
below are complete comparison records, not sampled symbols.

| Compiler and profile | Exact parity | Recorded difference | Exact accepted exception |
| --- | ---: | ---: | ---: |
| MSVC SSE4.2/128 diagnostic | 7 | 0 | 1 |
| MSVC AVX2/128 strict | 8 | 0 | 1 |
| MSVC AVX2/256 strict | 9 | 0 | 0 |
| clang-cl SSE4.2/128 diagnostic | 9 | 0 | 0 |
| clang-cl AVX2/128 strict | 10 | 0 | 0 |
| clang-cl AVX2/256 strict | 10 | 0 | 0 |
| GCC SSE4.2/128 diagnostic | 5 | 4 | 0 |
| GCC AVX2/128 strict | 10 | 0 | 0 |
| GCC AVX2/256 strict | 10 | 0 | 0 |
| Clang SSE4.2/128 diagnostic | 9 | 0 | 0 |
| Clang AVX2/128 strict | 10 | 0 | 0 |
| Clang AVX2/256 strict | 10 | 0 | 0 |

The sole accepted optimized exception is the exact MSVC 19.44 `/GS` security
cookie sequence for 128-bit `Register<double>::from_array`. The comparator has
separate exact recognizers for its SSE4.2 and AVX2 instruction forms and still
requires every other instruction to match. It is one compiler behavior observed
in two ISA profiles, not two independent exceptions.

AVX2 is the supported zero-overhead profile. SSE4.2 is an optimized diagnostic
profile: GCC's four differences are retained for inspection and do not enlarge
the strict claim. Clang and clang-cl happened to produce exact SSE4.2 parity,
but that observation does not promote SSE4.2 into the zero-overhead contract.
The Windows supported non-inline boundary is `VECTORCALL`; platform-default
aggregate return behavior remains diagnostic. GCC and GNU-like Clang use their
ordinary platform convention because `VECTORCALL` is empty there.

The complete exception and exclusion ledger is maintained in
[RegisterQualification.md](RegisterQualification.md). It also records the MSVC
constexpr bit-cast frontend failure, Windows platform-default hidden return
storage, Debug and sanitizer differential policy, memory-capable `/GS` paths,
and unsupported architectures, widths, and compiler floors. No additional
optimized exception was accepted during closeout.

### Supplemental benchmarks

The runtime-derived corpus completed all 12 wrapper/raw entries with 25 samples
per entry on MSVC 19.44, GCC 14.2, and Clang 22.1. The benchmark includes
128-bit and 256-bit floating add, mask selection, and unsigned integer division.
Inputs are runtime-derived and results remain observable. Timing is
supplemental: it neither sets a performance threshold nor overrides generated-
code parity.

### Reproduction commands

The native Windows configurations use the ordinary project options and the
same source-provided Catch dependency:

```powershell
cmake --preset msvc -DSIMDLIB_BUILD_TESTS=ON -DSIMDLIB_BUILD_EXAMPLES=ON -DSIMDLIB_BUILD_REGISTER_CODEGEN=ON -DSIMDLIB_STRICT_WARNINGS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure

cmake -S . -B build-clangcl-register -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang-cl -DSIMDLIB_BUILD_TESTS=ON -DSIMDLIB_BUILD_EXAMPLES=ON -DSIMDLIB_BUILD_REGISTER_CODEGEN=ON -DSIMDLIB_STRICT_WARNINGS=ON
cmake --build build-clangcl-register --parallel
ctest --test-dir build-clangcl-register --output-on-failure

cmake --build build --config Release --parallel --target SimdLibBenchmarks
.\build\Release\SimdLibBenchmarks.exe "[simdlib][benchmark][register]" --benchmark-samples 25
```

The pinned Linux runs were executed with:

```powershell
.\tools\Run-ContainerMatrix.ps1 -Mode Full -Compiler All -NoBuild
.\tools\Run-ContainerMatrix.ps1 -Mode Debug -Compiler All -NoBuild
.\tools\Run-ContainerMatrix.ps1 -Mode Sanitizer -Compiler Clang22 -NoBuild
.\tools\Run-ContainerMatrix.ps1 -Mode Codegen -Compiler All -NoBuild
.\tools\Run-ContainerMatrix.ps1 -Mode Benchmark -Compiler All -NoBuild
```
