# Unified Build Pipeline Baseline

This report freezes the build and validation surface that existed before the
unified pipeline refactor. It is execution evidence for the implementation
plan, not timeless user documentation.

## Evidence identity and method

- Repository revision: `87b3b915ea9b65dae1e9701800a4e1a42b279dd1`.
- Measurement date: 2026-07-25.
- Host architecture: x86-64.
- Native tools: CMake/CTest 4.4.0, MSVC 19.44.35222, clang-cl/Clang 22.1.8,
  Visual Studio generator 17 2022, and Ninja 1.12.1.
- Container images: `simdlib/gcc14:local` image
  `sha256:820ef59f8c1a31466939d26725d8792603fbf42a4fe96874f1230886e79368f0`
  (294,926,856 bytes) and `simdlib/clang22:local` image
  `sha256:0196fabc9bc09137e15f04e21d87d6897e0ad0157b8c1baab8e08baf1df3468e`
  (497,722,844 bytes).
- Container measurements used new directories below
  `out/container/baseline-20260725`; native measurements used new directories
  below `out/baseline-20260725/native`. Existing build trees were not removed
  or reused.
- Container clean-build durations come from each generated `.ninja_log` and
  cover the main CMake build. Native Ninja durations use the same source.
  Visual Studio durations were measured around `cmake --build` after the
  isolated target tree was cleaned. Warm durations are immediate subsequent
  `cmake --build` calls.
- Every warm build produced zero C++ compiler actions. Ninja still rechecked
  source globs and every default build reran the public-header assertion audit;
  these are inexpensive build-graph checks rather than recompilation.
- Current-operation wall time covers what the current user-facing operation
  actually does. Container operations include configure, main build, CTest,
  separate consumer configure/build/CTest, and benchmark execution where
  selected. Image construction is excluded because the images were already
  present. Native preset and CI operations include configure, build, and CTest,
  except `msvc-all`, whose checked-in workflow is build-only.
- GCC and Clang services, and paired native configurations, were measured
  concurrently to match the current aggregation model. These timings are a
  structural baseline, not a compiler-speed benchmark.

The exact sorted union of current CTest identities is frozen in
`UnifiedBuildPipelineExpectedTests.txt`: 251 names with SHA-256
`c0d75844cf024aef09495911777f1dd37ece00d5176a3d4750f5d551db13483f`.
The exact sorted logical target union is frozen in
`UnifiedBuildPipelineExpectedTargets.txt`: 137 names with SHA-256
`d9bdaa60ac22759a5868feb25068721887aeedb74c6151e747650af40e4474bd`.
The files contain names only, use ordinal sorting, and intentionally include
current names that the rename ledger retires.

## Current interface inventory

### Presets and native automation

| Definition | Current tree | Configuration and scope | Execution owner |
| --- | --- | --- | --- |
| configure `msvc` | `build` | MSVC, multi-config; runtime and BMI tests, strict warnings | build/test presets `msvc-release` and documentation |
| configure `msvc-all` | `build-all` | MSVC exhaustive Release graph, examples, benchmarks, Register codegen | workflow/build preset `msvc-all` and the default VS Code build task |
| configure `clang-coverage` | `build-coverage` | Clang Debug plus LLVM coverage, runtime and BMI tests | build/test preset `coverage`, CMake Tools coverage settings, documentation |
| hidden configure `container-base` | `$SIMDLIB_BUILD_ROOT/<preset>` | Ninja, C++20, strict warnings, configuration/header/smoke contracts | inherited by every container configure preset |
| configure `container-focused` | mode-owned `focused` tree | Release compile contracts only | runner `Focused`, reproducibility workflow |
| configure `container-full` | separate mode-owned `full` or `feature` tree | Release runtime/BMI/examples | runner `Full` and `Feature` |
| configure `container-codegen` | mode-owned `codegen` tree | Release compile contracts plus enforced Register codegen | runner `Codegen` |
| configure `container-debug` | mode-owned `debug` tree | Debug runtime/examples plus recorded Register differentials | runner `Debug` |
| configure `container-benchmark` | mode-owned `benchmark` tree | Release compile contracts plus benchmark executable | runner `Benchmark` |
| configure `container-sanitize` | mode-owned `sanitizer` tree | Clang Debug ASan+UBSan runtime/examples plus recorded differentials | runner `Sanitizer` |

The checked-in CI adds four native scenarios without presets:

- job `windows`, matrix configurations Debug and Release, using MSVC with
  runtime tests, examples, strict warnings, and BMI tests disabled;
- job `clang-cl`, matrix configurations Debug and Release, using clang-cl and
  Ninja with the same option surface;
- job `linux-containers`, running `Full`, then duplicate `Feature`, then Clang
  `Sanitizer`; and
- job `rebuild` in `container-reproducibility.yml`, rebuilding both images
  without cache and compiling the `Focused` contract graph.

### Compose, runner, and entrypoint

- Compose services are `gcc14` and `clang22`. Both advertise profiles
  `focused`, `full`, `feature`, `codegen`, `debug`, and `benchmark`; only
  `clang22` advertises `sanitizer`.
