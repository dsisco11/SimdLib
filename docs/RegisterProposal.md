# Register Class Proposal

Status: proposed design; no public API or compatibility commitment has been made.

## Summary

Add `SimdLib::Register<element_t, register_width>` as the recommended value-like
interface for operations on one complete SIMD register. Unlike
`SimdVector<element_t, element_count>`, a `Register` has no logical element
count that can be smaller than its hardware lane count. Every lane always
participates in loads, stores, arithmetic, comparisons, rearrangements, and
reductions.

`Register` will compose the existing `Api<register_width, element_t>` facade
instead of inheriting from it. This preserves the established implementation
and feature-routing behavior while presenting an interface that supports
natural expression chaining and keeps native intrinsic types out of ordinary
call sites.

The existing `Api` remains a supported compatibility and backend-facing
surface while `Register` reaches operation parity. Span-wide algorithms and
partial-register handling remain outside `Register`.

## Motivation

The current `Api` facade exposes register operations as static functions:

```cpp
using FloatApi = SimdLib::NativeApi<float>;

const auto scale = FloatApi::set1(0.02F);
const auto offset = FloatApi::set1(64.0F);
const auto input = FloatApi::load(source);
const auto output = FloatApi::add(FloatApi::multiply(input, scale), offset);
FloatApi::store(output, destination);
```

This is precise but verbose. Intermediate values have compiler intrinsic types,
so the type carrying the element and register-width contract is separate from
the value being manipulated. `SimdVector` provides a friendlier value-like
interface, but it also owns a logical element-count contract and must preserve
zero-filled inactive lanes. That behavior is valuable for fixed logical
vectors, but it is unnecessary and sometimes actively undesirable in
register-oriented code.

The proposed interface keeps the low-level, complete-register semantics while
making the value itself carry the contract:

```cpp
using FloatRegister = SimdLib::NativeRegister<float>;

const auto scale = FloatRegister::broadcast(0.02F);
const auto offset = FloatRegister::broadcast(64.0F);
const auto output = FloatRegister::load(source) * scale + offset;
output.store(destination);
```

## Goals

- Represent exactly one 128-bit or 256-bit SIMD register.
- Treat every hardware lane as active at all times.
- Provide a value-like, chainable interface over the operations currently
  curated by `Api`.
- Preserve the existing type, width, feature, fallback, and `constexpr`
  contracts wherever the corresponding `Api` operation already defines them.
- Remain a zero-overhead abstraction with one native register data member, no
  allocation, and no runtime metadata.
- Make broadcasts, native-register interoperation, numeric conversion, and bit
  reinterpretation explicit.
- Constrain unsupported operations away instead of accepting a call that fails
  inside an implementation body.
- Establish a credible migration path that does not prematurely remove or
  deprecate `Api`.

## Non-goals

- Representing a logical vector whose element count is smaller than a hardware
  register.
- Loading, storing, or constructing partial registers.
- Automatically filling lanes with zero, one, or any other neutral value.
- Iterating across arbitrary spans or handling a final partial batch.
- Owning dynamic storage, shapes, strides, or multidimensional data.
- Replacing `SimdVector`, `SimdAlgo`, `SimdResample`, or the future `Tensor`
  abstraction.
- Hiding whether a width-changing operation consumes or produces more than one
  register.

## Responsibility boundaries

| Surface | Responsibility | Partial data |
| --- | --- | --- |
| `Register<T, Bits>` | One complete hardware register | Rejected |
| `SimdVector<T, N>` | One fixed logical value | Inactive lanes are managed by the type |
| `SimdAlgo` and future `Tensor` operations | Collections and batches | Tail policy belongs to the algorithm |
| `Api<Bits, T>` | Compatibility facade and implementation routing | Existing behavior remains supported |

`Register` deliberately has no equivalent to `Api::load_partial`,
`Api::set_partial`, or `Api::setr_partial`. A caller with fewer than
`lane_count` elements must use a higher-level abstraction or explicitly stage
a complete register with a fill policy chosen by that caller.

## Type shape and availability

The proposed primary template puts the element type first, matching
`SimdVector`, and keeps the register width explicit:

