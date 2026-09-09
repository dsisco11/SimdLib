# uint128_t optimization report

## Scope and evidence

This report records optimization candidates identified by reviewing
[`UInt128.h`](../include/SimdLib/UInt128.h) and the generic operations in
[`Bmi.h`](../include/SimdLib/Bmi.h). It covers whole-value shifts, power-of-two
helpers, offset masks, arithmetic, bitwise operations, and additional BMI
overloads for `uint128_t`.

These are source-level opportunities, not measured performance improvements.
No candidate in this report has new assembly comparisons or runtime benchmark
results attached. Compilers may already eliminate some apparent overhead.
Implementation should follow focused code-generation checks and preserve the
existing public contracts documented in [TestCoverage.md](TestCoverage.md)
and [BmiContractMatrix.md](BmiContractMatrix.md).

The existing optimized `Bmi::bextr(uint128_t, control)` provides a relevant
design example: it operates directly on two scalar words on the x64 BMI1 path
and retains a separate constexpr/non-BMI fallback. Its success does not establish
that scalar operations will outperform SIMD for every other method.

## Candidate overview

| Priority | Candidate | Potential benefit | Qualification needed |
| --- | --- | --- | --- |
| High | Whole-value left/right shifts | Avoid synthesized SIMD runtime shifts and unnecessary scalar/vector transfers | Dynamic and constant counts, representative callers, enabled/disabled SIMD profiles |
| High | `bit_floor` | Select a word and use its scalar power-of-two operation | Zero, word transitions, highest bit, assembly comparison |
| Medium | `bit_ceil` | Avoid a general 128-bit subtraction and shift | Low/high word transitions and overflow semantics |
| Medium | `create_mask<width>(offset)` | Construct the covered words directly instead of shifting a 128-bit mask | Offset clamping, boundary crossing, truncation, constant widths |
| Investigate | Addition, subtraction, increment, decrement | Remove any surviving helper or carry-materialization overhead | Verify whether the compiler already emits a minimal carry/borrow chain |
| Investigate | AND, OR, XOR, NOT and compound forms | Avoid forced SIMD transitions in scalar arithmetic expressions | Compare scalar-oriented, vector-oriented, and memory-oriented consumers |
| Investigate | `Bmi::blsi`, `Bmi::blsr`, `Bmi::bzhi` for `uint128_t` | Replace composed 128-bit operations with word-specific operations | Zero and word-boundary semantics, generic dispatch, feature-profile equivalence |

## Whole-value shifts

### Current work

`shift_left` and `shift_right` select the SIMD runtime-count implementation when
SSE4.2 is enabled. That implementation synthesizes a complete 128-bit shift from
multiple lane shifts and cross-lane operations. The backing representation is
already two 64-bit words, so scalar consumers may pay unnecessary conversion and
instruction costs.

### Proposed direction

For an x64 scalar path, divide normalized counts into four cases:

- Zero: retain the input.
- 1 through 63: funnel-shift the word receiving cross-boundary bits and use an
  ordinary shift for the other word.
- 64 through 127: shift the one surviving source word and zero the other.
- 128 or greater: return zero.

Reuse the compiler-aware right-funnel operation where appropriate and provide
the corresponding left operation. Helper ownership must permit use by the class
without a declaration-order problem or a circular dependency. Use the existing
compiler/target configuration conventions and preserve constant evaluation.

The signed-count contract must remain unchanged: negative counts act as zero.
Boolean counts and large integral counts must retain their current behavior.
No C++ expression may shift a 64-bit word by 64 or more.

### Verification

Compare generated code for `<<`, `>>`, `<<=`, and `>>=` with both dynamic and
constant counts. Check representative composed expressions as well as isolated
calls. Exercise counts around 0, 63, 64, 127, and 128, negative counts, boolean
counts, and very large counts. Audit offset masks and power-of-two helpers as
downstream consumers before attributing their gains to separate changes.

## bit_floor

The current implementation finds the bit width and shifts a 128-bit value of one
to construct the result. Only one output word can be nonzero, so select the word
containing the highest set bit and apply `std::bit_floor` to that word:

```cpp
/** @brief Returns the greatest power of two not greater than the value. */
[[nodiscard]] constexpr uint128_t bit_floor(const uint128_t value) noexcept
{
    return value.high() != 0
        ? uint128_t{0, std::bit_floor(value.high())}
        : uint128_t{std::bit_floor(value.low()), 0};
}
```

This preserves zero naturally and avoids the general 128-bit shift. Verify zero,
exact powers of two, values on both sides of bit 64, bit 127, and the maximum
value. Compare assembly with the current implementation before claiming a gain.

