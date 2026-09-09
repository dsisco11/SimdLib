# Tensor proposal

Status: proposed design for discussion; no implementation or performance qualification.

## Recommendation

Introduce `SimdLib::Tensor<T, Extent = std::dynamic_extent>` as a non-owning,
contiguous numerical view. Pair it with a small expression system and explicit
evaluation into caller-provided storage. Element-wise expressions execute in
one traversal, processing full SIMD registers followed by the remaining elements.

The intended uses include terrain and voxel data, image and audio samples,
simulation fields, packed flags, and general numerical buffers. Execution is
CPU-only and initially synchronous on the calling thread. The abstraction's
value is managing traversal, composition, and memory contracts above the
existing register operations.

The first useful delivery should cover flat views, element-wise arithmetic and
bitwise expressions, predicates, selection, and explicit writes. Reductions,
conversion, and shaped views have additional contracts described below and
should follow independently rather than inflate the first implementation.

## Evidence from this checkout

| Existing surface | Consequence for Tensor |
| --- | --- |
| [Api.h](../include/SimdLib/Api.h) implements in-place unary, separate-output unary, and binary `transform`. | The full-register traversal pattern already exists. Tensor must add useful composition and contracts rather than just rename these methods. |
| Those transforms zero-initialize scratch arrays for the tail and invoke the same SIMD callback. They use the left input's length without an explicit equal-length check on all operands. | A new evaluator needs its own extent validation and tail semantics. Reusing these helpers indiscriminately would inherit assumptions unsuitable for general numerical expressions. |
| [SimdAlgo.h](../include/SimdLib/SimdAlgo.h) contains static-extent equality queries and packed comparisons, plus static/dynamic bitwise operations. | There is a concrete, modest migration surface. Numeric processing is substantially broader than the existing algorithm facade. |
| `SimdAlgo::SimdImpl` and `ChooseSimd` choose 128/256 bits from input size; `NativeApi` chooses from compile-target availability. | Tensor should select only compiled, available backends. Input length is a tuning decision, not CPU feature detection. |
| `SimdAlgo::ConvertInplace` is commented-out code. | In-place conversion is an unimplemented idea, not a compatibility obligation. |
| [Register.h](../include/SimdLib/Register.h) owns one complete register; [its collection rejection probe](../tests/compile_fail/register/RegisterCollectionOperations.cpp) excludes collection methods. | Tensor belongs above the register layer and must own tail and collection policies. |
| `Register::convert` initially covers 32-bit signed/unsigned integers to float and float to signed 32-bit integers; widening has explicit low-lane consumption. | Arbitrary span conversion cannot be promised by forwarding one register call. Width-changing conversions must consume every source element. |
| [Config.h](../include/SimdLib/Config.h) describes compile-target features, and [CMakeLists.txt](../CMakeLists.txt) separates C++20 core from opt-in C++23 Register. | Keep ordinary Tensor use C++20-compatible through `Api`. A C++23 custom-register interface can be optional later. |
| [Transform tests](../tests/TestSupport.h) exercise empty, single, exact-register, tail, and multiple-register extents. [Core benchmarks](../benchmarks/Core.benchmarks.cpp) mostly measure small primitive operations. | Existing tests supply useful cases; sustained buffer throughput and fusion still need new evidence. |

This is source inspection, not fresh build/test evidence. External consumers
were not inventoried: the expected sibling source directories were unavailable
in this workspace. Any downstream migration remains subject to that inventory.

The [development roadmap](SimdLibDevelopment.todo) explicitly leaves ownership,
rank, evaluation, and migration open. [project.todo](project.todo) additionally
requests multidimensional shape, broadcasting, reshaping, and slicing. The
design below gives those requirements an explicit extension boundary.

## View and ownership contract

`Tensor<T>` has runtime extent; `Tensor<T, N>` has compile-time extent. Both
borrow live `T` objects in contiguous storage. `Tensor<const T>` is read-only;
constness of the descriptor alone follows `std::span` semantics. Copying or
assigning a Tensor descriptor copies or rebinds the view. Numerical writes use
an explicit `assign(expression)` terminal, avoiding ambiguous copy assignment.

The initial element domain is the existing signed/unsigned 8-, 16-, 32-, and
64-bit integer lane types, `float`, and `double`. Exclude `bool`, `long double`,
`uint128_t`, and custom numeric types initially. Predicate expressions have
their own type and are not `Tensor<bool>` specializations.

Construction accepts compatible spans, C arrays, `std::array`, and borrowed
contiguous sized ranges. Require an lvalue for an owning range such as
`std::vector`; reject temporary owners and initializer-list storage. A temporary
span descriptor is acceptable if its underlying storage remains alive. Static
extent mismatches fail at compile time where known and otherwise are preconditions.

