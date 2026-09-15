# Catch2 instruction contracts

The provisional pilot defines codegen assertions with the project's Catch2
system. CMake compiles production fixture objects and test executables; CTest
discovers and schedules the Catch2 cases; LLVM FileCheck matches instructions.
These expectations do not prove runtime correctness, optimal code, or measured
performance. Migration validation is tracked in [Catch2Migration.md](Catch2Migration.md).

## Build and run

In a configured compiler environment, with Ninja and the
[explicit LLVM tools](ToolProvisioning.md):

```powershell
cmake -S tests/codegen/pilot -B out/instruction-pilot -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  "-DSIMDLIB_CATCH2_SOURCE=<existing Catch2 3 source checkout>" `
  "-DSIMDLIB_CODEGEN_LLVM_ROOT=$PWD/out/codegen-tools-provisioned"
cmake --build out/instruction-pilot --target CodegenPilot
ctest --test-dir out/instruction-pilot -L CODEGEN_CONTRACT -j 4 --output-on-failure
```

The same declarations can be enabled in a top-level development tree with
`SIMDLIB_BUILD_CODEGEN_PILOT=ON`. `CodegenPilot` and its fixture/receipt targets
have the `OPTIMIZED_CODEGEN` development owner and therefore join that profile's
existing artifact aggregate. The option defaults off during migration; existing
qualification gates retain their ownership. The standalone entry point exercises
the same helper with the public `SimdLib::Register` interface target and avoids
configuring unrelated runtime dependencies.

CTest never compiles fixtures. A missing build receipt, changed source/header,
object, selected executable, or configuration input fails test-only reuse.
Normal Ninja dependencies own recompilation. The conservative receipt additionally
binds the pilot's instruction rules and tool scripts. Its hashes establish input
freshness, never instruction acceptance. Build before testing after changes.

## Registration and results

The build and test responsibilities are separated:

- `simdlib_add_codegen_fixture`: compile one source with explicit definitions,
  options, geometry and configuration; produce a source/tool/configuration receipt.
- `CodegenPilot.cmake`: supplies explicit object/symbol/configuration metadata;
  operation-family C++ files own readable Catch2 test definitions.
- `CodegenCatch2.cmake`: discovers composite Catch2 results independently of
  whether runtime suites are enabled, assigning OPTIMIZED_CODEGEN ownership.
- `pilot/VerifyDiscovery.cmake` and `pilot/ExpectedRules.cpp`: independently
  validate the required case list and each test's observed rule applicability.
  Discovery is verified at build time, and missing or wrongly selected assertions
  fail the affected composite test.

Type, width, active extent, immediates, ISA, FMA, ABI and configuration travel in
the explicit geometry/configuration metadata and target definitions/options.
The pilot expands its shared declarations at widths 128 and 256 on MSVC,
clang-cl, GNU-style Clang and GCC 14. It does not clone a compiler's complete suite.
GCC 13's core-only scope does not include this Register pilot.

## Writing a case

The production transfer definition states its shared and additional facts directly:

```cpp
TEST_CASE_METHOD(CodegenFixture, "transfer.load_operate_store", "[codegen][register][transfer]")
{
    const auto function = inspect("simdlib_contract_transfer_load_operate_store");
    function.check("contracts/return.check");
    function.check("register/transfer.check");
    function.check_for(Compiler::MSVC, "contracts/msvc-cookie.check");
    function.check_for(Compiler::GCC, "contracts/no-call-stack.check");
    function.check_for(Compiler::Clang, "contracts/no-call-stack.check");
}
```

`Compiler::Clang` includes both Clang drivers. A `Scenario` can additionally name
an exact driver/configuration and numeric minimum/exclusive-upper version bounds
when a fact needs that narrower scope. Rule parameters such as register width,
mask immediate and ABI target come from the runner's explicit build metadata.
Add or change the independently authored required-rule ledger alongside an
intentional contract change; missing or extra assertions fail coverage.

## Composite result and artifacts

Each case/configuration has one CTest-discovered Catch2 result. Selecting that
case runs every primary and applicable supplemental rule. Nonfatal `check` and
`check_for` assertions collect all failures; inspection failure terminates the
affected case because there is no valid input to match. No separate `.qualified`
test or pass-marker protocol is required.

ABI groups list exact caller and callee symbols. Each receives its own extracted
body and applicable rule invocations. Microsoft vectorcall decoration is explicit
in the case declarations. One group result requires every listed member.

Artifacts live below `codegen-contracts/runner-<width>/artifacts/<unique-run>/`.
The fixture shares extraction within a process and revalidates inputs before
cache reuse. It binds cached text to the original build-receipt contents; a new
build snapshot requires restarting the test process. Separate CTest processes own
separate directories. Primary and
supplemental rules share one input per function; constant checks share a second
input when needed. FileCheck diagnostics report directly through Catch2.
The extraction receipt retains object and LLVM identities.
`configuration.txt`, `codegen-tools.txt`, `compile_commands.json`, `build.sha256`
and the authored expected-case/rule ledgers retain compilation, input and coverage evidence.
Failed checks retain their rules, diagnostics and full disassembly.

## Rule ownership

The shared Catch2 function assertions invoke each selected FileCheck file independently.
Captures are local to that invocation. Whole-body negatives use explicit
`CODEGEN-BEGIN`/`CODEGEN-END` sentinels around the unchanged LLVM output. Exact
counts exclude additional occurrences in every interval around positive matches.
Rules use LLVM's POSIX regular expressions; `\b` is not a supported substitute
for the explicit whitespace/register boundaries used here.

The optional mnemonic allowlist only identifies instruction lines in LLVM output
and checks their mnemonic membership. It does not decode bytes, select function
boundaries or normalize operands. Required instructions, counts, widths, immediate
values, operand relationships and targets remain FileCheck responsibilities.
Referenced masks are checked against the existing numeric-section LLVM hex dumps.

The pilot owns these limited facts:

| Production case | Primary | Additional facts |
| --- | --- | --- |
| Packed float addition | Exactly one full-width add of the two distinct input registers; register-only result | Clang drivers: no extra vector move |
| Zero | One zeroing operation; register-only result | None |
| Native identity | Return-only body, allowing alignment nops | None |
| Load/add/store | Full-width loads and store with captured intermediate/result registers; exactly one add | MSVC: bounded cookie sequence; others: no calls or stack traffic |
| Partial native import | Exactly one full-width projection and a returned value | Exact three/five-lane mask or blend, including constant contents; bounded MSVC cookie or stronger no-stack property |
| Aggregate binary ABI | Actual aggregate caller/callee, intended target, callee's distinct input/result registers | GCC: one call with a bounded alignment frame (32-byte realignment at width 256); others: stack-free tail jump |

The MSVC memory/import primaries express their common operation properties.
The mandatory cookie supplement checks the exact runtime target and bounded
frame; other drivers retain a mandatory no-call/no-stack supplement. No supplement
cancels a primary rule. GCC's provisional ABI call and the other compilers' tail
jump likewise have independent, bounded additional expectations.

Compilation uses explicit provisional `/O1 /Ob2 /Gy /arch:AVX2 /GS` or
`-O1 -ffunction-sections -mavx2 -mno-fma -fstack-protector-strong`, with `NDEBUG`
and FMA disabled. These are infrastructure probes, not the final optimization
policy or a claim that `/O1` and `-O1` are equivalent. Production code and its
attributes are unchanged. Observation boundaries preserve the established
explicit calling convention and partial-register public observation.

## Historical CMake-runner traceability

The table and execution receipts below describe the superseded outer runner.
They establish retained rule/extraction history, not Catch2 migration completion.
Current task-to-document mapping and evidence belong to
[Catch2Migration.md](Catch2Migration.md).

| Tasklist section 3 item | Controlling source and extracted requirement | Implementation / planned evidence |
| --- | --- | --- |
| Rule application | Proposal: Instruction contracts / Matching rules; MigrationInventory: Initial reusable contracts | FileCheck invocation, complete-body rules, mnemonic membership; positive and mutated rule harness |
| Primary and additive facts | Proposal: Primary cases and supplemental cases / Compiler-specific expectations | Independent invocations on shared extraction; deliberate failed-primary and failed-supplement CTest runs |
| Registration and matrices | MigrationInventory: Case identity and expansion / Execution matrices | Shared fixtures, explicit symbols, independent expected ledger; missing-registration and wrong-applicability harness |
| ABI groups and partial imports | Proposal: One production-code fixture; RegisterContract: aggregate boundary; PartialRegisterOperationLedger: native interoperation and zero suffix | Actual production signatures/imports; complete caller/callee and mask/blend evidence on each representative driver |
| Build-before-test and provenance | Proposal: CMake/CTest responsibilities; IntegrationInventory: Entry points/artifacts; BuildPipeline: Scoped CMake artifact graph; ContainerValidation: Build cells and artifacts | Normal object targets plus build receipt; missing/stale input regressions, incremental builds, parallel CTest and development ownership |
| Complete extraction and constants | FunctionExtraction: complete selection; proposal: Complete function inspection | Existing LLVM wrapper, shared read-only output, numeric constant dumps; extraction harness retained |
| Scope and support | RegisterQualification and PartialRegisterQualification supported matrices; RegisterImplementationMatrix ownership | Representative x64 driver evidence only; old gates and independent semantic/consumer owners preserved |

The implementation follows the [FileCheck manual](https://llvm.org/docs/CommandGuide/FileCheck.html)
and CTest fixture semantics. Tool/reference documentation informs implementation;
the repository proposal and tasklist control acceptance. Final validation and
independent audit results are recorded below only after execution.

## Historical CMake-runner validation

Receipts are retained locally under `out/codegen-contract-evidence`:

| Execution | Observed result / receipt |
| --- | --- |
| MSVC 19.44.35228, LLVM 22.1.8 | 44/44 pilot tests; `msvc-{build,ctest,qualified}.log` |
| clang-cl 22.1.8, LLVM 22.1.8 | 46/46 pilot tests; `clang-cl22-{build,ctest,qualified}.log` |
| GCC 14.2.0, LLVM 20.1.8 | 44/44 pilot tests; `gcc14/{build,ctest,qualified}.log` |
| GNU-style Clang 22.1.3, LLVM 22.1.3 | 46/46 pilot tests; `clang22/{build,ctest,qualified}.log` |
| Native top-level CUSTOM development integration | 49/49 selected tests, including five ownership checks; `integration-msvc-{build,ctest}.log` |
| Instruction, coverage and composition harness | 41/41; `rules-ctest.log` |
| Full native harness | 68/68, including 27 extraction checks and the 41 instruction/coverage/composition checks; `full-harness-ctest.log` |
| Incremental compilation | Two immediately repeated builds per driver performed no fixture compilation or receipt regeneration; `*-incremental-{1,2}.log` and container `incremental-{1,2}.log` |
| Additional independent rejection probes | Changed input/object and missing receipt rejected; neither a failed primary nor failed supplement qualified; `freshness-additive.log` |

Each compiler has eight object compilations (four source families at two widths),
shared by twelve primary cases and their applicable supplements. Parallel `-j 4`
CTest runs retain distinct output paths. Qualified-only selection automatically
ran the complete required fixture/check set. Compiler commands are retained in
each tree's `compile_commands.json`; configuration/tool identities and hashes are
in the corresponding fixture receipts. Native runs used the configured VS2022
environment. Linux runs used the provisioned GCC14/Clang22 extraction images and
the image's required C++/linker flags; `container-pilot.sh` retains the commands.

Second source/document review and the independent audit found and corrected
unchecked count intervals, insufficient input/zeroing operand constraints,
unbounded ABI surroundings, and target-prefix/suffix matches. Regression inputs
now exercise those failures. The final independent completion verdict is recorded
in the tasklist after review of the stable execution receipts.

This is representative x64 driver evidence, including clang-cl 22; it does not
claim a separate clang-cl 20 execution. No full CI, complete migrated corpus,
final optimization policy, runtime-semantic or installed-consumer qualification
is claimed by this pilot. Existing qualification and behavioral gates remain
unchanged.