- `Run-ContainerMatrix.ps1` exposes modes `Focused`, `Full`, `Feature`,
  `Sanitizer`, `Codegen`, `Debug`, and `Benchmark`; compilers `All`, `Gcc14`,
  and `Clang22`; switches `NoBuild`, `NoCache`, `DoctorOnly`, `Clean`; and the
  failure/cancellation controls `InjectFailure` and `CancelAfterSeconds`.
- `NoBuild` suppresses only `docker compose build`. It does not suppress CMake
  configuration or compilation. `NoCache` affects image layers only.
- The entrypoint accepts `--preset`, `--build-target`, `--test-regex`,
  `--test-label`, `--configuration`, `--sanitizer`, `--output-dir`,
  `--doctor-only`, and `--run-benchmarks`.
- Every non-inspection entrypoint run configures and builds the main project,
  runs main CTest, independently configures/builds/tests the external consumer,
  and optionally runs the Register benchmark. No build-only or test-only
  operation exists.
- Local entrypoint configuration preserves its CMake cache. Any nonempty
  supported CI indicator prepends `--fresh`, so every CI scenario reconfigures
  its tree before building.

### Tests, benchmarks, consumers, reports, and cleanup

- CTest identities are represented exactly by the frozen test inventory. The
  current registered totals are compiler- and option-dependent: 246 for
  `msvc-all`, 235 for `msvc`, 238 for Clang coverage, 198 for each MSVC CI
  cell, 201 for each clang-cl CI cell, 240 for each Linux Full tree, 210 for
  each Linux Debug/diagnostic tree, 13 for each codegen tree, and 4 for each
  focused or benchmark tree. Feature executes 163 of Full's 240 tests.
- Each container scenario separately builds the external consumer and runs its
  two CTest entries. Current native CI does not run the external consumer;
  `docs/Validation.md` owns separate manual MSVC and clang-cl consumer commands.
- `SimdLibBenchmarks` is built by `msvc-all` and `container-benchmark`. The
  container benchmark operation executes only
  `[simdlib][benchmark][register]` with 25 samples. Documentation separately
  describes the same supplemental MSVC invocation.
- Main container reports are `<service>/<mode>/ctest.xml`, consumer reports are
  `<service>/<mode>/consumer-ctest.xml`, provenance is
  `<service>/<mode>/provenance.txt`, and aggregate logs are
  `out/container/logs/<run-id>`.
- The coverage tree owns raw profiles, merged profile data, `coverage.info`,
  `SimdLibCoverageReset`, and `SimdLibCoverageReport`.
- Register codegen artifacts currently live below
  `<tree>/register-codegen/{sse42/128,avx2/128,avx2/256}`. Successful
  comparisons are represented by empty `comparison.stamp` files plus
  disassembly/diff artifacts.
- Each runner invocation uses `docker compose down --remove-orphans` in
  `finally`. `Run-ContainerMatrix.ps1 -Clean` removes matching project
  containers/networks, the two local image tags, and `out/container` after
  validating that the artifact root is inside the repository. There is no
  canonical native cleanup command.

Canonical preset-owned native directories are `build`, `build-all`, and
`build-coverage`. Active container directories are
`out/container/<service>/<mode>`. Manual validation documentation also names
`build-register-*` trees. Other root `build-*` trees carrying `phase`,
`doc-inventory`, or one-off consumer/sanitize labels are historical local
evidence, not supported interfaces, and receive no migration alias.

The observed root-level build directory inventory was:

```text
build
build-all
build-consumer-phase3
build-coverage
build-doc-inventory
build-phase11-clangcl-debug
build-phase11-clangcl-release-final
build-phase11-codegen-msvc
build-phase11-consumer-clangcl
build-phase11-consumer-msvc
build-phase8-sanitize
build-phase9-clangcl
build-phase9-clangcl-ninja
build-phase9-compile-time
build-phase9-consumer-clangcl
build-phase9-consumer-msvc
build-register-clangcl-debug
build-register-clangcl-release
build-register-consumer-clangcl
build-register-consumer-msvc
build-register-debug-clangcl
build-register-debug-msvc
build-register-phase0-clangcl
build-register-phase0-consumer-clangcl
build-register-phase0-consumer-msvc
build-register-phase0-gcc
build-register-phase0-msvc
build-register-phase0-sanitize
build-register-phase1-clang
build-register-phase1-clangcl
build-register-phase1-consumer-clang
build-register-phase1-consumer-clangcl
build-register-phase1-consumer-gcc
build-register-phase1-consumer-gcc-unsupported
build-register-phase1-consumer-msvc
build-register-phase1-gcc
build-register-phase1-msvc
```

Only the three preset-owned roots and the explicitly documented current
consumer/reproduction roots are interfaces. The remainder are ignored local
evidence directories and are intentionally not migrated into the unified
layout.

