# Tensor proposal

Status: proposed design for discussion; no implementation or performance qualification.

## Recommendation

Introduce `SimdLib::Tensor::TensorView<T, Extent = std::dynamic_extent>` as a non-owning,
contiguous numerical view. Pair it with a small expression system and explicit
evaluation into caller-provided storage. Element-wise expressions execute in
one traversal, processing full SIMD registers followed by the remaining elements.

`SimdLib::Tensor` is the namespace for the abstraction. Named operations live
there, giving call sites such as `Tensor::any(...)` and `Tensor::clamp(...)`
with `namespace Tensor = SimdLib::Tensor;`. The view type is explicitly named
`TensorView`, and can be imported with `using Tensor::TensorView;`.

The intended uses include terrain and voxel data, image and audio samples,
simulation fields, packed flags, and general numerical buffers. Execution is
CPU-only and initially synchronous on the calling thread. The abstraction's
value is managing traversal, composition, and memory contracts above the
existing register operations.

The first useful delivery should cover flat views, element-wise arithmetic and
bitwise expressions, predicates, selection, and explicit writes. Include a
`PackedTensorView` for unsigned sub-byte fields, with the packed operation subset
defined below. Numerical reductions, conversion, and shaped views have additional
contracts and should follow independently rather than inflate the first implementation.

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

`TensorView<T>` has runtime extent; `TensorView<T, N>` has compile-time extent. Both
borrow live `T` objects in contiguous storage. `TensorView<const T>` is read-only;
constness of the descriptor alone follows `std::span` semantics. Copying or
assigning a TensorView descriptor copies or rebinds the view. Numerical writes use
an explicit `assign(expression)` terminal, avoiding ambiguous copy assignment.

The initial ordinary-view element domain is the existing signed/unsigned 8-, 16-, 32-, and
64-bit integer lane types, `float`, and `double`. Exclude `bool`, `long double`,
`uint128_t`, and custom numeric types initially. Predicate expressions have
their own type and are not `TensorView<bool>` specializations.

Construction accepts compatible spans, C arrays, `std::array`, and borrowed
contiguous sized ranges. Require an lvalue for an owning range such as
`std::vector`; reject temporary owners and initializer-list storage. A temporary
span descriptor is acceptable if its underlying storage remains alive. Static
extent mismatches fail at compile time where known and otherwise are preconditions.

Expose `data()`, `size()`, `empty()`, `span()`, indexing, and contiguous
`subspan()` operations. Provide a static subspan overload that preserves known
extent. Ordinary storage needs only `alignof(T)`; use unaligned SIMD transfers
by default. An alignment promise is an optional, explicitly validated tuning
facility, not a condition of constructing TensorView.

No allocations, resizing, padding ownership, or storage lifetime extension are
implicit. A dynamic view can be rebound or sliced; it cannot resize its backing
allocation. If ownership later proves useful, add a separate `TensorBuffer<T>`
that produces views, without making static and dynamic TensorView mean different
ownership models.

## Packed element views

Add `SimdLib::Tensor::PackedTensorView<StorageWord, ElementBits,
Extent = std::dynamic_extent>` alongside `TensorView`. Storage words and logical
elements have separate widths: 64 `uint64_t` words contain 2,048 two-bit elements
in 512 bytes. Their dense equality mask occupies 2,048 bits, or 32 `uint64_t`
words (256 bytes).

Initially support unsigned 1-, 2-, and 4-bit elements in `uint32_t` or `uint64_t`
storage words. `StorageWord` may be const for read-only views. Field widths
divide the storage word width, so no element crosses a word boundary. Logical
element values use `std::uint8_t`, with a representable range of
`0 .. (1 << ElementBits) - 1`. Signed fields, arbitrary widths, and changes of
packed element width are subsequent work with separate contracts.

