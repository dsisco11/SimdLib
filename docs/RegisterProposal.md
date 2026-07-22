# Register Class Proposal

Status: proposed design; no public API or compatibility commitment has been made.

## Summary

Add `SimdLib::Register<element_t, register_width>` as the recommended value-like
interface for operations on one complete SIMD register when the translation
unit supports the required C++23 explicit-object feature. Unlike
`SimdVector<element_t, element_count>`, a `Register` has no logical element
count that can be smaller than its hardware lane count. Every lane always
participates in loads, stores, arithmetic, comparisons, rearrangements, and
reductions.

`Register` will compose the existing `Api<register_width, element_t>` facade
instead of inheriting from it. Ordinary operations delegate to that supported
surface. Operations that require a register-shaped result which `Api` currently
collapses to a scalar use one narrow internal backend adapter. This preserves
the established implementation and feature-routing behavior while presenting
an interface that supports natural expression chaining and keeps native
intrinsic and `Detail` types out of ordinary call sites.

The existing `Api` remains the C++20 interface and a supported compatibility and
backend-facing surface after `Register` reaches operation parity. Span-wide
algorithms and partial-register handling remain outside `Register`.

## Decision status

| Status | Decisions |
| --- | --- |
| Controlling requirement | Template order is `<T, Bits>`; every hardware lane is active; default construction uses the native zero-register operation; comparison behavior matches the selected hardware intrinsic; the abstraction has zero runtime overhead in supported configurations. |
| Proposed public design | Explicit register width with `NativeRegister<T>` for target-selected width; C++23 explicit-object members for register-consuming operations; explicit scalar broadcast; `RegisterMask<T, Bits>` predicates; fixed-extent element and byte transfers; operation names and results defined by the migration ledger. |
| Intentionally excluded | Partial and unsafe loads, automatic lane filling, collection transforms, native-order construction, ambiguous `expand`/`compress`, implementation-specific runtime rearrangements, and multi-register widening results. |
| Validation pending | Complete compiler/type/width behavior, generated-code equivalence, and the non-inlined calling-boundary evidence described below. |

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

### Replacement boundary

Where `SIMDLIB_REGISTER_INTERFACE_AVAILABLE` is nonzero, `Register` supersedes
`Api` as the recommended interface for operations whose inputs and outputs are
one or more complete registers. Supersession means that applicable new
documentation, examples, and register-local call sites use `Register`; it does
not mean that every static member currently located on `Api` becomes a
`Register` member. C++20 consumers continue to use `Api`.

`Api` retains these supported responsibilities:

- Span-wide `transform` and `transform_pack` collection algorithms.
- Partial-register staging used internally to implement collection tails.
- `load_unsafe`, whose dynamic-extent precondition is unsuitable for the
  restrictive `Register` interface.
- Native-order construction and implementation-specific overloads retained for
  compatibility.
- Existing callers that have not yet migrated.

The operation ledger below classifies every current public `Api` operation. An
operation marked compatibility-only is intentionally outside the preferred
`Register` surface and therefore does not block register-local replacement.

## Language and interface availability

The existing SimdLib target remains a C++20 interface. `Register` is an
optional C++23 public surface because its member-call syntax depends on explicit
object parameters. Availability is detected from the standardized feature-test
macro when the compiler advertises it, with a version-and-language-mode fallback
for Microsoft C++. MSVC has supported explicit object parameters since Visual
Studio 2022 version 17.2, but the tested MSVC 19.36, 19.38, and 19.44 toolsets do
not define `__cpp_explicit_this_parameter` even when `/std:c++latest` is active.
Syntax support alone is not the support contract: the initial Microsoft C++
floor is the MSVC 19.44 toolset on which the complete declaration forms and
initial ABI probes have been validated:

```cpp
#if defined(__cpp_explicit_this_parameter) && \
	__cpp_explicit_this_parameter >= 202110L
#define SIMDLIB_REGISTER_INTERFACE_AVAILABLE 1
#elif defined(_MSC_VER) && !defined(__clang__) && _MSC_VER >= 1944 && \
	defined(_MSVC_LANG) && _MSVC_LANG > 202002L
#define SIMDLIB_REGISTER_INTERFACE_AVAILABLE 1
#else
#define SIMDLIB_REGISTER_INTERFACE_AVAILABLE 0
#endif
```

`SIMDLIB_REGISTER_INTERFACE_AVAILABLE` means that the current translation unit
can parse and use the `Register` interface. It does not mean that every element
type and register width is available; `is_register_available_v<T, Bits>` retains
that per-specialization responsibility.

The MSVC fallback deliberately combines the compiler version with
`_MSVC_LANG`; neither value alone proves that the required syntax is enabled.
The `!defined(__clang__)` condition prevents clang-cl from entering the MSVC
fallback merely because it also defines `_MSC_VER`. clang-cl follows the
standard feature-test-macro path. The implementation must compile-probe every
explicit-object declaration form used by `Register` at the supported MSVC
floor. The floor may be lowered below 19.44 only after that toolset passes the
complete correctness, layout, ABI, and generated-code gates. Documented syntax
support is not sufficient by itself.

`_HAS_CXX23` is not used. It is an internal Microsoft runtime-library mode macro,
becomes visible only after a Microsoft header defines it, and is not specific to
explicit object parameters. No Microsoft header should be required merely to
determine whether SimdLib can expose `Register`.

After the initial validation ledger has passed, the umbrella header includes
`Register.h` only when `SIMDLIB_REGISTER_INTERFACE_AVAILABLE` is nonzero.
Directly including `Register.h` without the required feature produces a focused
preprocessing diagnostic. C++20 consumers can therefore continue using every
existing SimdLib surface without enabling C++23, while a translation unit
compiled with a supporting C++23 compiler gains `Register`.

No normalized SimdLib language-version macro is introduced. The standardized
explicit-object feature-test macro remains the primary capability check. The
MSVC fallback is a documented exception for a compiler that implements the
required syntax without defining that macro. The resulting availability macro
is computed by SimdLib and is never caller-overridable. The initial interface
provides no opt-out macro. `Config.h` must document this availability macro as
an exception to its current rule that all configuration macros are
caller-overridable.

No namespace-scope `inline constexpr` availability variable is added. Its value
could differ between C++20 and C++23 translation units and create an avoidable
ODR hazard. The preprocessor macro is the only public availability query.

### Supported compiler matrix

The core target retains its existing C++20 compiler matrix. Register support is
a narrower, separately validated matrix:

| Compiler family | Initial Register floor | Language mode | Availability path |
| --- | --- | --- | --- |
| Microsoft C++ | MSVC 19.44 | `/std:c++latest` | `_MSC_VER` and `_MSVC_LANG` fallback |
| clang-cl | 22 | C++23 | Standard feature-test macro |
| Clang | 22 | C++23 | Standard feature-test macro |
| GCC | 14 | C++23 | Standard feature-test macro |

GCC 13.2 remains in the core C++20 matrix and must compile the umbrella header
with `SIMDLIB_REGISTER_INTERFACE_AVAILABLE == 0`. A compiler is added to the
Register matrix only after all correctness and zero-overhead gates pass for the
supported architecture, ISA profile, type, and width combinations.

### CMake opt-in target

