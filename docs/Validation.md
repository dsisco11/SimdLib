# Validation evidence

This document records execution evidence for the validation-matrix ownership
refactor completed on 2026-07-29. Command semantics and prerequisites belong in
[Unified build and validation](BuildPipeline.md); the measurements and outcomes
below describe this execution only and are not timeless performance promises.

## Executed commands

The final acceptance run used the repository interfaces:

```powershell
tools/Build.ps1 -Scope All
tools/Build.ps1 -Scope All
tools/Run-Tests.ps1 -Scope All
tools/Run-NativeMatrix.ps1 -Action BuildCompilerContracts -Compiler All -Cell Release
tools/Run-ContainerMatrix.ps1 -Action BuildCompilerContracts -Compiler All -Cell Release
tools/Run-NativeMatrix.ps1 -Action TestCompilerContracts -Compiler All -Cell Release
tools/Run-ContainerMatrix.ps1 -Action TestCompilerContracts -Compiler All -Cell Release
tools/Build-Benchmarks.ps1 -Scope All
tools/Run-Benchmarks.ps1 -Scope All
tools/Record-Codegen.ps1 -Scope Native -Compiler Msvc -Cell Debug
```

The first `Build` followed removal of only `out/pipeline`; the second was an
immediate cached run. `Run-Tests` consumed the second build's exact completed
receipt. Compiler-contract, benchmark, and diagnostic operations remained
supplemental and did not become default-receipt requirements.

## Default ownership inventory

The final receipt references exactly eight default cells:

| Cell | Profile | Configured targets | Selected targets | Main tests |
| --- | --- | ---: | ---: | ---: |
| MSVC Release | Release | 149 | 148 | 269 |
| MSVC Debug | Debug | 19 | 19 | 216 |
| clang-cl Release | Release | 149 | 148 | 272 |
| Native Clang coverage | Coverage | 23 | 21 | 258 |
| GCC 13 core Release | Release | 76 | 75 | 225 |
| GCC 14 Release | Release | 148 | 147 | 272 |
| Clang 22 Release | Release | 148 | 147 | 272 |
| Clang 22 ASan+UBSan | Sanitizer | 22 | 22 | 258 |
| **Total** |  | **734** | **727** | **2,042** |

The five applicable Release cells also ran nine external-consumer tests:
core plus Register on MSVC, clang-cl, GCC 14, and Clang 22, and core-only on
GCC 13. The repository audit ran once for source digest
`7c3ffe3c2f67bdb2caec2bd777c01378d0add0b6f15fd2090ba8aedc069b5a79`
and was hash-bound into the unified receipt.

## Controlled timing comparison

The baseline used the same unified orchestration boundary before ownership
deduplication: twelve default cells, 1,485 configured targets, 2,837 main tests,
a 937.904-second clean build, a 102.191-second immediate cached build, and an
86.897-second build-free test run.

| Measurement | Baseline | Final | Change |
| --- | ---: | ---: | ---: |
| Default cells | 12 | 8 | -4 (-33.3%) |
| Configured targets | 1,485 | 734 | -751 (-50.6%) |
| Main tests | 2,837 | 2,042 | -795 (-28.0%) |
| Clean `Build` wall time | 937.904 s | 451.745 s | -486.159 s (-51.8%) |
| Cached `Build` wall time | 102.191 s | 74.095 s | -28.096 s (-27.5%) |
| Build-free `Run-Tests` wall time | 86.897 s | 62.026 s | -24.871 s (-28.6%) |
| Clean build plus tests | 1,024.801 s | 513.771 s | -511.030 s (-49.9%) |

The clean run rebuilt every retained tree after its generated root was removed.
All eight inventory audits were complete, every expected test remained
registered, all compiler/container operations completed, and the source digest
matched the receipt. The reduction therefore does not depend on a warm cache,
a missing manifest, a skipped compiler service, or a failed operation.

### Configure, build, discovery, and consumer boundaries

The top-level clean time includes image validation, configure, compile/link,
Catch2 `POST_BUILD` discovery, external-consumer work, inventory auditing, and
receipt creation. Preserved file-creation boundaries provide the following
per-cell attribution. These cells ran concurrently, so the rows and columns
must not be added to predict top-level wall time.

| Cell | Configure boundary | Build + discovery boundary | Consumer configure | Consumer build + audit | Cell boundary |
| --- | ---: | ---: | ---: | ---: | ---: |
| MSVC Release | 98.5 s | 186.2 s | 4.0 s | 18.6 s | 307.3 s |
| MSVC Debug | 9.3 s | 104.9 s | — | — | 114.2 s |
| clang-cl Release | 54.6 s | 63.1 s | 11.6 s | 11.4 s | 140.6 s |
| Native Clang coverage | 10.3 s | 34.1 s | — | — | 44.3 s |
| GCC 13 Release | 34.1 s | 88.1 s | 4.9 s | 14.3 s | 141.4 s |
| GCC 14 Release | 135.0 s | 204.9 s | 7.0 s | 52.0 s | 398.9 s |
| Clang 22 Release | 272.1 s | 146.0 s | 1.2 s | 10.8 s | 430.1 s |
| Clang 22 ASan+UBSan | 27.1 s | 263.6 s | — | — | 290.7 s |