`size()` and static `Extent` count logical elements, never storage words. A
whole-storage constructor exposes every complete field; an explicit logical
count permits a partially used last word. Contiguous `subspan` operates in
logical elements and retains a first-field offset into the underlying storage.
Validate capacity/count/offset arithmetic without overflow. Field zero occupies
the lowest bits of its storage word; subsequent fields occupy successively
higher bits, followed by the next storage word.

Packed views borrow storage under the same owner-lifetime rules as ordinary
views. Reading an element produces a value, not `T&`; writing one updates only
that field. Expose backing words explicitly as `storage_words()` rather than
implying there is a `std::span<element_type>` over the packed values. Writes,
including fills and subview assignments, preserve all fields outside the view
and unused storage bits. Values outside the representable range violate the
write precondition; do not silently truncate them. Exact in-place element-wise
assignments load their inputs before replacing a word; other overlapping output
storage is excluded. Concurrent writes to distinct fields in one storage word
are not independently safe.

The initial packed subset supports equality/inequality, membership, predicate
composition, `Tensor::any`, `Tensor::all`, `Tensor::count`, `Tensor::pack_bits`,
field-wise bitwise expressions, selection, and explicit assignment/fill.
Bitwise results stay within `ElementBits`; in particular, value NOT complements
only the logical field bits. Predicate composition aligns results by logical
element index even when packed inputs have different first-field offsets.
Packed arithmetic and ordered comparisons are not implied by the ordinary-view
operation table. Arithmetic needs field-aware carry/overflow rules before it
can share the numerical surface. Mixed packed/ordinary numerical expressions
also require a separately specified conversion path.

Proposed call sites:

```cpp
namespace Tensor = SimdLib::Tensor;
using Tensor::PackedTensorView;

std::array<std::uint64_t, 64> storage{};
PackedTensorView<std::uint64_t, 2> voxels{storage};
std::array<std::uint64_t, 32> mask{};

// Equality produces one logical predicate per two-bit voxel.
auto equalTwo = voxels == std::uint8_t{2};
Tensor::pack_bits(equalTwo, std::span{mask});
bool found = Tensor::any(equalTwo);
std::size_t matchingCount = Tensor::count(equalTwo);

// Membership compares stored field values directly; no palette remapping occurs.
const std::array<std::uint16_t, 3> selectedValues{1, 2, 300};
auto selected = Tensor::is_in(voxels, std::span<const std::uint16_t>{selectedValues});
Tensor::pack_bits(selected, std::span{mask});
Tensor::pack_bits(~selected, std::span{mask});
```

`Tensor::is_in` is a lazy membership predicate: equality results are ORed across
the supplied candidates. Candidate spans are borrowed and must outlive
evaluation; expressions copy the descriptor, not the candidate data. Duplicate
candidates are harmless. An empty list matches nothing; its complement matches
every logical element. Accept integral comparison candidates and inspect their
original values before conversion: negative or too-large candidates cannot
match an unsigned packed field. This comparison rule deliberately differs from
the representability precondition for writes. In the example, 300 cannot match
any two-bit element and is ignored without narrowing to a different value.

### Packed equality and dense masks

Use the user's supplied `MaskEqual<Invert>` sketch as the implementation model:
compare fields in place, combine exact field-high-bit equality flags, compact
them, and assemble complete output words locally. Do not unpack a buffer into
one byte per element. The sketch supplies the algorithm; Tensor must adapt its
fixed-store assumptions to dynamic extents and subviews.

For field width `B` in an unsigned word, let `H` contain the high bit of every
field and let `L = ~H` within the word width. For four-bit fields, these repeat
`1000` and `0111`, respectively. Repeat a representable candidate in every field
and compute the following unsigned word-width expressions:

```cpp
const auto difference = packed ^ repeatedCandidate;
const auto equalFlags = ~(((difference & lowBits) + lowBits)
                         | difference | lowBits);
matches |= equalFlags;
```

Clearing each difference field's high bit before adding its lower-bit mask
ensures no carry leaves that field. A nonzero lower part sets the high bit;
ORing the original difference detects a high bit that was already set. Forcing
the lower bits to one and then inverting leaves exactly one high-bit flag for
each zero difference field. All other bits are zero. For one-bit fields the
lower-bit mask is zero and the same expression still applies. Scalar evaluation
must explicitly preserve the unsigned storage word width.