The base `SimdLib::SimdLib` target remains C++20. A separate
`SimdLib::Register` interface target links the base target, requests
`cxx_std_23`, and publishes `SIMDLIB_REQUIRE_REGISTER_INTERFACE=1`. The focused
header reports an error when that requirement is present but
`SIMDLIB_REGISTER_INTERFACE_AVAILABLE` is zero:

```cmake
add_library(SimdLibRegister INTERFACE)
add_library(SimdLib::Register ALIAS SimdLibRegister)
target_link_libraries(SimdLibRegister INTERFACE SimdLib::SimdLib)
target_compile_features(SimdLibRegister INTERFACE cxx_std_23)
target_compile_options(
	SimdLibRegister
	INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>)
target_compile_definitions(
	SimdLibRegister
	INTERFACE SIMDLIB_REQUIRE_REGISTER_INTERFACE=1)
```

```cpp
#if defined(SIMDLIB_REQUIRE_REGISTER_INTERFACE) && \
	!SIMDLIB_REGISTER_INTERFACE_AVAILABLE
#error "SimdLib::Register requires supported C++23 explicit object parameters."
#endif
```

The CMake target requests C++23 generally and explicitly selects
`/std:c++latest` for Microsoft C++, which is the language mode used to validate
the MSVC fallback. Configuration probes must inspect the generated compiler
command and `_MSVC_LANG` so a future CMake or compiler change cannot silently
select a mode that lacks the required explicit-object syntax. The target does
not define or override the computed availability result. A consumer that only
links `SimdLib::SimdLib` does not inherit a C++23 requirement.

Translation units may use different language modes provided no C++20 unit names
or exchanges a `Register` type. All translation units that exchange `Register`
or `RegisterMask` values across a function boundary must use compatible ISA,
`VECTORCALL`, compiler ABI, and SimdLib configuration settings.

## Type shape and specialization availability

The proposed primary template puts the element type first, matching
`SimdVector`, and keeps the register width explicit. This ordering is the
canonical SimdLib order for new value types. The existing
`Api<register_width, element_t>` order is a legacy design mistake and must not
be copied into `Register` or its associated traits:

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

`is_register_available_v<T, Bits>` may delegate to the existing
`is_api_available_v<Bits, T>` implementation, but that delegation is an
internal compatibility detail. All new Register-facing templates, concepts,
aliases, documentation, and examples use the `<T, Bits>` order.

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
- Default construction invokes the appropriate `Api` or implementation
  zero-register operation and produces a fully initialized intrinsic zero
  register.

Zero-initialized default construction gives `Register{}` ordinary value-type
semantics. It does not represent inactive-lane filling: every resulting zero
lane is active. The runtime path must use the native zero-register operation,
preferably through `api_type::setzero()`, so the compiler can emit the target's
ordinary register-zeroing instruction. A constant-evaluation path, when
required by the compiler representation, must produce the same all-zero bits.
There is no public or private uninitialized `Register` construction path.

## Core interface sketch

The following declaration-only sketch is internally complete for construction,
transfer, native interoperation, representative arithmetic, and comparison.
The operation ledger defines the remaining operation names.

```cpp
/**
 * @brief Stores one Boolean predicate for every lane in a complete register.
 * @tparam element_t Scalar geometry associated with each predicate lane.
 * @tparam bits Width of the associated register in bits.
 */
template <class element_t, std::size_t bits>
	requires RegisterAvailable<element_t, bits>
class RegisterMask;

/**
 * @brief Owns one complete SIMD register whose lanes are all active.
 * @tparam element_t Scalar interpretation of each register lane.
 * @tparam bits Width of the native register in bits.
 */
template <class element_t, std::size_t bits>
	requires RegisterAvailable<element_t, bits>
class Register final
{
  public:
	using element_type = element_t;
	using api_type = Api<bits, element_type>;
	using native_type = typename api_type::vector_t;
	using mask_type = RegisterMask<element_type, bits>;

	constexpr static inline std::size_t register_width = bits;
	constexpr static inline std::size_t byte_count = api_type::byte_count;
	constexpr static inline std::size_t lane_count = api_type::element_count;

	/**
	 * @brief Constructs a register with every active lane set to zero through
	 *        the native zero-register operation.
	 */
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
	 * @brief Constructs a register from one complete fixed-size lane array.
	 * @param source Source containing every active lane in logical order.
	 * @return Register containing all source lane values.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static Register from_array(
		const std::array<element_type, lane_count> &source) noexcept;

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
	 * @brief Loads one complete register bit pattern from raw bytes.
	 * @param source Source containing exactly one register of bytes.
	 * @return Register containing the source bit pattern.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE static Register load_bytes(
		std::span<const std::byte, byte_count> source) noexcept;

	/**
	 * @brief Stores every active lane to potentially unaligned storage.
	 * @param value Register to store.
	 * @param destination Destination for exactly one register of elements.
	 */
	SIMDLIB_FORCE_INLINE void VECTORCALL store(
		this Register value,
		std::span<element_type, lane_count> destination) noexcept;

	/**
	 * @brief Stores every active lane to register-aligned storage.
	 * @param value Register to store.
	 * @param destination Aligned destination for one complete register.
	 */
	SIMDLIB_FORCE_INLINE void VECTORCALL store_aligned(
		this Register value,
		std::span<element_type, lane_count> destination) noexcept;

	/**
	 * @brief Stores the complete register bit pattern to raw bytes.
	 * @param value Register to store.
	 * @param destination Destination containing exactly one register of bytes.
	 */
	SIMDLIB_FORCE_INLINE void VECTORCALL store_bytes(
		this Register value,
		std::span<std::byte, byte_count> destination) noexcept;

	/**
	 * @brief Copies every active lane into a fixed-size array.
	 * @param value Register to copy.
	 * @return Array containing all lanes in low-to-high logical order.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr
	std::array<element_type, lane_count> VECTORCALL to_array(
		this Register value) noexcept;

	/**
	 * @brief Returns one compile-time-selected lane.
	 * @tparam index Logical lane index.
	 * @param value Register containing the selected lane.
	 * @return Copy of the selected lane.
	 */
	template <std::size_t index>
		requires(index < lane_count)
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr element_type VECTORCALL lane(
		this Register value) noexcept;

	/**
	 * @brief Returns the wrapped native register for intrinsic interoperation.
	 * @param value Register to unwrap.
	 * @return Complete native register value.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr native_type VECTORCALL native(
		this Register value) noexcept;

	/**
	 * @brief Adds corresponding lanes.
	 * @param lhs Left-hand register.
	 * @param rhs Right-hand register.
	 * @return Per-lane sum.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE Register VECTORCALL operator+(
		this Register lhs,
		Register rhs) noexcept;

	/**
	 * @brief Subtracts corresponding lanes.
	 * @param lhs Left-hand register.
	 * @param rhs Right-hand register.
	 * @return Per-lane difference.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE Register VECTORCALL operator-(
		this Register lhs,
		Register rhs) noexcept;

	/**
	 * @brief Multiplies corresponding lanes.
	 * @param lhs Left-hand register.
	 * @param rhs Right-hand register.
	 * @return Per-lane product.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE Register VECTORCALL operator*(
		this Register lhs,
		Register rhs) noexcept;

	/**
	 * @brief Compares corresponding lanes for equality.
	 * @param lhs Left-hand register.
	 * @param rhs Right-hand register.
	 * @return Register-shaped lane predicate.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr mask_type VECTORCALL compare_equal(
		this Register lhs,
		Register rhs) noexcept;

	/**
	 * @brief Tests whether every corresponding lane compares equal.
	 * @param lhs Left-hand register.
	 * @param rhs Right-hand register.
	 * @return `true` when all lanes compare equal.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL operator==(
		this Register lhs,
		Register rhs) noexcept;

	/**
	 * @brief Tests whether any corresponding lane compares unequal.
	 * @param lhs Left-hand register.
	 * @param rhs Right-hand register.
	 * @return `true` when at least one lane compares unequal.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL operator!=(
		this Register lhs,
		Register rhs) noexcept;

  private:
	native_type m_data;

	friend class RegisterMask<element_type, bits>;
};
```

