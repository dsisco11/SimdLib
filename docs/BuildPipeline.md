# Unified build and validation

SimdLib has one repository-owned build command and one correctness-validation
command. A complete local build is:

```powershell
tools/Build.ps1 -Scope All
```

This builds MSVC Release and the representative MSVC Debug cell, the
caller-selected Windows clang-cl Release, native Clang Debug coverage, GCC 13
core-only Release, GCC 14 Release,
and Clang 22 Release plus the representative ASan+UBSan Debug cell. It builds
the correctness, ABI, sanitizer, consumer, coverage, probe, example, and
header-validation artifacts, plus the mandatory optimized generated-code gates
in Release. The default matrix does not build ordinary clang-cl, GCC 13,
GCC 14, or Clang 22 Debug cells. Debug, sanitizer, and coverage cells do not
compile Register generated-code fixtures. The command does not compile
benchmark targets or run any executable.

GitHub Actions runs the Windows clang-cl Release ownership twice: the explicit
LLVM 20.1.8 compatibility floor inside a dedicated Visual Studio Build Tools
container, and LLVM 22.1.7 on the native Windows runner. The native job obtains
LLVM and CMake 4.4.0 through Chocolatey. Clang coverage belongs only to the LLVM
22.1.7 job. The LLVM 20 cell is not a claim about Visual Studio's default or
optional bundled Clang version.

Before starting compiler cells, `Build.ps1` performs two focused validations.
`tools/Validate-PipelineTooling.ps1` validates matrix topology, pipeline
regressions, configured ownership rules, and no-rebuild behavior once for the
reviewed tooling/configuration digest. It writes
`out/pipeline/provenance/pipeline-validation-<digest>.json`, and the unified
receipt binds the result path, hash, status, schema, and tooling digest.
Ordinary production-source changes do not rerun these synthetic tooling tests.
`tools/Test-PublicConsumerBoundary.ps1` separately checks every public example
and consumer fixture before compiler-cell execution and rejects use of
`SimdLib::Detail`; it is a source-boundary check, not a general repository
audit.

The corresponding complete validation command is:

```powershell
tools/Run-Tests.ps1 -Scope All
```

`Run-Tests.ps1` requires the matching receipt from a prior `Build.ps1`
invocation, validates its exact manifest and source ownership, and then starts
test-only operations. It rejects missing, stale, incomplete, or mismatched
evidence without configuring or building. The coverage cell
resets profiles, runs its instrumented tests, and generates `coverage.info`
plus `coverage-provenance.tsv`. The provenance file records the executable
identity and profile count used for every independently merged coverage target.
Benchmark compilation and execution remain separate:

```powershell
tools/Build-Benchmarks.ps1 -Scope All
tools/Run-Benchmarks.ps1 -Scope All
```

## Prerequisites and explicit scope

The complete `All` scope requires a Windows x64 host with:

- Visual Studio 2022 and the MSVC x64 C++ tools;
- LLVM 20 or newer with `clang-cl`, `clang++`, `llvm-profdata`, `llvm-cov`, and
  `llvm-readobj` available on `PATH`; the GitHub workflow qualifies LLVM 20.1.8
  as its explicit Windows clang-cl compatibility floor in a dedicated container
  and installs LLVM 22.1.7 and CMake 4.4.0 through Chocolatey for the newer
  native compiler and coverage cells;
- CMake 3.31 or newer; and
- Docker Desktop with a running Linux-container daemon.

`Build.ps1` deliberately has no implicit scope. Calling it without `-Scope`
fails, because silently falling back to only the current platform would make
an incomplete build look complete. Hosts that own only Linux container
validation use:

```powershell
tools/Build.ps1 -Scope Containers
tools/Run-Tests.ps1 -Scope Containers
```

Focused development and CI ownership use compiler filters:

```powershell
tools/Build.ps1 -Scope Native -Compiler Msvc
tools/Run-Tests.ps1 -Scope Native -Compiler ClangCl
tools/Run-Tests.ps1 -Scope Containers -Compiler Gcc14,Clang22
```

Native filters are `Msvc`, `ClangCl`, and `ClangCoverage`. Container filters
are `Gcc13`, `Gcc14`, and `Clang22`. A filter from the wrong scope is an error.
Compiler filters retain the default ownership policy: for example, selecting
`ClangCl` builds clang-cl Release, while selecting `Clang22` builds Clang 22
Release and ASan+UBSan Debug.

