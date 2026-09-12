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
| 2 | Runtime complete-register byte shifts | Eliminate fixed byte-shuffle selector preparation in unoptimized output. | Complete; existing implementation is optimal. |
| 3 | Signed minimum-position reductions | Replace procedural lane-index construction with native constant expressions. | Complete. |
| 4 | `RegisterMask::all()` | Use complete-register containment for 16-bit predicate lanes. | Complete. |
| 5 | Runtime 128-bit whole-register bit shifts | Execute only the runtime count range selected by the caller. | Complete; distribution-sensitive tradeoff documented. |

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
indices to construct `PSHUFB` controls. Optimized MSVC output loads both
`_mm_setr_epi8` values directly from the constant pool, but SimdLib evaluates
instruction implementations against unoptimized output so correctness does not
depend on optimizer recognition. The `/Od` paths therefore remain authoritative.

The left-shift control computes `indices - counts`; its constant must be the
destination of the two-operand SSE subtraction and therefore requires a register
load. The right-shift control computes `biasedIndices + counts`, so reversing the
commutative intrinsic operands was evaluated as a way to fold the constant into
`PADDB`. MSVC canonicalizes both source expressions to the same constant-first
sequence, leaving the separate constant-pool load unchanged.

The `/Od` audit confirms that the existing local `_mm_setr_epi8` expressions each
produce one aligned constant-pool load, with no byte-wise selector stores and no
selector-construction helper call. An explicit aligned static array produces the
same instruction, while `make_static_register()` introduces calls and procedural
stack construction when optimization is disabled. A static native `__m128i`
requires dynamic initialization and is also unsuitable.

The remaining `/Od` stack traffic belongs to ordinary intrinsic temporaries and
the public API or Register call boundaries rather than selector preparation.
Item 2 therefore requires no source change: the existing expression already
produces the optimal selector instruction under the unoptimized policy.

## 3. Signed minimum-position reductions

Signed 8-, 16-, 32-, and 64-bit minimum-position implementations previously
constructed their 128-bit lane-index vectors through `register_from_values()`.
Under MSVC `/Od`, that path emitted 16 byte, 8 word, 4 double-word, or 2
quad-word immediate stores respectively before loading the completed vector.

The implementations now express those constants with the corresponding native
`_mm_setr_epi*` intrinsic, or `_mm_set_epi64x` for 64-bit lanes. MSVC lowers
each expression to one constant-pool vector load even without optimization and
does not call a construction helper. The 256-bit reductions divide their
inputs into 128-bit halves and therefore inherit the improved construction for
each half without requiring a separate 256-bit constant.

Validation results:

- the focused SSE4.2 position test passed 32 assertions;
- the focused AVX2 position test passed 64 assertions across 128- and 256-bit
  registers;
- fresh MSVC `/Od /Ob0` SSE4.2 and AVX2 output uses one `MOVDQA` or `VMOVDQU`
  constant-pool load for each signed lane-index vector;
- no scalar immediate lane stores or register-construction helper calls remain;
- remaining unoptimized spills and reloads are ordinary debug lowering after
  the constant has been loaded.

## 4. `RegisterMask::all()`

Canonical predicate lanes permit a complete-register containment test against an
all-one vector. The 16-bit specializations now use `PTEST` or `VPTEST` for this
purpose, avoiding the byte shuffle, byte movemask, and scalar mask comparison
required by their previous compact-mask path. Constant evaluation and non-x86
backends retain the portable compact-mask comparison, and other x86 element
widths retain their existing paths because their direct movemask instructions
are already competitive.

The runtime path creates the all-one predicate by comparing the byte view with
itself. This is expressed directly with the width-appropriate native intrinsics
so MSVC `/Od /Ob0` does not introduce calls to the constant-evaluation,
bit-cast, comparison, test, or compact-mask helpers. Optimized MSVC folds the
all-one value into the memory operand of `PTEST` or `VPTEST`.

Validation results:

- the focused SSE4.2 mask test passed 558 assertions;
- the focused AVX2 mask test passed 1,342 assertions;
- fresh MSVC `/Od /Ob0` output uses `PCMPEQB` plus `PTEST` for 128-bit masks and
  `VPCMPEQB` plus `VPTEST` for 256-bit masks, with no reduction-helper calls;
- optimized 128-bit output decreased from 25 bytes to 15 bytes and removes
  `PSHUFB`, `PMOVMSKB`, and the scalar comparison;
- optimized 256-bit output decreased from 44 bytes to 18 bytes and removes
  `VPSHUFB`, `VPMOVMSKB`, scalar byte-mask compaction, and the scalar comparison;
- signed and unsigned 16-bit masks produce identical reduction sequences, while
  8-, 32-, and 64-bit integer and floating-point paths remain unchanged.

## 5. Runtime 128-bit whole-register bit shifts

The previous runtime whole-register shifts clamped the count, computed results
for counts below and above 64 bits, and combined them. The implementations now
branch according to the public count contract and compute only the selected
identity, zero, below-64, exactly-64, or above-64 result. This mirrors the static
shift structure and avoids calculating a range result that will be discarded.

The change deliberately favors predictable or range-skewed counts. A matched
MSVC `/O2 /arch:AVX2` microbenchmark used chained results, 65,536-count inputs,
256 repetitions per sample, 11 alternating baseline/candidate rounds, and three
process runs on an Intel Family 6 Model 151 processor. The candidate took
approximately 51-73% of the previous time for mostly-below-64,
mostly-above-64, and predictable mixed distributions. Uniformly randomized
counts from 1 through 127 instead took approximately 2.27-2.51 times as long
because the range branch was unpredictable. This is a distribution-sensitive
local tradeoff, not evidence of an application-level speedup.

Validation results:

- five focused SSE4.2 and AVX2 runtime suites passed, including exhaustive
  runtime counts from 0 through 127 and negative and above-width boundaries;
- the API and Register constant-evaluation probes compiled successfully;
- MSVC `/Od /Ob0` helper size decreased from `0x1CE` to `0x173` bytes for SSE4.2
  and from `0x1D2` to `0x176` bytes for AVX2;
- optimized helper size decreased from `0x58` to `0x50` bytes for SSE4.2 and to
  `0x53` bytes for AVX2, although the static instruction count increased from 23
  to 25 or 26 because all range branches are present;
- the selected optimized runtime paths execute approximately 4-16 instructions
  rather than the complete branchless calculation.
