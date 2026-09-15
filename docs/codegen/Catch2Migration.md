# Catch2 instruction-runner migration

Status: implemented and validated for the representative pilot; independent
completion-audit verdict is recorded in the tasklist. Full corpus and final
qualification remain separate work.

## Controlling documentation and evidence plan

Repository-relative paths are used below. The tasklist's proposal spelling resolves
to `docs/RegisterCodegenPolicyProposal.md`. The revised proposal supersedes the
inventories' older scheduling details, while their case and semantic obligations
remain controlling. Historical CMake-runner results are not Catch2 results.

| Task | Consulted document / section | Applicable requirement | Planned / observed evidence |
| --- | --- | --- | --- |
| Reconcile ownership | RegisterCodegenPolicyProposal: readable definitions, component ownership; MigrationInventory: identity, matrices, supplemental ledger; IntegrationInventory: consumers | One primary definition, named additive selectors, independent expected coverage; preserve legacy gates | Updated inventories; independent discovery, missing-rule and wrong-applicability negatives pass |
| Fixture and assertions | Proposal: instruction contracts, matching, readable definitions; InstructionContracts: rule ownership and pilot facts | Nonfatal independent FileCheck invocations, exact counts/operands/targets, complete shared input, composite failure | 12 production cases and 39 harness checks pass per representative driver |
| Inspection and freshness | FunctionExtraction: inputs/outputs and complete selection; ToolProvisioning: explicit development tools; proposal: complete inspection | LLVM owns bounds/decoding/constants; fail missing/stale/unsupported inputs and tools | Full native harness 78/78, including retained 27 extraction checks; cache/missing/stale/rerecorded-input probes pass |
| Build/discovery/process integration | BuildPipeline: scoped CMake artifact graph; ContainerValidation: build cells/artifacts; IntegrationInventory: entry points | OPTIMIZED_CODEGEN ownership, build-before-test, no installed tooling dependency, explicit provenance and isolated output | Native CUSTOM 56/56, effective CTest property audit, filtered transfer checks, two no-op builds per driver and parallel CTest |
| Production boundaries | RegisterContract: layout/residency; PartialRegisterOperationLedger: native interoperation and evaluation; qualification documents: support/evidence ownership; RegisterImplementationMatrix: compiler/evidence matrix | Preserve actual ABI signatures, partial suffix projection, independent behavioral owners and driver boundaries | Production fixtures unchanged; both widths and constant/ABI requirements pass on MSVC, clang-cl, GCC and GNU-style Clang |
| Process reliability | Revised proposal: bounded shell-free process execution; Catch2Prototype: migration limits | Unicode paths, quoting, launch/read/exit failures, cleanup and timeouts on Windows/Linux | Path/argument, large-output, missing-tool, exit-status and timeout regressions pass; resource/read-error branches source-reviewed |
| Remove obsolete runner | IntegrationInventory: replacement responsibilities; revised proposal: source layout; tasklist removal items | Migrate all outer-script consumers before removal; preserve LLVM wrapper and old wrapper/raw gates | Retired scripts/prototype removed; no executable references remain; replacement harness passes |
| Review and completion | RegisterCodegenPolicy.todo section 3A and complete-todo-phase/audit-stage-completion skills | Second review followed by independent audit before completion markers | Review corrections documented below; final independent verdict is recorded in the tasklist |

The initial prototype evidence in `Catch2Prototype.md` informs the design only.
Compiler/version compatibility expansion, final optimization policy, corpus
migration, CI cutover and semantic/package qualification remain separate work.

## Implementation structure

- `register/*.tests.cpp` and `partial-register/*.tests.cpp` own readable case
  definitions. Primary `check` calls always execute; named `check_for` calls add
  compiler facts without canceling shared rules.
- `support/CodegenFixture` resolves explicit build-declared symbols and records
  observed facts. `pilot/ExpectedRules.cpp` independently requires the complete
  symbol/rule set, including constants and mnemonic restrictions.
- `pilot/ExpectedCases.cmake` supplies the independent finite case inventory.
  Build-time executable listing and deferred CTest discovery checks both compare
  against it, including when CTest selects only one test.