The current command inventory is consumed by `docs/ContainerValidation.md`,
`docs/RegisterQualification.md`, `docs/TestCoverage.md`, `docs/Validation.md`,
`wiki/Technical-Reference.md`, `.github/workflows/*.yml`, `.vscode/tasks.json`,
and `.vscode/settings.json`. These files form one coordinated update boundary.

| Documentation owner | Current command inventory |
| --- | --- |
| `docs/ContainerValidation.md` | Full all/GCC-only, Focused, Feature/Sanitizer/Codegen/Debug/Benchmark with `-NoBuild`, Focused `-NoCache`, Focused `-DoctorOnly`, `-Clean`, two failure-injection commands, and cancellation |
| `docs/RegisterQualification.md` | Full, Codegen, Debug, Sanitizer, and Benchmark container commands |
| `docs/TestCoverage.md` | Clang coverage configure/build/reset/test/report, current MSVC/clang-cl/coverage/sanitizer reproduction commands, and their artifact paths |
| `docs/Validation.md` | explicit MSVC and clang-cl Release/Debug configure/build/test commands, both standalone consumers, direct Register tests, MSVC benchmark build/run, and Full/Debug/Sanitizer/Codegen/Benchmark container commands |
| `wiki/Technical-Reference.md` | `msvc-all` workflow/build-only guidance, `msvc-release`, Clang coverage, CTest, and coverage target commands |

## Current scenario and fingerprint map

Target-local variants remain distinct targets inside a tree: SSE4.2, AVX2,
FMA enabled/disabled, BMI portable/BMI1/BMI2/BMI1+BMI2, scalar, carry-enabled,
carry-disabled, and disabled-public-feature probes. They do not create whole-
tree fingerprints. `SIMDLIB_BUILD_*` cache values, compiler identity,
configuration, instrumentation, standard-library/linker policy, and global
compile/link flags do.

| Current scenario | Compiler/configuration/instrumentation | Main validation | Consumer | Codegen | Benchmark | Reports |
| --- | --- | --- | ---: | --- | ---: | --- |
| `msvc-release` | MSVC Release | runtime+BMI, compile/header/constexpr/smoke | 0 | off | 0 | CTest log |
| `msvc-all` workflow | MSVC Release | exhaustive build graph | 0 | enforce | 1 built | build output only |
| `coverage` | Clang Debug coverage | runtime+BMI, compile/header/constexpr/smoke | 0 | off | 0 | profiles and CTest log |
| CI MSVC Debug | MSVC Debug | runtime without BMI, examples, compile contracts | 0 | off | 0 | CTest log |
| CI MSVC Release | MSVC Release | runtime without BMI, examples, compile contracts | 0 | off | 0 | CTest log |
| CI clang-cl Debug | clang-cl Debug | runtime without BMI, examples, compile contracts | 0 | off | 0 | CTest log |
| CI clang-cl Release | clang-cl Release | runtime without BMI, examples, compile contracts | 0 | off | 0 | CTest log |
| GCC/Clang `Focused` | Release | compile/header/constexpr/smoke only | 2 tests | off | 0 | main/consumer JUnit+provenance |
| GCC/Clang `Full` | Release | complete runtime+BMI+examples | 2 tests | off | 0 | main/consumer JUnit+provenance |
| GCC/Clang `Feature` | Release, identical cache to Full | Full graph; AVX2/FMA/BMI/SCALAR test filter | 2 tests | off | 0 | main/consumer JUnit+provenance |
| GCC/Clang `Codegen` | Release | compile contracts | 2 tests | enforce | 0 | JUnit+14 comparison stamps+provenance |
| GCC/Clang `Debug` | Debug | runtime without BMI+examples | 2 tests | record | 0 | JUnit+14 comparison stamps+provenance |
| GCC/Clang `Benchmark` | Release | compile contracts | 2 tests | off | 1 built/run | JUnit+benchmark console+provenance |
| Clang `Sanitizer` | Debug ASan+UBSan | runtime without BMI+examples | 2 tests | record | 0 | JUnit+14 comparison stamps+provenance |

Clang container fingerprints additionally carry `-stdlib=libc++` and linker
flags `-fuse-ld=lld --rtlib=compiler-rt --unwindlib=libunwind`. The sanitizer
fingerprint adds `-fsanitize=address,undefined -fno-omit-frame-pointer` and the
matching linker flag. Generated-code targets on GCC and Clang carry
`-fstack-protector-strong`; optimized enforcement targets add `-O2`. These
effective values are fingerprint or target identity even when supplied through
the entrypoint rather than a preset.

## Baseline measurements

`Compile outputs` is both the build-system compiler-action count and resulting
object count because every measured translation-unit action emits one object.
Container consumer compiler actions/objects are shown after `+`.
Visual Studio counts use resulting target-tree object outputs; Ninja counts use
the clean `.ninja_log`. Artifact size covers the scenario tree after the clean
operation.

### Native scenarios