```cpp
namespace SimdLib
{

/**
 * @brief Reports whether a complete SIMD register is available for an element
 *        type and register width.
 */
template <class element_t, std::size_t register_width>
inline constexpr bool is_register_available_v =
	SimdLib::is_api_available_v<register_width, element_t>;

/**
 * @brief Constrains a type and width to a supported complete SIMD register.
 */
template <class element_t, std::size_t register_width>
concept RegisterAvailable =
	is_register_available_v<element_t, register_width>;

/**
 * @brief Owns one complete SIMD register whose lanes are all active.
 * @tparam element_t Scalar interpretation of each register lane.
 * @tparam register_width Width of the native register in bits.
 */
template <class element_t, std::size_t register_width>
	requires RegisterAvailable<element_t, register_width>
class Register final;

/**
 * @brief Selects the widest register available for an element type.
 * @tparam element_t Scalar interpretation of each register lane.
 */
template <class element_t>
	requires RegisterAvailable<element_t, 128>
using NativeRegister = Register<
	element_t,
	is_register_available_v<element_t, 256> ? 256 : 128>;

} // namespace SimdLib
```

The primary template should not default `register_width`. `NativeRegister<T>`
makes target-selected width visible at the call site, while
`Register<T, Bits>` remains suitable for stable storage, interfaces, and ABI
contracts. Consumers should avoid placing `NativeRegister<T>` in an ABI that
must remain identical across different compiler feature configurations.

## Complete-register invariant

For `Register<T, Bits>`:

- `lane_count == Bits / (sizeof(T) * 8)`.
- `byte_count == Bits / 8`.
- The object contains exactly one `Api<Bits, T>::vector_t` value.
- No active-lane count or active-lane mask is stored or computed.
- Every operation observes and produces all `lane_count` lanes.
- Whole-register equality and reductions include the highest lane.
- A full load requires a fixed-extent span of exactly `lane_count` elements.
- A full store writes exactly `lane_count` elements.
- Construction from lane values requires exactly `lane_count` arguments.
- Default construction produces a fully initialized zero register.

Zero-initialized default construction gives `Register{}` ordinary value-type
semantics. It does not represent inactive-lane filling: every resulting zero
lane is active. Callers that do not need the initial value can rely on normal
compiler dead-store elimination or initialize directly from a load or
operation.

## Core interface sketch

The following sketch defines the intended shape rather than an exhaustive
operation list:

```cpp
/**
 * @brief Owns one complete SIMD register whose lanes are all active.
 * @tparam element_t Scalar interpretation of each register lane.
 * @tparam register_width Width of the native register in bits.
 */
template <class element_t, std::size_t register_width>
	requires RegisterAvailable<element_t, register_width>
class Register final
{
  public:
	using element_type = element_t;
	using api_type = Api<register_width, element_type>;
	using native_type = typename api_type::vector_t;
	using mask_type = RegisterMask<element_type, register_width>;

	constexpr static inline std::size_t bit_count = register_width;
	constexpr static inline std::size_t byte_count = api_type::byte_count;
	constexpr static inline std::size_t lane_count = api_type::element_count;

	/** @brief Constructs a register with every active lane set to zero. */
	SIMDLIB_FORCE_INLINE constexpr Register() noexcept;

	/**
	 * @brief Wraps one complete native register without changing its bits.
	 * @param value Complete native register value.
	 */
	SIMDLIB_FORCE_INLINE constexpr explicit Register(native_type value) noexcept;

	/**
	 * @brief Returns a register with every active lane set to zero.
	 * @return Fully initialized zero register.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static Register zero() noexcept;

	/**
	 * @brief Broadcasts one scalar value to every active lane.
	 * @param value Scalar value to broadcast.
	 * @return Register containing `value` in every lane.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static Register broadcast(
		element_type value) noexcept;

	/**
	 * @brief Constructs a register from exactly one complete logical lane list.
	 * @param lanes Values in low-to-high logical lane order.
	 * @return Register containing all supplied lane values.
	 */
	template <std::convertible_to<element_type>... lane_types>
		requires(sizeof...(lane_types) == lane_count)
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static Register from_lanes(
		lane_types &&...lanes) noexcept;

	/**
	 * @brief Loads a complete register from potentially unaligned storage.
	 * @param source Source containing exactly one register of elements.
	 * @return Register loaded from `source`.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE static Register load(
		std::span<const element_type, lane_count> source) noexcept;

	/**
	 * @brief Loads a complete register from register-aligned storage.
	 * @param source Aligned source containing exactly one register of elements.
	 * @return Register loaded from `source`.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE static Register load_aligned(
		std::span<const element_type, lane_count> source) noexcept;

	/**
	 * @brief Stores every active lane to potentially unaligned storage.
	 * @param destination Destination for exactly one register of elements.
	 */
	SIMDLIB_FORCE_INLINE void store(
		std::span<element_type, lane_count> destination) const noexcept;

	/**
	 * @brief Stores every active lane to register-aligned storage.
	 * @param destination Aligned destination for one complete register.
	 */
	SIMDLIB_FORCE_INLINE void store_aligned(
		std::span<element_type, lane_count> destination) const noexcept;

	/**
	 * @brief Copies every active lane into a fixed-size array.
	 * @return Array containing all lanes in low-to-high logical order.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr std::array<element_type, lane_count>
	to_array() const noexcept;

	/**
	 * @brief Returns one compile-time-selected lane.
	 * @tparam index Logical lane index.
	 * @return Copy of the selected lane.
	 */
	template <std::size_t index>
		requires(index < lane_count)
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr element_type lane() const noexcept;

	/**
	 * @brief Returns the wrapped native register for intrinsic interoperation.
	 * @return Complete native register value.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr native_type native() const noexcept;

	/**
	 * @brief Adds corresponding lanes.
	 * @param rhs Right-hand register.
	 * @return Per-lane sum.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE Register operator+(Register rhs) const noexcept;

	/**
	 * @brief Subtracts corresponding lanes.
	 * @param rhs Right-hand register.
	 * @return Per-lane difference.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE Register operator-(Register rhs) const noexcept;

	/**
	 * @brief Multiplies corresponding lanes.
	 * @param rhs Right-hand register.
	 * @return Per-lane product.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE Register operator*(Register rhs) const noexcept;

	/**
	 * @brief Compares corresponding lanes for equality.
	 * @param rhs Right-hand register.
	 * @return Register-shaped lane predicate.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr mask_type compare_equal(
		Register rhs) const noexcept;

	/**
	 * @brief Tests whether every corresponding lane compares equal.
	 * @param rhs Right-hand register.
	 * @return `true` when all lanes compare equal.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr bool operator==(
		Register rhs) const noexcept;

  private:
	native_type m_data;
};
```