The wrapper must not expose an implicit conversion to `native_type`, an
implicit scalar-broadcast constructor, mutable span conversions, or a mutable
reference to the native register. `value.native()` is an explicit interoperation
boundary and returns by value. A complete intrinsic result can be wrapped with
the explicit native-value constructor.

Explicit-object members preserve ordinary value-like syntax such as
`value.absolute()`, `value.store(output)`, and `mask.bits()`. The object argument
is nevertheless declared by value, so there is no implicit `this` pointer and a
surviving call can use the same vector calling convention as a by-value free
function.

`Register::load()` and `value.store()` are the canonical potentially
unaligned operations; there are no redundant `load_unaligned()` or
`store_unaligned()` members.
`Register::load_bytes()` and `value.store_bytes(destination)` preserve the
complete register bit pattern without changing the `element_type`
interpretation. All transfer operations use fixed extents. `Register`
deliberately provides no dynamic-extent unsafe load.

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
Returning only the current scalar `Api::mask_t` would force a register-to-scalar
transition even when the next operation is a lane selection.
Returning `Register<T, Bits>` would allow arbitrary numeric registers to be
mistaken for valid predicates.

Introduce `RegisterMask<T, Bits>` in the same focused header. It stores exactly
one native register with an invariant that each lane is either all-zero or
all-one. Consumers normally name it through
`Register<T, Bits>::mask_type`. Only comparisons and mask bitwise operations
can create a mask; arbitrary numeric registers cannot be converted into one.

```cpp
/**
 * @brief Stores one Boolean predicate for every lane in a complete register.
 * @tparam element_t Scalar geometry associated with each predicate lane.
 * @tparam bits Width of the associated register in bits.
 */
template <class element_t, std::size_t bits>
	requires RegisterAvailable<element_t, bits>
class RegisterMask final
{
  public:
	using register_type = Register<element_t, bits>;
	using api_type = typename register_type::api_type;
	using native_type = typename register_type::native_type;
	using bits_type = std::conditional_t<
		(register_type::lane_count <= 32),
		std::uint32_t,
		std::uint64_t>;

	constexpr static inline std::size_t register_width = bits;
	constexpr static inline std::size_t lane_count = register_type::lane_count;

	/** @brief Constructs an all-false predicate register. */
	SIMDLIB_FORCE_INLINE constexpr RegisterMask() noexcept;

	/**
	 * @brief Tests whether any predicate lane is set.
	 * @param value Predicate register to test.
	 * @return `true` when at least one lane is true.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL any(
		this RegisterMask value) noexcept;

	/**
	 * @brief Tests whether every predicate lane is set.
	 * @param value Predicate register to test.
	 * @return `true` when every lane is true.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL all(
		this RegisterMask value) noexcept;

	/**
	 * @brief Tests whether no predicate lane is set.
	 * @param value Predicate register to test.
	 * @return `true` when every lane is false.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL none(
		this RegisterMask value) noexcept;

	/**
	 * @brief Returns one compact bit per logical predicate lane.
	 * @param value Predicate register to reduce.
	 * @return Bit `i` set exactly when lane `i` is true.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr bits_type VECTORCALL bits(
		this RegisterMask value) noexcept;

	/**
	 * @brief Returns the wrapped native predicate register for intrinsic
	 *        interoperation.
	 * @param value Predicate register to unwrap.
	 * @return Complete native predicate register value.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr native_type VECTORCALL native(
		this RegisterMask value) noexcept;

	/**
	 * @brief Selects lanes from two registers according to a predicate.
	 * @param condition Predicate controlling each selected lane.
	 * @param when_true Values selected for true predicate lanes.
	 * @param when_false Values selected for false predicate lanes.
	 * @return Register containing the selected values.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE register_type VECTORCALL select(
		this RegisterMask condition,
		register_type when_true,
		register_type when_false) noexcept;

	/**
	 * @brief Computes the intersection of two predicate registers.
	 * @param lhs Left-hand predicate register.
	 * @param rhs Right-hand predicate register.
	 * @return Predicate that is true where both inputs are true.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr RegisterMask VECTORCALL operator&(
		this RegisterMask lhs,
		RegisterMask rhs) noexcept;

	/**
	 * @brief Computes the union of two predicate registers.
	 * @param lhs Left-hand predicate register.
	 * @param rhs Right-hand predicate register.
	 * @return Predicate that is true where either input is true.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr RegisterMask VECTORCALL operator|(
		this RegisterMask lhs,
		RegisterMask rhs) noexcept;

	/**
	 * @brief Computes the exclusive union of two predicate registers.
	 * @param lhs Left-hand predicate register.
	 * @param rhs Right-hand predicate register.
	 * @return Predicate that is true where exactly one input is true.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr RegisterMask VECTORCALL operator^(
		this RegisterMask lhs,
		RegisterMask rhs) noexcept;

	/**
	 * @brief Inverts every predicate lane.
	 * @param value Predicate register to invert.
	 * @return Predicate containing the inverse of every input lane.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr RegisterMask VECTORCALL operator~(
		this RegisterMask value) noexcept;

	/**
	 * @brief Intersects this predicate with another predicate.
	 * @param lhs Predicate register to update.
	 * @param rhs Right-hand predicate register.
	 * @return Reference to the updated predicate.
	 */
	SIMDLIB_FORCE_INLINE constexpr RegisterMask &operator&=(
		this RegisterMask &lhs,
		RegisterMask rhs) noexcept;

	/**
	 * @brief Unites this predicate with another predicate.
	 * @param lhs Predicate register to update.
	 * @param rhs Right-hand predicate register.
	 * @return Reference to the updated predicate.
	 */
	SIMDLIB_FORCE_INLINE constexpr RegisterMask &operator|=(
		this RegisterMask &lhs,
		RegisterMask rhs) noexcept;

	/**
	 * @brief Exclusively combines this predicate with another predicate.
	 * @param lhs Predicate register to update.
	 * @param rhs Right-hand predicate register.
	 * @return Reference to the updated predicate.
	 */
	SIMDLIB_FORCE_INLINE constexpr RegisterMask &operator^=(
		this RegisterMask &lhs,
		RegisterMask rhs) noexcept;

  private:
	native_type m_data;

	/**
	 * @brief Wraps a comparison result whose lanes already satisfy the mask
	 *        invariant.
	 * @param value Native all-zero or all-one predicate lanes.
	 */
	SIMDLIB_FORCE_INLINE constexpr explicit RegisterMask(native_type value)
		noexcept;

	friend class Register<element_t, bits>;
};
```