| Scenario | Clean build (s) | Warm build (s) | Clean current operation (s) | Warm current operation (s) | Compile outputs | Tests executed/registered | Comparisons | Benchmarks | MiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `msvc-release` | 123.788 | 1.430 | 138.478 | 26.487 | 191 | 235/235 | 0 | 0 | 83.40 |
| `msvc-all` | 123.319 | 2.125 | 139.204 | 23.931 | 235 | 0/246 | 11 | 1 | 141.89 |
| `coverage` | 19.459 | 0.291 | 44.032 | 22.065 | 189 | 238/238 | 0 | 0 | 402.11 |
| CI MSVC Debug | 76.391 | 5.866 | 96.415 | 30.759 | 190 | 198/198 | 0 | 0 | 772.73 |
| CI MSVC Release | 99.294 | 1.448 | 107.844 | 29.109 | 190 | 198/198 | 0 | 0 | 80.77 |
| CI clang-cl Debug | 37.259 | 0.280 | 59.125 | 18.185 | 189 | 201/201 | 0 | 0 | 328.66 |
| CI clang-cl Release | 38.505 | 0.270 | 59.078 | 16.692 | 189 | 201/201 | 0 | 0 | 38.39 |

### Container scenarios

| Scenario | Clean main build (s) | Warm main build (s) | Clean current operation (s) | Warm current operation (s) | Compile outputs | Tests main+consumer | Comparisons | Benchmarks | MiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| GCC `Focused` | 7.473 | 1.201 | 37.879 | 22.024 | 51+2 | 4+2 | 0 | 0 | 1.23 |
| Clang `Focused` | 10.385 | 1.268 | 45.206 | 28.744 | 51+2 | 4+2 | 0 | 0 | 1.14 |
| GCC `Full` | 88.380 | 1.825 | 128.292 | 26.163 | 191+2 | 240+2 | 0 | 0 | 40.22 |
| Clang `Full` | 81.072 | 1.854 | 136.293 | 32.242 | 191+2 | 240+2 | 0 | 0 | 34.26 |
| GCC `Feature` | 79.429 | 1.776 | 120.378 | 26.497 | 191+2 | 163+2 | 0 | 0 | 40.09 |
| Clang `Feature` | 77.535 | 1.886 | 128.923 | 32.382 | 191+2 | 163+2 | 0 | 0 | 34.12 |
| GCC `Codegen` | 15.097 | 1.763 | 54.692 | 26.023 | 91+2 | 13+2 | 14 | 0 | 8.11 |
| Clang `Codegen` | 18.683 | 1.705 | 63.158 | 32.425 | 91+2 | 13+2 | 14 | 0 | 8.37 |
| GCC `Debug` | 234.505 | 2.600 | 267.827 | 31.968 | 228+2 | 210+2 | 14 | 0 | 411.50 |
| Clang `Debug` | 210.019 | 2.257 | 269.837 | 38.047 | 228+2 | 210+2 | 14 | 0 | 400.40 |
| GCC `Benchmark` | 33.540 | 1.692 | 87.082 | 33.747 | 159+2 | 4+2 | 0 | 1 | 7.60 |
| Clang `Benchmark` | 37.420 | 2.045 | 96.608 | 38.622 | 159+2 | 4+2 | 0 | 1 | 7.19 |
| Clang `Sanitizer` | 365.638 | 2.251 | 412.616 | 45.269 | 228+2 | 210+2 | 14 | 0 | 738.54 |

The 14-record counts above describe the pre-refactor execution baseline. The
rationalized permanent suite now owns eleven records for SSE4.2/128 and twelve
records for each AVX2 width: primary composition/memory, register-only,
reassignment, FMA-independent specialized operations, FMA-disabled
multiply-add, rearrangement/conversion, canonical common non-modulus type
matrix, isolated integer-modulus type matrix, consumer ABI, explicit-object ABI,
and platform-default ABI, plus the isolated FMA-enabled multiply-add record
under AVX2. The three profiles therefore own 35 records on each
Register-capable compiler. MSVC retains the same record partition; its exact
`Register<double>::from_array` security-cookie exception and narrowly scoped
diagnostic records are expressed by comparator policy rather than by omitting a
broad record.

## Duplicate-work findings

### Exact duplicates

`Full` and `Feature` are exact compilation-fingerprint duplicates for each
container compiler. Both select `container-full` with the same Release cache,
whole-tree flags, dependency, image, target graph, and CPU requirements. Only
the later CTest label differs. Because the current artifact root includes the
mode, Feature creates a second tree and repeats all 191 main and two consumer
compiler actions. The measured duplicate clean work is:

- GCC: 79.429 seconds of main compilation, 120.378 seconds end-to-end, and
  40.09 MiB of duplicated artifacts;
- Clang: 77.535 seconds of main compilation, 128.923 seconds end-to-end, and
  34.12 MiB of duplicated artifacts; and
- 163 already-covered feature-labelled tests plus both consumer tests are run
  a second time for each compiler.

A feature-only CTest filter against the Full tree would be a build-free repeat;
the accepted design removes it from the mandatory pipeline entirely while
retaining labels for diagnostics.

### Overlap that is not an exact fingerprint

