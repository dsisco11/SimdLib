# Register Code-Generation Instruction Contracts Proposal

Status: proposed

## Decision summary

Replace paired wrapper/raw generated-code comparisons with direct instruction
contracts for compiled production methods and representative expressions.

The agreed direction is:

- Use CTest as the test runner and LLVM FileCheck for instruction expectations.
- Write each fixture once, calling production code instead of maintaining a
  second implementation of the same logic.
- Define one primary test case per method or representative expression, with
  shared expectations applied across all supported compiler scenarios. Add
  compiler-specific cases only for additional facts specific to that compiler.
- Define reusable allow/deny instruction rules, supplemented by required
  instructions, counts, and operand relationships where the contract needs them.
- Make low-optimization behavior an explicit concern instead of relying solely
  on highly optimized builds to eliminate abstraction costs.
- Express compiler-specific expectations through the same instruction-contract
  mechanism, without a separate retained-hash exception system.
- Keep CMake responsible for compilation, dependencies, and test registration.
  This design introduces no Python or `lit` dependency.

The precise low-optimization flags and the coverage of wholly unoptimized builds
still require validation. This proposal recommends production-optimization checks
alongside the low-optimization suite; their purposes are described below.

This document proposes a replacement testing practice. It does not change the
current qualification contract or claim that the replacement has been validated.

## Motivation

The existing suites compare separately compiled wrapper and raw implementations.
That requires maintaining two expressions of the same operation and reviewing
differences between their generated instruction streams. Retained fingerprints
add another maintenance obligation when compiler output changes.

The current comparator also removes information that a stronger check needs:
vector-register identities and widths, symbolic targets, and instructions after
the first return. Its symbol inventory checks do not establish that every
expected case was compared. Reusing that normalization in a new policy would
preserve those weaknesses.

The replacement should state the intended generated-code property directly. For
example, a packed-add case can require one appropriate packed addition and forbid
calls and temporary stack storage. There is no need to implement addition again
in a raw fixture to express that requirement.

## Scope and evidence boundaries

The initial replacement covers the existing `Register`, `RegisterMask`,
`PartialRegister`, and `PartialRegisterMask` generated-code cases, including
compositions, transfers, shifts, arithmetic, rearrangements, conversions, and ABI
boundaries. Existing API partial-transfer and method-attribute checks should be
mapped to the shared infrastructure where they use the machinery being retired.
Their distinct contracts must remain visible in the migration inventory.

No production API or algorithm change is part of this proposal. Runtime,
constexpr, compile-failure, layout, and external-consumer tests retain their
existing responsibilities.

An instruction test establishes only its declared properties for the tested
compiler, options, specialization, and fixture context. It does not independently
prove semantic correctness, optimal machine code, measured speed, or instruction
identity with another implementation. Qualification wording must reflect this
boundary rather than inheriting the old wrapper/raw equality claim.

## One production-code fixture per case

A fixture exposes a concrete production operation or representative composition
through a small emitted function. It supplies runtime inputs where relevant and
returns or stores the result to keep it observable. This prevents accidental
constant folding of runtime-dependent work and unused-result elimination.