The default constructor invokes the same native zero-register operation as
`Register` and therefore creates an all-false mask. `bits_type` is a normalized
public unsigned type selected from `lane_count`; it does not inherit the legacy
backend `Api::mask_t` type. The initial 128-bit and 256-bit specializations have
at most 32 lanes and therefore use `std::uint32_t`. The 64-bit alternative keeps
the alias well-defined if a future supported width has between 33 and 64 lanes.
`mask.bits()` uses the element-granular movemask operation and guarantees that
bits at indices greater than or equal to `lane_count` are zero.
`mask.select(when_true, when_false)` chooses `when_true` for all-one predicate
lanes and `when_false` for all-zero predicate lanes. It can be implemented with
register bitwise operations when no direct blend instruction accepts the
predicate representation.

`mask.native()` is a read-only interoperation boundary and returns the complete
predicate register by value. It does not weaken the mask invariant because the
consumer cannot write through the result. There is no public native-value
constructor, `from_native_unchecked()`, or initial `from_bits()` factory.
Arbitrary native and numeric registers therefore cannot be introduced as masks;
safe scalar-to-mask construction may be considered later as an additive API if
real call sites justify its expansion cost.

`RegisterMask` must not provide an implicit conversion to `bool`; control-flow
decisions must spell `mask.any()`, `mask.all()`, or `mask.none()`.

### Internal comparison adapter

The current curated `Api` comparison functions return scalar masks and no
longer retain the register-shaped predicate needed by `RegisterMask`.
`Register.h` therefore defines a narrow
`Detail::RegisterBackend<element_t, bits>` adapter. It is the only new code in
`Register.h` permitted to name `Detail::SimdMappings` or its inherited backend
comparison functions.

The adapter returns complete native predicate registers for equality,
greater-than, and any other comparison directly supported by the selected
backend. Derived predicates such as greater-than-or-equal may combine those
native predicates with register bitwise operations. The explicit-object
comparison member wraps the result through the private
`RegisterMask(native_type)` constructor. Neither the adapter nor a native-mask
constructor is part of the consumer API.

Portable and constant-evaluated adapter paths construct the same all-zero or
all-one lane patterns as the runtime intrinsic. This adapter avoids expanding
the legacy `Api` surface solely to support the new value type and prevents
ordinary `Register` implementation code from depending broadly on `Detail`.

`Register::operator==` and `operator!=` should follow conventional value-type
semantics and return a whole-register Boolean. Lane-wise comparisons use named
explicit-object members such as `lhs.compare_equal(rhs)`,
`lhs.compare_greater(rhs)`, and `lhs.compare_less(rhs)`.
Relational operators should not mean an implicit all-lanes reduction.

Every comparison follows the semantics of the underlying hardware intrinsic
selected for that operation. The wrapper must not replace intrinsic behavior
with a different C++ interpretation. This includes floating-point ordered or
unordered behavior, NaN results, signed-zero behavior, signed versus unsigned
integer ordering, and the all-zero or all-one bit pattern produced for each
predicate lane. Where a portable, constant-evaluated, or emulated path is
needed, it must reproduce the selected runtime intrinsic's observable result.
The operation documentation must identify the intrinsic comparison predicate
whose semantics it exposes.

## Operation surface

The following ledger classifies every current public `Api` operation. Operation
availability continues to follow `docs/ApiOperationMatrix.md` and the selected
backend constraints.

Every non-static operation uses a C++23 explicit object parameter. Non-mutating
operations take that parameter by value, preserving ordinary member-call syntax
without an implicit `this` pointer. Mutating compound assignments take the
explicit object parameter by reference because mutation itself requires an
existing object. All register-shaped parameters and results use `VECTORCALL`
where enabled.

Constructors, compiler-generated special members, and static factories have no
explicit object parameter. They remain forced inline and are covered alongside
the explicit-object surface by generated-code and ABI tests.

### Construction and transfer ledger

| Current `Api` operation | Preferred `Register<T, Bits>` form | Decision |
| --- | --- | --- |
| `load` | `Register::load(fixed_span)` | Canonical potentially unaligned full load |
| `load_aligned` | `Register::load_aligned(fixed_span)` | Retained with alignment precondition |
| `load_unaligned` | `Register::load(fixed_span)` | Redundant spelling omitted |
| `load_partial` | None | Partial data belongs to higher-level types |
| `load_unsafe` | None | Dynamic-extent unsafe load remains on `Api` |
| `store` to element span | `value.store(fixed_span)` | Canonical potentially unaligned full store |
| `store_aligned` | `value.store_aligned(fixed_span)` | Retained with alignment precondition |
| `store_unaligned` | `value.store(fixed_span)` | Redundant spelling omitted |
| `store` to byte span | `value.store_bytes(fixed_byte_span)` | Renamed to make bit-pattern transfer explicit |
| Fixed-byte `load` | `Register::load_bytes(fixed_byte_span)` | Symmetric bit-pattern transfer |
| `construct(array)` | `Register::from_array(array)` | Static factory; no ambiguous storage constructor |
| `to_array` | `value.to_array()` | Retained as a value conversion |
| `setzero` | Default construction and `Register::zero()` | Uses intrinsic-backed zero construction |
| `set1` | `Register::broadcast(value)` | Explicit scalar broadcast |
| `setr` | `Register::from_lanes(...)` | Requires exactly `lane_count` logical-order values |
| `set` | None | Native intrinsic argument order remains compatibility-only |
| `set_partial`, `setr_partial` | None | No partial or automatically filled lanes |
| `FinishIntegerMagnitudeFromPairSums` | None | Implementation helper; must not be copied to `Register` |

### Arithmetic and reduction ledger

| Current `Api` operation | Preferred `Register<T, Bits>` form | Result |
| --- | --- | --- |
| `add` | `lhs + rhs`, `lhs += rhs` | Same register type |
| `subtract` | `lhs - rhs`, `lhs -= rhs` | Same register type |
| `multiply` | `lhs * rhs`, `lhs *= rhs` | Same register type |
| `divide` | `lhs / rhs`, `lhs /= rhs` | Same register type where supported |
| `modulus` | `lhs % rhs`, `lhs %= rhs` | Same integral register type |
| `negate` | `-value` | Same register type |
| `min` | `lhs.min(rhs)` | Same register type |
| `max` | `lhs.max(rhs)` | Same register type |
| `multiply_add` | `lhs.multiply_add(rhs, addend)` | Same register type |
| `widen` | `value.widen_low<target_t, target_bits>()` | Explicit target `Register`; consumed lanes documented |
| `absolute` | `value.absolute()` | Same register type and intrinsic edge behavior |
| `sqrt` | `value.sqrt()` | Same register type where supported |
| `magnitude` | `value.magnitude()` | Same register type and existing 128-bit grouping |
| `normalize` | `value.normalize()` | Same floating register type |
| `avg` | `lhs.average(rhs)` | Same register type |
| `add_horizontal` | `lhs.horizontal_add(rhs)` | Same register type |
| `subtract_horizontal` | `lhs.horizontal_subtract(rhs)` | Same register type |
| `multiply_add_adjacent` | `lhs.multiply_add_adjacent(rhs)` | Explicit operation-result Register alias |
| `multiply_add_unsigned_signed_bytes` | `lhs.multiply_add_unsigned_signed_bytes(rhs)` | Explicit signed promoted-result Register alias |
| `sum_absolute_byte_differences` | `lhs.sum_absolute_byte_differences(rhs)` | Explicit unsigned-result Register alias |
| `multi_sum_absolute_byte_differences` | `lhs.multi_sum_absolute_byte_differences<imm8>(rhs)` | Explicit unsigned-result Register alias |
| `min_position` | `value.min_position()` | `std::size_t` |
| `max_position` | `value.max_position()` | `std::size_t` |
| `add_saturated` | `lhs.add_saturated(rhs)` | Same register type |
| `subtract_saturated` | `lhs.subtract_saturated(rhs)` | Same register type |
| `hadd_saturated` | `lhs.horizontal_add_saturated(rhs)` | Same register type |
| `hsubtract_saturated` | `lhs.horizontal_subtract_saturated(rhs)` | Same register type |
| `add_subtract` | `lhs.add_subtract(rhs)` | Same floating register type |
| `dot_product` | `lhs.dot_product<imm8>(rhs)` | Same register type with intrinsic-selected output lanes |

