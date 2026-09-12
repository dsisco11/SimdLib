# Register Instruction Optimization Report

This report tracks narrowly scoped generated-code opportunities in SimdLib's
register implementation. An item is complete only when its semantics are covered
by tests and representative optimized output demonstrates the intended instruction
property. Instruction-count changes are local evidence and do not establish an
application-level speedup.

## Candidate summary

| Item | Area | Intended improvement | Status |
|---:|---|---|---|
| 1 | Runtime integer lane insertion | Replace 256-bit half selection with one native comparison and blend using static lane indices. | Complete. |
| 2 | Runtime complete-register byte shifts | Materialize the fixed byte-shuffle indices as static constants. | Candidate; codegen comparison required. |
| 3 | Signed minimum-position reductions | Audit signed lane-index constants for stack construction and use static registers where necessary. | Candidate; codegen inspection required. |
| 4 | `RegisterMask::all()` | Evaluate a complete-register containment test instead of compact-mask extraction. | Candidate; the all-one operand cost must be compared. |
| 5 | Runtime 128-bit whole-register bit shifts | Evaluate control-flow or selection strategies that avoid computing both count ranges. | Candidate; benchmark and codegen evidence required. |

## 1. Runtime integer lane insertion

The 128-bit signed and unsigned integer specializations create fixed lane-index
vectors with `_mm_setr_epi*` or `_mm_set_epi64x`. Optimized MSVC output already
uses those values directly from the constant pool, so changing their source
expression does not improve the generated instructions.

The original 256-bit integer specializations instead branch on the selected half,
extract or cast that half, perform a 128-bit comparison and blend, and then
recombine the result. Their native AVX2 implementations now use static native
lane-index constants, compare the runtime index across the complete 256-bit
register, and perform one byte blend. This removes the control flow and
half-register reconstruction while preserving the existing 128-bit path.

Direct use of `make_static_register()` at these eight sites produced correct
constant-pool operands but also caused MSVC to emit an otherwise unused aligned
stack frame in the real API instantiation. The native `_mm256_setr_epi*`
expressions produce the same constant-pool operands without that frame and are
therefore retained here.

Required evidence:

- existing runtime insertion tests pass for every supported integer element type
  and both register widths;
- representative optimized 128-bit and 256-bit callers contain no stack-built
  lane-index vectors;
- the fixed lane indices appear as constant-pool operands or an equally compact
  compiler-generated form;
- 256-bit callers contain no half-selection branch, extraction, or reinsertion.

Validation results:

- the MSVC Release SSE4.2 runtime-insertion test passed 66 assertions;
- the MSVC Release AVX2 runtime-insertion test passed 132 assertions;
- the SSE4.2 and AVX2 strict generated-code gates both passed;
- all eight signed and unsigned AVX2 integer forms use a constant-pool
  `VPCMPEQ*`, one full-width `VPBLENDVB`, and no stack frame, branch,
  half-register extraction, reinsertion, helper call, or spill;
- representative `uint32x8` output changed from a branched half-register
  sequence of approximately `0x5a` bytes to a straight-line `0x22`-byte
  sequence.

## 2. Runtime complete-register byte shifts

The 128-bit runtime byte-shift extensions use fixed ascending or biased byte
indices to construct `PSHUFB` controls. The constants can be expressed with
`make_static_register()`. Adoption requires matched generated-code inspection to
confirm that it removes constant preparation without adding loads or extending
live ranges.

## 3. Signed minimum-position reductions

Signed 8-, 16-, 32-, and 64-bit minimum-position implementations retain
`register_from_values()` lane-index constants. They should be compared with the
static-register accessor pattern used by the unsigned implementations. Only
specializations that currently materialize addressable storage or add stack
protection should change.

## 4. `RegisterMask::all()`

Canonical predicate lanes permit a complete-register containment test against an
all-one vector. This could replace compact lane-mask extraction and scalar
comparison. The change is conditional on the cost of producing the all-one
operand; generated-code comparison must show a net improvement for each supported
width before adoption.

## 5. Runtime 128-bit whole-register bit shifts

The runtime whole-register shifts currently compute results for counts below and
above 64 bits and combine them. A control-flow or vector-selection implementation
could execute fewer instructions for a given count, but its value depends on
branch predictability and surrounding register pressure. This item requires both
matched codegen and a representative microbenchmark before implementation.
