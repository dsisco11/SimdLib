# Unified build pipeline CMake profile evidence

This report records the initial modular CMake implementation and its 2026-07-25 execution
evidence. It is an execution record, not a claim about later revisions.

## Production and development boundary

The root `CMakeLists.txt` is 44 lines and always defines only the production
interface targets `SimdLib`, `SimdLib::SimdLib`, `SimdLibRegister`, and
`SimdLib::Register`. When `PROJECT_IS_TOP_LEVEL` is true, it loads the sole
development entrypoint, `cmake/development/Development.cmake`.

The pre-refactor root contained 1,407 lines. The extracted development modules
contain 1,709 lines including their guards, prerequisite diagnostics, scoped
state, and helper documentation:

| Module | Lines | Responsibility |
| --- | ---: | --- |
| `Development.cmake` | 39 | ordered composition and repeat-inclusion proof |
| `Options.cmake` | 80 | top-level options, retired-option diagnostics, CTest ownership |
| `Dependencies.cmake` | 28 | development-only Catch2 discovery |
| `TargetConfiguration.cmake` | 89 | warning, ISA, and coverage target policies |
| `SourceAudits.cmake` | 41 | public-consumer and static-assert source audits |
| `ConfigurationProbes.cmake` | 201 | positive and expected-failure configuration contracts |
| `ConstexprProbes.cmake` | 101 | compile-only constexpr matrix |
| `HeaderProbes.cmake` | 54 | first-and-only public-header probes |
| `RegisterCodegen.cmake` | 499 | one cohesive Register codegen and ABI target family |
| `SmokeTests.cmake` | 35 | ODR smoke executables |
| `RuntimeTests.cmake` | 307 | Catch2 runtime and feature-variant executables |
| `Examples.cmake` | 36 | executable examples |
| `Benchmarks.cmake` | 32 | benchmark executable |
| `Coverage.cmake` | 71 | LLVM coverage reset and report targets |
| `ArtifactAggregates.cmake` | 96 | build aggregates and target manifests |

Every development module has `include_guard(GLOBAL)` and an explicit top-level
or production-target prerequisite. Temporary module variables are contained in
`block(SCOPE_FOR VARIABLES)`; `TargetConfiguration.cmake` instead contains its
temporary state inside documented functions. `Dependencies.cmake` explicitly
exports only Catch2's required `CMAKE_MODULE_PATH` update. The coordinator
verifies every module path, includes modules in one documented order, and
includes itself again to prove repeat inclusion is inert.

The external consumer configures SimdLib through `add_subdirectory` and fails
if that operation creates `BUILD_TESTING`, a SimdLib development cache option,
a Catch2/development target, or a nested SimdLib test. Its own CTest inventory
contains only `CoreConsumerSmoke` and `RegisterConsumerSmoke` on supported
Register compilers.

## Compilation fingerprints

| Fingerprint | Configure preset | Aggregate build preset |
| --- | --- | --- |
| MSVC Release | `msvc-release-exhaustive` | `msvc-release-exhaustive` |
| MSVC Debug | `msvc-debug-diagnostics` | `msvc-debug-diagnostics` |
| clang-cl Release | `clangcl-release-exhaustive` | `clangcl-release-exhaustive` |
| clang-cl Debug | `clangcl-debug-diagnostics` | `clangcl-debug-diagnostics` |
| Linux GCC 13.2 core Release | `gcc13-core-release-exhaustive` | same name |
| Linux GCC 13.2 core Debug | `gcc13-core-debug-diagnostics` | same name |
| Linux GCC 14 Release | `gcc14-release-exhaustive` | same name |
| Linux GCC 14 Debug | `gcc14-debug-diagnostics` | same name |
| Linux Clang 22 Release | `clang22-release-exhaustive` | same name |
| Linux Clang 22 Debug | `clang22-debug-diagnostics` | same name |
| Linux Clang 22 Debug ASan+UBSan | `clang22-debug-asan-ubsan` | same name |
| Clang Debug coverage | `clang-debug-coverage` | same name |
| Selected Debug codegen diagnostic | compiler-specific `*-debug-codegen-diagnostic` | same name |
| Selected Clang sanitizer codegen diagnostic | `clang22-asan-ubsan-codegen-diagnostic` | same name |