The wrapper must not expose an implicit conversion to `native_type`, an
implicit scalar-broadcast constructor, mutable span conversions, or a mutable
reference to the native register. `native()` is an explicit interoperation
boundary and returns by value. A complete intrinsic result can be wrapped with
the explicit native-value constructor.

## Scalar operands

Arithmetic and bitwise operators should initially accept only another
`Register` of the same type. A scalar operation requires an explicit broadcast:

```cpp
const auto adjusted = values * FloatRegister::broadcast(scale) +
	FloatRegister::broadcast(offset);
```

This is intentionally more restrictive than `SimdVector`. It makes broadcast
cost and intent visible, avoids overload ambiguities, and encourages callers to
hoist loop-invariant broadcasts. Named convenience overloads can be considered
later if benchmarks and real call sites demonstrate that they improve clarity
without hiding meaningful work.

## Comparison and mask semantics

A low-level register interface needs a register-shaped comparison result.
Returning only the current byte-granular scalar `Api::mask_t` would force a
register-to-scalar transition even when the next operation is a lane selection.
Returning `Register<T, Bits>` would allow arbitrary numeric registers to be
mistaken for valid predicates.

Introduce `RegisterMask<T, Bits>` in the same focused header. It stores the
backend comparison result with an invariant that each lane is either all-zero
or all-one. Consumers normally name it through `Register<T, Bits>::mask_type`.

```cpp
/**
 * @brief Stores one Boolean predicate for every lane in a complete register.
 * @tparam element_t Scalar geometry associated with each predicate lane.
 * @tparam register_width Width of the associated register in bits.
 */
template <class element_t, std::size_t register_width>
class RegisterMask final
{
  public:
	using register_type = Register<element_t, register_width>;
	using bits_type = typename register_type::api_type::mask_t;

	/**
	 * @brief Tests whether any predicate lane is set.
	 * @return `true` when at least one lane is true.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr bool any() const noexcept;

	/**
	 * @brief Tests whether every predicate lane is set.
	 * @return `true` when every lane is true.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr bool all() const noexcept;

	/**
	 * @brief Tests whether no predicate lane is set.
	 * @return `true` when every lane is false.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr bool none() const noexcept;

	/**
	 * @brief Returns one compact bit per logical predicate lane.
	 * @return Bit `i` set exactly when lane `i` is true.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr bits_type bits() const noexcept;

	/**
	 * @brief Selects lanes from two registers according to this predicate.
	 * @param when_true Values selected for true predicate lanes.
	 * @param when_false Values selected for false predicate lanes.
	 * @return Register containing the selected values.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE register_type select(
		register_type when_true,
		register_type when_false) const noexcept;
};
```

