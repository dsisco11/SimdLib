# Register Code-Generation Policy Convergence Proposal

Status: proposed

This document proposes a single enforcement model for the `Register` and
`PartialRegister` generated-code suites. It is a decision artifact, not a
second qualification contract. Once the proposal is implemented and the final
policy is incorporated into the qualification documents, this file should be
removed.

## Decision summary

Optimized Release comparisons should enforce generated-code expectations by
default for every supported compiler, ISA, width, and fixture profile. A
comparison may pass in one of three ways:

1. normalized wrapper and raw instruction streams are identical;
2. a narrow semantic recognizer accepts one documented compiler transformation;
3. an exact compiler-qualified wrapper/raw hash pair matches a reviewed retained
   difference.

Record-only comparisons remain useful for deliberately diagnostic
configurations, but they do not satisfy or weaken an optimized zero-overhead
claim. The current blanket SSE4.2 diagnostic classification should therefore be
replaced with per-profile evidence, and broad records should be divided wherever
one compiler behavior currently relaxes unrelated symbols.

## Problem

The two generated-code suites currently express similar guarantees through
different policies:

- `PartialRegister` enforces every optimized comparison and stores reviewed,
  compiler-qualified wrapper/raw fingerprints in
  `../cmake/development/PartialRegisterCodegenProfiles.json`.
- `Register` enforces AVX2 by default but records the complete SSE4.2 corpus as
  diagnostic, even when the compared instruction streams are exact.
- MSVC forces the broad `primary-composition` profile and AVX2/256 modulus
  profile into record-only mode. The latter is currently exact and the former
  combines `/GS`-affected operations with unrelated operations.
- Several `Register` comparisons omit `CODEGEN_PROFILE`, causing their records
  to share the non-descriptive `default` identity.
- [RegisterImplementationMatrix.md](RegisterImplementationMatrix.md) describes
  strict SSE4.2 gates for clang-cl, Clang, and GCC, while
  [RegisterQualification.md](RegisterQualification.md) and the executable CMake
  policy classify all SSE4.2 comparisons as diagnostic.

This inconsistency makes a recorded success ambiguous: it may mean exact parity,
a reviewed difference, or merely that the comparison was allowed to differ.

## Goals

- Give every optimized supported cell an unambiguous, enforceable disposition.
- Make exact parity the default rather than maintaining allowlists for exact
  compiler output.
- Retain only narrow, reviewed compiler differences and detect any drift in
  either side of such a difference.
- Keep deterministic semantic exceptions in code when they can be recognized
  more safely than by whole-record hashes.
- Prevent one exceptional symbol family from weakening unrelated symbols.
- Use the same retained-profile schema and validation rules for `Register` and
  `PartialRegister` without forcing their fixtures or comparators into one file.
- Keep documentation claims identical to the policy executed by CMake and CI.

## Non-goals

- Changing the public `Register` or `PartialRegister` API.
- Requiring identical code across different compiler versions.
- Treating Debug, sanitizer, or platform-default ABI observations as
  zero-overhead evidence.
- Replacing instruction inspection with performance benchmarks.
- Accepting a new fingerprint only because a compiler upgrade produced it.

## Proposed enforcement model

### Comparison dispositions

| Disposition | Meaning | Qualification value |
| --- | --- | --- |
| Exact | Normalized wrapper and raw instructions match. | Satisfies the optimized gate. |
| Semantic exception | A narrowly scoped recognizer proves one documented instruction pattern and all remaining instructions match. | Satisfies the optimized gate as an explicit exception. |
| Retained difference | The wrapper and raw streams differ, but both hashes match a reviewed entry for the exact compiler identity and profile. | Satisfies the optimized gate as an explicit exception. |
| Diagnostic | The result is recorded without requiring parity or a retained difference. | Does not satisfy an optimized gate or support a zero-overhead claim. |

An optimized Release comparison must use one of the first three dispositions.
Diagnostic mode is reserved for explicitly selected investigation builds and
observational contracts such as the platform-default ABI record.

### Evaluation order

The comparison engine should evaluate a strict record in this order:

1. normalize and compare the complete selected wrapper/raw symbol corpus;
2. apply an applicable narrow semantic recognizer;
3. look up an exact retained-profile entry and verify both normalized hashes;
4. fail with the instruction difference and the unmatched profile identity.

This keeps ordinary exact output free of stored fingerprints. A retained entry
is consulted only after a real difference has been found.

### Retained-profile identity

Every comparison must provide a stable `CODEGEN_PROFILE`. A retained entry is
identified by at least:

- value type (`Register` or `PartialRegister`);
- compiler family and compiler version;
- profile name;
- register width and ISA profile;
- wrapper hash and raw hash;
- a concise reason naming the accepted transformation.

Operating system, ABI mode, FMA mode, or another compilation dimension must be
part of the identity when it can change the selected code. Compiler-version
wildcards should be rejected unless the compiler family exposes no stable
version boundary and the qualification document explicitly justifies that
choice.

Missing, duplicate, malformed, stale, or ambiguous entries are configuration
errors. Entries not exercised by the qualified matrix should also fail policy
validation so obsolete exceptions cannot accumulate silently.

## Corpus boundaries

The existing isolated records remain separate: immediate shifts, register-only,
reassignment, ABI, consumer ABI, specialized operations, FMA modes,
rearrangement/conversion, common type matrix, and modulus type matrix.

