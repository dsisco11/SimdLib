# Permanent Generated-Code Suite Audit

This document defines the ownership and retention policy for SimdLib's permanent
generated-code fixtures. The machine-readable, per-symbol decision ledger is
[`RegisterCodegenSymbolAudit.csv`](RegisterCodegenSymbolAudit.csv).

## Contract categories

Every retained symbol belongs to exactly one category:

| Category | Permanent observable contract |
|---|---|
| Public abstraction parity | A public `Register` operation adds no work relative to the matching public `Api` operation. |
| ABI boundary | A non-inlined `Register`, `RegisterMask`, explicit-object mirror, or native-vector signature preserves the documented calling boundary. |
| Compiler-attribute enforcement | `SIMD_FLAGS(...)` and the legacy declaration attributes produce the same ABI and generated code, including inlining and stack restrictions. |
| Instruction-property guarantee | A feature mode or immediate form retains a required instruction property, such as fused multiply-add presence or absence. |
| Composed-expression optimization | Multiple public operations optimize as one expression without wrapper temporaries or repeated work. |
| Register-pressure behavior | Simultaneously live values and opaque calls do not introduce wrapper-specific spills or reloads. |
| Explicitly diagnostic evidence | The artifact records compiler behavior but is excluded from zero-overhead pass/fail claims. |

The ledger has one row for each source-level fixture symbol. Force-inline and
flatten helper symbols are intentionally absent from optimized objects. Reuse of
a symbol name across width or ISA configurations is represented by its `applicability`
field. Feature-mode comparisons that deliberately compile the same symbol twice,
such as FMA enabled and disabled, identify both records in that row.

## Retained symbol ownership

| Owning fixture | Symbols | Category coverage | Distinct purpose |
|---|---:|---|---|
| `RegisterCodegenFixture.h` | 23 | Public parity, composition, instruction property, register pressure | Protects expression and lifetime behavior that an isolated operation cannot represent. |
| `RegisterTypeMatrixCodegenFixture.h` | 442 | Public parity | Canonical isolated operation matrix over every supported element type, register width, and ISA profile. |
| `RegisterSpecializedCodegenFixture.h` | 138 | Public parity | Covers specialized arithmetic and reduction methods that are absent from the basic type matrix. |
| `RegisterFmaCodegenFixture.h` | 2 | Instruction property | Isolates the two multiply-add symbols so FMA presence and absence cannot be satisfied by unrelated code. |
| `RegisterRearrangementCodegenFixture.h` | 181 | Public parity | Covers immediate selectors, complete-register shuffles, bit casts, numeric conversions, lower halves, and widening cells. |
| `RegisterAbi.cpp` | 12 | ABI boundary | Separates explicit-object signature mirrors from real downstream `Register` and `RegisterMask` boundaries. |
| `RegisterDefaultAbi.cpp` | 1 | Explicitly diagnostic evidence | Records the platform-default aggregate convention without treating it as a supported zero-overhead boundary. |
| `MethodFlagsFlagged.cpp` | 11 | Compiler-attribute enforcement | Compares `SIMD_FLAGS(...)` with equivalent legacy attributes and checks inlining and stack restrictions. |

The total is 810 retained source-level symbols. The CSV ledger is authoritative
for individual decisions; the table above is only a fixture summary.

## Raw-baseline policy

Public zero-overhead fixtures compare `Register` with the narrowest equivalent
public `Api` expression. A raw translation unit must not call `Register`, an
implementation specialization, or an extension helper. Sharing the production
implementation beneath the two public layers is intentional: the independent
boundary under test is the `Register` abstraction itself.

ABI fixtures instead compare aggregate signatures with native-vector signatures.
Method-flag fixtures compare `SIMD_FLAGS(...)` declarations with equivalent
legacy attribute declarations. The platform-default ABI fixture is a paired
diagnostic recording rather than an equality gate.

## Comparison records and owning validation

Each record appears exactly once in its profile's generated
`all-records.txt`. The profile also writes disjoint `enforced-records.txt` and
`diagnostic-records.txt` indexes. Release validation requires every enforced
record to report `ENFORCE`; a record-only result can appear only in the
diagnostic index and cannot satisfy that gate. `RegisterExpressionCodegen<profile>`
and `RegisterConsumerAbi<profile>` are build-only orchestration targets and do
not own validation.