`RegisterMask` should support `&`, `|`, `^`, `~`, and their compound forms so
predicates can remain in registers. It must not provide an implicit conversion
to `bool`; control-flow decisions must spell `.any()`, `.all()`, or `.none()`.

`Register::operator==` and `operator!=` should follow conventional value-type
semantics and return a whole-register Boolean. Lane-wise comparisons use named
methods such as `compare_equal`, `compare_greater`, and `compare_less`.
Relational operators should not mean an implicit all-lanes reduction.

## Operation surface

The first implementation should audit every entry in
`docs/ApiOperationMatrix.md` and classify it as a direct `Register` member, a
static factory, a result-type-changing operation, a collection algorithm, or a
compatibility-only operation.

| Existing `Api` family | Proposed `Register` form |
| --- | --- |
| `setzero` | `Register::zero()` and default construction |
| `set1` | `Register::broadcast(value)` |
| `setr` | `Register::from_lanes(...)` with exactly `lane_count` arguments |
| `set`, partial setters | No preferred counterpart; native-order `set` remains compatibility-only |
| `load`, `load_aligned` | Static `Register::load` and `Register::load_aligned` |
| `store`, `store_aligned` | Const members `store` and `store_aligned` |
| `construct`, `to_array` | Array constructor or factory and `to_array()` |
| `add`, `subtract`, `multiply`, `divide`, `modulus` | Operators and named compound forms where supported |
| Saturating and horizontal arithmetic | Named members returning the correctly typed `Register` |
| Bitwise operations | Operators; `andnot` remains a named member |
| Per-lane shifts | Shift operators for integral registers |
| Whole-register byte or bit shifts | Explicitly named members |
| Scalar comparison masks | `RegisterMask::bits()` |
| Lane-wise comparisons | Named methods returning `RegisterMask` |
| `min`, `max`, `absolute`, `sqrt` | Named members |
| Shuffle, blend, unpack, insert, extract | Named members with compile-time selectors where possible |
| `convert_to_float`, `convert_to_int` | `convert<target_t>()` when lane counts are preserved |
| `transform`, `transform_pack` | Remain collection algorithms; not `Register` members |
| `load_partial`, partial setters | No `Register` counterpart |

Operations should return wrapped values. A method must not expose a raw
intrinsic result merely because the existing backend uses a different native
type internally. If an operation changes element interpretation, its return
type must state that change, for example `Register<std::int32_t, Bits>`.

## Conversion and width-changing operations

Numeric conversion and bit reinterpretation are distinct operations:

- `bit_cast<target_t>()` preserves every register bit and requires a supported
  target lane interpretation at the same register width.
- `convert<target_t>()` performs numeric conversion and is initially available
  only where the existing API has a defined same-lane-count conversion.
- `widen_low<target_t>()` explicitly converts only the source lanes consumed by
  one wider-lane result register.
- A future `widen_all<target_t>()` may return a fixed array of registers that
  represents every active source lane.
- Narrowing or packing operations must name their saturation/truncation policy
  and accept the number of source registers required to populate every result
  lane.

The existing generic names `expand` and `compress` should not automatically be
promoted as preferred `Register` names until their lane consumption, result
type, signedness, and saturation behavior are documented for each supported
specialization. A complete-register abstraction must not silently discard
active high lanes.

## Rearrangement policy

Compile-time selectors should be preferred when an instruction requires an
immediate. Examples include `shuffle<indices...>()`, `blend<mask>()`,
`extract<index>()`, and `with_lane<index>(value)`. Runtime-selector overloads
should exist only where the current implementation supports them without
misrepresenting an immediate-only instruction as a cheap dynamic operation.

Lane order at the public boundary is always logical low-to-high order. Native
intrinsic argument order remains available only through explicit native
interoperation or compatibility `Api` calls.

## Layout and performance contract

Each supported specialization should satisfy the following where the compiler
permits the corresponding type trait:

```cpp
static_assert(sizeof(Register<float, 128>) == sizeof(__m128));
static_assert(alignof(Register<float, 128>) == alignof(__m128));
static_assert(std::is_trivially_copyable_v<Register<float, 128>>);
```

The implementation must:

- Store only `native_type m_data`.
- Add no virtual functions, allocator state, active-lane metadata, or hidden
  heap allocation.
