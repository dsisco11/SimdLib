# Unified build and validation

SimdLib has one repository-owned build command and one correctness-validation
command. A complete local build is:

```powershell
tools/Build.ps1 -Scope All
```

This builds the Windows MSVC and clang-cl Release and Debug cells, native Clang
Debug coverage, Linux GCC 13 core-only Release and Debug, Linux GCC 14 Release
and Debug, and Linux Clang 22 Release, Debug, and ASan+UBSan cells. It builds
the correctness, ABI, generated-code, sanitizer, consumer, coverage, probe,
example, and header-validation artifacts, but does not compile benchmark
targets or run any executable.

The corresponding complete validation command is:

```powershell
tools/Run-Tests.ps1 -Scope All
```

`Run-Tests.ps1` invokes `Build.ps1` exactly once, validates the exact set of
completed manifests, and then starts test-only operations. The coverage cell
resets profiles, runs its instrumented tests, and generates `coverage.info`.
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

For CI or advanced local reuse, tests may skip their one build invocation:

```powershell
tools/Run-Tests.ps1 -Scope All -SkipBuild
```

This succeeds only when the matching unified-build receipt contains exactly
the requested cells, its source-input digest matches the current tree, every
manifest is unchanged, and each cell's cache, test inventory, consumer
inventory, and generated-code records remain valid. Test operations contain no
configure or build command.

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
