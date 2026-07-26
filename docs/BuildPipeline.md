# Unified build and validation

SimdLib has one repository-owned build command and one correctness-validation
command. A complete local build is:

```powershell
tools/Build.ps1 -Scope All
```

This builds the MSVC Release and Debug, clang-cl Release and Debug, native
Clang Debug coverage, GCC 13 core-only Release and Debug, GCC 14 Release and
Debug, and Clang 22 Release, Debug, and ASan+UBSan cells. It then builds each
Release cell's benchmark target in the same configure tree. It does not run a
test or benchmark executable.

The corresponding complete validation command is:

```powershell
tools/Run-Tests.ps1 -Scope All
```

`Run-Tests.ps1` invokes `Build.ps1` exactly once, validates the exact set of
completed manifests, and then starts test-only operations. The coverage cell
resets profiles, runs its instrumented tests, and generates `coverage.info`.
Benchmark execution remains separate:

```powershell
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

Benchmark builds reuse validated Release trees. Benchmark execution requires
their completed benchmark manifests and never configures or builds.

Coverage is development infrastructure owned only by a top-level SimdLib
build. The root CMake boundary does not load development modules for
`add_subdirectory` consumers, and the external-consumer contract fails if a
coverage option, instrumented test, or report target leaks downstream.
