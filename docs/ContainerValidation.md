# Container validation

SimdLib uses repository-owned Linux images for its GCC 14 and GNU-like Clang
22 validation. The same Dockerfiles and PowerShell runner are used locally and
in GitHub Actions. Native Windows jobs remain authoritative for MSVC,
clang-cl, Windows ABI behavior, and `VECTORCALL`; Linux containers do not claim
to validate those boundaries.

## Environment contract

The images intentionally use the smallest stable Alpine release that provides
each required compiler:

| Service | Base | Compiler | Build tools |
| --- | --- | --- | --- |
| `gcc14` | Alpine 3.22.5, pinned by manifest digest | GCC/G++ 14.2.0-r6 | CMake 4.4.0, Ninja 1.12.1 |
| `clang22` | Alpine 3.24.1, pinned by manifest digest | Clang 22.1.3-r2 | CMake 4.4.0, Ninja 1.13.2 |

The Dockerfile frontend is also pinned by immutable digest so a no-cache build
cannot silently select a different BuildKit frontend implementation.

Alpine packages do not provide CMake 4.4. Each Dockerfile therefore builds the
official CMake 4.4.0 source archive in a disposable stage after verifying its
SHA-256 digest, then copies only the installed result into the runtime image.
The exact Catch2 v3.8.1 commit is also baked into the image and supplied through
`FETCHCONTENT_SOURCE_DIR_CATCH2`; test runs do not resolve a movable tag.
Each configure uses CMake's fresh-toolchain mode so an image refresh cannot
retain a previously missing compiler tool in a persistent build-tree cache.

The runtime containers:

- run without root privileges and with all Linux capabilities dropped;
- use a read-only root filesystem and source mount;
- provide an executable temporary filesystem only at `/tmp`;
- write build trees, JUnit reports, provenance, and logs only below
  `out/container`;
- use UTC and the C locale; and
- reject unexpected compiler or CMake versions before configuring SimdLib.

The full and sanitizer profiles also require the host CPU to expose SSE4.2,
AVX2, FMA, BMI1, and BMI2 because containers inherit host CPU features and
SimdLib's complete runtime suite exercises those instruction families.

## Commands

Run the complete GCC and Clang matrix:

```powershell
tools/Run-ContainerMatrix.ps1 -Mode Full
```

Run one compiler or the focused compile-time contract surface:

```powershell
tools/Run-ContainerMatrix.ps1 -Mode Full -Compiler Gcc14
tools/Run-ContainerMatrix.ps1 -Mode Focused
```

Run the feature selection, sanitizer, or generated-code-ready environments
without rebuilding images that were already built:

```powershell
tools/Run-ContainerMatrix.ps1 -Mode Feature -NoBuild
tools/Run-ContainerMatrix.ps1 -Mode Sanitizer -NoBuild
tools/Run-ContainerMatrix.ps1 -Mode Codegen -NoBuild
tools/Run-ContainerMatrix.ps1 -Mode Debug -NoBuild
tools/Run-ContainerMatrix.ps1 -Mode Benchmark -NoBuild
```

Rebuild both images without cache and rerun focused contracts:

```powershell
tools/Run-ContainerMatrix.ps1 -Mode Focused -NoCache
```

Print and validate compiler, CMake, Ninja, libc, operating-system, dependency,
architecture, and CPU provenance without compiling:

```powershell
tools/Run-ContainerMatrix.ps1 -Mode Focused -DoctorOnly
```

Remove only the Compose containers, local image tags, and ignored artifact tree
owned by this repository:

```powershell
tools/Run-ContainerMatrix.ps1 -Clean
```

## Profiles and result aggregation

Compose declares common security, mount, environment, entrypoint, and artifact
rules. The PowerShell runner owns matrix membership and starts selected services
concurrently with `docker compose run --rm`. It waits for every service and
returns failure if any service exits nonzero, while retaining separate standard
output and error logs for each compiler.

| Mode | Services | Purpose |
| --- | --- | --- |
| `Focused` | GCC 14, Clang 22 | Configuration, header, constexpr, ODR, and external-consumer contracts |
| `Full` | GCC 14, Clang 22 | Complete Release test and optional-feature matrix |
| `Feature` | GCC 14, Clang 22 | AVX2, FMA, BMI, and scalar-labelled tests |
| `Sanitizer` | Clang 22 | Debug ASan and UBSan matrix |
| `Codegen` | GCC 14, Clang 22 | Optimized SSE4.2/128 diagnostics plus strict AVX2/128 and AVX2/256 wrapper/raw, ABI, and consumer-boundary gates |
| `Debug` | GCC 14, Clang 22 | Debug correctness plus recorded wrapper-versus-raw differentials |
| `Benchmark` | GCC 14, Clang 22 | Runtime-derived supplemental Register/raw performance comparisons |

Direct `docker compose up` is useful for interactive inspection but is not the
canonical result aggregator: its selected-service exit-code mode cannot express
the required aggregate status. The wrapper keeps Compose as the declarative
environment layer while making matrix membership, per-service logs, and all-exit
status explicit.

Evidence is retained beneath `out/container`:

- `<service>/<mode>/provenance.txt` records environment identity;
- `<service>/<mode>/ctest.xml` records the main suite;
- `<service>/<mode>/consumer-ctest.xml` records external consumers; and
- `logs/<run-id>/` contains separate standard output and error logs.

Code-generation artifacts are separated by ISA and width below
`<service>/codegen/build/container-codegen/register-codegen/`: `sse42/128`,
`avx2/128`, and `avx2/256`. Each provenance file records the selected ISA
profile explicitly.

## Failure and cancellation checks

The runner has an intentional-failure switch used only to prove aggregation:

```powershell
tools/Run-ContainerMatrix.ps1 -Mode Focused -NoBuild -InjectFailure Gcc14
tools/Run-ContainerMatrix.ps1 -Mode Focused -NoBuild -InjectFailure All
tools/Run-ContainerMatrix.ps1 -Mode Full -NoBuild -CancelAfterSeconds 2
```

All three commands must return nonzero. The first two identify every failed
service; the third exercises the same interruptible wait and `finally` cleanup
used by Ctrl-C without depending on interactive terminal input. Every
invocation uses a unique `simdlib-register-<run-id>` Compose project.
The runner's `finally` cleanup stops and removes only that invocation's
containers and network, including after cancellation. Logs already received
from completed services remain in the artifact tree.

## Refresh procedure

Image refreshes are deliberate review changes:

1. Select the smallest maintained Alpine release that provides the required
   compiler and retrieve its immutable multi-platform manifest digest.
2. Update every exact `apk` package version, the CMake source version and
   checksum, and the Catch2 commit as applicable.
3. Build with `-Mode Focused -NoCache`, save the new provenance and
   `docker image inspect` output, and review the identity and size differences.
4. Run `Full`, `Feature`, and `Sanitizer` from those exact images.
5. Confirm the native Windows matrix separately; Linux success never replaces
   MSVC, clang-cl, Windows ABI, or calling-convention evidence.

The scheduled `container-reproducibility.yml` workflow performs the no-cache
focused rebuild weekly. Pull requests and normal CI use `ci.yml` and the same
Dockerfiles, entrypoint, presets, and runner as local validation.
