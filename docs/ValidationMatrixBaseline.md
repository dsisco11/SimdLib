# Validation matrix deduplication baseline

This is execution reporting for the validation-matrix deduplication plan. It
records the pre-change pipeline shape and timing evidence; it is not enduring
documentation that the listed commands or results remain current.

## Evidence boundary

The structural inventory is derived from:

- the current `Build.ps1`, native/container runners, presets, CMake development
  modules, and external-consumer project;
- the latest completed per-cell manifests, generated
  `development-targets.txt` files, external-consumer inventories, and JUnit
  reports available when the audit began; and
- `UnifiedBuildPipelineBaseline.md`, which contains the controlled clean/warm
  measurements from the preceding pipeline refactor.

The current refresh was captured in native, container, and coverage segments
after the timing harness that launched the initial top-level command exited
before its children. An immediately following cached `Build -Scope All`
produced the pipeline's twelve-manifest receipt. Documentation files are
excluded from the host digest, so recording this report does not invalidate
the measured native artifacts.

## Current cell inventory

The pre-change default build owns twelve validation cells. Every main build
targets `ExhaustiveArtifacts`; benchmarks reuse applicable Release trees through
the separate `BenchmarkArtifacts` action.

| Cell | Driver and language surface | ISA surface | Configuration and instrumentation | Main targets | Main tests | Consumer | Register codegen |
| --- | --- | --- | --- | ---: | ---: | --- | --- |
| MSVC Release | MSVC-style; core C++20, Register C++23 | SSE4.2, AVX2, FMA, BMI | Release | 150 | 262 | core+Register, 2 tests | enforce |
| MSVC Debug | MSVC-style; core C++20, Register C++23 | SSE4.2, AVX2, FMA | Debug | 131 | 222 | core+Register, 2 tests | record |
| clang-cl Release | MSVC-style; core C++20, Register C++23 | SSE4.2, AVX2, FMA, BMI | Release | 150 | 265 | core+Register, 2 tests | enforce |
| clang-cl Debug | MSVC-style; core C++20, Register C++23 | SSE4.2, AVX2, FMA | Debug | 131 | 225 | core+Register, 2 tests | record |
| Native Clang coverage | GNU-like driver on Windows; core C++20, Register C++23 | SSE4.2, AVX2, FMA, BMI | Debug LLVM coverage | 94 | 262 | none | off |
| GCC 13 core Release | GNU; core C++20, Register unavailable | SSE4.2, AVX2, FMA, BMI | Release | 79 | 218 | core, 1 test | unavailable |
| GCC 13 core Debug | GNU; core C++20, Register unavailable | SSE4.2, AVX2, FMA | Debug | 62 | 178 | core, 1 test | unavailable |
| GCC 14 Release | GNU; core C++20, Register C++23 | SSE4.2, AVX2, FMA, BMI | Release | 149 | 265 | core+Register, 2 tests | enforce |
| GCC 14 Debug | GNU; core C++20, Register C++23 | SSE4.2, AVX2, FMA | Debug | 130 | 225 | core+Register, 2 tests | record |
| Clang 22 Release | GNU-like; core C++20, Register C++23 | SSE4.2, AVX2, FMA, BMI | Release | 149 | 265 | core+Register, 2 tests | enforce |
| Clang 22 Debug | GNU-like; core C++20, Register C++23 | SSE4.2, AVX2, FMA | Debug | 130 | 225 | core+Register, 2 tests | record |
| Clang 22 ASan+UBSan | GNU-like; core C++20, Register C++23 | SSE4.2, AVX2, FMA | Debug address+undefined | 130 | 225 | core+Register, 2 tests | record |

The logical union contains 153 development-target identities and 265 main
CTest identities. The external consumer adds `CoreConsumerSmoke` and, where
Register is supported, `RegisterConsumerSmoke`.

Benchmark compilation is an explicit supplemental action in the five Release
trees: MSVC, clang-cl, GCC 13 core, GCC 14, and Clang 22. Benchmark execution is
not part of `Run-Tests`.

## One-owner audit

The ownership rules in `ValidationMatrixOwnership.md` were mechanically applied
to the logical unions.

