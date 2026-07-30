# Validation matrix ownership

This document defines the accepted ownership of SimdLib validation work. It is
the design contract for the validation-matrix deduplication work; it does not
claim that every current preset already implements this distribution.

The user-facing workflow remains unified:

- `Build` produces every artifact required by the default validation matrix
  across the selected compiler scope.
- `Run-Tests` consumes the matching completed build receipt without building.
- Benchmarks and investigative diagnostics remain explicit supplemental
  operations because they are not default correctness gates.

Splitting the internal build graph into scoped aggregates does not split the
pipeline. The scoped aggregates prevent a cell from compiling evidence owned
by another cell while the top-level command continues to orchestrate all
required cells.

## Validation categories

Every development target and CTest identity has exactly one category owner.
Instrumented instances retain their functional category; sanitizer and coverage
describe the profile in which that instance is compiled and executed.

| Category | Contract |
| --- | --- |
| Production/support aggregate | Header-only public targets, warning policy, and aggregate targets that organize work but emit no independent validation evidence |
| Repository audit | Source-text invariants that are independent of compiler, configuration, ISA, and instrumentation |
| Compiler-front-end contract | Header isolation, preprocessing, language availability, configuration adapters, negative compilation, representation, and declaration/ABI compatibility |
| Compile-time contract | Constant-evaluation assertions and compile-only constexpr artifacts |
| Runtime correctness | Behavioral, oracle, equivalence, feature-path, and operation-matrix execution |
| Checks/preconditions | Explicit checks-enabled observation and isolated expected-failure processes |
| Smoke/ODR/example | Public examples, umbrella/header-only smoke tests, and multi-translation-unit ODR checks |
| External consumer | Separate-project `add_subdirectory`, usage-requirement, language-mode, ABI-boundary, and option/target-isolation checks |
| Optimized codegen/ABI | Mandatory optimized Release wrapper/raw, expression, specialized-operation, and ABI comparison |
| Optional diagnostic codegen | Record-only Debug, sanitizer, or investigation-specific disassembly that cannot satisfy an optimized gate |
| Coverage | Profile reset, execution data, merge, report generation, and coverage provenance |
| Sanitizer | ASan+UBSan instrumentation applied to runtime and selected consumer contracts; it is not a generated-code category |
| Benchmark | Supplemental Release-only benchmark compilation and execution |

## Accepted default matrix

The following cells compose the future default `Build`. “Full runtime” means
the runtime correctness and explicit checks/precondition categories applicable
to that compiler's supported surface.

| Cell | Unique default contract | Compiler contracts | Constexpr | Runtime | Smoke/ODR/examples | Consumer | Codegen | Instrumentation |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| MSVC Release | Windows MSVC optimizer, ISA mappings, `VECTORCALL`, Release ABI, and zero-overhead qualification | yes | yes | full | yes | core+Register | enforce | none |
| MSVC Debug | Representative ordinary Debug behavior, default checks/preconditions, Windows Debug runtime, and Debug consumer use | narrow Debug-state probe only | no | full | no | core+Register | off | none |
| clang-cl Release | Windows Clang frontend/optimizer, MSVC-style driver, `VECTORCALL`, and Release ABI | yes | yes | full | yes | core+Register | enforce | none |
| GCC 13 core Release | C++20 core compatibility floor and unavailable-Register contract | yes | core only | core only | core only | core only | unavailable | none |
| GCC 14 Release | GNU optimizer, core/Register language surface, GNU ABI, and zero-overhead qualification | yes | yes | full | yes | core+Register | enforce | none |
| Clang 22 Release | GNU-like Clang optimizer, core/Register language surface, GNU ABI, and zero-overhead qualification | yes | yes | full | yes | core+Register | enforce | none |
| Clang 22 ASan+UBSan Debug | Instrumented Linux runtime correctness and cross-translation-unit consumer boundary | no | no | full | no | core+Register | off | address+undefined |
| Native Clang coverage | Runtime source-coverage provenance and report generation | no | no | full | only if coverage-producing | none | off | LLVM coverage |
| Repository audit | One source-revision-wide source audit represented in the unified receipt | n/a | n/a | n/a | n/a | n/a | n/a | none |

The MSVC Debug cell is the only ordinary Debug cell in the default matrix. Its
ownership is configuration behavior, not compiler breadth: MSVC Release still
owns MSVC optimizer evidence, while the checks/precondition fixtures explicitly
force their hooks where the contract must also be validated in Release.

The sanitizer consumer remains because it exercises downstream functions and
cross-translation-unit Register boundaries under instrumentation. It does not
repeat structural compiler-contract probes.

## Accepted optional matrix

Optional operations remain accessible without becoming prerequisites of
`Build` or `Run-Tests`.