Expose `data()`, `size()`, `empty()`, `span()`, indexing, and contiguous
`subspan()` operations. Provide a static subspan overload that preserves known
extent. Ordinary storage needs only `alignof(T)`; use unaligned SIMD transfers
by default. An alignment promise is an optional, explicitly validated tuning
facility, not a condition of constructing Tensor.

No allocations, resizing, padding ownership, or storage lifetime extension are
implicit. A dynamic view can be rebound or sliced; it cannot resize its backing
allocation. If ownership later proves useful, add a separate `TensorBuffer<T>`
that produces views, without making static and dynamic Tensor mean different
ownership models.

## Composition and evaluation

Use view members for observation and explicit mutation terminals; use constrained
operators and free functions for expressions spanning multiple inputs. This
keeps multi-input operations symmetric and avoids a large algorithm class.

Proposed spelling, not currently compilable API:

```cpp
using SimdLib::Tensor;
using SimdLib::clamp;

Tensor<const float> heights{std::span<const float>{heightStorage}};
Tensor<float> output{std::span<float>{outputStorage}};

// Compute the complete expression in one traversal of the buffers.
output.assign(clamp(heights * 0.02F + 64.0F, -500.0F, 8'000.0F));

// Exact same-view mutation is supported for element-wise expressions.
output.assign(clamp(output * 0.5F, 0.0F, 1.0F));

std::array<std::uint32_t, 64> flags{};
std::array<std::uint32_t, 64> allowed{};
Tensor<std::uint32_t, 64> f{flags};
Tensor<const std::uint32_t, 64> a{allowed};
f.assign((f & a) | std::uint32_t{0x10});
```

Arithmetic builds a typed, lazy expression; `assign`, predicate reductions,
mask packing, and numerical reductions execute immediately. Scalar operands
broadcast explicitly through their position in an expression. Initially require
the same element type across Tensor operands and matching scalar types; explicit
conversion nodes handle type changes. Do not silently apply C++ integer
promotion to change the storage/result type.

Expression nodes own copies of child expressions, scalar constants, and view
descriptors. This prevents dangling references to temporary expression nodes.
Backing buffers must outlive evaluation and remain stable during it. Expressions
observe buffer contents at evaluation time; they are not snapshots, and using
one twice recomputes it.

Fusion is limited to pure, element-wise operations. Build typed nodes rather
than a runtime graph, virtual dispatcher, optimizer, or scheduler. Do not promise
common-subexpression elimination or unlimited fusion without spills. Callers
can materialize an intermediate explicitly when reuse or register pressure
makes that worthwhile.

Initial building blocks:

| Family | Proposed operations |
| --- | --- |
| Numerical | `+`, `-`, `*`, `/`, `min`, `max`, `clamp`, and separately named saturated integer operations where supported |
| Integer bits | `&`, `|`, `^`, `~`; shifts after their count semantics are specified |
| Predicates | `==`, `!=`, `<`, `<=`, `>`, `>=`, predicate `&`, `|`, `~`, and `select` |
| Writes | `destination.assign(expression)` and `destination.fill(value)` |
| Predicate terminals | `any`, `all`, `count`, and `pack_bits` |
| Subsequent additions | `sum`, `min_value`, `max_value`, `dot`, and explicitly typed numerical conversion |

Selection evaluates both value branches; it is not scalar short-circuit
control flow. For example, `select(x != 0, 1 / x, 0)` must not be advertised
as a safe integer divide-by-zero guard. Arbitrary user callbacks should be a
later extension requiring compatible scalar and SIMD implementations plus a
documented element-wise, side-effect-free contract. Existing `Api::transform`
continues to serve callers with raw register callbacks.

## Memory, tails, and numerical semantics

All non-scalar operands and the destination must have equal logical extent.
Statically known mismatches are rejected at compile time. Validate dynamic
contracts once at the evaluation boundary through `SIMDLIB_PRECONDITION`, whose
default checks disappear under `NDEBUG`; do not imply unconditional runtime
error handling. Named built-in operations allocate nothing and are `noexcept`.

For element-wise writes, allow disjoint output or exact same-type, same-range
aliasing with any input. Load all inputs for a block before storing its result.
Reject shifted or partial output overlap as a precondition; do not silently
allocate a temporary. This permits `a.assign(a + b)` but excludes writing into
`a.subspan(1)` while reading an overlapping prefix. Type-changing writes require
disjoint destination storage. Future permutations and stencils need separate
alias rules.

Use full-register processing plus a scalar remainder initially. Scalar
operation definitions must match the SIMD contract, including modular integer
arithmetic and floating-point edge cases. Blind zero padding can introduce
invalid inactive-lane division, change floating exception flags, or corrupt
horizontal reductions. Masking the final result does not undo those effects.
No public partial-register type or `SimdVector` invariant is needed for this
tail strategy. Later optimized tails require operation-specific proof.