Retired ordinary Debug cells remain directly available for troubleshooting but
do not produce manifests accepted by the unified default receipt:

```powershell
tools/Run-NativeMatrix.ps1 -Action Build -Compiler ClangCl -Cell Debug
tools/Run-ContainerMatrix.ps1 -Action Build -Compiler Gcc13 -Cell Debug
tools/Run-ContainerMatrix.ps1 -Action Build -Compiler Gcc14 -Cell Debug
tools/Run-ContainerMatrix.ps1 -Action Build -Compiler Clang22 -Cell Debug
```

## Artifact reuse and manifests

Every compiler/configuration cell has an independent directory:

```text
out/pipeline/<platform>-<compiler>/<configuration>-<fingerprint>/
  build/
  consumer/
  reports/
  provenance/
```

The readable prefix is followed by the first 16 hexadecimal characters of a
SHA-256 over the canonical compilation fingerprint. The complete fingerprint
is retained in `fingerprint.json`; a short-name collision with different
canonical data is rejected. Compiler identity, generator, configuration,
instrumentation, language policy, dependencies, and required CPU features
participate in the fingerprint. Source inputs do not: their separate digest is
bound into each completed build manifest so editing a source file invalidates
test-only reuse without creating a new toolchain directory.

## Scoped CMake artifact graph

### Validation ownership policy

Every validation artifact has one logical category and the narrowest compiler,
configuration, and instrumentation scope that proves its contract. Pipeline
tooling validation is keyed by its reviewed configuration inputs, while
compiler-front-end and compile-time
contracts belong to applicable Release compiler identities; runtime and
checks/precondition contracts additionally run in the representative MSVC
Debug and Clang ASan+UBSan cells; public examples, smoke, ODR, external
consumer, and optimized generated-code contracts belong to applicable Release
cells. Coverage and sanitizer describe how runtime contracts are compiled and
executed rather than creating duplicate logical owners.

MSVC Debug is the sole ordinary Debug cell in the default matrix because it
owns the distinct unoptimized Windows and default-check configuration
contract. The clang-cl, GCC 13, GCC 14, and Clang ordinary Debug cells remain
available only for focused troubleshooting: their compiler, language, ABI,
runtime, consumer, and optimizer contracts are already owned by their Release
cells, while the Clang ASan+UBSan cell owns instrumented Linux Debug behavior.

`tools/validation-matrix.json` is the single machine-readable authority for
cell, operation order, profile, category, test-owner, consumer,
instrumentation, and generated-code policy. Native and container runners
resolve their cells from this file, and CMake reads its profile category
definitions directly. A new
compiler, configuration, instrumentation mode, target, or test may join the
default matrix only when it proves a stated contract that no existing owner
proves. New development targets must declare one scoped category; generated
inventory audits reject missing ownership, duplicate ownership, and profile
membership outside the matrix contract.

Every top-level development target declares exactly one validation category
when it is created. Configuration fails if a project-owned target is unowned,
is assigned more than once, or belongs to a category forbidden by the selected
`SIMDLIB_VALIDATION_PROFILE`. The supported profiles are `RELEASE`, `DEBUG`,
`SANITIZER`, `COVERAGE`, `COMPILER_CONTRACTS`, `CODEGEN_DIAGNOSTIC`, and
`CUSTOM` for explicitly configured local development trees.

Compiler-tree category targets are exposed through globally unique aggregates:

- `SimdLibCompilerContractArtifacts`;
- `SimdLibConstexprContractArtifacts`;
- `SimdLibRuntimeValidationArtifacts`;
- `SimdLibChecksValidationArtifacts`;
- `SimdLibSmokeValidationArtifacts`;
- `SimdLibOptimizedCodegenArtifacts`;
- `SimdLibDebugDiagnosticArtifacts`; and
- `SimdLibCoverageSupportArtifacts`.

`ExhaustiveArtifacts` is the profile umbrella used by the pipeline. It depends
only on the category aggregates selected by its configured profile. Sanitizer
and coverage trees use `SimdLibSanitizerValidationArtifacts` and
`SimdLibCoverageValidationArtifacts`, respectively, so inherited development
options cannot pull compiler probes, constexpr probes, or generated-code work
into those builds. `BenchmarkArtifacts` remains a separate Release-only
aggregate and is never a dependency of `ExhaustiveArtifacts`.