| Record | Symbol selection | Wrapper input | Raw input | Owning validation |
|---|---|---|---|---|
| `primary-composition` | Memory-capable and composed primary symbols | `RegisterCodegen.cpp` | `RegisterCodegenRaw.cpp` | `RegisterCodegen.<profile>` |
| `register-only` | Register-only primary symbols | `RegisterCodegen.cpp` | `RegisterCodegenRaw.cpp` | `RegisterCodegen.<profile>` |
| `reassignment` | Ordinary reassignment arithmetic | `RegisterCodegen.cpp` | `RegisterCodegenRaw.cpp` | `RegisterCodegen.<profile>` |
| `specialized` | All FMA-independent specialized symbols | `RegisterSpecializedCodegen.cpp` | `RegisterSpecializedCodegenRaw.cpp` | `RegisterCodegen.<profile>` |
| `fma-disabled` | `multiply_add_f32` and `multiply_add_f64` | `RegisterFmaCodegen.cpp` with FMA disabled | `RegisterFmaCodegenRaw.cpp` with FMA disabled | `RegisterCodegen.<profile>` |
| `fma-enabled` | `multiply_add_f32` and `multiply_add_f64` | `RegisterFmaCodegen.cpp` with FMA enabled | `RegisterFmaCodegenRaw.cpp` with FMA enabled | AVX2 `RegisterCodegen.<profile>` |
| `rearrangement-conversion` | All applicable rearrangement symbols | `RegisterRearrangementCodegen.cpp` | `RegisterRearrangementCodegenRaw.cpp` | `RegisterCodegen.<profile>` |
| `common-type-matrix` | All applicable non-modulus type-matrix symbols | `RegisterTypeMatrixCodegen.cpp` | `RegisterTypeMatrixCodegenRaw.cpp` | `RegisterCodegen.<profile>` |
| `modulus-type-matrix` | Integer modulus symbols | `RegisterTypeMatrixCodegen.cpp` | `RegisterTypeMatrixCodegenRaw.cpp` | `RegisterCodegen.<profile>` |
| `abi` | Explicit-object ABI mirrors | `RegisterAbi.cpp` | `RegisterAbiRaw.cpp` | `RegisterCodegen.<profile>` |
| `consumer-abi` | Real downstream Register and RegisterMask boundaries | `RegisterAbi.cpp` | `RegisterAbiRaw.cpp` | `RegisterCodegen.<profile>` |
| `default-abi` | Platform-default aggregate boundary | `RegisterDefaultAbi.cpp` | `RegisterDefaultAbiRaw.cpp` | `RegisterCodegen.<profile>` |
| `method-flags` | `SIMD_FLAGS(...)` declaration fixtures | `MethodFlagsFlagged.cpp` | `MethodFlagsRaw.cpp` | `MethodFlagsCodegen` |

SSE4.2/128 owns 11 Register records because it has no FMA-enabled record.
AVX2/128 and AVX2/256 each own 12. The method-flags comparison is owned by its
single configuration-probe validation.

Unified native and container runners aggregate only these CMake-owned indexes.
They do not recursively discover residual JSON files in reused build trees, so
retired artifacts cannot acquire validation ownership. Ordinary Debug,
sanitizer, and coverage profiles configure no Register codegen targets or
indexes. Explicit diagnostic profiles contain only record-only codegen targets.

## Source and build inventory

| Fixture family | Complete source inventory |
|---|---|
| Primary | `tests/codegen/RegisterCodegen.cpp`, `RegisterCodegenRaw.cpp`, and `RegisterCodegenFixture.h` |
| Specialized | `tests/codegen/RegisterSpecializedCodegen.cpp`, `RegisterSpecializedCodegenRaw.cpp`, and `RegisterSpecializedCodegenFixture.h` |
| FMA | `tests/codegen/RegisterFmaCodegen.cpp`, `RegisterFmaCodegenRaw.cpp`, and `RegisterFmaCodegenFixture.h` |
| Rearrangement | `tests/codegen/RegisterRearrangementCodegen.cpp`, `RegisterRearrangementCodegenRaw.cpp`, and `RegisterRearrangementCodegenFixture.h` |
| Type matrix | `tests/codegen/RegisterTypeMatrixCodegen.cpp`, `RegisterTypeMatrixCodegenRaw.cpp`, and `RegisterTypeMatrixCodegenFixture.h` |
| Explicit-object and consumer ABI | `tests/codegen/RegisterAbi.cpp` and `RegisterAbiRaw.cpp` |
| Platform-default ABI | `tests/codegen/RegisterDefaultAbi.cpp` and `RegisterDefaultAbiRaw.cpp` |
| Method attributes | `tests/method_flags/codegen/MethodFlagsFlagged.cpp` and `MethodFlagsRaw.cpp` |

`cmake/development/RegisterCodegen.cmake` owns the per-profile object targets,
records, aggregate build targets, policy-separated record indexes, and three
Register CTests.
`cmake/development/MethodFlagsCodegen.cmake` owns the method-flags pair and its
CTest. `CompareRegisterCodegen.cmake`, `RecordRegisterDefaultAbi.cmake`,
`ValidateCodegenRecords.cmake`, `ValidateRegisterCodegenProfile.cmake`,
`VerifyCodegenProfileIsolation.cmake`, `VerifyMethodFlagsCodegen.cmake`, and
`VerifyMethodFlagsCodegenRecords.cmake` are the complete comparison, diagnostic,
record-integrity, profile-isolation, and attribute-verification script inputs.