Operations whose intrinsic changes the lane type use constrained namespace-level
alias templates. Keeping these aliases outside `Register` avoids conditional
member declarations or helper-base storage that could complicate the exact
one-native-member representation:

| Alias | Exact result mapping |
| --- | --- |
| `multiply_add_adjacent_result_t<T, Bits>` | `Register<int16_t, Bits>` for `int8_t`, `Register<uint16_t, Bits>` for `uint8_t`, then the corresponding signedness at twice the lane width through 64 bits; 64-bit lanes remain 64-bit |
| `byte_multiply_add_result_t<T, Bits>` | `Register<int16_t, Bits>` for supported signed/unsigned byte inputs |
| `sad_result_t<T, Bits>` | `Register<uint64_t, Bits>` |
| `multi_sad_result_t<T, Bits>` | `Register<uint16_t, Bits>` |

The aliases are declared only when the corresponding backend operation is
available. Each public operation names its exact alias as the return type rather
than using an undifferentiated `auto` or exposing a raw intrinsic type. Alias
availability and mapping are tested for every supported source type and width;
unsupported combinations remain absent even when a result element type could be
formed mechanically.

### Bitwise and comparison ledger

| Current `Api` operation | Preferred `Register<T, Bits>` form | Result |
| --- | --- | --- |
| `bitwise_and` | `lhs & rhs`, `lhs &= rhs` | Same register type |
| `bitwise_or` | `lhs \| rhs`, `lhs \|= rhs` | Same register type |
| `bitwise_xor` | `lhs ^ rhs`, `lhs ^= rhs` | Same register type |
| `bitwise_not` | `~value` | Same register type |
| `bitwise_andnot` | `lhs.andnot(rhs)` | Same register type with existing operand polarity |
| `movemask` | `value.movemask()` | Scalar mask with the selected intrinsic's native granularity |
| `movemask_slim` | `value.lane_sign_bits()` | Scalar mask with one bit per lane |
| `cmp_eq`, `cmp_eq_mask` | `lhs.compare_equal(rhs).bits()` | Duplicate scalar spellings collapse into one compact lane-mask path |
| `cmp_gt` | `lhs.compare_greater(rhs)` | `RegisterMask<T, Bits>` |
| `cmp_ge` | `lhs.compare_greater_equal(rhs)` | `RegisterMask<T, Bits>` |
| `cmp_lt` | `lhs.compare_less(rhs)` | `RegisterMask<T, Bits>` |
| `cmp_le` | `lhs.compare_less_equal(rhs)` | `RegisterMask<T, Bits>` |

The legacy scalar comparison-mask layout is not uniform across integral and
floating backends. `mask.bits()` deliberately normalizes it to one bit
per logical lane. Callers requiring the exact legacy scalar representation
continue to use the corresponding `Api::cmp_*` function.

`Register::operator==` is equivalent to `lhs.compare_equal(rhs).all()`.
`operator!=` is equivalent to `lhs.compare_equal(rhs).all() == false`; this
preserves whole-value inequality and does not mean that every lane must differ.
For floating registers these operators retain the selected intrinsic's ordered
equality behavior: a NaN lane is not equal, while positive and negative zero are
equal. They are not bitwise-equality operators; exact bit-pattern comparison
requires an explicit integer reinterpretation followed by integer comparison.

### Rearrangement ledger

| Current `Api` operation | Preferred `Register<T, Bits>` form | Decision |
| --- | --- | --- |
| `expand` | None | Ambiguous legacy widening alias remains compatibility-only |
| `compress` | None | Ambiguous legacy narrowing alias remains compatibility-only |
| `extract<index>` | `value.lane<index>()` | Compile-time logical lane extraction |
| Runtime `extract` | None initially | Implementation-specific selector remains compatibility-only |
| `lower_half` | `value.lower_half()` | Returns `Register<T, 128>` from a 256-bit source |
| `insert` | `value.with_lane<index>(lane)` | Compile-time logical lane replacement |
| `unpack_lo` | `lhs.unpack_low(rhs)` | Wrapped backend result |
| `unpack_hi` | `lhs.unpack_high(rhs)` | Wrapped backend result |
| `shuffle<indices...>` | `value.shuffle<indices...>()` | Compile-time logical selector |
| Generic `shuffle(args...)` | None initially | Implementation-specific signature remains compatibility-only |
| `shuffle_lo` | `value.shuffle_low<imm8>()` | Compile-time immediate form |
| `shuffle_hi` | `value.shuffle_high<imm8>()` | Compile-time immediate form |
| `blend` | `lhs.blend<imm8>(rhs)` | Immediate blend; predicate blend uses `mask.select(lhs, rhs)` |

### Shift and conversion ledger

| Current `Api` operation | Preferred `Register<T, Bits>` form | Result |
| --- | --- | --- |
| `shift_left` | `value << count`, `value <<= count` | Per-lane integral shift |
| `shift_right` | `value.logical_shift_right(count)` | Per-lane logical shift for signed or unsigned lanes |
| `shift_right_arithmetic` | `value >> count`, `value >>= count` | Per-lane arithmetic shift for signed lanes |
| `byte_shift_left` | `value.byte_shift_left(count)` | Complete 128-bit register byte shift |
| `byte_shift_right` | `value.byte_shift_right(count)` | Complete 128-bit register byte shift |
| Runtime `bit_shift_left` | `value.bit_shift_left(count)` | Complete 128-bit bit-string shift |
| Compile-time `bit_shift_left` | `value.bit_shift_left<count>()` | Complete 128-bit bit-string shift |
| Runtime `bit_shift_right` | `value.bit_shift_right(count)` | Complete 128-bit bit-string shift |
| Compile-time `bit_shift_right` | `value.bit_shift_right<count>()` | Complete 128-bit bit-string shift |
| `convert_to_float` | `value.convert<float>()` | `Register<float, Bits>` from supported 32-bit integer lanes |
| `convert_to_int` | `value.convert<std::int32_t>()` | `Register<std::int32_t, Bits>` from float lanes |
| `convert` | `value.convert<target_t>()` | Explicit target type; no complementary-type inference |

`operator>>` is available only when it has one unambiguous hardware meaning.
Unsigned lanes use the logical shift. Signed lanes use the arithmetic shift.
`logical_shift_right()` remains available for signed lanes that intentionally
request zero fill.

