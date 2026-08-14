# PartialRegister Generated-Code and ABI Audit

This inventory owns the permanent qualification evidence for `PartialRegister`
and `PartialRegisterMask`. The paired raw fixtures perform the same active-lane
masking, inactive-divisor neutralization, result projection, and normalization
required by the public wrapper; a raw expression that omits that work is not an
acceptable baseline.

| Fixture pair | Contract | Profiles | Gate |
|---|---|---|---|
| `PartialRegisterMaskCodegen.cpp` / `PartialRegisterMaskCodegenRaw.cpp` | predicate composition, native import, closed addition, neutralized division | SSE4.2/128, AVX2/256 | `PartialRegisterMaskCodegen` |
| `PartialRegisterArithmeticCodegen.cpp` / `PartialRegisterArithmeticCodegenRaw.cpp` | unary and binary arithmetic, division/modulus, reductions, specialized and type-changing results | SSE4.2/128, AVX2/256 | `PartialRegisterArithmeticCodegen` |
| `PartialRegisterGeneralCodegen.cpp` / `PartialRegisterGeneralCodegenRaw.cpp` | construction, active-extent transfer, bitwise unary work, per-lane and payload shifts, comparisons and mask use, rearrangement, conversion, and composition | SSE4.2/128, AVX2/256 | `PartialRegisterGeneral` |
| `PartialRegisterAbi.cpp` / `PartialRegisterAbiRaw.cpp` | value parameters and returns, reassignment, mask/scalar/type-changing results, opaque calls, register pressure | SSE4.2/128, AVX2/256 | `PartialRegisterAbi` |

All four gates are dependencies of the strict Release `RegisterCodegen` target.
Artifacts are written below `partial-register-codegen/<isa>/<width>`. Each
comparison record contains compiler identity and version, ISA profile, exact
symbol set, normalized disassembly, and the enforced decision. `symbols.txt`
lists the ordered public fixture symbols. `instruction-differences.txt` maps
every differing symbol to its complete normalized wrapper and raw instruction
blocks. Exact-parity profiles produce an empty difference file.

The ABI fixtures additionally instantiate every available active count for all
signed and unsigned 8-, 16-, 32-, and 64-bit lanes plus `float` and `double`,
and assert that each specialization has exactly the size and alignment of its
native vector.
Non-inlined wrapper boundaries are compared with native-vector signatures for
every ABI shape listed above. Explicit non-inlined identity boundaries cover
all 56 available type/count cells in each native width; together with the eight
named ABI-shape functions, each ABI profile owns 64 wrapper/raw symbol pairs.

## Accepted exceptions

Every comparison runs in `ENFORCE` mode. Exact parity passes directly. A known
compiler difference passes only when both complete normalized profiles match
the wrapper and raw SHA-256 values selected in
`cmake/development/PartialRegisterCodegen.cmake`; any instruction or symbol
change fails before a record is published. The record and provenance include
the actual and expected hashes, compiler version, ISA, exception identifier,
and pointers to the exact symbol/instruction sidecars.

The currently retained Clang differences are commutative operand selection in
`add` and `multiply`, register selection in `sparse_adjacent`, and scalar lane
load scheduling in `divide`/`modulus`; SSE also retains the equivalent operand
order in `multiply_add`. The value profile retains the same `add` distinction
and the SSE `divide` scheduling distinction. General and ABI profiles require
exact Clang parity.

GCC raw mirrors preserve the wrapper's logical operand roles even when an
operation is commutative. This avoids fixture-induced return moves or scheduling
changes: GCC 14 requires exact parity for both arithmetic profiles rather than
retaining differences caused only by reversed raw-fixture arguments.

MSVC retains only the exact hashed profiles identified by these exception IDs:

| Exception | Exact instruction categories |
|---|---|
| `msvc-gs-predicate-composition-cookie` | `/GS` prologue, check call, and epilogue around predicate composition |
| `msvc-equivalent-import-normalization-gs-and-value-operation-allocation` | import-boundary normalization and `/GS` frame shape, `movups`/`movdqu` spelling, and equivalent scalar division lane scheduling/register allocation |
| `msvc-equivalent-unaligned-moves-and-register-allocation` | equivalent unaligned move spelling, commutative operand choice, and temporary register allocation |
| `msvc-equivalent-vector-moves-mask-materialization-register-allocation-and-gs-cookie` | `/GS`, `movups`/`movdqu`, constant memory versus temporary-register masks, and equivalent operand allocation |
| `msvc-partial-invariant-boundary-normalization-and-gs-cookie` | required inactive-suffix normalization at checked boundaries plus `/GS` sequences |

For each compiler/ISA cell, `instruction-differences.txt` is the authoritative
exact affected-symbol and instruction-delta list. Its complete wrapper/raw
contents are protected by the retained profile hashes, so it is also the
retention test rather than a diagnostic snapshot.

## Benchmarks

`benchmarks/Register.benchmarks.cpp` contains separate PartialRegister cases for
representative 128-bit and 256-bit active counts, zero-closed addition,
invariant-maintaining division, comparison/selection, and rearrangement. These
measurements are supplemental and are not correctness or generated-code gates.
Performance claims may be made only after the strict Release generated-code
targets pass, and must identify the produced benchmark artifact.