- `Focused`, `Codegen`, and `Benchmark` are separate Release configure trees
  that recompile configuration/header/constexpr/smoke/Catch2 inputs already
  represented by the exhaustive Release graph. Their cache option graphs are
  different today, so they are not byte-for-byte fingerprint duplicates, but
  their responsibilities can become targets/actions inside the exhaustive
  tree. The scheduled image job needs environment provenance, not another
  project compilation.
- `Codegen` and `Debug` overlap because Debug enables the same 14 codegen
  records with record-only policy. They cannot share objects across Release
  and Debug, but Release codegen belongs in Release's exhaustive tree rather
  than a codegen-specific tree.
- `Benchmark` repeats 159 main and two consumer compiler actions per compiler.
  Moving the benchmark target into the exhaustive Release configuration and
  building it as a separate target action removes that repetition without
  coupling timing execution to validation.
- Native `msvc`, `msvc-all`, and the MSVC Release CI cell all use the same
  compiler/ABI/configuration but differ in cache-controlled target inventory.
  The final exhaustive Release tree supersedes the narrow variants. MSVC Debug,
  clang-cl Debug, sanitizer, and coverage remain intentionally distinct.
- GCC and Clang, MSVC and clang-cl, Release and Debug, sanitizer and ordinary
  Debug, and coverage and ordinary Debug are incompatible fingerprints. Their
  repeated source files are required compiler/configuration qualification, not
  removable duplicate object work.

Every warm CTest codegen/constexpr build driver still invokes the build tool.
The baseline warm builds compile zero objects, so those current invocations are
no-op graph checks; they nevertheless violate the intended test-only process
boundary and must become build dependencies plus build-free record checks.

## CTest build-driver audit

| Current CTest family | Count when enabled | Current command | Build owner after refactor | Build-free validation after refactor |
| --- | ---: | --- | --- | --- |
| `SimdLib.ConstexprProbes.Build` | 1 | builds `SimdLibConstexprProbes` | `ExhaustiveArtifacts` depends on the constexpr aggregate and assertion audit | verify the expected object outputs and audit record exist and match the manifest |
| `RegisterCodegen.<profile>` | 3 | validates the already-built complete profile record index | `RegisterCodegen<profile>` depends on the expression and consumer-ABI aggregate build targets and every comparison output | validate every retained comparison record and accepted-exception policy exactly once |

`RegisterExpressionCodegen<profile>` and `RegisterConsumerAbi<profile>` remain
build-only convenience targets. They do not register CTests or separate record
indexes, so they cannot revalidate records owned by `RegisterCodegen.<profile>`.

No other current CTest definition invokes `cmake --build`. The public-header
audit and result-set comparisons invoke CMake script mode but do not compile;
they remain validation actions unless their artifacts are promoted into the
build manifest.

## Canonical rename ledger

No compatibility aliases are permitted because no SimdLib version has been
published. A retired CMake cache option supplied explicitly must fail with a
message naming its replacement; retired script arguments fail as unknown.

### CMake options

| Current | Disposition |
| --- | --- |
| `SIMDLIB_BUILD_TESTS` | rename to `SIMDLIB_BUILD_RUNTIME_TESTS` |
| `SIMDLIB_BUILD_TESTS_128` | rename to `SIMDLIB_BUILD_API_SSE42_TESTS` |
| `SIMDLIB_BUILD_TESTS_256` | rename to `SIMDLIB_BUILD_API_AVX2_TESTS` |
| `SIMDLIB_BUILD_TESTS_FMA` | rename to `SIMDLIB_BUILD_FMA_TESTS` |
| `SIMDLIB_BUILD_TESTS_OPTIONAL` | rename to `SIMDLIB_BUILD_BMI_TESTS` |
| `SIMDLIB_BUILD_CONFIGURATION_TESTS` | rename to `SIMDLIB_BUILD_CONFIGURATION_PROBES` |
| `SIMDLIB_BUILD_HEADER_TESTS` | rename to `SIMDLIB_BUILD_HEADER_PROBES` |
| `SIMDLIB_BUILD_REGISTER_CODEGEN` | rename to `SIMDLIB_BUILD_REGISTER_CODEGEN_GATES` |
| `SIMDLIB_REGISTER_CODEGEN_RECORD_ONLY` | replace with `SIMDLIB_REGISTER_CODEGEN_MODE=ENFORCE|RECORD` |
| `SIMDLIB_BUILD_SMOKE_TESTS`, `SIMDLIB_BUILD_VECTOR_ALGORITHM_TESTS`, `SIMDLIB_BUILD_BENCHMARKS`, `SIMDLIB_BUILD_EXAMPLES`, `SIMDLIB_FETCH_TEST_DEPENDENCIES`, `SIMDLIB_STRICT_WARNINGS`, `SIMDLIB_ENABLE_COVERAGE` | retain |
| `SIMDLIB_BUILD_REGISTER_CONSUMER` | retain in the standalone consumer project |

All development options move below the top-level project gate. The consumer's
forced overrides of development options are removed rather than renamed.

### Targets and CTest

The frozen target list is completely covered by these rules:

- retain production targets `SimdLib`, `SimdLib::SimdLib`, `SimdLibRegister`,
  and `SimdLib::Register`;
