# Instruction-contract integration ownership

Companion to [MigrationInventory.md](MigrationInventory.md). Paths are relative
to the repository root. This inventory describes source-reviewed integration
obligations; it does not claim tool provisioning or compiled validation.

## Legacy consumers and replacement responsibilities

| Existing owner | Consumed or produced data | Replacement disposition |
| --- | --- | --- |
| `cmake/development/RegisterCodegen.cmake` | Paired expression/type/specialized/FMA/rearrangement/ABI objects; comparison records; immediate-shift stamp; all/enforced/diagnostic indexes | Thin primary/supplemental declarations through `CodegenTests.cmake`; preserve aggregate target and build-before-test integration until cutover |
| `cmake/development/PartialRegisterCodegen.cmake` | Paired predicate/value/arithmetic/general/ABI/API-transfer objects; profile selection; individual records and validators | Preserve all P/A entries and owners in the case inventory; replace profile selection with additive facts after validated migration |
| `cmake/development/PartialRegisterCodegenProfiles.json` | Sixteen retained hash entries | Retire only after every affected configuration/function has validated replacement facts; hashes are not new instruction policy |
| `cmake/development/MethodFlagsCodegen.cmake` | Flagged/raw objects, parity record, verification text, all-records index | Preserve distinct core/compiler-attribute owner; reuse shared extraction/check registration |
| `cmake/CompareRegisterCodegen.cmake` | Disassembly, normalized profiles, symbol/difference sidecars, comparison record, timing and tool provenance | Retire lossy normalization/equality/exception logic; `ExtractFunction.cmake` owns complete functions; `CheckInstructions.cmake` and FileCheck own assertions |
| `cmake/RecordRegisterDefaultAbi.cmake` | Default-convention paired disassembly and diagnostic record | Preserve platform-default observation and provenance under B groups; no Windows equality claim |
| `cmake/VerifyCompleteRegisterShiftCodegen.cmake` | Raw object -> instruction stamp | Move its exact instruction requirements to production shift cases; parity plus raw checks currently own these jointly |
| `cmake/VerifyMethodFlagsCodegen.cmake` | Both objects -> no-cookie/no-leaf-call verification | Move properties to flagged fixture contracts; retain stack-policy validation |
| `cmake/ValidateCodegenRecords.cmake` | Record/index existence, status, hashes, policy/configuration checks | Adapt freshness/result validation to new per-case records; do not delete provenance validation with instruction hashes |
| `cmake/ValidateRegisterCodegenProfile.cmake` | Enforced and diagnostic indexes -> legacy validator | Replace with exact expected primary/supplemental/configuration result coverage; observations cannot fill missing enforced results |
| `cmake/VerifyMethodFlagsCodegenRecords.cmake` | Method record index plus verification text | Replace only when equivalent shared case results and attribute evidence are selected |
| `cmake/VerifyCodegenPolicySeparation.cmake` | Synthetic legacy records -> validator acceptance/rejection | Migrate policy/freshness failure tests to the new schema |
| `cmake/VerifyCodegenProfileIsolation.cmake` | Target names and register/method artifact roots | Update names/roots while retaining OFF/diagnostic isolation; include new partial/API roots |
| `cmake/SummarizeCodegenDiagnostic.cmake` | RECORD-only index, tool data, flags, source/fingerprint/timing -> JSON | Adapt optional diagnostic summary; known mixed immediate-shift enforcement must not be mislabeled a RECORD-only success |
| `tools/Run-NativeMatrix.ps1` | Four hardcoded register/method `all-records.txt` paths, `provenance/codegen-records.index`, validation-build manifests and diagnostic JSON | Replace index collection/validation/summary together; current collection does not independently enumerate partial/API record roots |
| `containers/container-entrypoint.sh` | Same four indexes; `ValidateCodegenRecords.cmake`; diagnostic summarizer; manifest hashes | Same migration obligation for Linux; tests stay compile-free |
| `tools/Record-Codegen.ps1`, `tools/Run-ContainerMatrix.ps1`, `tools/Run-WindowsClang20Container.ps1` | Route selected diagnostic/compiler work to the runners | Preserve explicit selection, ownership, result aggregation, and failure propagation |
| `tools/validation-matrix.json`, `tools/Pipeline.Common.psm1`, `tools/Test-ValidationPipeline.ps1` | ENFORCE/OFF/RECORD cell policy, receipts and regression fixtures | Integrate new configurations intentionally; keep source-bound manifests and policy separation |
| `cmake/development/{Development,Options,ArtifactOwnership,ArtifactAggregates}.cmake`, `CMakePresets.json` | Suite inclusion, gate options, selected categories and aggregate dependencies | Add shared helper with directional dependencies; no installed/runtime FileCheck dependency |
| `cmake/{AuditValidationInventory,VerifyArtifactAggregateInventory,VerifyArtifactAggregateFailure,RecordTestInventory}.cmake`, `tools/{Audit-ValidationMatrix,Verify-ValidationMatrix}.ps1` | Exact target/test ownership and aggregate membership | Reconcile new per-case CTest inventory and failure behavior; no compiler-specific primary duplicates |
| `docs/RegisterQualification.md`, `docs/PartialRegisterQualification.md`, `docs/RegisterImplementationMatrix.md`, `docs/RegisterContract.md`, `README.md` | Current equality claims, exceptions, ownership and reproduction | Update at cutover to properties actually enforced; retain semantic/layout/availability contracts |

