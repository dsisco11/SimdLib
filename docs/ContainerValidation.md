# Container validation

SimdLib uses repository-owned Linux images for GCC 13, GCC 14, and GNU-like
Clang 22 validation. The same Dockerfiles, Compose definition, entrypoint, and
PowerShell runner are used locally and in GitHub Actions. Native jobs remain
authoritative for MSVC, clang-cl, Windows ABI behavior, and `VECTORCALL`.

## Environment contract

| Service | Scope | Base | Compiler |
| --- | --- | --- | --- |
| `gcc13` | Core-only | Alpine 3.20.8, digest pinned | GCC/G++ 13.2.1 |
| `gcc14` | Full | Alpine 3.22.5, digest pinned | GCC/G++ 14.2.0 |
| `clang22` | Full | Alpine 3.24.1, digest pinned | Clang 22.1.3 |

GCC 13 remains a qualified core-only compiler. Its profiles do not claim
support for `SimdLib::Register`. GCC 14 and Clang 22 own the complete core and
Register surface.

Each image builds the checksum-verified CMake 4.4.0 source release and contains
the exact Catch2 commit declared by its Dockerfile. Package versions, Alpine
images, and the Dockerfile frontend are pinned. The entrypoint rejects an
unexpected compiler or CMake version before configuring the project.

The runtime containers:

- run without root privileges and with all Linux capabilities dropped;
- use a read-only root filesystem and source mount;
- provide an executable temporary filesystem only at `/tmp`;
- write build trees and reports only below `out/container`;
- use UTC and the C locale; and
- validate CPU features before executing ISA-specific tests or benchmarks.

## Commands

Build and run the exhaustive Release contracts for all supported container
compilers:

```powershell
tools/Run-ContainerMatrix.ps1 -Mode Release
```

Select one compiler or one diagnostic profile:

```powershell
tools/Run-ContainerMatrix.ps1 -Mode Release -Compiler Gcc14
tools/Run-ContainerMatrix.ps1 -Mode Debug -Compiler Clang22
tools/Run-ContainerMatrix.ps1 -Mode AsanUbsan -Compiler Clang22
tools/Run-ContainerMatrix.ps1 -Mode Benchmarks -Compiler All
```

`Contracts` performs environment and configure-contract validation without
building the full artifact graph:

```powershell
tools/Run-ContainerMatrix.ps1 -Mode Contracts
```

Reuse already-built images, rebuild without Docker cache, or inspect only the
toolchain contract:

```powershell
tools/Run-ContainerMatrix.ps1 -Mode Release -SkipImageBuild
tools/Run-ContainerMatrix.ps1 -Mode Contracts -NoImageCache
tools/Run-ContainerMatrix.ps1 -Mode Contracts -InspectEnvironment
```

Remove only the Compose containers, local image tags, and ignored artifact
tree owned by this repository:

```powershell
tools/Run-ContainerMatrix.ps1 -Clean
```

## Profiles and artifacts

| Mode | Services | Configuration | Artifact target |
| --- | --- | --- | --- |
| `Contracts` | GCC 13, GCC 14, Clang 22 | Release configure contracts | none |
| `Release` | GCC 13, GCC 14, Clang 22 | optimized exhaustive validation | `ExhaustiveArtifacts` |
| `Debug` | GCC 13, GCC 14, Clang 22 | diagnostic, record-only codegen | `ExhaustiveArtifacts` |
| `AsanUbsan` | Clang 22 | Debug with AddressSanitizer and UndefinedBehaviorSanitizer | `ExhaustiveArtifacts` |
| `Benchmarks` | GCC 13, GCC 14, Clang 22 | optimized benchmark build and execution | `BenchmarkArtifacts` |

Release and benchmark operations share each compiler's Release configure tree,
so benchmark compilation does not create or rebuild the exhaustive validation
targets. Debug and sanitizer profiles use separate trees because their flags
are distinct compilation fingerprints.

The runner owns matrix membership and starts selected services concurrently
with `docker compose run --rm`. It retains separate output and error logs and
returns failure when any selected service fails. Build trees use
`out/container/<compiler>/build/<preset>`. Provenance, main CTest XML, and
consumer CTest XML use `out/container/<compiler>/<preset>`.

## Failure and cancellation checks

The runner retains intentional-failure and cancellation controls for testing
its aggregation behavior:

```powershell
tools/Run-ContainerMatrix.ps1 -Mode Contracts -SkipImageBuild -InjectFailure Gcc14
tools/Run-ContainerMatrix.ps1 -Mode Contracts -SkipImageBuild -InjectFailure All
tools/Run-ContainerMatrix.ps1 -Mode Release -SkipImageBuild -CancelAfterSeconds 2
```

These commands must return nonzero. Cleanup is scoped to the unique Compose
project created for that invocation, while logs already received from completed
services remain available.

## Refresh procedure

Image refreshes are deliberate review changes:

1. Select the smallest maintained Alpine release that provides the required
   compiler and retrieve its immutable multi-platform manifest digest.
2. Update every exact package version, CMake checksum, and Catch2 commit.
3. Run `Contracts` with `-NoImageCache` and review the environment identities.
4. Run `Release`, `Debug`, `AsanUbsan`, and `Benchmarks` as applicable.
5. Confirm the native MSVC and clang-cl profiles separately.

The scheduled container reproducibility workflow performs the no-cache
contract rebuild. Pull requests and normal CI use the same repository-owned
definitions and runner.
