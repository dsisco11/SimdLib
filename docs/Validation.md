# Validation evidence

This document records execution evidence for the unified build and validation
pipeline completed on 2026-07-26. Command semantics and prerequisites belong in
[Unified build and validation](BuildPipeline.md); the measurements and outcomes
below describe this execution only and are not timeless performance promises.

## Executed commands

The acceptance run used the formal repository interfaces:

```powershell
tools/Build.ps1 -Scope All
tools/Run-Tests.ps1 -Scope All
tools/Run-Benchmarks.ps1 -Scope All
```

The default test command invoked the unified build exactly once, validated its
receipt, and then ran the native and container test-only operations. A separate
`tools/Run-Tests.ps1 -Scope All -SkipBuild` run validated reuse without a
configure or build invocation. Benchmarks remained outside correctness testing.

## Compiler and configuration ownership

| Fingerprint owner | Configuration and instrumentation | Main tests | Consumer tests | Result |
| --- | --- | ---: | ---: | --- |
| MSVC 19.44 | Release exhaustive | 246 | 2 | No failures |
| MSVC 19.44 | Debug diagnostics | 207 | 2 | No failures |
| clang-cl 22.1.8 | Release exhaustive | 249 | 2 | No failures |
| clang-cl 22.1.8 | Debug diagnostics | 210 | 2 | No failures |
| native Clang 22.1.8 | Debug source coverage | 240 | 0 | No failures |
| GCC 13.2.1 | Alpine x64 core-only Release | 200 | 1 | No failures |
| GCC 13.2.1 | Alpine x64 core-only Debug | 161 | 1 | No failures |
| GCC 14.2.0 | Alpine x64 Release exhaustive | 249 | 2 | No failures |
| GCC 14.2.0 | Alpine x64 Debug diagnostics | 210 | 2 | No failures |
| Clang 22.1.3 | Alpine x64 Release exhaustive | 249 | 2 | No failures |
| Clang 22.1.3 | Alpine x64 Debug diagnostics | 210 | 2 | No failures |
| Clang 22.1.3 | Alpine x64 Debug, ASan+UBSan | 210 | 2 | No failures or sanitizer diagnostics |

GCC 13 is deliberately core-only and does not claim `SimdLib::Register`
support. The coverage fingerprint owns instrumented project tests but does not
repeat the external consumer; consumer isolation is exercised by the other 11
fingerprints. The standalone parent fixture additionally proved that
`add_subdirectory` adds only the four production interface targets, introduces
no development cache options or Catch2 targets, and registers no SimdLib tests
in the parent's CTest inventory.

Every exhaustive build includes strict warnings, configuration and constexpr
probes, first-and-only header probes, ODR executables, examples, runtime scalar
oracles, instruction-family variants, generated-code records, and ABI gates as
applicable to its owner. The runtime inventory audit requires AVX2, FMA, BMI,
and scalar labels plus their mandatory test families before CTest runs.

## Receipt-bound artifacts

The final `All` receipt references exactly:

- 12 completed validation manifests and canonical fingerprint documents;
- 12 main-test inventories and JUnit reports;
- 11 nonempty external-consumer inventories and JUnit reports;
- five Release benchmark manifests and executables;
- 282 generated-code and ABI records; and
- 2,637 object files across 17,820 artifact files.

The receipt is stored below `out/pipeline/provenance`. Each referenced cell uses
the following stable layout:

```text
out/pipeline/<platform>-<compiler>/<configuration>-<fingerprint>/
  build/
  consumer/
  reports/
  provenance/
```

Native MSVC, clang-cl, and coverage cells use `windows-*` platform prefixes.
The GCC and GNU-like Clang containers use `linux-*`. Console output for each
aggregate operation is retained under `out/pipeline/logs/<run-id>`.

## Incremental and incompatibility evidence

The clean unified build completed in 951.999 seconds. An unchanged second build
completed in 101.473 seconds while validating the complete graph. Before/after
hashing and timestamps showed all 2,637 object files unchanged: no SimdLib,
test, example, benchmark, consumer, or Catch2 translation unit recompiled. All
three local compiler image IDs and filesystem layers also remained unchanged.

The test-only command completed in 82.328 seconds. Process tracing for ordinary
container cells contained no CMake configure, `cmake --build`, Ninja, Make, or
MSBuild execution. LeakSanitizer cannot run under `ptrace`, so the ASan+UBSan
cell used the same manifest-validated inner test operation without tracing.

A controlled public-header edit recompiled 599 affected objects across exactly
the ten Register-capable fingerprints. Both GCC 13 core-only fingerprints and
all compiler-image layers remained unchanged. Restoring the header made the old
receipt stale until the affected build manifests were refreshed.

A controlled GCC 14 image-identity change produced a new fingerprint. Test-only
execution rejected the original artifacts because the new fingerprint had no
completed validation manifest. Building the affected Release cell created only
that new fingerprint; the original image tag was then restored.

## Generated-code and ABI policy

The 282 final records contain:

| Result | Records |
| --- | ---: |
| Exact parity | 110 |
| Recorded diagnostic | 27 |
| Recorded Debug or sanitizer difference | 143 |
| Accepted compiler exception | 2 |