Shift-count behavior is part of the public contract and matches the existing
backend operation rather than C++ scalar-shift rules:

| Shift family | Count contract |
| --- | --- |
| Per-lane left or logical right | Runtime count must be nonnegative; counts at least the lane width produce zero lanes |
| Per-lane arithmetic right | Runtime count must be nonnegative; counts at least the lane width clamp to `lane_width - 1` and therefore sign-fill |
| 128-bit byte shifts | Counts at most zero return the input; counts at least 16 return zero |
| Runtime 128-bit whole-register bit shifts | Counts at most zero return the input; counts at least 128 return zero |
| Compile-time 128-bit whole-register bit shifts | Negative counts are rejected; counts at least 128 produce zero |

The implementation must not introduce release-only undefined behavior for a
documented count. Negative per-lane shift counts are invalid runtime inputs and
follow the SimdLib precondition policy; tests cover the boundary values `0`,
`width - 1`, `width`, and `width + 1`.

### Collection and internal ledger

| Current `Api` operation | `Register` decision |
| --- | --- |
| `transform_pack` | Remains a collection algorithm on `Api` or its future algorithm owner |
| Unary in-place `transform` | Remains a collection algorithm |
| Unary separate-output `transform` | Remains a collection algorithm |
| Binary `transform` | Remains a collection algorithm |
| `TransformForMaxPosition` | Internal helper; no public `Register` counterpart |
| `compare_each_element` | Internal fallback helper used by the comparison adapter |

All preferred register-local operations return `Register`, `RegisterMask`, or
an explicitly documented scalar. No preferred operation exposes a raw intrinsic
result. Availability is expressed with `requires` clauses that mirror the
corresponding supported backend operation.

## Conversion and width-changing operations

Numeric conversion and bit reinterpretation are distinct operations:

- `bit_cast<target_t>()` preserves every register bit and requires a supported
  target lane interpretation at the same register width. The target lane count
  may differ because this operation reinterprets the complete bit pattern.
- `convert<target_t>()` performs numeric conversion and is initially available
  only where the existing API has a defined conversion into one complete
  target register.
- `widen_low<target_t, target_bits>()` is the preferred spelling for the
  existing `Api::widen` behavior. It explicitly converts only the lowest
  source lanes needed to populate one complete target register.
- No `widen_all` member is included in the initial preferred surface. Producing
  multiple registers is a separate algorithm contract rather than a value
  operation on one result register.
- No generic narrowing or packing member is included until its
  saturation/truncation policy and required source-register count have a
  dedicated design.

The existing generic `expand` and `compress` names remain supported only on
`Api`. They are not promoted to `Register`. Their lane consumption, result
type, signedness, and saturation behavior are too specialization-specific for
the preferred interface. `widen_low` makes discarded high source lanes
explicit; no other preferred operation may silently discard active lanes.

## Rearrangement policy

Compile-time selectors should be preferred when an instruction requires an
immediate. Examples include `value.shuffle<indices...>()`,
`lhs.blend<mask>(rhs)`, `value.lane<index>()`, and
`value.with_lane<index>(lane_value)`. Runtime-selector overloads should exist
only where the current implementation supports them without misrepresenting an
immediate-only instruction as a cheap dynamic operation.

Every `imm8` template control is constrained to the inclusive range `0..255`;
operation-specific unused bits retain the underlying intrinsic behavior. Lane
selectors require `index < lane_count`. Logical `shuffle<indices...>` overloads
require exactly the documented result selector count and reject every index
outside the documented input-lane range. These requirements participate in
overload constraints instead of relying on a late intrinsic diagnostic.

Lane order at the public boundary is always logical low-to-high order. Native
intrinsic argument order remains available only through explicit native
interoperation or compatibility `Api` calls.

## Layout and zero-overhead contract

Each supported specialization should satisfy the following where the compiler
permits the corresponding type trait:

```cpp
static_assert(sizeof(Register<float, 128>) == sizeof(__m128));
static_assert(alignof(Register<float, 128>) == alignof(__m128));
static_assert(std::is_standard_layout_v<Register<float, 128>>);
static_assert(std::is_trivially_copyable_v<Register<float, 128>>);
static_assert(std::is_trivially_copy_constructible_v<Register<float, 128>>);
static_assert(std::is_trivially_move_constructible_v<Register<float, 128>>);
static_assert(std::is_trivially_copy_assignable_v<Register<float, 128>>);
static_assert(std::is_trivially_move_assignable_v<Register<float, 128>>);
static_assert(std::is_trivially_destructible_v<Register<float, 128>>);
static_assert(sizeof(RegisterMask<float, 128>) == sizeof(__m128));
static_assert(alignof(RegisterMask<float, 128>) == alignof(__m128));
static_assert(std::is_standard_layout_v<RegisterMask<float, 128>>);
static_assert(std::is_trivially_copyable_v<RegisterMask<float, 128>>);
static_assert(std::is_trivially_destructible_v<RegisterMask<float, 128>>);
```

`Register` is required to have zero runtime performance overhead relative to
the equivalent supported `Api` or direct-intrinsic expression. This guarantee
applies to storage, alignment, argument passing, return values, construction,
loads, stores, arithmetic, comparisons, masks, selection, rearrangement, and
destruction. It is not limited to expressions that happen to be inlined.

Zero overhead is a relative guarantee. Neither C++ nor raw SIMD intrinsics can
guarantee that a value never leaves a physical SIMD register. Finite register
capacity, register pressure, opaque calls, disabled optimization, diagnostic
instrumentation, or an explicit address escape can cause the compiler to spill
a native intrinsic value. Such a spill is not caused by `Register` when the
equivalent raw-intrinsic implementation spills in the same context. It is a
`Register` defect when the wrapper introduces a move, spill, reload, temporary,
or indirection that the equivalent raw implementation does not require.

### Register-residency strategy

The preferred implementation uses these mechanisms together:

- Every `Register` and `RegisterMask` contains exactly one native vector and
  remains trivially copyable and destructible.
- Small operations are defined in the focused header and marked
  `SIMDLIB_FORCE_INLINE` so an optimized chain becomes one vector expression in
  the compiler's intermediate representation.
- Every non-mutating operation that consumes an existing wrapper is an
  explicit-object member taking that object by value. It uses `VECTORCALL`
  where enabled and returns register-shaped results by value. This includes
  named operations as well as overloaded operators. If a call survives
  optimization, its operands and result can use the platform's vector or
  homogeneous-vector-aggregate calling convention without an implicit `this`
  pointer.
- Constructors, compiler-generated special members, and static factories remain
  ordinary members because they either must be members or consume no existing
  wrapper. Compound assignment operators use explicit object parameters by
  reference because they mutate an existing wrapper. They are forced inline and
  subject to a dedicated materialization gate.
- Deliberately out-of-line register operations, if any are later justified,
  retain their explicit-object parameter and `VECTORCALL` where supported so
  their ABI does not silently regress to an implicit `this` boundary.
- No operation returns a mutable native reference, mutable span, proxy tied to
  object storage, or other value that requires the wrapper to acquire a stable
  memory address.

`VECTORCALL` controls a surviving function-call boundary; it does not pin a
value to a physical register and has no effect after a function is inlined. In
the current configuration it is enabled for MSVC and Clang on x86 targets and
is empty for GCC. MSVC and Clang are expected to classify a one-vector wrapper
as a one-element homogeneous vector aggregate, but that classification is a
compiler ABI property and must be verified. GCC uses its target ABI and must be
validated independently against the same raw-vector baseline.