External consumers remain separate CMake projects because a main-tree marker
target could not truthfully represent their configure and build operations.
Their applicable targets are recorded in `external-consumer-targets.txt` for
the pipeline orchestrator.
Each supported compiler's Release cell configures, builds, and tests that
project once. Ordinary Debug, sanitizer, coverage, and diagnostic cells record
`consumer_scope=none` and contain no consumer tree. The build manifest binds
the owning scope and consumer test-artifact inventory, so `Run-Tests` cannot
substitute a consumer-free cell for Release evidence.

`ApiExamples` is the executable C++20 public-API usage contract, and
`RegisterExamples` is its C++23 Register counterpart. `HeaderOnlySmoke` proves
multi-translation-unit umbrella-header linkage, `FormatOdr` proves formatter
specializations link across translation units, and `RegisterOdr` proves the
same multi-translation-unit contract for Register and RegisterMask. Applicable
Release cells own these compiler-facing public-surface contracts; GCC 13 owns
only the core variants because its supported surface is core-only.

Each configured tree writes deterministic audit inputs:

- `development-targets.txt` lists configured project targets and aggregates;
- `development-profile-targets.txt` lists targets selected by the profile;
- `development-target-ownership.tsv` maps every development target to its
  category, owning aggregate, and selection state; and
- `development-aggregate-membership.tsv` records exact aggregate dependency
  membership.

Compiler-front-end contracts are Release-owned for each compiler and supported
language/feature profile. Ordinary Debug, sanitizer, and coverage trees do not
configure header, availability, language-failure, representation, constexpr,
or method-flags contract families. `ConfigDefaultChecksReleaseProbe` verifies
the Release default. The retained MSVC Debug and Clang sanitizer cells build
`ConfigDefaultChecksDebugProbe`, which also rejects `NDEBUG`; these narrow
targets are the only deliberate default-check configuration probes.
`VectorChecksTests` and `PreconditionTests` explicitly define
`SIMDLIB_ENABLE_CHECKS=1`, while `RegisterPreconditionTests` installs its
failure hook before including the Register API, so their contracts do not
depend on the selected build type.

`Run-Tests.ps1` always consumes existing artifacts. It succeeds only when the
matching unified-build receipt contains exactly the requested cells, its
source-input digest matches the current tree and every embedded manifest, every
manifest is unchanged, and its pipeline-tooling validation result remains
current and unchanged. Receipt schema v5 binds the pipeline-validation schema,
status, tooling digest, path, and hash alongside each cell's canonical matrix
identity, scoped aggregate, target and test inventory hashes, configured-tree
inventory result, matrix-contract hash, configuration, instrumentation,
generated-code mode, and consumer scope. Test operations contain no
artifact-tree configure or build command.

The expected default, benchmark, compiler-contract, coverage, sanitizer, and
optional diagnostic cells are defined in `tools/validation-matrix.json`.
Generated target and CTest inventories can be checked directly with:

```powershell
tools/Audit-ValidationMatrix.ps1 `
    -Cell msvc-release `
    -BuildDirectory out/pipeline/windows-msvc/<release-tree>/build `
    -Configuration Release