- retain dependency targets `Catch2` and `Catch2WithMain` as dependency-owned;
- rename the validation aggregates to `ExhaustiveArtifacts` and
  `BenchmarkArtifacts`;
- rename `SimdLibApiExamples`, `SimdLibRegisterExamples`,
  `SimdLibBenchmarks`, `SimdLibDevelopmentWarnings`,
  `SimdLibCoverageReset`, and `SimdLibCoverageReport` to `ApiExamples`,
  `RegisterExamples`, `Benchmarks`, `DevelopmentWarnings`, `CoverageReset`,
  and `CoverageReport`;
- rename `SimdLibTests128`, `SimdLibTests256`,
  `SimdLibTestsRegisterSse42`, and `SimdLibTestsRegister` to `ApiSse42Tests`,
  `ApiAvx2Tests`, `RegisterSse42Tests`, and `RegisterAvx2Tests`;
- rename BMI runtime and constexpr targets to the unambiguous families
  `BmiPortable`, `Bmi1`, `Bmi2`, and `Bmi1Bmi2`, followed by `Tests` or
  `ConstexprProbe` as appropriate;
- rename `SimdLibPreconditionTests` and
  `SimdLibRegisterPreconditionTests` to `PreconditionTests` and
  `RegisterPreconditionTests`;
- rename the standalone consumer-project targets `SimdLibConsumerSmoke` and
  `SimdLibRegisterConsumerSmoke` to `CoreConsumerSmoke` and
  `RegisterConsumerSmoke`;
- for every remaining top-level-only `SimdLibConfig*`, `SimdLibConstexpr*`,
  `SimdLibHeader*`, `SimdLibRegister*`, `SimdLibTests*`, smoke, ODR, FMA,
  UInt128, vector, resampling, and generated-code target in the frozen file,
  remove only the ownership prefix and retain the subject/profile/kind in the
  order `<subject><ISA-or-policy><kind>`; and
- remove obsolete mode aggregates only after their artifacts are dependencies
  of `ExhaustiveArtifacts` or `BenchmarkArtifacts`.

The 251 frozen CTest names are covered by an explicit family migration:

- remove the top-level-only `SimdLib.` ownership prefix;
- map `Tests.SSE42` and `Tests.AVX2` to `Api.SSE42` and `Api.AVX2`;
- map `Tests.RegisterSse42` and `Tests.Register` to `Register.SSE42` and
  `Register.AVX2`;
- map the BMI, FMA, UInt128, vector, resampling, format, and precondition
  families to the same subject/profile vocabulary used by their targets;
- retain the remainder of each discovered Catch2 case name verbatim after its
  owning family; and
- replace the ten build-driver identities with build-free `Artifacts` or
  `Codegen` record-validation identities described in the audit table.

The pre- and post-migration inventory comparison must account for each line of
both frozen files; a pattern rule is not permission to drop an entry.

### Presets, commands, profiles, artifacts, tasks, and jobs

| Current | Canonical disposition |
| --- | --- |
| configure/build/workflow `msvc-all` | rename to scoped `msvc-release-exhaustive` |
| configure `msvc`, build/test `msvc-release` | remove after hidden MSVC fragments and scoped unified commands replace them |
| `clang-coverage`, generic build/test `coverage` | rename to `clang-debug-coverage` |
| `container-base` | rename to hidden `container-common` |
| `container-focused`, runner `Focused`, profile `focused` | rename retained diagnostic scope to `container-release-contracts`/`Contracts`; remove project compilation from image-only reproducibility when it is unnecessary |
| `container-full`, runner `Full`, profile `full` | rename to `container-release-exhaustive`; replace mode with build-cell/action vocabulary |
| runner/profile `Feature`/`feature` | remove; labels remain available for ad hoc CTest filtering |
| `container-codegen`, runner/profile `Codegen`/`codegen` | remove configure tree/profile; use codegen build/validation actions in Release tree |
| `container-benchmark`, runner/profile `Benchmark`/`benchmark` | remove configure tree/profile; use `Build-Benchmarks.ps1` and `Run-Benchmarks.ps1` against Release tree |
| `container-debug`, runner/profile `Debug`/`debug` | rename fingerprint to `container-debug-diagnostics` |
| `container-sanitize`, runner/profile `Sanitizer`/`sanitizer` | rename fingerprint to `container-debug-asan-ubsan` |
| `-NoBuild` | rename to `-SkipImageBuild`; no alias |
| `-NoCache` | rename to `-NoImageCache`; no alias |
| `-DoctorOnly`, `--doctor-only` | rename to `-InspectEnvironment`, `--inspect-environment` |
| `--output-dir` | rename to `--artifact-root` |
| `--configuration` | replace with authoritative fingerprint input or validate against selected profile |
| mode directories `out/container/<service>/<mode>` | replace with `out/pipeline/<compiler>/<cell>-<fingerprint-id>` |
| root trees `build`, `build-all`, `build-coverage` | replace with the owning fingerprint directory; historical one-off trees are removed manually and receive no alias |
| VS Code `Build: All Targets` | rename to `Build` and invoke `tools/Build.ps1` |
| VS Code coverage target/path settings | update atomically to `CoverageReset`, `CoverageReport`, and fingerprint report discovery |
| CI jobs `windows`, `clang-cl`, `linux-containers`, `rebuild` | rename to `native-msvc`, `native-clangcl`, `container-compilers`, and `container-reproducibility`; invoke scoped `Build` then `Run-Tests -SkipBuild` |
| benchmark source `benchmarks/SimdLib.benchmarks.cpp` | rename to `benchmarks/Core.benchmarks.cpp` |