Hidden presets own common development controls, exhaustive Release controls,
ordinary Debug controls, optional codegen-diagnostic controls, sanitizer flags,
coverage controls, compiler-driver selection, and container defaults. Every
visible configure preset has its own stable binary directory. MSVC Release and
ordinary Debug additionally restrict `CMAKE_CONFIGURATION_TYPES` to `Release`
and `Debug`, respectively.

Release exhaustive caches use strict warnings, BMI variants, examples,
benchmarks, `SIMDLIB_REGISTER_CODEGEN_MODE=ENFORCE`, and configure-time target
inventory validation. Ordinary Debug, sanitizer, and coverage caches set
`SIMDLIB_REGISTER_CODEGEN_MODE=OFF`; they contain no Register codegen targets.
Explicit diagnostic caches use `SIMDLIB_REGISTER_CODEGEN_MODE=RECORD`, retain
`/Od` or the GNU-like Debug flags, and build only the selected record-only
fixtures. The sanitizer cache adds `-fsanitize=address,undefined` and
`-fno-omit-frame-pointer` without inheriting Release optimization or
enforcement.

## Aggregate ownership

`ExhaustiveArtifacts` depends only on the scoped category aggregates selected
by `SIMDLIB_VALIDATION_PROFILE`. Release includes its compiler, constexpr,
runtime, checks, smoke, and optimized-codegen owners. Ordinary Debug,
sanitizer, and coverage select narrower owners and cannot absorb Register
generated-code targets through inherited development options.

`BenchmarkArtifacts` depends only on `Benchmarks`. Neither aggregate depends on
the other. Release benchmark presets reuse the Release configure tree, so the
benchmark operation compiles only benchmark sources and required dependency
objects that are not already present.

Configure-time and expected-failure probes remain configuration contracts and
are recorded separately because they cannot be build dependencies. External
consumer targets likewise remain in their own project and are listed in
`external-consumer-targets.txt`.

The generated `development-targets.txt` is the canonical per-fingerprint target
inventory and excludes CTest dashboard utilities. Its codegen portion contains
one common specialized wrapper/raw pair per profile, isolated enabled/disabled
FMA pairs, canonical type-matrix pairs, separate common non-modulus and
integer-modulus comparison records, rearrangement pairs, primary pairs, ABI
pairs, and build-only expression and consumer-ABI aggregates. Retired
logical-shuffle intrinsic targets and specialized-matrix-per-FMA duplicates do
not appear.

The CTest inventory contains one `RegisterCodegen.<profile>` validation for each
of SSE4.2/128, AVX2/128, and AVX2/256. The expression and consumer-ABI aggregate
targets do not create CTests, so each generated comparison record has one
validation owner. The frozen unions in `UnifiedBuildPipelineExpectedTargets.txt`
and `UnifiedBuildPipelineExpectedTests.txt` remain evidence of the pre-refactor
baseline identified by `UnifiedBuildPipelineBaseline.md`; they are not current
target manifests.

`RegisterCodegenSymbolAudit.csv` is the canonical per-symbol ownership ledger;
`RegisterCodegenAudit.md` inventories the corresponding CMake targets, record
inputs, validation owners, CI publication roots, and enduring documentation.

## Execution evidence

The following configure and aggregate operations completed with the final
module layout:

- native MSVC Release and Debug;
- native clang-cl Release and Debug;
- native Clang Debug coverage;
- container GCC 13.2 core-only Release and Debug;
- container GCC 14 Release and Debug;
- container Clang 22 Release and Debug;
- container Clang 22 Debug ASan+UBSan; and
- separate MSVC, clang-cl, GCC 13.2, GCC 14, and Clang 22 benchmark aggregates.

The three container Release aggregates were rerun together with
`Run-ContainerMatrix.ps1 -Action Build -SkipImageBuild`; the later
`-Action Test` operation consumed those artifacts without rebuilding them.
These operations also exercised the standalone consumer projects. No native
CTest suite was executed while validating the native aggregate targets.

Additional structural checks covered CMake preset parsing, Compose rendering,
POSIX shell syntax, PowerShell parsing, JSON parsing, the retired-option
expected failure, downstream CTest isolation, exact profile cache values, and
the target/CTest inventory reconciliation above.