This exact per-field result matters for dense masks: a subtract-based zero-field
existence test can propagate a borrow into a neighboring field and mark it
incorrectly. Do not use such an existence-only test to construct a mask or count
individual matches. OR the exact equality flags across candidates first. For
non-membership, complement the completed union rather than complementing each
candidate comparison independently. When retaining dispersed flags, predicate
NOT is `(~matches) & validFieldHighBits`; it must not turn lower field bits or
padding into additional matches. `all` compares against that same valid-field
mask, while `any` and `count` ignore flags outside it.

Complete SIMD batches contain storage words as lanes; every lane still contains
multiple packed elements. Broadcast `L` and each repeated candidate, then apply
the same XOR/AND/add/OR/NOT sequence to every lane. Hoist invariant masks and
single-candidate broadcasts out of the hot loop where practical. The supplied
sketch uses `NativeRegister`; implement the ordinary C++20 path through `Api`
with equivalent operations, preserving the existing language-level contract.
Remaining source words use the identical arithmetic one word at a time, not a
loop over individual elements. No allocation is required.

Compact each word's high-bit flags with `SimdLib::Bmi::pext_u32` or `pext_u64`
using `H`. These helpers already provide a BMI2 path when compiled for it and
a portable fallback in [Bmi.h](../include/SimdLib/Bmi.h). Assemble the compacted
pieces in ascending logical order into a local output word, then store that word
once. `Tensor::any`, `Tensor::all`, and `Tensor::count` can inspect the valid
dispersed flags directly, avoiding compaction and materialized output.

For a word-aligned source and equal source/destination word widths `W`, one full
output word describes `W` elements and consumes `B` source words. An offset
subview can touch an additional source word. That relationship is a useful packing
rule, not a restriction on SIMD batch size. In particular, two-bit fields in
64-bit words consume only two source words per output, while a 256-bit register
holds four source words. Permit batches to span output-word boundaries, or use
an appropriately qualified narrower backend; do not let an output-local loop
silently eliminate SIMD for this principal use case. Source and output word
widths can also differ, so the general assembler tracks logical bits produced.

For arbitrary view lengths, recompute the available source words and valid
fields for each final batch; a single global `WordsPerOutput` is insufficient.
For example, 65 two-bit elements use three 64-bit source words and two output
words: the last output uses one source word, not two. Load only complete SIMD
batches within storage bounds and process remaining existing words individually.
Subview prefixes discard fields before the logical start, tails discard fields
after the logical end, and compaction maps the first selected logical element
to output bit zero. No read beyond the supplied word span is permitted.

After optional inversion, clear unused high bits in the last dense output word,
even when source padding is dirty. A full-word result must avoid shifts by `W`;
use a defined full-width mask case or the corresponding Bmi helper. An empty
view writes no output. Packed mask destinations have the exact required size
and must not overlap either packed source storage or borrowed candidate storage.
SIMD eligibility and BMI2 availability are independent compile-target decisions;
benchmark the fallback and extraction/assembly overhead as well as field comparison.

## Composition and evaluation

Use view members for observation and explicit mutation terminals; use constrained
operators and free functions in `SimdLib::Tensor` for expressions spanning
multiple inputs. This keeps multi-input operations symmetric and avoids a
large algorithm class. View types, expression types, and their operators share
that namespace so argument-dependent lookup finds the expression operators.

Proposed spelling, not currently compilable API:

```cpp
namespace Tensor = SimdLib::Tensor;
using Tensor::TensorView;

TensorView<const float> heights{std::span<const float>{heightStorage}};
TensorView<float> output{std::span<float>{outputStorage}};

// Compute the complete expression in one traversal of the buffers.
output.assign(Tensor::clamp(heights * 0.02F + 64.0F, -500.0F, 8'000.0F));

// Exact same-view mutation is supported for element-wise expressions.
output.assign(Tensor::clamp(output * 0.5F, 0.0F, 1.0F));

std::array<std::uint32_t, 64> flags{};
std::array<std::uint32_t, 64> allowed{};
TensorView<std::uint32_t, 64> f{flags};
TensorView<const std::uint32_t, 64> a{allowed};
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
| Numerical | `+`, `-`, `*`, `/`, `Tensor::min`, `Tensor::max`, `Tensor::clamp`, and separately named saturated integer operations where supported |
| Integer bits | `&`, `|`, `^`, `~`; shifts after their count semantics are specified |
| Predicates | `==`, `!=`, `<`, `<=`, `>`, `>=`, predicate `&`, `|`, `~`, and `Tensor::select` |
| Writes | `destination.assign(expression)` and `destination.fill(value)` |
| Predicate terminals | `Tensor::any`, `Tensor::all`, `Tensor::count`, and `Tensor::pack_bits` |
| Subsequent additions | `Tensor::sum`, `Tensor::min_value`, `Tensor::max_value`, `Tensor::dot`, and explicitly typed numerical conversion |

Selection evaluates both value branches; it is not scalar short-circuit
control flow. For example, `Tensor::select(x != 0, 1 / x, 0)` must not be advertised
as a safe integer divide-by-zero guard. Arbitrary user callbacks should be a
later extension requiring compatible scalar and SIMD implementations plus a
documented element-wise, side-effect-free contract. Existing `Api::transform`
continues to serve callers with raw register callbacks.

## Memory, tails, and numerical semantics

Element-wise tensor operands and their logical destination must have equal
logical extent. Membership candidate spans are sets and may have independent
lengths; dense mask word spans use the required packed output size instead.
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

For ordinary numerical views, use full-register processing plus a scalar
element remainder initially. Packed comparisons instead retain packed-word
processing for remaining words, as specified above. Scalar
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
  Define `Tensor::clamp(x, lo, hi)` as `Tensor::min(Tensor::max(x, lo), hi)` with `lo <= hi` and non-NaN
  bounds as preconditions.
- Expression traversal fusion does not itself authorize floating reassociation
  or contraction into FMA. Compiler floating-point options remain relevant;
  avoid a cross-toolchain bitwise promise. Any explicitly fused operation needs
  a separate rounding/fallback contract before exposure.
- No global exception-flag/trapping equivalence with a scalar loop is promised.
  Avoid performing extra operations on fictitious tail elements nonetheless.

Ordinary-view predicate expressions remain SIMD masks within a block; packed
predicates can retain exact field-high-bit flags until a terminal requires a
dense mask. Their public composition and logical ordering agree. `any`/`all` may stop
early; `count` returns `std::size_t`. Empty predicates give false/true/zero,
respectively. Materialize only on request:

```cpp
auto selected = heights > 100.0F;
bool found = Tensor::any(selected);
std::size_t count = Tensor::count(selected);
output.assign(Tensor::select(selected, heights, 0.0F));
Tensor::pack_bits(selected, std::span<std::uint32_t>{maskWords});
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
contracts. For example, a proposed `Tensor::convert<std::int32_t>(values,
Tensor::rounding::toward_zero)` requires finite, representable results; saturated
conversion is a separate operation. No generic `static_cast` promise should
hide different SIMD rounding behavior.

Widening/narrowing preserves logical element count while changing byte count.
It may emit or consume several registers per block. Do not forward `widen_low`
and accidentally discard the upper source elements. Same-size element-wise
`Tensor::bit_cast<U>` is distinct from numeric conversion. Raw byte reinterpretation
that changes element count, alignment, or object lifetime is outside the
initial Tensor contract. General in-place type conversion is deferred.

Retain `TensorView<T, N>` as the flat contiguous primitive. Add a separate
`ShapedTensorView<T, Rank>` when multidimensional workflows are implemented:
it lives in `SimdLib::Tensor` and combines a flat TensorView with fixed-rank runtime extents and a canonical
row-major layout. Check extent products for overflow and agreement with flat
size. `reshape` changes metadata only when the element count is preserved.
Equal-shape element-wise work delegates to the flat evaluator.