- `support/Inspection` invokes the retained LLVM wrapper and verifies source,
  object, configuration and tool receipts before initial or cached inspection.
  Cache identity includes object path, manifest, receipt, emitted symbol,
  constants and configuration. Cached text also binds the receipt contents from
  extraction; regenerating a valid receipt for changed inputs cannot reuse old
  disassembly. Separate processes do not share this cache.
- `support/FileCheck` launches independent matches and owns the narrow mnemonic
  membership check. `support/Process` owns platform encoding, pipes, child cleanup
  and deadlines; it does not invoke a shell.

Artifacts are retained under each runner's build-owned `artifacts` directory,
with an atomically reserved subdirectory per inspection. No automatic deletion
occurs during parallel testing. They can be removed with the disposable build
tree when its evidence is no longer needed; installed consumers own none of it.
FileCheck output is captured in the Catch2/CTest report rather than a pass-marker
protocol. Build metadata still records compiler/options/tool identities.

## Validation evidence

Final receipts are retained locally under `out/codegen-catch2-migration`:

| Execution | Result | Receipt |
| --- | --- | --- |
| MSVC 19.44.35228 | 51/51 | `msvc-verified-ctest.log` |
| clang-cl 22.1.8 | 51/51 | `clang-cl22-verified-ctest.log` |
| GCC 14.2.0 / Linux | 51/51 | `gcc14/ctest.log` |
| Clang 22.1.3 / Linux | 51/51 | `clang22/ctest.log` |
| Native full extraction + instruction harness | 78/78 | `extraction-msvc-verified-ctest.log` |
| Native top-level CUSTOM integration | 56/56, including five ownership checks | `integration-ctest.log` |

Each driver passed two unchanged builds without fixture compilation or receipt
regeneration, and filtered transfer execution still ran its complete primary and
supplemental contract. Effective CTest properties carry the OPTIMIZED_CODEGEN
owner on all 51 tests, with 180-second limits on the 19 discovered tests and
90-second limits on 32 explicit synthetic-rule tests. Inspection and rule tools
retain their own bounded subprocess execution.

The native trees use explicit provisioned LLVM 22.1.8 tools; GCC14 and Clang22
use their existing LLVM 20.1.8 and 22.1.3 tool environments. Cached Catch2 3.8.1
supplies the test framework. This is representative x64 pilot evidence; it does
not claim clang-cl 20, full CI, the complete migrated corpus, final optimization
policy, or new behavioral/package qualification.

## Second source and documentation review

The second review compared the implemented pilot with the revised proposal,
the required-fact table in InstructionContracts, and the independent inventories.
It checked all seven emitted function identities at both widths, including ABI
decoration and partial-import constants; additive rule continuation; discovery
and rule coverage; configuration binding; process resource/error paths; and
retirement references. `git diff --check` passes, and no executable reference to
the retired outer-runner scripts or registration APIs remains under `cmake` or
`tests`.

Review corrections include owning cached object metadata rather than retaining
temporary string pointers, avoiding Windows CRT header shadowing, numeric
version/driver/configuration selector bounds, UTF-8 source and executable paths,
constant-suffix rejection coverage, and preserving core-only extraction mode.
Production implementations and the legacy wrapper/raw gates remain untouched.
The independent audit reads the final receipts and source corrections before
the tasklist's completion markers are changed.

The independent audit identified a cached-snapshot gap: a valid regenerated
receipt could previously admit an old cached body. The implementation now rejects
that reuse, and the regression covers changed configuration and changed object
with regenerated receipts, followed by successful restoration. A malformed
mnemonic rule also now reports nonfatally and the continuation test verifies that
later checks execute. The timeout probe allows two seconds for child startup and
bounded execution, avoiding a 200 ms startup assumption under concurrent builds.
The auditor's source re-review found these issues resolved, and the final
execution receipts include both regression probes.

Generated CTest inspection additionally exposed Catch2's flattening of a
semicolon-separated label value passed through `PROPERTIES`, losing both the
owner label and intended timeout. The shared discovery helper now applies quoted
labels and timeout after discovery, matching the existing runtime-test pattern.
Final evidence inspects effective CTest properties rather than inferring
ownership from the authored helper arguments; the corrected properties and
top-level ownership checks pass.
