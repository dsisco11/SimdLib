# Catch2 codegen runner prototype

This historical evaluation used the isolated `tests/codegen/catch2-prototype`
project to assess Catch2 as owner of instruction-check assertions. That temporary
project has been removed during the maintained-runner migration; current build
instructions are in [InstructionContracts.md](InstructionContracts.md).
The comparison uses its production `Register<float, 128>` transfer fixture,
the same two primary FileCheck rules, and the same mandatory compiler supplement.

## Process

1. CMake/Ninja builds the fixture object and its existing freshness receipt, plus
   the Catch2 executable. CTest does not compile anything.
2. The Catch2 fixture invokes one CMake bridge to verify freshness and run the
   existing LLVM extraction wrapper (`llvm-readobj` and `llvm-objdump`).
3. The fixture writes one sentinel-wrapped input, shared by all applicable rules.
4. It launches FileCheck directly for each rule and captures diagnostics in memory.
   Catch2 `CHECK` assertions report every primary and supplemental result.
5. CTest discovers and runs the Catch2 tests normally. No separate qualification
   test or success-marker files are needed for this one composite contract.

Extraction is cached within each executable process. Separately discovered CTest
tests use separate processes and therefore extract separately. Each process
reserves its own artifact directory; diagnostic artifacts are retained.

## Historical build and run

The following records the former experiment's invocation shape and is not a
current reproduction command; its source directory has been retired:

```powershell
cmake -S tests/codegen/catch2-prototype -B out/catch2-codegen -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  "-DSIMDLIB_CATCH2_SOURCE=<absolute Catch2 source directory>" `
  "-DSIMDLIB_CODEGEN_LLVM_ROOT=$PWD/out/codegen-tools-provisioned"
cmake --build out/catch2-codegen
ctest --test-dir out/catch2-codegen --output-on-failure
```

An installed Catch2 3 package can supply the dependency instead. The prototype
does not download dependencies. Its evidence runs use Catch2 3.8.1.

For an actual failing production test, set either
`SIMDLIB_PROTOTYPE_FAIL_PRIMARY` or `SIMDLIB_PROTOTYPE_FAIL_SUPPLEMENT` in the test
environment and select `production transfer primary and supplement`. These
prototype-only switches substitute an impossible rule. Unset them afterward.

## Validation and comparison

On 2026-09-15, MSVC 19.44 on Windows and GCC 14.2 on Linux each passed all four
discovered tests. These cover the production contract, deliberately incorrect
primary and supplemental rules, and an extra operation after the return.
Separately injecting either failure into the actual production test made CTest
exit with failure on both platforms. Two subsequent builds per platform performed
no compilation or receipt regeneration. On Windows, removing the build receipt
or modifying object bytes also failed; restoring the original files restored a
passing test.

Ten warm samples per runner, with alternating execution order, measured the
existing qualified transfer test against the prototype production test:

| Environment | Existing runner median | Catch2 prototype median |
| --- | ---: | ---: |
| Windows / MSVC 19.44 | 1.32 s | 0.78 s |
| Linux container / GCC 14.2 | 2.31 s | 0.80 s |

These are local test-runner wall times, including CTest, hashing, and tool startup;
they do not measure generated-code performance. The prototype also removes
redundant freshness and FileCheck identity checks from the outer execution path,
so the timings do not isolate Catch2 framework overhead alone.

For the successful Windows production case, runtime artifacts decrease from
21 files to 11. The baseline additionally has three generated case/check manifests;
one obsolete log from an earlier run was excluded from the runtime count.
The prototype retains ten extraction artifacts and one shared FileCheck input.
Source-derived process counts decrease from 17 to 10 descendants of CTest,
including the Catch2 executable where applicable. These counts were not traced
at the operating-system level. The prototype itself adds 366 physical source
lines across six files, including the process helper and deliberate-negative tests.

The result supports migrating assertion ownership to Catch2: the primary and
mandatory supplement naturally form one test result, diagnostics need no
intermediate result files, and this case runs with less orchestration overhead.
It does not yet establish that a complete migration will use fewer source lines.

Logs, injected failures, incremental checks, and raw timing samples are under
`out/codegen-catch2-evidence/` (`msvc-timings.csv` and
`gcc14-comparison/*.time`). These ignored local artifacts are not committed.

## Scope and migration limits

This evaluates outer orchestration, not a complete replacement implementation.
The shared CMake extraction wrapper still writes LLVM diagnostics and verifies
object boundaries. Build inputs and receipts remain on disk. The prototype
checks freshness once before extraction; the existing runner also repeats it
before each separately scheduled check and validates FileCheck identity there.

A full migration must retain independent expected-case coverage, all geometries,
constant and ABI checks, compiler/version selection, development-profile and CI
integration, and the existing deliberate-negative corpus. Those are not proven
by this one-case prototype.

The native process helper adds C++ maintenance: it currently uses Windows ANSI
paths and has no timeout/cancellation support. Production adoption needs robust
process handling and artifact-retention policy. Counting fewer CMake scripts
alone would therefore overstate the reduction in total implementation complexity.