Specify semantics by operation rather than accepting whatever a fallback does:

- Integer add/subtract/multiply use lane-width modular results, implemented
  without signed C++ overflow in scalar evaluation. Saturation has separate names.
- Integer division requires nonzero divisors and excludes signed minimum divided
  by negative one. Existing integer division is synthesized from scalar lane
  work; a Tensor name does not make every operation a native packed instruction.
- Floating comparisons follow the corresponding documented backend semantics;
  define `!=` explicitly as the complement of equality, including NaNs.
- `min` and `max` must retain a specified operand order and reproduce the
  selected intrinsic's NaN and signed-zero behavior in the scalar path.
  Define `clamp(x, lo, hi)` as `min(max(x, lo), hi)` with `lo <= hi` and non-NaN
  bounds as preconditions.
- Expression traversal fusion does not itself authorize floating reassociation
  or contraction into FMA. Compiler floating-point options remain relevant;
  avoid a cross-toolchain bitwise promise. Any explicitly fused operation needs
  a separate rounding/fallback contract before exposure.
- No global exception-flag/trapping equivalence with a scalar loop is promised.
  Avoid performing extra operations on fictitious tail elements nonetheless.

Predicate expressions remain SIMD masks within a block. `any`/`all` may stop
early; `count` returns `std::size_t`. Empty predicates give false/true/zero,
respectively. Materialize only on request:

```cpp
auto selected = heights > 100.0F;
bool found = SimdLib::any(selected);
std::size_t count = SimdLib::count(selected);
output.assign(SimdLib::select(selected, heights, 0.0F));
SimdLib::pack_bits(selected, std::span<std::uint32_t>{maskWords});
```

Packed output uses unsigned words, ascending input index into ascending bit
position, exactly `size / word_bits + (size % word_bits != 0)` words, and zeroed
unused high bits in the last word. Packed output must not overlap inputs.
Scalar `any` and `all` avoid building a whole mask buffer.

Numerical reductions are separate kernels; register-local horizontal operations
are not automatically whole-buffer reductions. Require explicit accumulator
type for integer sums and dot products, with a documented overflow contract.
Provide an ordered floating sum as the conservative default and an explicitly
named fast reduction that permits SIMD regrouping. Fast reduction results may
vary by width and target. Empty sums return their identity; extrema should
return `std::optional<T>`, with NaN behavior separately specified. Do not ship
an underspecified generic `reduce` first.

## Conversion and shape

Expose numerical conversion as an expression evaluated into separately typed
storage, with explicit rounding and range behavior. Start with the existing
equal-lane-count 32-bit integer/float combinations after selecting their
contracts. For example, a proposed `convert<std::int32_t>(values,
rounding::toward_zero)` requires finite, representable results; saturated
conversion is a separate operation. No generic `static_cast` promise should
hide different SIMD rounding behavior.

Widening/narrowing preserves logical element count while changing byte count.
It may emit or consume several registers per block. Do not forward `widen_low`
and accidentally discard the upper source elements. Same-size element-wise
`bit_cast<U>` is distinct from numeric conversion. Raw byte reinterpretation
that changes element count, alignment, or object lifetime is outside the
initial Tensor contract. General in-place type conversion is deferred.

Retain `Tensor<T, N>` as the flat contiguous primitive. Add a separate
`ShapedTensorView<T, Rank>` when multidimensional workflows are implemented:
it combines a flat Tensor view with fixed-rank runtime extents and a canonical
row-major layout. Check extent products for overflow and agreement with flat
size. `reshape` changes metadata only when the element count is preserved.
Equal-shape element-wise work delegates to the flat evaluator.

Contiguous row/slab slices can return Tensor views. A column, transpose, or
step slice generally needs a distinct strided view and execution path; it must
not masquerade as contiguous storage. Initially support scalar broadcasting;
axis broadcasting, strided slices, and axis reductions are subsequent shaped
operations with explicit rules. This supports the roadmap's multidimensional
direction without putting runtime shape/stride overhead on every flat buffer.

## Execution and source organization

Keep ordinary Tensor C++20-compatible and delegate SIMD operations to the
public `Api` facade. Select 256-bit operations only when available for the
compile target, otherwise 128-bit operations when available, with scalar
execution for supported operations when no compiled SIMD path applies.
This does not extend the library's supported platform matrix beyond its
currently qualified targets.

Use scalar evaluation during constant evaluation for the supported operation
subset. Both static and dynamic views may participate when their backing
storage is valid in that constant expression; static extent does not imply
all numerical operations are constexpr.