Raw translation units to retire after mapped replacements pass:
`RegisterCodegenRaw.cpp`, `RegisterTypeMatrixCodegenRaw.cpp`,
`RegisterSpecializedCodegenRaw.cpp`, `RegisterFmaCodegenRaw.cpp`,
`RegisterRearrangementCodegenRaw.cpp`, `RegisterAbiRaw.cpp`,
`RegisterDefaultAbiRaw.cpp`, `PartialRegisterMaskCodegenRaw.cpp`,
`PartialRegisterArithmeticCodegenRaw.cpp`, `PartialRegisterGeneralCodegenRaw.cpp`,
`PartialRegisterAbiRaw.cpp`, `ApiPartialTransferCodegenRaw.cpp` under
`tests/codegen`, and `tests/method_flags/codegen/MethodFlagsRaw.cpp`.
Shared fixture headers' raw branches retire with those sources. This list does
not authorize deletion of independent runtime scalar oracles or benchmarks.

## Entry points, artifacts, and provisioning

[BuildPipeline.md](../BuildPipeline.md#scoped-cmake-artifact-graph) and
[ContainerValidation.md](../ContainerValidation.md#build-cells-and-artifacts)
control invocation and provenance. `tools/Build.ps1 -Scope Native|Containers`
builds selected compiler artifacts; `tools/Run-Tests.ps1` consumes the matching
receipt without configuring or compiling. `ExhaustiveArtifacts` includes
`SimdLibOptimizedCodegenArtifacts` in its owning Release profile.
`RegisterCodegen`, `RegisterExpressionCodegen<profile>`,
`RegisterConsumerAbi<profile>`, partial/API convenience targets, and
`MethodFlagsCodegen` remain migration entry points. New tests check prebuilt
objects using ordinary CMake header/source dependencies. Benchmarks are separate
and cannot qualify codegen.

Existing roots beneath each fingerprint-owned build tree:

- `register-codegen/{sse42/128,avx2/128,avx2/256}`;
- `partial-register-codegen/{sse42/128,avx2/256}`;
- `api-partial-transfer-codegen/{sse42/128,avx2/256}`;
- `method-flags-codegen`.

Preserve source digest, compiler executable/version, target, generator, effective
options, object identity, configuration, checks/instrumentation, FMA, ABI and
stack policy, selected contracts, exact function identity, tool executable/version,
and results. Tool/input/artifact hashes may prove freshness; instruction hashes
must not decide acceptability. New outputs are isolated by case, geometry,
configuration and primary/supplemental fact. Shared objects remain read-only.
Retain full disassembly, extracted bodies, FileCheck stdout/stderr and result
metadata, including failures and elapsed times. Build receipts, JUnit reports,
`fingerprint.json`, manifests and `codegen-records.index` live under
`out/pipeline/<platform>-<compiler>/<configuration>-<fingerprint>`.

| Provisioning/upload owner | Current source evidence / required integration |
| --- | --- |
| `.github/workflows/ci.yml` native MSVC | Runner-provided VS/LLVM environment; upload is `if: always()` and currently includes register and method JSON/TXT, not partial/API roots. Add complete replacement diagnostics. |
| Same workflow clang-cl 20 | Dedicated Windows container; upload includes partial/register/method roots but omits API root. Preserve compatibility-floor evidence. |
| Same workflow newer clang-cl/coverage | Chocolatey CMake 4.4.0 and LLVM 22.1.7; uploads partial/register/method data. Coverage is runtime evidence, not another primary codegen owner. |
| Same workflow Linux matrix | Compiler-specific jobs and Buildx cache; uploads register/method JSON/TXT, omits partial/API roots. Add all replacement roots and FileCheck outputs, including non-JSON/TXT artifacts. |
| `.github/workflows/container-reproducibility.yml` | Always-upload environment evidence from uncached inspection; tool provisioning changes belong in environment identity. |
| `containers/Dockerfile.{gcc13,gcc14,clang22}`, `compose.yml` | Pinned Alpine toolchains and shared local/CI environment; GCC 13 core-only, GCC 14.2.0 and Clang 22.1.3 Register cells. Select FileCheck/disassembler packages and versions deliberately. |
| `containers/Dockerfile.windows-clang20` | LLVM 20.1.8, CMake 3.31.6, VS Build Tools, Windows SDK/runtime; verify actual FileCheck availability instead of assuming the LLVM installer contains it. |
| Local native tooling and `docs/BuildPipeline.md` prerequisites | Explicit tool paths and version checks are required by replacement design; current CMAKE_OBJDUMP/PATH discovery does not prove reproducible selection. |

There is no new Python or lit dependency. FileCheck/disassembler selection and
COFF/ELF extraction remain unqualified until tool/extraction work succeeds.
Installed library/package consumers must not acquire these development tools.

## Named remaining decisions

These are required downstream implementation tasks, not missing inventory rows:

| Decision | Tasklist owner | Required evidence |
| --- | --- | --- |
| Exact tool versions, distribution, explicit paths, COFF/ELF symbol/boundary/relocation handling | 2: provisioning and complete extraction | Real objects for each supported driver plus positive/negative extraction harness |
| Exact registration interface, expanded expected symbol lists, FileCheck invocation and per-case artifact schema | 3: shared contracts/CTest | Primary/supplemental composition failures, coverage and stale/missing-object rejection |
| Low optimization, production companion flags, wholly unoptimized checked/observational scope, checks/sanitizer policy | 4: optimization/configuration | Representative matrix evidence; no inferred equivalence of `/O1` and `-O1` |
| Exact per-symbol instruction bounds and stronger supplemental properties from existing profile variations | 5/6: corpus migration | Real complete-body inspection and negative variants, before any legacy profile deletion |
| Bind the positional MSVC array-cookie recognizer to actual emitted identities | 5: Register/type-matrix migration | Exact legacy symbol/object mapping and bounded target-aware new contract |
| Preserve API excluded-array observation and adjacent attribute contracts | 6: adjacent consumer migration | Explicit result ownership and observation/enforcement disposition |
| Update runners, indexes, receipts, CI upload paths, qualification wording, and remove obsolete raw/profile machinery | 7: cutover | All applicable replacements selected and passing, retirement/reference audit |
| Full supported qualification and external consumers | 8: final qualification | Native and pinned container receipts, distinct from source review |

## Documentation traceability and review evidence

| Tasklist section 1 task | Consulted source / applicable requirement | Evidence in this inventory |
| --- | --- | --- |
| Read qualification/support boundaries | RegisterQualification: supported matrix, record ownership, exceptions; PartialRegisterQualification: supported matrix, generated ownership/policy; RegisterImplementationMatrix: availability and evidence ownership | Matrix R/P/A/M, existing-versus-proposed claims, independent semantic owners |
| Inventory fixtures/records/consumers | Three development suite CMake files; every wrapper fixture family; shift and method validators; comparator/profile JSON; runners and CI | Closed suffix/type expansions, record and validation owners, consumer/retirement tables |
| Durable replacement mapping | Proposal: one production fixture, coverage, primary/supplemental ownership | Stable family/suffix IDs, symbol and geometry rules, contracts, matrices and supplemental ledger |
| Availability and emitted identity | RegisterContract: layout/residency; ApiOperationMatrix; PartialRegisterOperationLedger: representation, evaluation, operation/result rules | Expected symbols derived before disassembly; valid minimal bodies; ABI groups; required invariant work |
| Initial contracts | Proposal: instruction contracts, matching, complete inspection, compiler-specific expectations | R/L/I/F/B/G/P families; exact legacy shift/attribute facts; bounded /GS obligations |
| Entry points/artifacts/provisioning | BuildPipeline scoped graph/manifests; ContainerValidation environment/artifacts; workflows, Dockerfiles, matrix and runners | Explicit integration owners, missing upload roots, provenance and downstream decisions |

The tasklist's source-proposal spelling `docs/RegisterCodegenPolicyProposal.md`
is repository-relative although the tasklist itself is in `docs`; the controlling
file was resolved as `docs/RegisterCodegenPolicyProposal.md`, not nonexistent
`docs/docs/RegisterCodegenPolicyProposal.md`. Qualification documents resolve next
to the tasklist. The proposal's external manuals are reference material for
implementation tooling, not controlling evidence for this source inventory.

Source-review commands included `Get-Content` on the documents/fixtures/CMake
owners and `rg` for symbols, macro instantiations, comparator/profile/index
consumers and CI/provisioning paths. Inventory creation runs no compiler,
disassembler, CMake build, CTest, runtime test or consumer. The resulting evidence
is source and documentation review only. Completion review and audit outcomes are
recorded in the planning tasklist after they occur.