| Operation | Available scope | Ownership |
| --- | --- | --- |
| Debug codegen diagnostics | MSVC, clang-cl, GCC 14, and Clang 22; selected compiler/profile only | Record wrapper/raw and ABI differences under identical unoptimized flags |
| Sanitizer codegen diagnostic | Selected Clang profile only when an investigation specifically requires instrumented disassembly | Record-only investigation; never a default or optimized gate |
| Ordinary Debug troubleshooting | clang-cl, GCC 13 core, GCC 14, or Clang 22 selected explicitly | Reproduce compiler-specific Debug behavior without joining the default receipt |
| Benchmarks | Existing validated Release trees | Build and run supplemental benchmarks without rebuilding default validation aggregates |
| Focused compiler contracts | Selected compiler | Diagnose preprocessing, header, constraint, or language failures without running the complete matrix |

An optional operation cannot satisfy a missing default manifest. Record-only
codegen cannot satisfy an enforced optimized codegen result.

Generated-code investigations use an explicit compiler and cell selection:

```powershell
tools/Record-Codegen.ps1 -Scope Native -Compiler Msvc -Cell Debug
tools/Record-Codegen.ps1 -Scope Containers -Compiler Clang22 -Cell Debug
tools/Record-Codegen.ps1 -Scope Containers -Compiler Clang22 -Cell AsanUbsan
```

The operation builds only the selected fixture/comparison graph and records its
own provenance; it is not part of the unified default build receipt.

## Development-target ownership rules

The current logical target union is completely covered by the following ordered
rules. The baseline report records the mechanical zero-unmatched,
zero-multiple-owner audit.

| Current target identity or pattern | Category | Future default owner |
| --- | --- | --- |
| `SimdLib`, `SimdLibRegister`, `DevelopmentWarnings`, `ExhaustiveArtifacts`, `SimdLib*Artifacts` | Production/support aggregate | Profile-local build graph |
| `Header*Probe` | Compiler-front-end contract | Each supported Release compiler identity |
| `Config*Probe` | Compiler-front-end contract | Each supported Release compiler identity; a new narrow Debug-state probe belongs to MSVC Debug |
| `Availability*Probe`, `ImmediateControlSlowPathProbe` | Compiler-front-end contract | Each supported Release compiler identity |
| `MethodFlagsConfig*Probe`, `MethodFlagsContractPass`, `MethodFlagsPlacement` | Compiler-front-end contract | Each supported Release compiler identity |
| `RegisterClangClFallbackExclusionProbe`, `RegisterMsvcFallbackProbe`, `RegisterCxx20UmbrellaProbe`, `RegisterEnabledProbe`, `RegisterRepresentation128`, `RegisterRepresentation256` | Compiler-front-end contract | Applicable Release compiler identity |
| `ConstexprProbe`, `ConstexprProbes`, `*ConstexprProbe`, `RegisterConstexpr*Probe` | Compile-time contract | Applicable Release compiler identity |
| `ApiSse42Tests`, `ApiAvx2Tests`, `Bmi*Tests`, `Fma*Tests`, `FormatTests`, `LogicalShuffleImpl*Tests`, `RegisterSse42Tests`, `RegisterAvx2Tests`, `ResampleScalarTests`, `UInt128*Tests`, `VectorAlgorithmsTests` | Runtime correctness | Every applicable Release compiler; additionally MSVC Debug and Clang sanitizer |
| `PreconditionTests`, `RegisterPreconditionTests`, `VectorChecksTests` | Checks/preconditions | Every applicable Release compiler; additionally MSVC Debug and Clang sanitizer |
| `ApiExamples`, `RegisterExamples`, `HeaderOnlySmoke`, `FormatOdr`, `RegisterOdr` | Smoke/ODR/example | Applicable Release compiler identity |
| `MethodFlagsCodegen*` | Optimized codegen/ABI | Applicable Release compiler identity |
| `RegisterAbi*`, `RegisterCodegen*`, `RegisterConsumerAbi*`, `RegisterDefaultAbi*`, `RegisterExpressionCodegen*`, `RegisterFma*`, `RegisterRearrangement*`, `RegisterSpecialized*`, `RegisterTypeMatrix*` | Optimized codegen/ABI in Release; optional diagnostic codegen otherwise | Enforced Release cell or explicitly selected diagnostic operation |
| `CoverageReset`, `CoverageReport` | Coverage | Native Clang coverage operation |
| `Benchmarks`, `BenchmarkArtifacts` | Benchmark | Explicit benchmark operation reusing a validated Release tree |

`BenchmarkArtifacts` and the category-scoped aggregates are organizational
targets. Their category is inherited from their dependencies, and they do not
create an additional validation result.