Runtime CPU dispatch is a separate future facility: it needs separately
compiled target variants and CPU/OS feature checks. Choosing based on length
is not a substitute. Keep ISA width out of Tensor's public representation;
do not exchange target-dependent expression implementation types across
translation units with incompatible configuration. Use consistent header
configuration, or explicit out-of-line dispatch boundaries for multiversioning.

Proposed ownership of files:

```text
include/SimdLib/Tensor.h                    # Focused public umbrella
include/SimdLib/Tensor/View.h               # Contiguous descriptor
include/SimdLib/Tensor/Arithmetic.h         # Arithmetic expression builders
include/SimdLib/Tensor/Bitwise.h            # Bitwise expression builders
include/SimdLib/Tensor/Comparison.h         # Predicates and selection
include/SimdLib/Tensor/Evaluation.h         # Explicit evaluation terminals
include/SimdLib/Tensor/Reduction.h          # Whole-span reductions, when added
include/SimdLib/Tensor/Conversion.h         # Conversion contracts, when added
include/SimdLib/Detail/Tensor/Expression.h   # Typed expression representation
include/SimdLib/Detail/Tensor/Evaluator.h    # Validation, traversal, tail handling
```

Operation families own their scalar/SIMD semantics; the evaluator owns traversal.
Dependencies flow toward `Api`, never from Register or Api back into Tensor.
Keep the umbrella thin. Memory-writing terminals use the appropriate
`SIMD_FLAGS(Neither, ...)` boundary and must not claim `RegisterOnly`.
Do not create a separate customization framework merely to share simple calls.

## Migration

| Existing operation | Proposed equivalent or disposition |
| --- | --- |
| Static `AnyEqual(read, value)` | `any(tensor == value)` for static and dynamic views |
| Static `AllEqual(read, value)` | `all(tensor == value)` |
| Static `Compare(read, write, value)` | `pack_bits(tensor == value, write)`; preserve existing valid-input bit order and add defined arbitrary tails |
| Static/dynamic `BitwiseAnd` | `out.assign(a & b)` |
| Static/dynamic `BitwiseOr` | `out.assign(a | b)` |
| Static/dynamic `BitwiseXor` | `out.assign(a ^ b)` |
| Static/dynamic `BitwiseNot` | `out.assign(~a)` |
| Static/dynamic `BitwiseAndNot` | `out.assign((~a) & b)`; preserve intrinsic operand order |
| Commented `ConvertInplace` | No live API to migrate; use explicit disjoint conversion initially |
| Internal size-based chooser | Evaluator backend selection constrained by compiled availability |
| `Api::transform` overloads | Retain low-level callbacks; Tensor handles built-in composable expressions |
| General `Api::transform_pack<K>` | Retain low-level arbitrary packed fields; Tensor predicates initially cover one-bit results |
| `SimdResample` | Retain its specialized byte-mask packing/expansion role |

Add Tensor without deprecating existing APIs. Migrate downstream call sites only
after their contracts are inventoried and equivalent behavior and performance
are demonstrated. Do not change roadmap completion markers for this proposal.

## Evidence required before adoption

Correctness should use independent scalar oracles across supported types and
compiled widths: empty inputs, lengths around every register boundary, offsets
that break SIMD alignment, static/dynamic extents, exact aliasing, rejected
partial overlap, expression lifetime, and guard-protected tails. Cover modular
overflow, valid/invalid division, NaNs, infinities, signed zero, conversion
boundaries, mask ordering, and final-word padding. Include compile rejection
for mismatched static extents, mutation through const elements, unsupported
types, and temporary owning sources.

Compare against existing SimdAlgo on its supported valid inputs. Check focused
and umbrella headers, C++20 core integration, constexpr subsets, installation,
and configuration consistency. Preserve the project's compiler and generated-code
qualification practices; no benchmark substitutes for semantic tests.

Benchmark small static inputs, cache-resident buffers, and buffers exceeding
last-level cache. Include aligned and offset spans, exact widths and tails,
unary/binary writes, a scale/add/clamp composition, bitwise compositions,
early/late/no-match predicates, and eventually conversions and reductions.
Compare Tensor against handwritten scalar loops with normal optimization,
direct fused Api loops, separate Api passes, and SimdAlgo where applicable.
Report compiler/options, ISA, sizes, throughput, repetitions, and observable
outputs. Inspect assembly for vector work, accidental allocation, redundant
passes, and spills. Measure compile time and object size as expression depth grows.

Acceptance requires clear call sites, verified allocation-free terminals, one
traversal for representative expressions, correct tails and aliases, and no
material regression against equivalent fused Api kernels. Set numerical
regression budgets before measurements. No speedup or strict zero-overhead
claim is established by this document. Ownership naming, flat/shaped separation,
and the expression/terminal model are the principal design decisions to settle
before implementation.