Every `<profile>` suffix is one of `128Sse42`, `128Avx2`, or `256Avx2`:

| Target family | Complete generated target inventory |
|---|---|
| Primary objects | `RegisterCodegenWrapper<profile>`, `RegisterCodegenRaw<profile>` |
| Default ABI objects | `RegisterDefaultAbiWrapper<profile>`, `RegisterDefaultAbiRaw<profile>` |
| Explicit-object and consumer ABI objects | `RegisterAbiWrapper<profile>`, `RegisterAbiRaw<profile>` |
| Specialized objects | `RegisterSpecializedWrapper<profile>`, `RegisterSpecializedRaw<profile>` |
| FMA-disabled objects | `RegisterFmaDisabledWrapper<profile>`, `RegisterFmaDisabledRaw<profile>` |
| FMA-enabled objects | `RegisterFmaEnabledWrapper<profile>`, `RegisterFmaEnabledRaw<profile>` for AVX2 profiles |
| Rearrangement objects | `RegisterRearrangementWrapper<profile>`, `RegisterRearrangementRaw<profile>` |
| Type-matrix objects | `RegisterTypeMatrixWrapper<profile>`, `RegisterTypeMatrixRaw<profile>` |
| Register orchestration | `RegisterExpressionCodegen<profile>`, `RegisterConsumerAbi<profile>`, `RegisterCodegen<profile>`, and `RegisterCodegen` |
| Method attributes | `MethodFlagsCodegenFlagged`, `MethodFlagsCodegenLegacy`, and `MethodFlagsCodegen` |

The orchestration targets do not define additional contracts. The complete CTest
inventory is `RegisterCodegen.128Sse42`, `RegisterCodegen.128Avx2`,
`RegisterCodegen.256Avx2`, and `MethodFlagsCodegen`.
## Artifact and documentation inventory

Register artifacts live below:

- `register-codegen/sse42/128`;
- `register-codegen/avx2/128`; and
- `register-codegen/avx2/256`.

Method-attribute artifacts live below `method-flags-codegen`. Default pipeline
publication uses Release roots. `tools/Record-Codegen.ps1` creates a separate
selected Debug or Clang sanitizer record set and dedicated provenance containing
compiler flags, stack-protector mode, disassembly tools, source identity, record
hashes, and separate compilation and comparison timings.

Documentation references have these roles:

| Documentation | Role |
|---|---|
| `RegisterQualification.md` | Supported compiler/profile matrix, enforcement policy, and diagnostic exception ledger. |
| `RegisterProposal.md` | Public zero-overhead and ABI requirements. |
| `RegisterImplementationMatrix.md` | Public-operation-to-generated-code traceability. |
| `MethodFlagsContract.md` | Compiler-attribute promises, compiler mappings, and extension policy. |
| `BuildPipeline.md` and `ContainerValidation.md` | Reproduction commands and execution-reporting boundaries. |
| `MethodFlagsSourceAudit.md` | Canonical method-flags declaration policy and repository source-audit ownership. |
| `SimdLibDevelopment.todo`, `TestCoverageExpansion.todo`, and `project.todo` | Active planning and project backlog; not normative pass claims. |
| `README.md` and `wiki/Technical-Reference.md` | User-facing support and performance guidance. |

## Removed redundant fixtures

| Removed fixture or symbol family | Redundancy reason |
|---|---|
| `LogicalShuffleCodegenRaw.cpp` and `LogicalShuffleIntrinsic` | Reimplemented the intrinsic algorithm; public `Register::shuffle` versus public `Api::shuffle` is the permanent boundary. |
| Handwritten scalar remainder baselines | Duplicated the selected production algorithm; algorithm comparison belongs in execution evidence or benchmarks. |
| Direct type-matrix implementation-layer runtime extract/insert symbols | Compared `Api` with its implementation rather than testing a public `Register` contract. |
| Primary isolated unary, binary, scalar, mask, construction, transfer, arithmetic, sign-bit, and runtime per-lane shift symbols | Duplicated canonical isolated type-matrix cells. |
| Uninstantiated aggregate `evaluate` and `transfer` helpers | Emitted no permanent contract and added fixture complexity. |
| Specialized fixtures rebuilt under both FMA modes | Revalidated FMA-independent symbols; only isolated multiply-add cells require the mode split. |
| Overlapping lane and full-primary comparison records | Revalidated symbols already owned by narrower nonoverlapping records. |
| Identity-return fixtures for unavailable operation/type cells | Produced code without a supported public operation and could hide availability mistakes. |

Temporary candidate-implementation comparisons are not permanent fixtures.
Reusable throughput or latency investigations belong in benchmarks; one-time
compiler decisions belong in execution reporting.