`InjectFailure`, `CancelAfterSeconds`, `Clean`, compiler filters, test regex and
label filters, sanitizer identity, provenance inputs, and CI indicator support
are retained capabilities with names adjusted only where the final command
scope makes ownership explicit.

### Coordinated consumer boundaries

| Boundary | Names consumed | Required coordinated update |
| --- | --- | --- |
| `CMakeLists.txt` and `CMakePresets.json` | every option, target, preset, CTest identity, and build directory | apply module split, target graph, and atomic rename together |
| `tests/consumer/CMakeLists.txt` | production targets, forced development options, register-consumer option | remove forced development options; assert top-level isolation; retain production targets |
| `compose.yml` and both Dockerfiles | profiles, default preset, entrypoint arguments, image/compiler identity | move from mode selection to build-cell/action inputs without changing security or provenance |
| `containers/container-entrypoint.sh` | preset, configuration, artifact, build/test/benchmark arguments | split build-only/test-only and rename arguments atomically |
| `tools/Run-ContainerMatrix.ps1` | modes, profiles, parameters, artifact paths, cleanup | refactor to documented build/test cells; keep failure aggregation and owned cleanup |
| `.github/workflows/*.yml` | runner modes/parameters, native commands, job names, artifact paths | switch each platform scope only after the new commands cover its complete responsibility |
| `.vscode/tasks.json` and `.vscode/settings.json` | `msvc-all`, coverage targets/path, user-facing task labels | update to `Build`, `Run Tests`, and manifest-based coverage paths |
| `docs/ContainerValidation.md`, `docs/RegisterQualification.md`, `docs/TestCoverage.md`, `docs/Validation.md`, `wiki/Technical-Reference.md` | all current commands, names, directories, cleanup, and evidence paths | replace user guidance atomically; retain historical results only in execution evidence |

## Required fingerprint matrix and responsibility ownership

The unified unqualified build is complete only when all twelve fingerprints
below exist. GCC 13.2 is Linux x64 core-only; GCC 14 adds
`SimdLib::Register`.

| Canonical fingerprint | Required ownership |
| --- | --- |
| Native MSVC Release | exhaustive core+Register targets, BMI variants, strict warnings, examples, enforced Register codegen/ABI, benchmarks built separately, core+Register consumer |
| Native MSVC Debug | Debug core+Register correctness, examples, recorded Register differentials, core+Register consumer |
| Native clang-cl Release | exhaustive core+Register targets, BMI variants, strict warnings, examples, enforced Register codegen/ABI, benchmarks built separately, core+Register consumer |
| Native clang-cl Debug | Debug core+Register correctness, examples, recorded Register differentials, core+Register consumer |
| Linux GCC 13.2 Core Release | exhaustive C++20 core, BMI/core ISA variants, strict warnings, core examples/benchmarks/consumer, negative unavailable-Register probe |
| Linux GCC 13.2 Core Debug | Debug C++20 core, core examples/consumer, negative unavailable-Register probe |
| Linux GCC 14 Release | exhaustive core+Register, BMI variants, strict warnings, examples, enforced Register codegen/ABI, benchmarks built separately, core+Register consumer |
| Linux GCC 14 Debug | Debug core+Register correctness, examples, recorded Register differentials, core+Register consumer |
| Linux Clang Release | exhaustive core+Register, BMI variants, strict warnings, examples, enforced Register codegen/ABI, benchmarks built separately, core+Register consumer |
| Linux Clang Debug | Debug core+Register correctness, examples, recorded Register differentials, core+Register consumer |
| Linux Clang Debug ASan+UBSan | instrumented core+Register correctness/examples/consumer and recorded generated-code diagnostics |
| Clang Debug Coverage | instrumented main-project tests and report generation; no downstream consumer instrumentation or coverage controls |

Within each fingerprint, configuration/header/constexpr/availability probes,
public-header audit, header-only/format/Register ODR, precondition isolation,
result-set equivalence, runtime correctness, examples, and assigned codegen/ABI
records each have exactly one CMake target or test owner. Compiler repetition is
intentional qualification. Consumers run once per assigned fingerprint, not
once per later test selection. Benchmarks are built once per Release
fingerprint and run only after validation. Coverage owns its report only.

Cross-fingerprint responsibilities have these owners:

- `Build.ps1`: matrix completeness, compiler availability, image construction,
  bounded concurrency, manifests, and aggregate build failure;