Every project-owned development target declares its category through
`simdlib_register_development_target` when it is created. Configuration writes
deterministic target, ownership, profile-membership, aggregate-membership, and
external-consumer inventories, and rejects unowned targets, duplicate
assignments, or categories forbidden by the selected validation profile.
External consumers remain separate configure trees rather than being
represented by an empty main-tree aggregate.

Repository auditing is intentionally not a development target. The unified
`Build` command invokes `Run-RepositoryAudit.ps1` once for its source digest
before starting compiler cells, then binds the machine-readable result into
the unified receipt. `Run-Tests` rejects a missing, changed, or stale audit
result.

## CTest ownership rules

The current 265-name logical CTest union is completely covered by stable
identity prefixes.

| Current CTest identity or prefix | Logical count at baseline | Category | Future default owner |
| --- | ---: | --- | --- |
| `PublicHeaderStaticAssertAudit` | 1 | Repository audit | Replaced by the source-revision audit receipt; not repeated as CTest in every cell |
| `MethodFlagsPreprocessor`, `MethodFlagsConfiguration`, `MethodFlagsPlacementAbi` | 3 | Compiler-front-end contract | Applicable Release compiler identity |
| `ConstexprProbes.*` | 1 | Compile-time contract | Applicable Release compiler identity |
| `Preconditions.*`, `Register.AVX2Preconditions.*`, `VectorChecks.*` | 19 | Checks/preconditions | Applicable Release compiler, MSVC Debug, and Clang sanitizer |
| `ApiExamples`, `RegisterExamples`, `HeaderOnlySmoke`, `FormatOdr`, `RegisterOdr` | 5 | Smoke/ODR/example | Applicable Release compiler identity |
| `MethodFlagsCodegen`, `RegisterCodegen.*` | 4 | Optimized or optional diagnostic codegen/ABI | Enforced Release or explicitly selected diagnostic operation |
| `Api.*`, `Bmi*`, `FMA.*`, `Format.*`, `LogicalShuffle.*`, `Register.SSE42*`, `Register.AVX2.*`, `ResampleScalar.*`, `UInt128*`, `VectorAlgorithms.*` | 232 | Runtime correctness | Applicable Release compiler, MSVC Debug, and Clang sanitizer |

The external-consumer project owns two additional logical identities:
`CoreConsumerSmoke` on every supported Release compiler, MSVC Debug, and the
Clang sanitizer cell; and `RegisterConsumerSmoke` on the same Register-capable
cells.

## Configuration sensitivity

Release and Debug remain incompatible compilation fingerprints. That does not
make every target configuration-sensitive.

- `NDEBUG` controls the default `SIMDLIB_ENABLE_CHECKS` value in `Config.h`.
- The default `SIMDLIB_PRECONDITION` maps to `assert`, which is also affected by
  `NDEBUG`.
- `VectorChecksTests` explicitly sets `SIMDLIB_ENABLE_CHECKS=1` and installs an
  observing precondition hook.
- `PreconditionTests` and `RegisterPreconditionTests` install explicit failure
  hooks, so their failure contracts do not depend on the standard `assert`
  mapping.
- Repository audits, header/configuration/availability probes, negative
  compilation, constexpr probes, examples, ODR structure, and consumer
  isolation do not gain a second contract merely from Debug optimization flags.
- Runtime correctness is optimizer-sensitive and therefore remains complete in
  every Release compiler cell.
- The representative MSVC Debug runtime cell owns the unoptimized/default-check
  configuration. A narrow compiler probe must assert the Debug and Release
  `SIMDLIB_ENABLE_CHECKS` defaults before the broader Debug cells are retired.
- Method-flags codegen applies its own optimized compiler flags and therefore
  belongs to the optimized codegen owner rather than every runtime profile.
- Register codegen requires optimization only for the mandatory zero-overhead
  claim. Unoptimized and instrumented records are diagnostic.

## Policy reconciliation

The existing Register proposal and qualification documents require Debug and
sanitizer wrapper/raw differentials. That capability remains supported, but its
pipeline ownership changes:

- optimized Release wrapper/raw and ABI comparisons remain mandatory default
  gates;
- ordinary Debug and sanitizer runtime correctness remain mandatory in their
  assigned default cells;
- Debug and sanitizer disassembly records move to explicit diagnostic
  operations and do not participate in the default build receipt; and
- historical execution evidence remains historical evidence rather than a
  requirement to rebuild every diagnostic artifact on every normal invocation.

This ownership rule supersedes any future-work wording that requires the
default pipeline to prove optimized code shape through unoptimized Debug
records. Diagnostic records may reveal abstraction structure, but they cannot
replace optimized Release qualification.

## Expansion rule

A new compiler, configuration, instrumentation mode, target, or test may enter
the default matrix only when its unique contract is stated and no existing
owner proves that contract. New targets must join one scoped category rather
than being absorbed automatically by a directory-wide target sweep.
