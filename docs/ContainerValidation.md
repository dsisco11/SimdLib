# Container validation

SimdLib uses repository-owned Linux images for GCC 13, GCC 14, and GNU-like
Clang 22 validation. The same Dockerfiles, Compose definition, entrypoint, and
PowerShell runner are used locally and in GitHub Actions. Native jobs remain
authoritative for MSVC, clang-cl, Windows ABI behavior, and vector calling-convention behavior.

## Environment contract

| Service | Scope | Base | Compiler |
| --- | --- | --- | --- |
| `gcc13` | Core-only | Alpine 3.20.8, digest pinned | GCC/G++ 13.2.1 |
| `gcc14` | Core and Register | Alpine 3.22.5, digest pinned | GCC/G++ 14.2.0 |
| `clang22` | Core and Register | Alpine 3.24.1, digest pinned | Clang 22.1.3 |

GCC 13 remains a qualified core-only compiler. Its cells do not claim support
for `SimdLib::Register`. GCC 14 and Clang 22 own the complete core and Register
surface.

Each image builds the checksum-verified CMake 4.4.0 source release and contains
the exact Catch2 commit declared by its Dockerfile. Package versions, Alpine
images, and the Dockerfile frontend are pinned. The entrypoint rejects an
unexpected compiler or CMake version before configuring the project.
Building these images requires Docker Compose 2.39.0 or newer so the runner can
disable BuildKit provenance without changing the image-identity contract.

The runtime containers:

- run without root privileges and with all Linux capabilities dropped;
- use a read-only root filesystem and source mount;
- provide an executable temporary filesystem only at `/tmp`;
- write only below `out/pipeline`;
- use UTC and the C locale; and
- validate CPU features before executing ISA-specific tests or benchmarks.

## Operations

The formal cross-platform commands and fingerprint reuse contract are
documented in [Unified build and validation](BuildPipeline.md). Direct use of
the container runner remains available for Linux-cell diagnostics and CI
ownership.

One build operation creates every Linux validation artifact. One later test
operation consumes those artifacts without configuring or compiling:

```powershell
tools/Run-ContainerMatrix.ps1 -Action Build
tools/Run-ContainerMatrix.ps1 -Action Test
```

Select one compiler or configuration when diagnosing a specific cell:

```powershell
tools/Run-ContainerMatrix.ps1 -Action Build -Compiler Gcc14 -Cell Release
tools/Run-ContainerMatrix.ps1 -Action Test -Compiler Clang22 -Cell Debug
tools/Run-ContainerMatrix.ps1 -Action Test -Compiler Clang22 -Cell AsanUbsan
```

Optional `-TestRegex` and `-TestLabel` filters only narrow a test operation;
they never define a build profile or alter artifact identity.

There is no mandatory Feature build cell. AVX2, FMA, BMI, and scalar tests are
registered in the exhaustive runtime inventory, audited before execution, and
run once in each owning cell. A label filter is an optional diagnostic view of
that existing inventory, not a second compilation scenario.

Benchmark compilation and execution are separate operations. Both own only the
existing Release cells, and building benchmarks does not rebuild validation
targets:

```powershell
tools/Run-ContainerMatrix.ps1 -Action BuildBenchmarks
tools/Run-ContainerMatrix.ps1 -Action RunBenchmarks
```

Rebuild images without Docker cache, reuse existing images during a build, or
inspect only the pinned environments without compiling SimdLib:

```powershell
tools/Run-ContainerMatrix.ps1 -Action InspectEnvironment -NoImageCache
tools/Run-ContainerMatrix.ps1 -Action Build -SkipImageBuild
tools/Run-ContainerMatrix.ps1 -Action InspectEnvironment -SkipImageBuild
```

Remove the selected local images and compiler artifact roots together with
abandoned `simdlib-container-*` containers and networks:

```powershell
tools/Run-ContainerMatrix.ps1 -Action Clean
tools/Run-ContainerMatrix.ps1 -Action Clean -Compiler Clang22
```

`Clean` is intentionally destructive to the selected generated state below
`out/pipeline`; it does not touch source files or artifacts owned by an
unselected compiler. Normal incremental work does not require cleaning.