The calling convention on Register members does not propagate into an ordinary
consumer-defined function. A non-inlined consumer function that passes or
returns `Register` or `RegisterMask` must declare `VECTORCALL` to participate in
the vector-calling-convention guarantee where that convention is supported:

```cpp
using FloatRegister = SimdLib::Register<float, 128>;

/**
 * @brief Applies a consumer-defined complete-register transformation.
 * @param value Input register.
 * @return Transformed register.
 */
FloatRegister VECTORCALL transform_register(FloatRegister value) noexcept;
```

Consumer functions using the platform's default convention receive no stronger
call-boundary guarantee than equivalent raw native-vector functions under that
same convention. The validation suite compares wrapper and raw signatures under
both the supported vector convention and the platform default. Any wrapper-only
default-convention overhead is documented explicitly; it cannot be attributed
to Register member chaining or hidden by a `VECTORCALL` result.

Ordinary non-static member functions carry an implicit `this` pointer. If such
a function is not inlined, the left operand may need an addressable object even
when a by-value operation could receive it in a vector register. Focused
clang-cl 22.1.8 Windows x64 probes demonstrated this distinction for a
non-inlined 128-bit floating-point addition: the ordinary const member form
used addressable left-operand and result storage, while both the hidden-friend
and explicit-object member forms received their values in vector registers and
returned the result in a vector register. The explicit-object body was one
`vaddps`, and its caller emitted a tail call while retaining `lhs.add(rhs)`
syntax. A separate explicit-object `operator+` probe produced the same ABI and
single-instruction body, while a forced-inline reference-taking `operator+=`
also reduced to one `vaddps`. The inlined forms were equivalent. This evidence
motivates the explicit-object default, but the complete supported compiler,
type, and width matrix remains an acceptance test rather than an assumed ABI
guarantee.

The implementation must:

- Store only `native_type m_data` in each `Register` and `RegisterMask`.
- Add no virtual functions, allocator state, active-lane metadata, or hidden
  heap allocation.
- Preserve `SIMDLIB_FORCE_INLINE`, `VECTORCALL`, `noexcept`, and `constexpr`
  where the delegated `Api` operation supports them.
- Use the native zero-register operation for default construction without
  introducing a memory clear, temporary array, or store/reload sequence.
- Avoid a store/reload round trip for ordinary arithmetic, bitwise,
  comparison, selection, and rearrangement chains.
- Pass and return `Register` and `RegisterMask` values without extra stack
  traffic, hidden copies, branches, register moves, or indirection compared
  with the corresponding native register type under the supported calling
  convention.
- Preserve the existing scalar fallback behavior when that behavior is part of
  the documented `Api` contract.
- Use mandatory generated-code checks to demonstrate that every public
  operation family, overload shape, supported element type, and register width
  produces code equivalent to the corresponding direct `Api` or intrinsic
  expression. Benchmarks are supplemental evidence only and cannot replace a
  missing generated-code comparison.

A compiler can theoretically classify a class containing an intrinsic vector
differently from the intrinsic type itself at a non-inlined function boundary.
That possibility is not an accepted exception. The supported compiler and
calling-convention matrix must be tested with both inlined expressions and
separately compiled, non-inlined functions. If any wrapper specialization is
passed, returned, spilled, copied, or otherwise handled less efficiently than
the native register, the difference must be identified and discussed before
the design can be accepted. The implementation or public calling convention
must then be adjusted, or that compiler/type/width combination must be
explicitly excluded from the zero-overhead support claim.

The zero-overhead support claim is configuration-specific. Each accepted result
records the compiler and version, target architecture, ISA switches, SimdLib
configuration, optimization mode, and calling convention used for both wrapper
and raw baselines. Optimized Release builds are the mandatory machine-code gate.
Debug and sanitizer builds run correctness and wrapper-versus-raw differential
checks under identical flags; they are not claimed to have optimized Release
assembly. Any wrapper-only overhead found in those builds is still recorded and
discussed explicitly rather than hidden by the narrower Release claim.

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

- Add `<SimdLib/Register.h>` to the umbrella header conditionally when
  `SIMDLIB_REGISTER_INTERFACE_AVAILABLE` is nonzero and before headers that
  consume it.
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

- C++20 compile probes proving that
  `SIMDLIB_REGISTER_INTERFACE_AVAILABLE == 0`, the umbrella header remains
  usable, and existing public surfaces retain their current language baseline.
- Supporting C++23 compile probes proving that
  `SIMDLIB_REGISTER_INTERFACE_AVAILABLE == 1`, `Register.h` is exposed, and the
  explicit-object declarations compile. A negative direct-header probe verifies
  the focused diagnostic when the feature is unavailable.
- CMake consumer probes proving that `SimdLib::SimdLib` retains its C++20
  requirement, `SimdLib::Register` requests C++23 and the requirement macro,
  Microsoft C++ receives `/std:c++latest`, and an unsupported compiler receives
  the focused diagnostic. The Microsoft probe verifies the generated compiler
  command, `_MSVC_LANG > 202002L`, and the required explicit-object syntax.
- Dedicated availability probes for both detection paths: the standardized
  `__cpp_explicit_this_parameter >= 202110L` path on clang-cl, Clang, and GCC,
  and the `_MSC_VER >= 1944` plus `_MSVC_LANG > 202002L` fallback on Microsoft
  C++. MSVC probes cover named methods, overloaded arithmetic and comparison
  operators, and mutating compound-assignment operators. The same MSVC toolset
  is also compiled in C++20 mode to prove that the fallback remains disabled.
- A configuration probe proving that clang-cl cannot enter the Microsoft C++
  fallback through its compatibility definition of `_MSC_VER`.
- Compile-time availability checks for every supported element type at 128 and
  256 bits under the existing feature profiles.
- Compile-time rejection of partial lane lists and wrong-extent spans.
- Compile-time result-alias checks for every supported type-changing operation,
  plus rejection of aliases and operations for unsupported backend
  combinations.
- Compile-time rejection of out-of-range immediates and selectors, plus runtime
  and constant-evaluation tests at every documented shift-count boundary.
- Compile-only validation that the declaration sketch, forward declarations,
  constraints, private-access relationships, and focused-header include boundary are
  self-contained.
- Layout and trivial-copy checks for integer, float, and double register
  families on each supported compiler.
- Runtime construction, load, store, and operation tests that use distinctive
  values in every lane, especially the highest lane.
- Element and raw-byte transfer tests proving exact full-register bit
  preservation and the absence of partial or dynamic-extent unsafe overloads.
- Direct parity tests against the public `Api` contract for every migrated
  operation and supported type/width combination.
- Mask tests covering all-false, all-true, alternating, first-lane-only, and
  highest-lane-only predicates, compact lane bits, bitwise composition,
  selection polarity, and cleared unused scalar bits. Static assertions verify
  that `bits_type` is the documented unsigned type for every supported width
  and lane geometry.
- Mask-native interoperation tests proving that `native()` returns the complete
  predicate bits by value without a store/reload round trip, while arbitrary
  native registers, scalar bit fields, and numeric Registers cannot publicly
  construct a `RegisterMask`.