The two accepted records represent one MSVC 19.44 behavior observed in the
SSE4.2 and AVX2 128-bit profiles: `/GS` inserts the recognized security-cookie
sequence for `Register<double>::from_array`. The comparator still requires the
remaining wrapper instructions to match the raw fixture. The pure AVX2
register-only subset accepts no cookie exception.

AVX2 Release is the strict zero-overhead profile. SSE4.2 remains diagnostic;
Debug and sanitizer fingerprints record differences rather than importing the
Release optimization policy. Windows non-inline Register boundaries use
`VECTORCALL`. Platform-default aggregate return behavior remains diagnostic.
The full exception and exclusion rationale is maintained in
[Register qualification](RegisterQualification.md).

## Failure, cleanup, and downstream evidence

Intentional single-service and two-service failures started all selected
compiler operations, reported every started result, named every failing cell,
and preserved the per-cell logs. Timed cancellation and a simulated interactive
PowerShell stop removed their invocation-owned containers and networks.

Additional negative probes produced exact failures for:

- a stale unified receipt before any test executable changed;
- Docker absent from `PATH`;
- a configured C++ compiler absent from the image;
- a host CPU inventory without the required SSE4.2 flag; and
- a mandatory runtime-test family absent from a configured tree.

The clean external consumer and parent-project fixtures configured, built, and
tested independently. Development targets, options, dependencies, coverage,
and SimdLib-owned tests did not leak through `add_subdirectory`.

## Measured comparison with the frozen baseline

The frozen pre-refactor scenarios in
[UnifiedBuildPipelineBaseline.md](UnifiedBuildPipelineBaseline.md) totalled
2,492.967 seconds when their separately owned clean operations were added,
with 3,449 compile outputs and 3,580.72 MiB of artifacts. The unified clean run
used 951.999 seconds, 2,637 object outputs, and 3,731.11 MiB.

The wall-time comparison is directional rather than perfectly like-for-like:
the baseline is a serial sum of separate scenarios, while the unified command
is one parallel aggregate covering 12 fingerprints, consumers, generated-code
and ABI gates, and benchmark compilation. It nevertheless demonstrates the
structural result: 812 fewer compile outputs, a 23.54% reduction, with no
duplicate Feature tree. Artifact storage increased by 150.39 MiB, or 4.20%,
because the final receipt retains the broader complete compiler and
instrumentation matrix rather than a smaller sampled scenario set.

The final unchanged build completed in 99.93 seconds, the default build-and-test
command in 164.93 seconds, and benchmark-only execution in 27.12 seconds. These
measurements are execution evidence for this machine and revision; they are not
thresholds or guarantees.

## Interface migration audit

The final interface audit parsed all seven PowerShell scripts and modules, the
four workspace and preset JSON files, both GitHub Actions workflows,
`compose.yml`, and the POSIX container entrypoint. CMake accepted every preset,
Docker Compose accepted the resolved service configuration, and all relative
targets in the repository's 30 Markdown files existed.

Current commands, examples, workflows, presets, VS Code tasks, and CTest
documentation contain only the canonical action, scope, compiler, target, and
fingerprint vocabulary. Retired names remain only where their text is required:
the planning rename ledger, frozen pre-refactor inventories, and CMake's focused
failure diagnostics for explicitly supplied retired cache options. Those cache
entries are rejected and are not compatibility aliases.

Representative object, log, coverage-profile, disassembly, and temporary-probe
paths were all covered by repository ignore rules. A complete tracked-path audit
found no generated build tree, binary, object, log, profile, disassembly, or
temporary probe. The interface corrections described in this subsection changed
documentation only, so that audit reused the completed compiler evidence above.
The later supported-platform cleanup below changed top-level CMake qualification
and was therefore rebuilt and retested separately.

## Supported-platform cleanup evidence

The published support contract now assigns MSVC and clang-cl to Windows x64 and
assigns Clang and GCC to Linux x64. GCC 13.2 remains core-only, while GCC 14 or
newer owns the Linux Register surface. Top-level CMake likewise recognizes GNU
Register qualification only for a 64-bit Linux system; generic GNU compiler
handling remains available for the supported Linux GCC cells.

A case-insensitive scan of every tracked file found zero occurrences of the
retired platform's conventional name. A separate scan found no non-planning
reference or platform association and no unified command, compiler filter,
preset, Compose profile, workflow, or failure diagnostic that recognizes the
retired target.

The final validation used:

```powershell
tools/Build.ps1 -Scope All
tools/Run-Tests.ps1 -Scope All -SkipBuild
```

The completed receipt matched the current source digest and owned all twelve
required fingerprints. All five native cells and all seven container cells
completed, including Linux GCC 13 core-only Release and Debug, Linux GCC 14
Release and Debug, and the Linux Clang Release, Debug, and ASan+UBSan cells.

## Supplemental benchmarks

All five Release benchmark owners completed the runtime-derived wrapper/raw
suite with 25 samples per entry. The suite covers 128-bit and 256-bit floating
addition, mask selection, and unsigned integer division. Benchmark timing is
supplemental and cannot override correctness, ABI, or generated-code gates.