Container orchestration occupied approximately 448 seconds of the 451.745-second
critical path. External-consumer configure/build/audit boundaries totalled
135.8 seconds across five concurrently scheduled owners. The eight main JUnit
reports recorded 73 seconds of summed per-cell CTest wall time; the nine
consumer tests completed below the reports' one-second precision.

Catch2 discovery remains part of the build because `POST_BUILD` output is needed
for the receipt inventory. Ninja recorded 18 to 21 logical discovery commands
per applicable runtime tree, represented by paired relative/absolute log
outputs. The longest discovery edge was 74.21 seconds on GCC 14 Release and
27.26 seconds on GCC 13 Release; the other Ninja cells' longest discovery edges
ranged from 1.79 to 3.40 seconds. Host CTest cannot rediscover container trees
directly because their generated include paths intentionally use
`/workspace/out`; container-side inventory audits verified those trees.

## Compiler work and critical outputs

Before test execution, the clean default build contained 1,682 object outputs
totalling 424,209,873 bytes:

| Cell | Object outputs | Size |
| --- | ---: | ---: |
| MSVC Release | 266 | 54.5 MiB |
| MSVC Debug | 143 | 121.4 MiB |
| clang-cl Release | 266 | 19.2 MiB |
| Native Clang coverage | 144 | 105.7 MiB |
| GCC 13 Release | 189 | 7.1 MiB |
| GCC 14 Release | 263 | 11.9 MiB |
| Clang 22 Release | 265 | 11.1 MiB |
| Clang 22 ASan+UBSan | 146 | 73.7 MiB |

Ninja's longest non-benchmark edges identify the retained critical outputs:

| Cell | Critical output | Edge time |
| --- | --- | ---: |
| clang-cl Release | `RegisterAvx2Tests` / `Register.tests.cpp` | 24.48 s |
| Native Clang coverage | `RegisterAvx2Tests` / `Register.tests.cpp` | 15.93 s |
| GCC 13 Release | `ApiAvx2Tests` / `Api256.tests.cpp` | 48.91 s |
| GCC 14 Release | `RegisterAvx2Tests` / `Register.tests.cpp` | 105.93 s |
| Clang 22 Release | `RegisterAvx2Tests` / `Register.tests.cpp` | 62.28 s |
| Clang 22 ASan+UBSan | Catch2 debug archive | 107.50 s |

MSBuild's text log does not expose a comparable scheduler critical path.
Target/object counts and the controlled cell boundary are reported for MSVC
instead of inferring one.

## No-rebuild and supplemental evidence

The immediate cached build emitted no translation-unit compilation and every
Ninja owner reported no work. Before `Run-Tests`, hashes, sizes, and timestamps
were recorded for all 1,682 default objects. Afterwards all 1,682 were
unchanged, none were missing, and the test log contained no build invocation.
Ten new tiny objects were expected: the five Release owners each compile a raw
and wrapper object for the `CodegenPolicy.RejectRecordAsEnforced` negative
fixture. Those test-owned objects are not rebuilt project targets.

The focused compiler-contract workflow retained one owner per compiler
identity: 224 configured and selected targets across five cells, with nine
tests per cell. All five contract inventories completed.

Benchmark compilation reused the five matching Release trees and stayed outside
the default build. The native and container benchmark executions completed from
their benchmark manifests.

The selected MSVC Debug codegen diagnostic used its independent
`debug-codegen-5240ba90331fe415` fingerprint. Its provenance records
`codegenMode=RECORD`, MSVC `/GS`, 35 indexed records, and 12.289 seconds of
measured compile/comparison work. The records remain below
`out/pipeline/windows-msvc/debug-codegen-5240ba90331fe415` and cannot satisfy
the mandatory optimized Release gate.

Coverage generated `coverage.info` and `coverage-provenance.tsv` from 256
profiles mapped to 21 executable identities. The Clang sanitizer cell completed
its 258-test runtime/checks inventory without sanitizer diagnostics. Release
cells retained optimized generated-code enforcement, examples, smoke/ODR,
constexpr, compiler-facing, and external-consumer ownership.

These values are execution evidence for revision
`8caa6d2efd582f23d70c989b30122ac391cdac1f`; they do not assert that future
revisions retain the same timing or outcome.
