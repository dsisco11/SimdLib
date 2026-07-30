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
| Sanitizer | ASan+UBSan instrumentation applied to runtime contracts; it is not a generated-code category |
| Benchmark | Supplemental Release-only benchmark compilation and execution |

## Accepted default matrix

The following cells compose the default `Build`. “Full runtime” means
the runtime correctness and explicit checks/precondition categories applicable
to that compiler's supported surface.

| Cell | Unique default contract | Compiler contracts | Constexpr | Runtime | Smoke/ODR/examples | Consumer | Codegen | Instrumentation |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| MSVC Release | Windows MSVC optimizer, ISA mappings, `VECTORCALL`, Release ABI, and zero-overhead qualification | yes | yes | full | yes | core+Register | enforce | none |
| MSVC Debug | Representative ordinary Debug behavior, default checks/preconditions, and Windows Debug runtime | no; narrow checks-state probe | no | full | no | none | off | none |
| clang-cl Release | Windows Clang frontend/optimizer, MSVC-style driver, `VECTORCALL`, and Release ABI | yes | yes | full | yes | core+Register | enforce | none |
| GCC 13 core Release | C++20 core compatibility floor and unavailable-Register contract | yes | core only | core only | core only | core only | unavailable | none |
| GCC 14 Release | GNU optimizer, core/Register language surface, GNU ABI, and zero-overhead qualification | yes | yes | full | yes | core+Register | enforce | none |
| Clang 22 Release | GNU-like Clang optimizer, core/Register language surface, GNU ABI, and zero-overhead qualification | yes | yes | full | yes | core+Register | enforce | none |
| Clang 22 ASan+UBSan Debug | Instrumented Linux runtime correctness | no | no | full | no | none | off | address+undefined |
| Native Clang coverage | Runtime source-coverage provenance and report generation | no | no | full | no | none | off | LLVM coverage |
| Repository audit | One source-revision-wide source audit represented in the unified receipt | n/a | n/a | n/a | n/a | n/a | n/a | none |

The MSVC Debug cell is the only ordinary Debug cell in the default matrix. Its
ownership is configuration behavior, not compiler breadth: MSVC Release still
owns MSVC optimizer evidence, while the checks/precondition fixtures explicitly
force their hooks where the contract must also be validated in Release.

External consumers are compiler-facing header-only consumption contracts.
Each compiler's Release cell owns its core-only or core-and-Register consumer
inventory. Debug CRT selection and sanitizer flags affect the consumer
executable rather than a SimdLib binary or propagated usage requirement, so
they do not create additional consumer owners.

Coverage owns only runtime correctness and checks/preconditions executables.
Examples, header smoke tests, and ODR tests are public-surface contracts owned
by applicable Release compilers. Coverage processing matches every raw profile
to its executable build identity, merges raw profiles only per executable, and
records the mapping in `coverage-provenance.tsv` before combining LCOV traces.
Compile-only constexpr evidence retains its Release compiler and feature
provenance and does not contribute to runtime coverage percentages.

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

Ordinary Debug troubleshooting uses the lower-level matrix runners explicitly:

```powershell
tools/Run-NativeMatrix.ps1 -Action Build -Compiler ClangCl -Cell Debug
tools/Run-ContainerMatrix.ps1 -Action Build -Compiler Gcc13 -Cell Debug
tools/Run-ContainerMatrix.ps1 -Action Build -Compiler Gcc14 -Cell Debug
tools/Run-ContainerMatrix.ps1 -Action Build -Compiler Clang22 -Cell Debug
```

`Pipeline.Common.psm1` owns the canonical default preset list consumed by the
build receipt, test receipt, and both matrix runners. `-Cell All` follows that
list; explicit `-Cell Debug` bypasses default membership only for the selected
troubleshooting operation.

## Removed ordinary Debug cells

| Removed default cell | Replacement evidence | Remaining direct use |
| --- | --- | --- |
| clang-cl Debug | clang-cl Release owns the Clang frontend, Windows ABI, `VECTORCALL`, language, runtime, consumer, and optimizer contracts; MSVC Debug owns unoptimized Windows and default-check behavior. No separate clang-cl Debug CRT, ABI, or calling-convention contract was identified. | Explicit reproduction of a clang-cl-only Debug failure |
| GCC 13 core Debug | GCC 13 core Release owns the C++20 compatibility floor, core runtime/consumer surface, and unavailable-Register contract; MSVC Debug owns configuration-sensitive default checks. | Explicit reproduction of a GCC 13 Debug compatibility failure |
| GCC 14 Debug | GCC 14 Release owns GNU language, ABI, runtime, consumer, and optimizer contracts; MSVC Debug owns ordinary Debug configuration and Clang ASan+UBSan owns instrumented Linux Debug runtime behavior. | Explicit reproduction of a GCC 14 Debug failure |
| Clang 22 Debug | Clang 22 Release owns Clang language, ABI, runtime, consumer, and optimizer contracts; Clang 22 ASan+UBSan owns Linux Debug runtime instrumentation. | Explicit reproduction of a non-sanitized Clang Debug failure |

## Development-target ownership rules

The current logical target union is completely covered by the following ordered
rules. The baseline report records the mechanical zero-unmatched,
zero-multiple-owner audit.

| Current target identity or pattern | Category | Default owner |
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

| Current CTest identity or prefix | Logical count at baseline | Category | Default owner |
| --- | ---: | --- | --- |
| `PublicHeaderStaticAssertAudit` | 1 | Repository audit | Replaced by the source-revision audit receipt; not repeated as CTest in every cell |
| `MethodFlagsPreprocessor`, `MethodFlagsConfiguration`, `MethodFlagsPlacementAbi` | 3 | Compiler-front-end contract | Applicable Release compiler identity |
| `ConstexprProbes.*` | 1 | Compile-time contract | Applicable Release compiler identity |
| `Preconditions.*`, `Register.AVX2Preconditions.*`, `VectorChecks.*` | 19 | Checks/preconditions | Applicable Release compiler, MSVC Debug, and Clang sanitizer |
| `ApiExamples`, `RegisterExamples`, `HeaderOnlySmoke`, `FormatOdr`, `RegisterOdr` | 5 | Smoke/ODR/example | Applicable Release compiler identity |
| `MethodFlagsCodegen`, `RegisterCodegen.*` | 4 | Optimized or optional diagnostic codegen/ABI | Enforced Release or explicitly selected diagnostic operation |
| `Api.*`, `Bmi*`, `FMA.*`, `Format.*`, `LogicalShuffle.*`, `Register.SSE42*`, `Register.AVX2.*`, `ResampleScalar.*`, `UInt128*`, `VectorAlgorithms.*` | 232 | Runtime correctness | Applicable Release compiler, MSVC Debug, and Clang sanitizer |

The external-consumer project owns two additional logical identities:
`CoreConsumerSmoke` on every supported Release compiler and
`RegisterConsumerSmoke` on the same Register-capable Release compilers. GCC 13
therefore retains the core-only consumer qualification explicitly.

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
  configuration. The retained MSVC Debug and Clang sanitizer cells compile the
  checks-state probe with `SIMDLIB_ENABLE_CHECKS=1` and an explicit rejection of
  `NDEBUG`; Release compilers separately assert the disabled default.
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