| Inventory | Union | Classified once | Unmatched | Multiple owners |
| --- | ---: | ---: | ---: | ---: |
| Development targets | 153 | 153 | 0 | 0 |
| Main CTest identities | 265 | 265 | 0 | 0 |
| External-consumer identities | 2 | 2 | 0 | 0 |

Logical target categories at baseline:

| Category | Targets |
| --- | ---: |
| Production/support aggregate | 4 |
| Repository audit | 1 |
| Compiler-front-end contract | 44 |
| Compile-time contract | 15 |
| Runtime correctness | 18 |
| Checks/preconditions | 3 |
| Smoke/ODR/example | 5 |
| Optimized or diagnostic codegen/ABI | 59 |
| Coverage | 2 |
| Benchmark | 2 |

CTest categories at baseline:

| Category | Tests |
| --- | ---: |
| Repository audit | 1 |
| Compiler-front-end contract | 3 |
| Compile-time contract | 1 |
| Runtime correctness | 232 |
| Checks/preconditions | 19 |
| Smoke/ODR/example | 5 |
| Optimized or diagnostic codegen/ABI | 4 |

## Debug and Release intersections

Every ordinary Debug target and test identity is also present in its compiler's
Release inventory. There is no Debug-only target or CTest identity.

| Compiler family | Release targets | Debug targets | Shared Debug targets | Debug-only targets | Release tests | Debug tests | Shared Debug tests | Debug-only tests |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| MSVC | 150 | 131 | 131 | 0 | 262 | 222 | 222 | 0 |
| clang-cl | 150 | 131 | 131 | 0 | 265 | 225 | 225 | 0 |
| GCC 13 core | 79 | 62 | 62 | 0 | 218 | 178 | 178 | 0 |
| GCC 14 | 149 | 130 | 130 | 0 | 265 | 225 | 225 | 0 |
| Clang 22 | 149 | 130 | 130 | 0 | 265 | 225 | 225 | 0 |

The Clang 22 ordinary Debug and ASan+UBSan cells have identical 130-target and
225-test identity sets. Instrumentation, not inventory, is their only current
distinction.

Release-only work consists of BMI feature variants, constexpr probes, and the
benchmark target. Configuration flags and generated-code enforcement still
make Release and Debug incompatible object fingerprints even though Debug owns
no unique logical identity.

## Timing evidence

### Controlled clean and warm baseline

`UnifiedBuildPipelineBaseline.md` owns the controlled measurement procedure:
isolated trees were removed before clean measurements, warm measurements were
immediate reruns, consumer actions were counted separately, and container
operations included orchestration, main build/test, and consumer work.

The measurements most relevant to the new deduplication work were:

| Historical scenario | Clean main build (s) | Warm main build (s) | Clean operation (s) | Warm operation (s) |
| --- | ---: | ---: | ---: | ---: |
| MSVC Release | 123.788 | 1.430 | 138.478 | 26.487 |
| MSVC Debug | 76.391 | 5.866 | 96.415 | 30.759 |
| clang-cl Release | 38.505 | 0.270 | 59.078 | 16.692 |
| clang-cl Debug | 37.259 | 0.280 | 59.125 | 18.185 |
| Native Clang coverage | 19.459 | 0.291 | 44.032 | 22.065 |
| GCC Debug diagnostic | 234.505 | 2.600 | 267.827 | 31.968 |
| Clang Debug diagnostic | 210.019 | 2.257 | 269.837 | 38.047 |
| GCC benchmark | 33.540 | 1.692 | 87.082 | 33.747 |
| Clang benchmark | 37.420 | 2.045 | 96.608 | 38.622 |
| Clang ASan+UBSan | 365.638 | 2.251 | 412.616 | 45.269 |

Those historical scenarios predate the unified preset layout, so they are used
as controlled clean/warm compiler and orchestration evidence rather than as
current target-count claims.

The most recent controlled unified-pipeline baseline before this audit measured
937.904 seconds for default validation, followed by a separately measured
13.505-second benchmark-build operation, for 951.408 seconds total. Its
unchanged warm rerun took 102.191 seconds and emitted no compiler-output lines.
Those measurements predate the newest source changes but use the same unified
orchestration model and preserve the clean-versus-cached comparison without
destroying the current build trees.