Intentional constant and identity lowering remains valid. For example, `zero()`
produces a constant value, and `shift_bytes_left<0>()` may require no operation
instructions. Do not add artificial inputs or barriers to prevent these intended
results. A valid emitted function containing only a return can satisfy its
contract; it is distinct from a missing function or empty extraction output,
which [complete function inspection](#complete-function-inspection) must reject.

Fixture requirements:

- Call the public production method being tested. Do not reproduce its algorithm
  using intrinsics, scalar code, or a raw `Api` counterpart.
- Instantiate the required element type, width, active extent, and immediate
  parameters explicitly. An arbitrary template has no inspectable machine code
  until a concrete specialization is emitted.
- Give each case a stable logical ID and an explicit emitted-symbol identity.
- Keep the observation function emitted and non-inlined where necessary, while
  leaving production inlining attributes and behavior intact.
- Match the boundary being tested. Native-vector inputs can isolate an operation;
  ABI cases must retain the actual aggregate parameter and return signatures.
- Avoid fixture-only optimization attributes or barriers that manufacture the
  desired result or introduce artificial spills into the measured body.
- Preserve required partial-register invariant work. Observations must use the
  appropriate public boundary, including checks when the configuration enables
  them.

Related cases may share a source file and compilation target. Each function still
has its own contract and result. Compilation grouping must not determine the
scope of compiler-specific allowances.

The primary case is defined once and instantiated across the supported compiler,
ISA, type, width, and configuration matrix. These are executions of the same
case, not separately authored compiler-specific copies. Parameterize genuine
specialization differences rather than duplicating the case definition.

Independent scalar references remain appropriate in behavioral tests. Their role
is semantic validation, not a second generated-code implementation.

## Instruction contracts

Each case selects a reusable contract family and adds operation-specific
expectations. Shared rules belong to the contract family rather than being
copied into every fixture.

### Primary cases and supplemental cases

Each method's primary case owns the established expectations that hold across
all supported compiler scenarios for its declared configuration and operation
geometry. Every applicable matrix execution must run that primary case.

A compiler-specific case supplements the primary case and contains only facts
that are specific to the selected compiler. It reuses the same production-code
fixture and extracted function body. Do not copy the common assertions, source,
or full case registration into separate MSVC, Clang, and GCC definitions.

For example, if a method has compiler-independent arithmetic requirements but
different instruction ordering on one compiler:

- The primary case owns the common arithmetic, operand, and forbidden-work
  requirements.
- A supplemental case owns only that compiler's established ordering requirement.
- The other compilers need no supplemental case unless there is another specific
  fact to check.

The effective requirements are the primary case plus every applicable
supplemental case. A supplemental case cannot replace, skip, or override the
primary case. A method with no compiler-specific facts has only its primary case.

Keep shared and supplemental checks independently applicable to the complete
function body, for example through separate FileCheck invocations on the same
extracted input. Do not force their patterns into an artificial combined order
or depend on captures from a different invocation.

| Contract family | Typical requirements |
| --- | --- |
| Register-only | No calls, stack traffic, or data-memory operands; only the permitted instruction families |
| Load/operate/store | Required loads and stores with appropriate widths; no temporary stack storage or unexpected calls |
| Immediate operation | Required instruction family and immediate operand; forbidden fallback sequences |
| FMA behavior | Required fusion when specified, or forbidden fused instructions when fusion is disabled |
| ABI boundary | Required argument/result locations and calling convention; no unexpected hidden return buffer or indirection |
| Regression case | A specific redundant move, spill, branch, or instruction sequence remains absent |

These are starting families, not universal restrictions. Memory operations and
opaque-call cases legitimately need instructions that register-only cases forbid.
An ABI contract may also require caller and callee fixtures to expose hidden
arguments or result storage; those fixtures still call production code once.

### Matching rules

FileCheck owns required patterns, forbidden patterns, bounded sequences, and
captured operand relationships. A small shared CMake helper may supplement it
with instruction-line allowlist validation where expressing a whole-body
allowlist through FileCheck would be unnecessarily obscure.

- Allowing an instruction family does not establish the number of operations.
  Add counts or bounded sequences when redundant work matters.
- A required mnemonic alone does not establish data flow. Capture and reuse
  operands where the contract depends on input, output, or temporary roles.
- Preserve vector widths and architectural register aliases. Do not replace all
  registers with one placeholder.
- Scope negative checks to the complete selected function. A `CHECK-NOT` applies
  between surrounding matches; placing it in one interval does not forbid an
  instruction everywhere in the function.
- When a count is intended to be exact, also exclude additional occurrences
  outside the counted sequence within the relevant body.
- Avoid freezing an entire assembly listing when a smaller set of requirements
  expresses the intended property.

Patterns are reviewable expectations, not formal proofs of general assembly
equivalence. The suite should not grow a general-purpose equivalence engine.

## Complete function inspection

Use object disassembly as the common inspection input for the supported compiler
drivers. Select and verify a disassembler explicitly; do not depend on whichever
unrelated executable happens to appear first on `PATH`.

The inspection helper must:

1. Resolve every expected case to its intended emitted function and reject
   missing or ambiguous matches.
2. Extract the complete body using trustworthy function boundaries, including
   reachable blocks after an early return. A return instruction is not a
   function-boundary delimiter.
3. Retain instruction operands, widths, immediates, and control-flow information.
4. Retain relocation and target information where a call, memory reference, or
   branch contract depends on it. Inspect referenced constant data when its value
   is part of the contract.
5. Remove only irrelevant presentation details, such as instruction addresses,
   without erasing information required by the checks.
6. Fail on unsupported or unrecognized extraction formats instead of producing
   an empty successful check.

FileCheck must receive non-empty, correctly scoped input. Its pattern matching
does not replace function extraction or expected-case coverage validation.

## Optimization and configuration policy

The design goal is efficient production code without depending on aggressive
optimization to rescue avoidable abstraction costs. Optimization is a declared
test dimension, separate from assertions, sanitizers, ISA, FMA, and ABI settings.

| Configuration | Proposed role |
| --- | --- |
| Low optimization | Primary robustness checks for efficient code with limited optimization |
| Production optimization | Companion checks for the settings users execute, including optimization-induced regressions |
| Wholly unoptimized | Selected meaningful contracts and investigation of abstraction costs; coverage to be established explicitly |

GCC/Clang `-O1` is the initial candidate for the low-optimization configuration.
MSVC `/O1` is a size-optimization preset, not a directly equivalent first
optimization level. Select and document the MSVC optimization and inlining
options deliberately after inspecting representative cases.

Wholly unoptimized code is not a worst-case performance bound. It can retain
parameter storage, temporary storage, and function boundaries even for direct
intrinsic expressions. In particular, MSVC `/Od` defaults to disabled inline
expansion through `/Ob0`, which conflicts with a universal no-call contract for
an inline library.

Do not silently relax a selected low-optimization contract because it fails. The
failure must lead to investigation of the implementation, fixture, compiler
settings, or stated support boundary. Exact flags remain an implementation
decision requiring evidence, not an assumption that all compilers interpret
optimization levels identically.

Use the same fixture source across configurations. A low-optimization pass does
not imply a production-optimization pass. Diagnostic instrumentation and
assertion costs must be identified explicitly rather than confused with the
optimization level.

## Compiler-specific expectations

Compiler-specific facts are supplemental cases using the same contract mechanism
as the primary case. Each has a narrow scope, an explicit reason, and a defined
set of supported configurations. Shared ISA and width parameters do not require
a separate full definition for every compiler.

For example, when a method has a documented MSVC `/GS` sequence, its primary case
must express the operation's genuinely common requirements. It cannot declare a
universal no-stack/no-call rule and then let the MSVC case cancel that rule.
Supplemental cases check the bounded `/GS` sequence where it occurs and the
stronger absence-of-stack/call property where that property is established.
Compilers sharing the latter property can share one supplemental declaration.
This preserves both the common facts and the stronger compiler-specific facts
without copying the entire test or allowing arbitrary calls and stack traffic.
The contract must identify the intended check target where its identity matters.

No retained wrapper/raw hash pair, baseline-update workflow, or separate
exception-enforcement engine is required. Hashes may still identify build inputs
and artifacts for freshness; they do not decide instruction acceptability.

A passing case means its stated contract holds. If that contract permits known
overhead, its documentation must say so. This is a claim-description requirement,
not an additional pass/fail mechanism.

## CMake, CTest, and FileCheck responsibilities

| Component | Ownership |
| --- | --- |
| CMake build targets | Compile fixtures with explicit settings and ordinary source/header dependencies |
| Shared CMake registration helper | Expand each primary case across the supported matrix and register only applicable supplemental cases, reusing fixtures and artifacts |
| CTest | Select, schedule, execute, and report instruction checks |
| Inspection helper | Extract and validate complete named functions and preserve relevant object information |
| FileCheck and shared contract rules | Enforce generated-instruction expectations |

The intended workflow is:

```text
Build selected fixture objects through CMake/Ninja
    -> CTest selects a case and configuration
    -> extract its complete disassembly
    -> run the primary case's shared rules and method expectations
    -> run any applicable supplemental compiler-specific cases on the same body
    -> retain diagnostics and report pass/fail
```

CTest does not implicitly build fixtures. The project build/test commands must
preserve the existing build-before-test workflow and freshness checks. Missing or
stale inputs must fail clearly; tests must not silently validate an earlier build.

Keep incremental compilation under the build system rather than launching a
compiler separately for every CTest invocation. Give parallel checks distinct
output paths and treat shared object inputs as read-only.

CTest names and results should distinguish the primary case from supplemental
cases and identify the execution configuration. A compiler selector controls only
the supplemental case it owns; it must never remove the primary case from that
compiler's required tests. Supplemental executions reuse the compiled fixture
instead of introducing duplicate compilation targets.

Provision FileCheck and the selected disassembler as explicit development tools
in local and CI environments. Python and `lit` are not required. Runtime tests
continue to use their existing runner and reporting integration.

### Proposed source layout

- `cmake/development/CodegenTests.cmake`: shared target and test registration.
- `cmake/development/RegisterCodegen.cmake` and
  `PartialRegisterCodegen.cmake`: thin suite-specific case declarations.
- `cmake/codegen/ExtractFunction.cmake`: complete function extraction and input
  validation.
- `cmake/codegen/CheckInstructions.cmake`: rule application and FileCheck
  invocation.
- `tests/codegen/contracts/`: reusable instruction-family expectations.
- `tests/codegen/register/` and `tests/codegen/partial-register/`: production-code
  fixtures, primary method expectations, and optional supplemental expectations
  containing only compiler-specific facts. Organize these by method or operation
  family rather than mirroring the whole suite into compiler-specific trees.
- `tests/codegen/harness/`: positive and negative tests of extraction and rule
  enforcement.

These paths describe proposed responsibilities. Keep each file focused on one
responsibility and avoid reproducing the current monolithic comparator under a
new filename. The concrete FileCheck invocation and extraction format should be
proven in the initial implementation before expanding the corpus.

## Coverage, diagnostics, and qualification

Create an explicit migration inventory mapping every existing case to its new
fixture, contract, supported configurations, and validation owner. Register exact
expected cases independently of what the disassembler happens to emit. Preserve
the operation-availability tests that distinguish supported and unavailable
specializations.

Record the primary case once with its supported execution matrix, and list only
the supplemental facts and their applicability separately. Coverage validation
must require the primary case on every supported compiler and must not accept a
supplemental result as a substitute. Preserve established stronger facts in
supplemental checks when they cannot be expressed as universal requirements.

The inventory must include existing compiler families and drivers, SSE4.2/128 and
AVX2 widths where supported, FMA modes, active extents, and explicit versus
platform-default ABI observations. It must not silently convert an unavailable
compiler or missing tool into a successful qualification result.

For a failed check, report the case ID, compiler identity, effective options,
configuration, selected contract, function identity, and failed expectation.
Retain the full disassembly, extracted body, and FileCheck output. Preserve
existing useful provenance without retaining generated assembly as a universal
golden baseline.

Update [RegisterQualification.md](RegisterQualification.md) and
[PartialRegisterQualification.md](PartialRegisterQualification.md) to describe
the contracts actually enforced. Reconcile
[RegisterImplementationMatrix.md](RegisterImplementationMatrix.md) and the README
with those documents. Do not replace old equality claims with a blanket claim
that a few permitted mnemonics prove zero overhead.

## Adoption sequence

1. Inventory existing cases, define each method's primary instruction contract,
   and identify only the additional facts requiring supplemental cases.
2. Prove CMake/CTest/FileCheck integration with representative register-only,
   memory, partial-invariant, and ABI cases across the supported compiler drivers.
3. Validate extraction and rule enforcement with deliberate failing inputs before
   trusting positive results.
4. Establish low-optimization settings and the unoptimized support boundary;
   verify the companion production-optimization checks.
5. Rewrite the remaining fixtures to invoke production code once and migrate
   every inventory entry without losing coverage.
6. Replace the old paired comparison gates, retained-profile data, and obsolete
   raw fixtures once their replacement contracts are validated.
7. Run the complete applicable qualification matrix and update documentation and
   CI tool provisioning together.

Temporary coexistence during migration is acceptable. The final suite must not
retain two implementations merely to preserve the retired comparison mechanism.

## Acceptance criteria

- Every migrated case tests one production-code fixture or an
  [ABI caller/callee fixture group](#primary-cases-and-supplemental-cases), with an
  explicit instruction contract and stable identity. An ABI fixture group exposes
  the required boundaries without introducing a duplicate implementation.
- Shared rules are reusable across methods without duplicating algorithms.
- Each primary method case is authored once and runs across its supported
  compiler matrix; compiler-specific copies of the full case are absent.
- Supplemental cases contain only compiler-specific facts, reuse the primary
  fixture, and never replace or weaken the primary checks.
- CTest runs the suite with FileCheck and no new Python or `lit` requirement.
- Expected functions and configurations cannot disappear silently.
- Checks inspect complete function bodies and retain the information their
  contracts need.
- Harness tests reject empty or missing functions, ambiguous selection, forbidden
  instructions after an early return, extra counted operations, incorrect vector
  widths, broken captured operand relationships, and incorrect required targets.
- Valid cases and narrowly scoped compiler variations pass; nearby invalid
  variants fail.
- Harness checks establish that a shared-rule violation fails even when a
  supplemental case passes, and that an applicable supplemental-rule violation
  fails even when the primary case passes.
- Low-optimization settings are explicit and evidence-backed. Production and
  unoptimized results are reported according to their declared roles.
- No retained wrapper/raw instruction hashes remain as an acceptance mechanism.
- Existing behavioral and ABI coverage is preserved, and qualification claims
  describe the properties actually checked.

## Reference documentation

- [CTest manual](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- [LLVM FileCheck manual](https://llvm.org/docs/CommandGuide/FileCheck.html)
- [GCC optimization options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [MSVC /O1 and /O2](https://learn.microsoft.com/en-us/cpp/build/reference/o1-o2-minimize-size-maximize-speed)
- [MSVC inline-expansion options](https://learn.microsoft.com/en-us/cpp/build/reference/ob-inline-function-expansion)