## bit_ceil

The current implementation subtracts one as a full 128-bit value, computes its
bit width, and performs a general 128-bit shift. Compute the destination word
and bit directly instead.

For values fitting in the low word, rounding can either remain in that word or
carry into bit 64. For a nonzero high word, whether the low word is zero matters:
a high word that is already a power of two only represents an exact 128-bit
power of two when the low word is also zero.

Preserve the library contract: zero and one return one; an unrepresentable
rounded result returns zero. Do not call scalar `std::bit_ceil` with an input
whose result is unrepresentable in its scalar type. Test transitions around
bits 63, 64, and 127, exact powers, nonzero low words beneath high-word powers,
and overflow. Compare against any newly optimized shift implementation to avoid
overstating incremental benefit.

## Offset masks

`create_mask<width>(offset)` currently constructs a low-aligned 128-bit mask and
shifts it by the runtime offset. Construct each output word from its intersection
with the requested bit interval instead.

Compile-time widths may eliminate entire branches or one output word. Preserve
zero-width behavior, the compile-time width range of 0 through 128, nonpositive
offsets acting as zero, offsets of at least 128 returning zero, and truncation at
the upper boundary. Handle empty and full-word intersections without shifts by
64. Compare widths around 0, 1, 63, 64, 65, 127, and 128 with dynamic offsets.

## Arithmetic and increment/decrement

Addition and subtraction call carry/borrow helpers for both words. The outgoing
carry or borrow from the high word is computed by the helper but discarded by
the public operation. Optimizers may already remove that work and inline the
helpers into a short carry/borrow chain.

Inspect generated code before changing these methods. If overhead survives,
consider computing only the final high-word value, improving targeted inlining,
or specializing increment/decrement to propagate only low-word wraparound.
Compare the existing compiler-intrinsic and portable implementations rather than
assuming the intrinsic version is optimal.

Verify modulo-128-bit arithmetic, carry/borrow at bit 64, full-width wraparound,
self-aliasing compound assignments, and pre/post-increment/decrement return
semantics. Include composed arithmetic expressions in assembly comparisons.

## Bitwise operations

AND, OR, XOR, and NOT currently force a SIMD path when SSE4.2 is enabled.
Direct expressions on the two stored words may be preferable when surrounding
arithmetic already uses scalar registers. Conversely, SIMD can be advantageous
for vector-resident values or full-width memory operations.

Compare both approaches for isolated operations and mixed arithmetic/bitwise
expressions. Inspect loads, stores, register transfers, calls, and spills; do not
replace the SIMD path solely because two scalar expressions look simpler.
Include compound assignments and self-aliasing inputs. Keep the representation
and alignment contract unchanged.

## Additional uint128_t BMI overloads

Generic BMI helpers compose existing `uint128_t` arithmetic and bitwise
operations. Like `bextr`, some can instead operate on individual words:

- `blsi`: if the low word is nonzero, isolate its lowest bit and zero the high
  result word; otherwise isolate the lowest bit of the high word.
- `blsr`: if the low word is nonzero, clear its lowest bit and preserve the high
  word; otherwise clear the lowest set bit of the high word.
- `bzhi`: retain the requested prefix of low bits by handling indices below 64,
  64 through 127, and at least 128 separately. Only the boundary word needs a
  scalar mask operation.

Place type-specific overloads alongside the other `uint128_t` BMI overloads in
`UInt128.h`. Check lookup from generic consumers as well as direct calls:
existing explicit template calls may continue to select the generic template.
Preserve zero semantics and the public `bzhi` contract for arbitrary indices,
including indices above 255; do not allow a native control field to wrap them.

Compare each candidate under enabled and disabled BMI profiles, with an
independent word oracle and constexpr tests. Include low-word zero, high-word
zero, full zero, all ones, and the bits around the word boundary.

## Recommended order and acceptance evidence

Start with left/right shifts, then `bit_floor` and `bit_ceil`. Evaluate offset
masks against the improved shifts. Investigate arithmetic, bitwise operations,
and additional BMI overloads using representative callers before implementing
changes whose benefit is uncertain.

For each implemented candidate, record compiler versions, optimization flags,
enabled instruction families, and the actual generated code. Separate inlined
call-site evidence from out-of-line ABI costs. Run the existing focused runtime,
constexpr, and profile-equivalence checks, extending them only for missing
contracts. Use matched runtime benchmarks when claiming speedups; instruction
counts alone are not timing results.

This report does not authorize implementation or change the public API. It
records the candidates and the evidence needed to choose between them.