### Current generated-tree critical paths

Ninja logs retain start/end milliseconds for compiler and custom-command edges.
The maximum completion time in the audited trees was:

| Current tree | Main-build completion (s) |
| --- | ---: |
| GCC 13 Release | 83.939 |
| GCC 13 Debug | 158.406 |
| GCC 14 Release | 75.639 |
| GCC 14 Debug | 256.787 |
| Clang 22 Release | 176.581 |
| Clang 22 Debug | 256.742 |
| Clang 22 ASan+UBSan | 342.745 |
| clang-cl Release | 87.103 |
| clang-cl Debug | 123.656 |
| Native Clang coverage | 71.658 |

These logs are build-edge timelines, not additive CPU totals. Parallel cells
and parallel edges must not be summed to predict unified wall time.

### Critical outputs

The longest Debug and sanitizer edges demonstrate why target ownership matters:

| Tree | Critical output | Edge duration (s) |
| --- | --- | ---: |
| GCC 14 Debug | AVX2/256 Register type-matrix common comparison record | 211.42 |
| Clang 22 Debug | AVX2/256 Register type-matrix common comparison record | 102.98 |
| Clang 22 ASan+UBSan | AVX2/256 Register type-matrix common comparison record | 273.95 |
| clang-cl Debug | AVX2/256 Register type-matrix comparison record | 120.53 |

In the sanitizer tree, Register codegen completed at approximately 342.74
seconds while the last obvious non-codegen runtime target completed at
approximately 102.39 seconds. This is direct critical-path evidence for removing
record-only codegen from the default sanitizer build.

GCC Debug builds also expose expensive Catch2 post-build discovery. Multiple
test-list generation edges took approximately 128–160 seconds in the GCC 14
Debug tree and approximately 62–71 seconds in the GCC 13 Debug tree. Moving
discovery to test time would change command attribution but is not a complete
pipeline saving unless `Build` plus `Run-Tests` improves.

The available MSBuild text logs do not contain per-target elapsed timing.
Controlled `Measure-Command` scenario measurements, compiler-output counts, and
the target completion order are therefore supplemented by the controlled MSVC
`/Bt+` compiler-stage profile. Its costliest production-owned outputs were:

| MSVC target and output source | Compiler-stage time (s) |
| --- | ---: |
| `VectorAlgorithmsTests` — `SimdVector.tests.cpp` | 14.822 |
| `RegisterAvx2Tests` — `Register.tests.cpp` | 10.247 |
| `RegisterAvx2Tests` — `RegisterBasicOperations.tests.cpp` | 9.328 |
| `RegisterSse42Tests` — `Register.tests.cpp` | 4.954 |
| `RegisterAvx2Tests` — `RegisterSpecializedOperations.tests.cpp` | 4.805 |

At the target level, `RegisterAvx2Tests` accumulated 27.903 compiler-job
seconds and `VectorAlgorithmsTests` accumulated 20.251 seconds. These are
measured compiler stages and identify the expensive native output families;
they are not inferred from object size or target count. They do not reconstruct
MSBuild's exact parallel scheduler path. Future before/after qualification
should also enable an MSBuild performance summary or binary log so scheduler
critical-path attribution matches Ninja's strength.

## Compiler work versus CTest work

The twelve refreshed main JUnit reports contain 2,837 test executions and
approximately 99 seconds of summed per-cell CTest wall time. The four ordinary
Debug cells proposed for removal—clang-cl, GCC 13, GCC 14, and Clang 22—account
for 853 executions but only about 31 seconds of that sum.

The build-edge evidence is much larger: the same ordinary Debug trees complete
at approximately 123.7, 158.4, 256.8, and 256.7 seconds respectively before
consumer and orchestration costs. The primary opportunity is repeated
compilation, discovery, and disassembly rather than test-body execution.

## Current requirement conflicts and disposition