- `Run-Tests.ps1`: manifest validation, CPU-feature validation, all assigned
  build-free test cells, coverage report generation, and aggregate test failure;
- `Build-Benchmarks.ps1`: `BenchmarkArtifacts` in existing Release trees;
- `Run-Benchmarks.ps1`: supplemental benchmark execution without build;
- `InspectEnvironment`: compiler/image/tool/dependency/CPU provenance only;
- failure/cancellation probes: runner integration validation, not another
  compilation fingerprint; and
- project-owned cleanup: only manifests, processes, containers, networks, and
  artifact roots created by the selected operation.

## Canonical source-input digest

The source-input digest is SHA-256 over a canonical sequence of records. It is
stored in the manifest but excluded from the artifact-directory fingerprint so
compatible source edits reuse the same configure tree.

1. Enumerate tracked paths from `git ls-files --cached` and relevant untracked,
   non-ignored paths from `git ls-files --others --exclude-standard`.
2. Retain build-relevant roots and files: `CMakeLists.txt`,
   `CMakePresets.json`, `compose.yml`, `.clang-format` only when formatting is
   itself an assigned validation input, and all files below `include`, `cmake`,
   `tests`, `examples`, `benchmarks`, `containers`, and `tools`.
3. Exclude `.git`, ignored files, every `build*` and `out` artifact/report root,
   editor state, logs, profiles, disassembly, generated manifests, and this
   planning/evidence documentation. The digest never consumes its own output.
4. Represent each entry as its repository-relative forward-slash path, Git
   mode/type, byte length, and SHA-256 of the exact working-tree bytes. Paths
   use ordinal UTF-8 ordering; timestamps, filesystem enumeration order, host
   separators, and locale are ignored. A missing tracked input is represented
   by an explicit deletion record.
5. Include relevant untracked inputs under the retained roots, so a new header
   or test cannot be tested against a manifest built before it existed.
6. For a submodule, record the gitlink path and expected commit, then recursively
   record its checked-out commit and dirty source-input digest. There are no
   current submodules inside this nested repository's source inventory.
7. Generated compilation inputs must be declared by a generator-input registry.
   Hash the generator, its source inputs, effective arguments, and tool identity;
   do not hash files emitted below an excluded build directory. There are no
   current generated C++ source inputs.
8. External dependencies outside the source tree are not recursively hashed.
   Record their immutable identity separately in the manifest and compilation
   fingerprint, currently Catch2 commit
   `2b60af89e23d28eefc081bc930831ee9d45ea58b` and the container image identity.
9. Store the Git revision and dirty/untracked summary as provenance separate
   from the content digest. Content equality, not commit-name equality, decides
   source compatibility for `Run-Tests.ps1 -SkipBuild`.

## Canonical compilation fingerprint

The canonical fingerprint document uses a versioned schema and JSON Canonical
Serialization (RFC 8785). Arrays whose order changes compiler semantics retain
order; sets and maps are normalized before serialization. UTF-8 bytes of that
document are hashed with SHA-256 and rendered as lowercase hexadecimal.

Required fields are:

- schema version;
- operating-system family and version boundary, architecture, compiler target
  triple, and ABI family;
- compiler frontend family, exact version, resolved executable identity, and
  MSVC toolset/runtime or GNU-like standard-library identity;
- CMake and generator family/version, because one build directory cannot be
  safely reused across incompatible generators;
- build configuration;
- sanitizer and coverage instrumentation as explicit ordered sets;
- whole-tree language-standard/extensions policy;
- ordered whole-tree compile and link options, definitions, runtime-library,
  exception/RTTI, stack-protection, standard-library, linker, and coverage
  policies after environment and preset resolution;
- every effective cache option that changes configuration contracts, target
  inventory, compilation, linking, or generated-code policy;
- immutable dependency identities and container image ID/base digest where
  applicable; and
- required runtime CPU-feature contract used by the built executables.

Target-local standards, ISA flags, FMA/BMI/scalar definitions, and test labels
remain target/test identity inside the tree and are not promoted into another
tree fingerprint. Source revision/digest, dirty state, test selection, report
format/path, CI provider, parallelism, image-layer cache policy, and later
benchmark execution are not fingerprint fields unless they alter an effective
compile/link value.

Artifact directories use a readable compiler/configuration key followed by the
first 16 hexadecimal characters (64 bits) of the fingerprint digest, for
example `clang22/debug-asan-ubsan-0123456789abcdef`. The manifest stores the
full 64-character digest and canonical document. Before reuse, the orchestrator
must compare both to the directory manifest. A missing manifest, incomplete
state, full-digest mismatch, or canonical-document mismatch is an error; a
short-prefix collision fails with both full digests and never reuses, deletes,
or silently extends the existing directory.

## Completion invariant

The frozen target and test files, scenario map, rename/consumer ledgers,
required fingerprint table, digest contracts, and measurements are the
pre-refactor comparison point. Later consolidation is incomplete if any frozen
responsibility lacks an explicit retained, renamed, replaced, or intentionally
removed owner, even if the resulting build is faster or its remaining tests
pass.
