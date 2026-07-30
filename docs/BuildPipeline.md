# Unified build and validation

SimdLib has one repository-owned build command and one correctness-validation
command. A complete local build is:

```powershell
tools/Build.ps1 -Scope All
```

This builds MSVC Release and the representative MSVC Debug cell, clang-cl
Release, native Clang Debug coverage, GCC 13 core-only Release, GCC 14 Release,
and Clang 22 Release plus the representative ASan+UBSan Debug cell. It builds
the correctness, ABI, sanitizer, consumer, coverage, probe, example, and
header-validation artifacts, plus the mandatory optimized generated-code gates
in Release. The default matrix does not build ordinary clang-cl, GCC 13,
GCC 14, or Clang 22 Debug cells. Debug, sanitizer, and coverage cells do not
compile Register generated-code fixtures. The command does not compile
benchmark targets or run any executable.

Before starting compiler cells, `Build.ps1` invokes
`tools/Run-RepositoryAudit.ps1`. That operation audits source-text contracts
once for the canonical source digest and writes
`out/pipeline/provenance/repository-audit-<digest>.json`. The unified receipt
binds the result path, hash, and source digest; no compiler tree contains a
duplicate repository-audit target or CTest.

The corresponding complete validation command is:

```powershell
tools/Run-Tests.ps1 -Scope All
```

`Run-Tests.ps1` invokes `Build.ps1` exactly once, validates the exact set of
completed manifests, and then starts test-only operations. The coverage cell
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
- LLVM 22 with `clang-cl`, `clang++`, `llvm-profdata`, `llvm-cov`, and
  `llvm-readobj` available on `PATH`;
- CMake 4.4.0; and
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

For CI or advanced local reuse, tests may skip their one build invocation:

```powershell
tools/Run-Tests.ps1 -Scope All -SkipBuild
```

This succeeds only when the matching unified-build receipt contains exactly
the requested cells, its source-input digest matches the current tree, every
manifest is unchanged, the repository-audit result remains current and
unchanged, and each cell's cache, test inventory, consumer inventory, and
generated-code records remain valid. Test operations contain no configure or
build command.

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

## Diagnostic runners and cleanup

`Run-NativeMatrix.ps1` and `Run-ContainerMatrix.ps1` are lower-level diagnostic
and CI implementation interfaces. Normal repository workflows use `Build.ps1`,
`Run-Tests.ps1`, `Build-Benchmarks.ps1`, and `Run-Benchmarks.ps1`; the
lower-level scripts do not define additional mandatory modes.

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