- Preserve `SIMDLIB_FORCE_INLINE`, `VECTORCALL`, `noexcept`, and `constexpr`
  where the delegated `Api` operation supports them.
- Avoid a store/reload round trip for ordinary arithmetic, bitwise,
  comparison, selection, and rearrangement chains.
- Preserve the existing scalar fallback behavior when that behavior is part of
  the documented `Api` contract.
- Use focused code-generation checks or benchmarks to demonstrate that a
  representative `Register` expression produces equivalent instructions to
  the corresponding direct `Api` expression.

## Error and precondition policy

Fixed-extent spans enforce full-register load and store sizes at compile time.
Aligned operations retain the existing runtime precondition that the pointer
meets `byte_count` alignment. Compile-time lane selectors are constrained to
valid indices. Unsupported type, width, and operation combinations are removed
from overload resolution with concepts or `requires` clauses.

`Register` adds no exception-based error handling. It follows the current
SimdLib precondition configuration for invalid runtime inputs such as alignment
or shift counts.

## Migration and compatibility

`Register` should become the recommended interface only after it has direct
tests and documented behavior for the intended register-local `Api` surface.
Until then, `Api` remains the authoritative supported interface.

Once parity is demonstrated:

- Add `<SimdLib/Register.h>` to the umbrella header before headers that consume
  it.
- Change README register examples from `NativeApi<T>` to
  `NativeRegister<T>`.
- Keep `Api` documented for compatibility, specialized low-level access, and
  existing collection helpers.
- Migrate internal SimdLib consumers where doing so improves clarity without
  introducing circular header dependencies.
- Do not add a deprecation attribute to `Api` merely because `Register` is now
  recommended. Any removal or warning policy requires a separate compatibility
  decision and versioning plan.

Representative migration:

```cpp
// Existing interface.
using U32Api = SimdLib::Api<128, std::uint32_t>;
const auto old_result = U32Api::bitwise_or(
	U32Api::add(lhs, rhs),
	U32Api::set1(1));

// Proposed interface.
using U32Register = SimdLib::Register<std::uint32_t, 128>;
const auto new_result =
	(U32Register{lhs} + U32Register{rhs}) |
	U32Register::broadcast(1);
```

## Validation strategy

The implementation requires evidence in each of these areas:

- Compile-time availability checks for every supported element type at 128 and
  256 bits under the existing feature profiles.
- Compile-time rejection of partial lane lists and wrong-extent spans.
- Layout and trivial-copy checks for integer, float, and double register
  families on each supported compiler.
- Runtime construction, load, store, and operation tests that use distinctive
  values in every lane, especially the highest lane.
- Direct parity tests against the public `Api` contract for every migrated
  operation and supported type/width combination.
- Mask tests covering all-false, all-true, alternating, first-lane-only, and
  highest-lane-only predicates.
- Conversion tests that prove numeric conversion and bit reinterpretation do
  not overlap semantically.
- Rearrangement tests that document lane order and selector behavior.
- `constexpr` probes for every operation whose `Api` counterpart supports
  constant evaluation.
- Debug-contract and sanitizer runs that confirm full-register access does not
  read beyond caller storage.
- MSVC, clang-cl, Clang, and GCC validation consistent with the existing
  support matrix.
- Focused generated-code or benchmark comparisons for chained arithmetic,
  comparison plus selection, load/operate/store, and explicit broadcast reuse.

Tests should treat the current `Api` as a parity oracle only while migration is
underway. Independent scalar references remain necessary for behavioral
correctness so both surfaces cannot agree on the same defect unnoticed.

## Acceptance criteria

The proposal is ready for implementation approval when the following decisions
are accepted:

- Template order is `Register<element_t, register_width>`.
- Width selection is expressed through `NativeRegister<element_t>`, not a
  defaulted primary-template argument.
- Default construction produces a zero register.
- Scalar arithmetic requires an explicit broadcast.
- Every load, store, and lane-list constructor covers one complete register.
- Lane-wise comparisons return `RegisterMask`; whole-value equality returns
  `bool`.
- Numeric conversion and bit reinterpretation have separate names.
- Width-changing operations cannot silently discard active lanes.
- Collection transforms and partial-register operations remain outside
  `Register`.
- `Api` remains supported throughout migration and is not immediately marked
  deprecated.

Implementation is complete only when the intended register-local operation
matrix is mapped, tests pass across the supported compiler and feature matrix,
documentation recommends `Register`, and representative generated code shows
no abstraction penalty relative to direct `Api` use.