Contiguous row/slab slices can return TensorView objects. A column, transpose, or
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
is not a substitute. Keep ISA width out of TensorView's public representation;
do not exchange target-dependent expression implementation types across
translation units with incompatible configuration. Use consistent header
configuration, or explicit out-of-line dispatch boundaries for multiversioning.

Proposed ownership of files:

```text
include/SimdLib/Tensor.h                    # Focused public umbrella
include/SimdLib/Tensor/TensorView.h         # Contiguous descriptor
include/SimdLib/Tensor/PackedTensorView.h    # Packed storage descriptor
include/SimdLib/Tensor/Arithmetic.h         # Arithmetic expression builders
include/SimdLib/Tensor/Bitwise.h            # Bitwise expression builders
include/SimdLib/Tensor/Comparison.h         # Predicates and selection
include/SimdLib/Tensor/Evaluation.h         # Explicit evaluation terminals
include/SimdLib/Tensor/Reduction.h          # Whole-span reductions, when added
include/SimdLib/Tensor/Conversion.h         # Conversion contracts, when added
include/SimdLib/Detail/Tensor/Expression.h   # Typed expression representation
include/SimdLib/Detail/Tensor/Evaluator.h    # Validation, traversal, tail handling
include/SimdLib/Detail/Tensor/PackedComparison.h # Exact packed field comparisons
include/SimdLib/Detail/Tensor/MaskWriter.h   # Dense predicate output assembly
```

Operation families own their scalar/SIMD semantics; the evaluator owns traversal.
Dependencies flow toward `Api` and `Bmi`, never from those layers or Register
back into Tensor.
Keep the umbrella thin. Memory-writing terminals use the appropriate
`SIMD_FLAGS(Neither, ...)` boundary and must not claim `RegisterOnly`.
Do not create a separate customization framework merely to share simple calls.

## Migration

| Existing operation | Proposed equivalent or disposition |
| --- | --- |
| Static `AnyEqual(read, value)` | `Tensor::any(tensor == value)` for static and dynamic views |
| Static `AllEqual(read, value)` | `Tensor::all(tensor == value)` |
| Static `Compare(read, write, value)` | `Tensor::pack_bits(tensor == value, write)`; preserve existing valid-input bit order and add defined arbitrary tails |
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

For packed views, verify every supported field/storage width against an
independent per-element oracle. Include singleton, duplicate, multiple, empty,
negative, and too-large candidates; equality unions and their complements;
adjacent fields that expose borrow-based false flags; dirty prefix/tail bits;
non-word-aligned subviews; full and partial output words; and lengths around
storage-word, output-word, and SIMD-batch boundaries. Test 65 two-bit elements
explicitly to catch fixed final-batch bounds, and the 2,048-element voxel example.
Verify mutation preserves neighboring fields, write range checks occur before
narrowing, candidate/output overlap is excluded, and all/count/any ignore padding.
Qualify both BMI2-enabled and portable compaction independently of SIMD width.

Compare against existing SimdAlgo on its supported valid inputs. Check focused
and umbrella headers, C++20 core integration, constexpr subsets, installation,
and configuration consistency. Preserve the project's compiler and generated-code
qualification practices; no benchmark substitutes for semantic tests.

Benchmark small static inputs, cache-resident buffers, and buffers exceeding
last-level cache. Include aligned and offset spans, exact widths and tails,
unary/binary writes, a scale/add/clamp composition, bitwise compositions,
early/late/no-match predicates, and eventually conversions and reductions.
Include packed 1-/2-/4-bit equality and membership with varying candidate counts,
inverted masks, subviews, dirty tails, and lengths that expose output-local SIMD
batching limits. Compare fused packed-field evaluation against a per-element
reference and equivalent direct Api/Bmi loops, with and without compiled BMI2.
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