```

The audit rejects duplicate targets or tests, missing ownership, and categories
that are not permitted by the selected profile. Build manifests bind the audit
result, and `Run-Tests.ps1` rejects a receipt whose matrix contract or inventory
audit is stale or belongs to a different cell.

Focused compiler-front-end diagnosis has explicit lower-level operations that
do not enter the default receipt:

```powershell
tools/Run-NativeMatrix.ps1 -Action BuildCompilerContracts -Compiler Msvc -Cell Release
tools/Run-NativeMatrix.ps1 -Action TestCompilerContracts -Compiler Msvc -Cell Release
tools/Run-ContainerMatrix.ps1 -Action BuildCompilerContracts -Compiler Clang22 -Cell Release
tools/Run-ContainerMatrix.ps1 -Action TestCompilerContracts -Compiler Clang22 -Cell Release
```

Benchmark compilation and execution are intentionally isolated:

```powershell
tools/Build-Benchmarks.ps1 -Scope All
tools/Run-Benchmarks.ps1 -Scope All
```

`Build-Benchmarks.ps1` requires completed validation manifests and builds only
`BenchmarkArtifacts` in their existing exhaustive Release trees. It does not
create a benchmark-specific configure tree or rebuild the validation
aggregates. `Run-Benchmarks.ps1` requires current completed benchmark manifests
and never configures or builds. `Run-Tests.ps1` does not require benchmark
artifacts or manifests.

## Instrumentation boundaries

Release, Debug, Clang ASan+UBSan, and native Clang coverage are incompatible
compilation fingerprints and always use separate trees. Debug diagnostics do
not inherit Release optimization enforcement. Sanitizer objects are never
consumed by ordinary Debug tests, and coverage objects are never consumed by a
non-instrumented cell. Benchmark compilation is the sole additional aggregate
that reuses an existing fingerprint, and it reuses only validated Release
trees.

Compile-only constant-evaluation contracts are owned by each compiler's
exhaustive Release tree instead of being repeated under Debug, sanitizer, or
coverage instrumentation. Native Clang coverage builds only execution-bearing
runtime and checks targets. Examples, header smoke tests, and ODR tests are
public-surface contracts owned by applicable Release compiler identities.
Runtime tests continue to exercise Debug and sanitizer behavior.

Coverage profiles are matched to executable build identities before merging.
Raw profiles are merged only within one executable identity, so mutually
exclusive feature configurations never share a raw-profile merge. The
per-executable LCOV traces are combined only after LLVM has interpreted each
profile against its owning executable. `coverage-provenance.tsv` records that
mapping and states that compile-only constexpr evidence is excluded.

Catch2 discovery uses `POST_BUILD` explicitly. Build receipts record the
complete CTest inventory immediately after compilation; `PRE_TEST` would move
discovery into that inventory-recording step rather than remove it from the
receipt-producing workflow.

Register generated-code diagnostics are explicit supplemental operations:

```powershell
tools/Record-Codegen.ps1 -Scope Native -Compiler Msvc -Cell Debug
tools/Record-Codegen.ps1 -Scope Containers -Compiler Clang22 -Cell Debug
tools/Record-Codegen.ps1 -Scope Containers -Compiler Clang22 -Cell AsanUbsan
```

The command requires one compiler and one cell. It configures a diagnostic-only
tree, builds only the Register fixture objects and record comparisons, and
writes dedicated provenance containing the compiler flags, stack-protector
mode, disassembly tools, source identity, record index, and separate compilation
and comparison timings. These `RECORD` results cannot satisfy an `ENFORCE`
Release gate. Debug diagnostics are run only for a compiler involved in an
active investigation; the sanitizer variant is reserved for investigating how
instrumentation changes wrapper/raw memory, control-flow, or ABI paths.

Coverage is development infrastructure owned only by a top-level SimdLib
build. The root CMake boundary does not load development modules for
`add_subdirectory` consumers, and the external-consumer contract fails if a
coverage option, instrumented test, or report target leaks downstream.

The external-consumer project snapshots the parent cache before
`add_subdirectory`, requires the nested target inventory to contain only the
two production interface targets, and rejects nested tests or development
options. It independently verifies the C++20 core target, the C++23 Register
target where supported, and their published usage requirements. Because both
production targets are header-only, Debug CRT and sanitizer propagation do not
create additional consumer contracts.

## Diagnostic runners and cleanup

`Run-NativeMatrix.ps1`, `Run-ContainerMatrix.ps1`, and
`Run-WindowsClang20Container.ps1` are lower-level diagnostic and CI
implementation interfaces. Normal repository workflows use `Build.ps1`,
`Run-Tests.ps1`, `Build-Benchmarks.ps1`, and `Run-Benchmarks.ps1`; the lower-level
scripts do not define additional mandatory modes. The Windows container runner
is the exception used to isolate the LLVM 20.1.8 compatibility cell from a
host's newer LLVM installation.

Container images and selected Linux fingerprint roots can be removed with:

```powershell
tools/Run-ContainerMatrix.ps1 -Action Clean
tools/Run-ContainerMatrix.ps1 -Action Clean -Compiler Clang22
```

All pipeline output is generated below the ignored `out/pipeline` directory.
When no pipeline command is running, removing that directory discards every
native and container fingerprint, report, log, and receipt without touching
source files. A later `Build.ps1` invocation recreates only its selected scope.

Pre-release option and mode names have no compatibility aliases. Supplying a
retired CMake option is a configuration error with a replacement diagnostic;
the PowerShell commands accept only the canonical action, scope, compiler, and
cell vocabulary documented here.