- Backend-adapter tests proving that runtime, portable, emulated, and
  constant-evaluated comparisons produce the same intrinsic-defined predicate
  lanes.
- Conversion tests that prove numeric conversion and bit reinterpretation do
  not overlap semantically.
- Rearrangement tests that document lane order and selector behavior.
- `constexpr` probes for every operation whose `Api` counterpart supports
  constant evaluation.
- Debug-contract and sanitizer runs that confirm full-register access does not
  read beyond caller storage.
- Separate validation of the core C++20 matrix and the narrower Register matrix:
  MSVC 19.44, clang-cl 22, Clang 22, and GCC 14 or newer. GCC 13.2 is a required
  unavailable-interface probe for the core matrix.
- Mandatory generated-code comparisons for chained arithmetic, comparison plus
  selection, load/operate/store, and explicit broadcast reuse. Benchmarks may
  supplement these comparisons but never replace them.
- Automatically generated code probes for the actual forced-inline
  explicit-object members covering every public operation family, overload
  shape, supported element type, register width, and ISA profile. The probes
  include overloaded operators, named arithmetic, comparisons, reductions,
  conversions, rearrangements, stores, and native observation. Each category
  compares optimized wrapper chains with equivalent direct-intrinsic chains
  compiled with identical options and rejects wrapper-only stack traffic,
  moves, spills, reloads, temporaries, branches, or indirection.
- Forced-inline probes for constructors, compiler-generated special members,
  static factories, and reference-taking compound assignments. The supported
  performance gate fails if a wrapper is unnecessarily materialized when the
  equivalent direct operation remains in registers.
- Test-only, separately compiled, non-inlined ABI mirrors for the explicit-object
  signature families: unary, binary, ternary, scalar-result, mask-result,
  native-result, store, and mutating-reference operations. These compare `Register`,
  `RegisterMask`, `Api::vector_t`, and direct-intrinsic calling conventions for
  every supported compiler, element type, and register width.
- Paired consumer-defined function probes using `VECTORCALL` and the platform
  default convention. The vector-convention gate rejects any wrapper-only ABI
  overhead. Default-convention differences are recorded explicitly and remain
  outside the supported call-boundary guarantee unless that compiler and
  signature also pass the raw-vector comparison.
- Controlled register-pressure and opaque-call probes that distinguish spills
  required equally by raw values from additional spills introduced by the
  wrapper.
- Configuration-provenance records for every code-generation and ABI artifact,
  including compiler version, architecture, ISA switches, SimdLib configuration,
  optimization mode, and calling convention. Debug and sanitizer results are
  reported separately from optimized Release evidence.

Tests should treat the current `Api` as a parity oracle only while migration is
underway. Independent scalar references remain necessary for behavioral
correctness so both surfaces cannot agree on the same defect unnoticed.

## Acceptance criteria

The proposal is ready for implementation approval when the following decisions
are accepted:

- Template order is `Register<element_t, register_width>`; the legacy
  `Api<register_width, element_t>` order is not propagated to new types.
- The core SimdLib target remains C++20. The Register interface is exposed only
  when `SIMDLIB_REGISTER_INTERFACE_AVAILABLE` detects either
  `__cpp_explicit_this_parameter >= 202110L` or the documented Microsoft C++
  fallback of `_MSC_VER >= 1944` and `_MSVC_LANG > 202002L`; no normalized
  general language-version macro is introduced, and `_HAS_CXX23` is not used.
- Availability is computed by SimdLib, cannot be overridden, and has no initial
  opt-out. `SimdLib::Register` is the C++23 opt-in target; the base target does
  not impose that requirement. The opt-in target explicitly selects
  `/std:c++latest` for Microsoft C++ and compile-probes the resulting language
  mode.
- Register support has a separate validated compiler matrix from the C++20 core
  matrix. A compiler's language-feature support alone does not admit it to the
  zero-overhead support claim.
- Width selection is expressed through `NativeRegister<element_t>`, not a
  defaulted primary-template argument.
- Default construction explicitly uses the appropriate intrinsic-backed
  zero-register operation and never leaves a register uninitialized.
- Scalar arithmetic requires an explicit broadcast.
- Every load, store, and lane-list constructor covers one complete register.
- `load` and `store` are the canonical unaligned element transfers;
  `load_bytes` and `store_bytes` are the exact-width raw-bit transfers, and no
  unsafe or partial transfer is exposed.
- Lane-wise comparisons return `RegisterMask`; whole-value equality returns
  `bool`.
- `RegisterMask` contains one native predicate register, has no public
  arbitrary-native constructor, and exposes compact lane bits, Boolean
  reductions, bitwise composition, lane selection, and a by-value native
  observer. Its normalized unsigned `bits_type` is selected from `lane_count`
  rather than inherited from `Api::mask_t`.
- Comparison behavior exactly matches the selected underlying hardware
  intrinsic, including floating-point edge cases and predicate-lane bit
  patterns.
- Register-shaped comparisons use the single internal
  `Detail::RegisterBackend<T, Bits>` seam instead of expanding the legacy
  `Api` surface or leaking `Detail` names to consumers.
- Numeric conversion and bit reinterpretation have separate names.
- Width-changing operations cannot silently discard active lanes.
- Type-changing operations return the exact constrained namespace-level result
  alias documented in the operation ledger.
- Shift boundaries, immediate domains, and selector ranges are explicit public
  contracts and are checked in runtime, constant-evaluation, and compile-failure
  tests as applicable.
- Collection transforms and partial-register operations remain outside
  `Register`.
- The operation ledger is the controlling migration boundary: every current
  public `Api` operation has a preferred `Register` spelling or an explicit
  compatibility-only classification.
- Zero overhead means that `Register` introduces no additional instructions,
  moves, spills, reloads, stack traffic, temporaries, branches, or indirection
  relative to equivalent raw-intrinsic code compiled in the same context; it
  does not claim that raw SIMD values can never spill.
- Every non-static operation uses an explicit object parameter and
  `VECTORCALL` where supported. Non-mutating operations take the object by value
  to preserve member-call syntax without an implicit `this` pointer; compound
  assignments take it by reference to express mutation.
- Call-boundary behavior is validated separately for MSVC, clang-cl, Clang,
  and GCC because `VECTORCALL` is a calling-convention tool, not a physical
  register-residency guarantee.
- Non-inlined consumer-defined functions must declare `VECTORCALL` where it is
  supported to participate in the vector-calling-convention guarantee. Default
  convention signatures are compared with raw vectors separately and are not
  included unless they independently pass the zero-overhead gate.
- Generated-code comparisons are mandatory for every public operation family,
  overload shape, supported type, width, and ISA profile under identical
  optimized settings; benchmarks are supplemental only, and each artifact
  records its complete configuration provenance.
- All translation units exchanging `Register` or `RegisterMask` values use
  compatible ISA, calling-convention, compiler-ABI, and SimdLib settings.
- `Api` remains supported throughout migration and is not immediately marked
  deprecated.

Implementation is complete only when the intended register-local operation
matrix is mapped, tests pass across the supported compiler and feature matrix,
documentation recommends `Register`, and generated-code plus call-boundary
evidence shows no abstraction penalty relative to direct `Api` or intrinsic
use. Any observed exception must be identified and discussed explicitly before
the affected configuration can be described as supported.