The broad `primary-composition` comparison should be divided by compiler-relevant
behavior. At minimum, pure composition, memory transfer/mutation, and opaque-call
boundaries must not share one disposition. The final split should be driven by
the generated symbol differences, with each record containing only symbols that
can legitimately share one exception reason.

The missing stable `Register` profile names should be made explicit, including
`register-only`, `reassignment`, `abi`, and `consumer-abi`. The platform-default
ABI record remains observational and should be named independently rather than
participating in strict wrapper/raw policy.

The existing exact MSVC `/GS` recognizers should remain semantic exceptions.
They describe bounded instruction sequences more precisely than a whole-record
fingerprint. Any `/GS` behavior that cannot be recognized narrowly should be
isolated before considering a retained hash.

## Configuration layout

The steady-state layout should separate orchestration, retained data, and
comparison logic:

- `cmake/development/RegisterCodegen.cmake` owns `Register` fixture targets,
  profile boundaries, and comparison registration.
- `cmake/development/PartialRegisterCodegen.cmake` owns `PartialRegister`
  fixture targets, profile boundaries, and comparison registration.
- `cmake/development/RetainedCodegenProfiles.cmake` owns the shared JSON schema,
  validation, and exact lookup rules.
- `cmake/development/RegisterCodegenProfiles.json` owns reviewed `Register`
  retained differences.
- `cmake/development/PartialRegisterCodegenProfiles.json` continues to own
  reviewed `PartialRegister` retained differences.
- The comparison scripts continue to own disassembly normalization and
  type-specific semantic recognizers.

The shared loader should not know fixture names or decide which comparisons are
diagnostic. Conversely, the orchestration files should contain no hashes and
should not duplicate JSON parsing or schema validation.

## Evidence required before policy changes

The complete optimized `Register` corpus must be regenerated for the supported
matrix before reclassifying records:

- MSVC 19.44 on Windows;
- clang-cl 20.1.8 on Windows;
- the qualified clang-cl 22 version on Windows;
- Clang 22 on the pinned Linux image;
- GCC 14 on the pinned Linux image.

For each compiler, run SSE4.2/128, AVX2/128, and AVX2/256 where available, with
both FMA dispositions where the fixture defines them. Clang-cl 20 evidence is a
required input, not something inferred from clang-cl 22.

Each non-exact record must be reviewed at symbol and instruction level. The
review must establish whether the difference is:

- a deterministic pattern suitable for a narrow semantic recognizer;
- a compiler-qualified allocation or scheduling difference suitable for an
  exact retained hash pair; or
- real wrapper overhead, in which case the optimized gate remains failing and
  the zero-overhead claim must exclude that precise cell.

Retained hashes should be reproduced by a second clean build with the same
qualified toolchain before they are committed. A compiler upgrade requires a
new review; it must not inherit a nearby version's hashes.

## Documentation ownership

[RegisterQualification.md](RegisterQualification.md) remains authoritative for
the supported compiler matrix, optimized claims, diagnostic exclusions, and
accepted exceptions. [PartialRegisterQualification.md](PartialRegisterQualification.md)
has the equivalent responsibility for `PartialRegister`.

[RegisterImplementationMatrix.md](RegisterImplementationMatrix.md) should link
to the qualification contract instead of restating a stronger platform policy.
The README should contain only a short user-facing summary. Generated comparison
records remain execution evidence and are not permanent repository documents.

Documentation and executable policy must change together. No document should
claim strict coverage for a cell that CMake records diagnostically, and no
strictly enforced cell should remain described as diagnostic.

## Adoption sequence

1. Assign stable names to every `Register` comparison and collect the complete
   qualified compiler evidence without changing pass/fail policy.
2. Split broad records until every observed difference has one owning symbol
   family and reason.
3. Introduce the shared retained-profile loader and migrate the existing
   `PartialRegister` parser without changing its accepted entries.
4. Add only reviewed `Register` retained differences, preserving the existing
   semantic recognizers where they are narrower.
5. Remove blanket SSE4.2 and stale per-profile record-only overrides, making all
   optimized Release comparisons strict by default.
6. Re-run the complete local/container matrix and Windows CI, including the
   dedicated clang-cl 20 and clang-cl 22 jobs.
7. Reconcile the qualification contract, implementation matrix, and README with
   the final evidence-backed boundary.

## Acceptance criteria

The proposal is satisfied when:

- every optimized Release comparison has a unique stable profile identity;
- exact records enforce without a JSON entry;
- every non-exact passing record is owned by one semantic recognizer or one
  exact compiler-qualified JSON entry;
- no broad record-only switch masks an unrelated optimized comparison;
- Debug, sanitizer, and observational ABI records are visibly diagnostic and
  excluded from optimized claims;
- the supported MSVC, clang-cl 20, clang-cl 22, Clang, and GCC matrices pass
  their required correctness, ABI, and generated-code gates;
- negative tests reject malformed, duplicate, ambiguous, stale, and incorrect
  retained-profile data; and
- the qualification documents, implementation matrix, README, CMake policy,
  and CI jobs describe the same boundary.

## Alternatives not recommended

Keeping all SSE4.2 comparisons diagnostic preserves the current ambiguity and
provides no regression gate for records that are already exact. Hashing every
record creates unnecessary churn and turns ordinary compiler output into policy
data. Maintaining hashes directly in CMake mixes data with orchestration and
makes review harder. Finally, a single broad hash for the composition corpus
would preserve the same excessive exception boundary in a different format.