| Source | Current requirement | Accepted disposition |
| --- | --- | --- |
| `project.todo` | Prove optimal Debug codegen through unoptimized SimdLib code and optimized comparison code | Replace with mandatory optimized Release qualification plus explicit Debug diagnostic recording |
| `RegisterImplementation.todo` | Run Debug and sanitizer wrapper/raw differential checks | Preserve the capability and historical evidence; move future records to explicit diagnostic operations |
| `RegisterProposal.md` | Debug and sanitizer correctness plus wrapper/raw differentials | Keep correctness in the default assigned cells; make differentials optional diagnostics |
| `RegisterQualification.md` | Debug on every supported compiler and Clang sanitizer, with recorded disassembly differences | Retain as the current/historical qualification description until migration; `ValidationMatrixOwnership.md` defines the accepted future owner |
| `RegisterImplementationMatrix.md` | Core support listed under Debug and Release | Continue supporting downstream Debug compilation; stop interpreting support as a requirement for a full default Debug suite on every compiler |
| `Validation.md`, `BuildPipeline.md`, `ContainerValidation.md`, `UnifiedBuildPipelineCMakeProfiles.md` | Document the current twelve-cell pipeline | Keep accurate until implementation changes; update during final documentation migration |

No performance or correctness guarantee is removed. The conflict is resolved by
separating “supported diagnostic capability” from “mandatory default build
artifact.”

## Current-revision refresh

The audited implementation revision is
`16870b7dae18614dc0c95382f016e1c5d85901a3`. The host source digest recorded by
the unified receipt and all five native manifests is
`4e3a0404da9863beee72101f585c35e8722fe6600874ace177575090d66bc0d7`.

| Operation | Wall time (s) | Result and boundary |
| --- | ---: | --- |
| Native current-revision refresh | 194.9 | MSVC and clang-cl Release/Debug children completed after the initial timing harness exited; coverage had not started |
| Container current-revision refresh | 718.657 | Seven Linux build-validation cells, including image orchestration and the sanitizer critical path |
| Native coverage refresh | 40.431 | Configure and build only; coverage execution remained owned by `Run-Tests` |
| Cached `Build -Scope All` | 142.181 | All twelve cells and unified receipt `build-58ea88d008095ac6.json`; no source translation unit required recompilation |
| `Run-Tests -Scope All -Compiler All -SkipBuild` | 86.897 | 2,837 main and 20 consumer executions; coverage reset, execution, merge, and report; no build command |

The cached build was not a receipt-only operation. It reconfigured every tree,
reran configure-time compile-failure probes, regenerated dependency metadata,
rescanned MSBuild targets, and checked the container images. Representative
cached stage evidence was:

| Cell | Configure and generate (s) | Main-build boundary (s) | Consumer configure (s) | Consumer-build boundary (s) |
| --- | ---: | ---: | ---: | ---: |
| MSVC Release | 36.0 | 3.568 | 0.2 reported, 0.875 wall boundary | 0.524 |
| clang-cl Release | 23.6 | 2.831 | 0.0 reported, 0.142 wall boundary | 0.090 |
| GCC 14 Release | 40.5 | 2.358 | 0.330 wall boundary | 0.084 |
| Clang 22 ASan+UBSan | 40.9 | incremental build recorded separately in its stage log | 0.2 reported | incremental build recorded separately in its stage log |

The coverage JUnit report completed at `14:46:17.112`; `coverage.info` and the
coverage report completed at `14:46:30.620`, so refreshed profile merge and
report processing occupied approximately 13.509 seconds after CTest. Test
discovery and codegen comparison costs remain represented by the Ninja critical
edges above rather than being folded into CTest time.

### Receipt source-digest inconsistency

All twelve manifest file hashes match the unified receipt. The seven container
manifests nevertheless embed source digest
`94a1806ee91d2b24138804f9f31b1c1fd3f4d856b7005268abb8e9442fe31787`,
which differs from the host receipt and native-manifest digest.

The cause is deterministic: the host hashes a byte stream containing each
relative path, a NUL byte, the file-content hash, and a newline. The container
implementation appends the complete `sha256sum` output, which also contains the
absolute container path. Each side validates only its own algorithm, while
`Write-BuildReceipt` records the manifest file hash without comparing the
manifest's embedded source digest to the receipt digest. The current
`Run-Tests` command therefore accepts an internally hashed but cross-layer
inconsistent receipt. The orchestration work now explicitly requires one
canonical relative-path byte stream and cross-layer digest validation.

These values are execution evidence, not enduring claims that the commands
remain green or retain the same timing after implementation changes.