## Build cells and artifacts

| Cell | Services | Configuration | Artifact target |
| --- | --- | --- | --- |
| `Release` | GCC 13, GCC 14, Clang 22 | optimized exhaustive validation | `ExhaustiveArtifacts` |
| `Debug` | GCC 13, GCC 14, Clang 22 | unoptimized runtime validation without Register generated-code work | `ExhaustiveArtifacts` |
| `AsanUbsan` | Clang 22 | instrumented runtime validation without Register generated-code work | `ExhaustiveArtifacts` |

Record-only generated-code work is selected separately and never joins a
normal build receipt:

```powershell
tools/Record-Codegen.ps1 -Scope Containers -Compiler Gcc14 -Cell Debug
tools/Record-Codegen.ps1 -Scope Containers -Compiler Clang22 -Cell Debug
tools/Record-Codegen.ps1 -Scope Containers -Compiler Clang22 -Cell AsanUbsan
```

The Debug operation is intended for an active compiler investigation, rather
than routine coverage across every compiler. The sanitizer operation has the
narrow purpose of exposing instrumentation-induced wrapper/raw memory,
control-flow, or ABI differences that runtime sanitizer execution cannot show.
It is not a correctness or optimized generated-code gate.

The runner builds selected images once under the stable
`simdlib-container-images` Compose project, then executes cells with bounded
parallelism controlled by `-MaxParallel`. Each operation has a unique Compose
project and independent standard-output and standard-error logs. Stable image
build ownership prevents an invocation-only Compose label from changing image
identity. A failure in one cell does not hide failures from the remaining
cells.

Each cell has a canonical JSON fingerprint. The full SHA-256 is stored in the
fingerprint document, while its first 16 hexadecimal characters disambiguate
the readable directory name:

```text
out/pipeline/linux-<compiler>/<cell>-<fingerprint>/
  build/
  consumer/
  reports/
  provenance/
```

Compiler image content identity, pinned base image, toolchain, configuration,
sanitizers, required flags, generator, dependencies, and CPU requirements
participate in the fingerprint. Source revision, source digest, test selection,
CI state, and parallelism do not. Build manifests separately bind a completed
artifact to its source digest and revision, so tests reject stale source inputs.
Image builds disable BuildKit source-context provenance so unrelated project
source changes cannot alter an otherwise identical toolchain image identity.
The content identity covers the filesystem layer chain and runtime image
configuration while excluding Compose's per-invocation project labels.

Release and benchmark operations share each compiler's Release tree. Debug and
sanitizer configurations have separate fingerprints and trees.

## Failure and cancellation checks

The runner retains intentional-failure and cancellation controls for testing
aggregation and cleanup:

```powershell
tools/Run-ContainerMatrix.ps1 -Action InspectEnvironment -SkipImageBuild -InjectFailure gcc14-release
tools/Run-ContainerMatrix.ps1 -Action InspectEnvironment -SkipImageBuild -InjectFailure All
tools/Run-ContainerMatrix.ps1 -Action Build -SkipImageBuild -CancelAfterSeconds 2
```

These commands return nonzero. Cleanup is scoped to the unique Compose project
created for the invocation, while logs received from completed cells remain
available.

## Refresh procedure

Image refreshes are deliberate review changes:

1. Select the smallest maintained Alpine release that provides the required
   compiler and retrieve its immutable multi-platform manifest digest.
2. Update every exact package version, CMake checksum, and Catch2 commit.
3. Run `InspectEnvironment` with `-NoImageCache` and review the identities.
4. Run `tools/Build.ps1 -Scope Containers`, then
   `tools/Run-Tests.ps1 -Scope Containers` and
   `tools/Build-Benchmarks.ps1 -Scope Containers` followed by
   `tools/Run-Benchmarks.ps1 -Scope Containers`.
5. Confirm the native MSVC and clang-cl configurations separately.

The scheduled reproducibility workflow performs the no-cache environment
rebuild without compiling SimdLib. Pull requests and normal CI use the same
repository-owned definitions and runner.
