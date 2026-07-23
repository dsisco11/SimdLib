#pragma once

#include <SimdLib/Config.h>

#if !SIMDLIB_REGISTER_INTERFACE_AVAILABLE && !SIMDLIB_REQUIRE_REGISTER_INTERFACE
#error "SIMDLIB_REGISTER_HEADER_REQUIRES_CXX23: <SimdLib/Register.h> requires C++23 explicit object parameter support"
#endif

#include <SimdLib/RegisterFwd.h>
#include <SimdLib/RegisterMask.h>

#include <array>
#include <concepts>
#include <cstddef>
#include <span>
#include <utility>

namespace SimdLib
{

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

	/** @brief Owns the complete native register value represented by this aggregate. */
	native_type native = api_type::setzero();

	/**
	 * @brief Returns a register with every active lane set to zero.
	 * @return Fully initialized zero register.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static Register zero() noexcept
	{
		return Register{api_type::setzero()};
	}

	/**
	 * @brief Broadcasts one scalar value to every active lane.
	 * @param value Scalar value to broadcast.
	 * @return Register containing `value` in every lane.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static Register broadcast(
		element_type value) noexcept
	{
		return Register{api_type::set1(value)};
	}

	/**
	 * @brief Constructs a register from exactly one complete logical lane list.
	 * @tparam lane_types Scalar argument types convertible to `element_type`.
	 * @param lanes Values in low-to-high logical lane order.
	 * @return Register containing all supplied lane values.
	 */
	template <std::convertible_to<element_type>... lane_types>
		requires(sizeof...(lane_types) == lane_count)
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static Register from_lanes(
		lane_types &&...lanes) noexcept
	{
		return Register{api_type::setr(static_cast<element_type>(std::forward<lane_types>(lanes))...)};
	}

	/**
	 * @brief Constructs a register from one complete fixed-size lane array.
	 * @param source Source containing every active lane in logical order.
	 * @return Register containing all source lane values.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static Register from_array(
		const std::array<element_type, lane_count> &source) noexcept
	{
		return Register{api_type::construct(source)};
	}

	/**
	 * @brief Loads a complete register from potentially unaligned storage.
	 * @param source Source containing exactly one register of elements.
	 * @return Register loaded from `source`.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY static Register load(
		std::span<const element_type, lane_count> source) noexcept
	{
		return Register{api_type::load(source)};
	}

	/**
	 * @brief Loads a complete register from register-aligned storage.
	 * @param source Aligned source containing exactly one register of elements.
	 * @return Register loaded from `source`.
	 * @pre `source.data()` is aligned to `byte_count` bytes.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY static Register load_aligned(
		std::span<const element_type, lane_count> source) noexcept
	{
		return Register{api_type::load_aligned(source)};
	}

	/**
	 * @brief Loads one complete register bit pattern from raw bytes.
	 * @param source Source containing exactly one register of bytes.
	 * @return Register containing the source bit pattern.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY static Register load_bytes(
		std::span<const std::byte, byte_count> source) noexcept
	{
		return Register{api_type::load(source)};
	}

	/**
	 * @brief Stores every active lane to potentially unaligned storage.
	 * @param value Register to store.
	 * @param destination Destination for exactly one register of elements.
	 */
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE void VECTORCALL store(
		this Register value,
		std::span<element_type, lane_count> destination) noexcept
	{
		api_type::store(value.native, destination);
	}

	/**
	 * @brief Stores every active lane to register-aligned storage.
	 * @param value Register to store.
	 * @param destination Aligned destination for one complete register.
	 * @pre `destination.data()` is aligned to `byte_count` bytes.
	 */
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE void VECTORCALL store_aligned(
		this Register value,
		std::span<element_type, lane_count> destination) noexcept
	{
		api_type::store_aligned(value.native, destination);
	}

	/**
	 * @brief Stores the complete register bit pattern to raw bytes.
	 * @param value Register to store.
	 * @param destination Destination containing exactly one register of bytes.
	 */
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE void VECTORCALL store_bytes(
		this Register value,
		std::span<std::byte, byte_count> destination) noexcept
	{
		api_type::store(value.native, destination);
	}

	/**
	 * @brief Copies every active lane into a fixed-size array.
	 * @param value Register to copy.
	 * @return Array containing all lanes in low-to-high logical order.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr std::array<element_type, lane_count> VECTORCALL to_array(
		this Register value) noexcept
	{
		return api_type::to_array(value.native);
	}

	/**
	 * @brief Returns one compile-time-selected lane.
	 * @tparam index Logical lane index.
	 * @param value Register containing the selected lane.
	 * @return Copy of the selected lane.
	 */
	template <std::size_t index>
		requires(index < lane_count)
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr element_type VECTORCALL lane(
		this Register value) noexcept
	{
		if consteval
		{
			return lane_constexpr<index>(value);
		}
		else
		{
			return api_type::template extract<static_cast<int>(index)>(value.native);
		}
	}

	/**
	 * @brief Returns a copy with one compile-time-selected lane replaced.
	 * @tparam index Logical lane index.
	 * @param value Register containing the lanes to copy.
	 * @param replacement Replacement value for the selected lane.
	 * @return Register with lane `index` replaced.
	 */
	template <std::size_t index>
		requires(index < lane_count)
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL with_lane(
		this Register value,
		element_type replacement) noexcept
	{
		value.native = api_type::template insert<index>(value.native, replacement);
		return value;
	}

#pragma region Arithmetic Operations

	/** @brief Adds corresponding lanes. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL operator+(
		this Register lhs,
		Register rhs) noexcept
		requires requires(native_type left, native_type right) { api_type::add(left, right); }
	{
		return Register{api_type::add(lhs.native, rhs.native)};
	}

	/** @brief Subtracts corresponding lanes. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL operator-(
		this Register lhs,
		Register rhs) noexcept
		requires requires(native_type left, native_type right) { api_type::subtract(left, right); }
	{
		return Register{api_type::subtract(lhs.native, rhs.native)};
	}

	/** @brief Multiplies corresponding lanes. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL operator*(
		this Register lhs,
		Register rhs) noexcept
		requires requires(native_type left, native_type right) { api_type::multiply(left, right); }
	{
		return Register{api_type::multiply(lhs.native, rhs.native)};
	}

	/**
	 * @brief Divides corresponding lanes.
	 * @pre Every divisor lane is nonzero and signed minimum is not divided by negative one.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL operator/(
		this Register lhs,
		Register rhs) noexcept
		requires requires(native_type left, native_type right) { api_type::divide(left, right); }
	{
		return Register{api_type::divide(lhs.native, rhs.native)};
	}

	/**
	 * @brief Computes corresponding-lane remainders.
	 * @pre Every divisor lane is nonzero and signed minimum is not divided by negative one.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE Register VECTORCALL operator%(
		this Register lhs,
		Register rhs) noexcept
		requires requires(native_type left, native_type right) { api_type::modulus(left, right); }
	{
		return Register{api_type::modulus(lhs.native, rhs.native)};
	}

	/** @brief Negates every lane with the selected backend's edge behavior. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL operator-(
		this Register value) noexcept
		requires requires(native_type operand) { api_type::negate(operand); }
	{
		return Register{api_type::negate(value.native)};
	}

	/*
	 * Disabled compound assignment operators: their convenience does not justify the mutable-reference API surface,
	 * and MSVC 19.44 emits a redundant 32-byte stack-alignment frame for 256-bit wrapper mutation through references.
	 * Prefer `lhs = lhs + rhs`, `lhs = lhs - rhs`, `lhs = lhs * rhs`, `lhs = lhs / rhs`, or `lhs = lhs % rhs`.
	 *
	/// @brief Adds another register into this register.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE Register &VECTORCALL operator+=(
		this Register &lhs,
		Register rhs) noexcept
		requires requires(native_type left, native_type right) { api_type::add(left, right); }
	{
		lhs.native = api_type::add(lhs.native, rhs.native);
		return lhs;
	}

	/// @brief Subtracts another register from this register.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE Register &VECTORCALL operator-=(
		this Register &lhs,
		Register rhs) noexcept
		requires requires(native_type left, native_type right) { api_type::subtract(left, right); }
	{
		lhs.native = api_type::subtract(lhs.native, rhs.native);
		return lhs;
	}

	/// @brief Multiplies this register by another register.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE Register &VECTORCALL operator*=(
		this Register &lhs,
		Register rhs) noexcept
		requires requires(native_type left, native_type right) { api_type::multiply(left, right); }
	{
		lhs.native = api_type::multiply(lhs.native, rhs.native);
		return lhs;
	}

	///
	/// @brief Divides this register by another register.
	/// @pre Every divisor lane is nonzero and signed minimum is not divided by negative one.
	///
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE Register &VECTORCALL operator/=(
		this Register &lhs,
		Register rhs) noexcept
		requires requires(native_type left, native_type right) { api_type::divide(left, right); }
	{
		lhs.native = api_type::divide(lhs.native, rhs.native);
		return lhs;
	}

	///
	/// @brief Replaces this register with corresponding-lane remainders.
	/// @pre Every divisor lane is nonzero and signed minimum is not divided by negative one.
	///
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE Register &VECTORCALL operator%=(
		this Register &lhs,
		Register rhs) noexcept
		requires requires(native_type left, native_type right) { api_type::modulus(left, right); }
	{
		lhs.native = api_type::modulus(lhs.native, rhs.native);
		return lhs;
	}
	 */
#pragma endregion

#pragma region Bitwise Operations

	/** @brief Computes the bitwise intersection of two registers. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL operator&(
		this Register lhs,
		Register rhs) noexcept
	{
		return Register{api_type::bitwise_and(lhs.native, rhs.native)};
	}

	/** @brief Computes the bitwise union of two registers. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL operator|(
		this Register lhs,
		Register rhs) noexcept
	{
		return Register{api_type::bitwise_or(lhs.native, rhs.native)};
	}

	/** @brief Computes the bitwise exclusive union of two registers. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL operator^(
		this Register lhs,
		Register rhs) noexcept
	{
		return Register{api_type::bitwise_xor(lhs.native, rhs.native)};
	}

	/** @brief Complements every bit in a register. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL operator~(
		this Register value) noexcept
	{
		return Register{api_type::bitwise_not(value.native)};
	}

	/** @brief Computes `(~lhs) & rhs` with the existing backend operand polarity. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL andnot(
		this Register lhs,
		Register rhs) noexcept
	{
		return Register{api_type::bitwise_andnot(lhs.native, rhs.native)};
	}

	/*
	 * Disabled compound assignment operators: their convenience does not justify the mutable-reference API surface,
	 * and MSVC 19.44 emits a redundant 32-byte stack-alignment frame for 256-bit wrapper mutation through references.
	 * Prefer `lhs = lhs & rhs`, `lhs = lhs | rhs`, or `lhs = lhs ^ rhs`.
	 *
	/// @brief Intersects this register with another register.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register &VECTORCALL operator&=(
		this Register &lhs,
		Register rhs) noexcept
	{
		lhs.native = api_type::bitwise_and(lhs.native, rhs.native);
		return lhs;
	}

	/// @brief Unites this register with another register.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register &VECTORCALL operator|=(
		this Register &lhs,
		Register rhs) noexcept
	{
		lhs.native = api_type::bitwise_or(lhs.native, rhs.native);
		return lhs;
	}

	/// @brief Exclusively combines this register with another register.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register &VECTORCALL operator^=(
		this Register &lhs,
		Register rhs) noexcept
	{
		lhs.native = api_type::bitwise_xor(lhs.native, rhs.native);
		return lhs;
	}
	 */
	/** @brief Returns the selected intrinsic's native-granularity sign-bit mask. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr typename api_type::mask_t
		VECTORCALL movemask(this Register value) noexcept
	{
		return api_type::movemask(value.native);
	}

	/** @brief Returns one scalar sign bit for every logical lane. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr typename api_type::mask_t
		VECTORCALL lane_sign_bits(this Register value) noexcept
	{
		return api_type::movemask_slim(value.native);
	}

#pragma endregion

#pragma region Shifting Operations

	/**
	 * @brief Left-shifts every integral lane.
	 * @pre `count >= 0`; counts at least the lane width produce zero lanes.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL operator<<(
		this Register value,
		int count) noexcept
		requires std::is_integral_v<element_type>
	{
		return Register{api_type::shift_left(value.native, count)};
	}

	/**
	 * @brief Right-shifts every integral lane with zero fill.
	 * @pre `count >= 0`; counts at least the lane width produce zero lanes.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL
		logical_shift_right(this Register value, int count) noexcept
		requires std::is_integral_v<element_type>
	{
		return Register{api_type::shift_right(value.native, count)};
	}

	/**
	 * @brief Right-shifts unsigned lanes logically and signed lanes arithmetically.
	 * @pre `count >= 0`; oversized signed counts clamp and unsigned counts produce zero lanes.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL operator>>(
		this Register value,
		int count) noexcept
		requires std::is_integral_v<element_type>
	{
		if constexpr (std::is_signed_v<element_type>)
			return Register{api_type::shift_right_arithmetic(value.native, count)};
		else
			return Register{api_type::shift_right(value.native, count)};
	}

	/*
	 * Disabled compound assignment operators: their convenience does not justify the mutable-reference API surface,
	 * and MSVC 19.44 emits a redundant 32-byte stack-alignment frame for 256-bit wrapper mutation through references.
	 * Prefer `value = value << count` or `value = value >> count`.
	 *
	/// @brief Left-shifts every integral lane in this register.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register &VECTORCALL operator<<=(
		this Register &value,
		int count) noexcept
		requires std::is_integral_v<element_type>
	{
		value.native = api_type::shift_left(value.native, count);
		return value;
	}

	/// @brief Right-shifts every integral lane in this register using its signedness.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register &VECTORCALL operator>>=(
		this Register &value,
		int count) noexcept
		requires std::is_integral_v<element_type>
	{
		if constexpr (std::is_signed_v<element_type>)
			value.native = api_type::shift_right_arithmetic(value.native, count);
		else
			value.native = api_type::shift_right(value.native, count);
		return value;
	}
	 */
	/** @brief Byte-shifts a complete 128-bit integral register toward higher byte indices. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register VECTORCALL byte_shift_left(
		this Register value,
		int count) noexcept
		requires(std::is_integral_v<element_type> && register_width == 128)
	{
		return Register{api_type::byte_shift_left(value.native, count)};
	}

	/** @brief Byte-shifts a complete 128-bit integral register toward lower byte indices. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register VECTORCALL byte_shift_right(
		this Register value,
		int count) noexcept
		requires(std::is_integral_v<element_type> && register_width == 128)
	{
		return Register{api_type::byte_shift_right(value.native, count)};
	}

	/** @brief Shifts a complete 128-bit integral register left as one bit string. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register VECTORCALL bit_shift_left(
		this Register value,
		int count) noexcept
		requires(std::is_integral_v<element_type> && register_width == 128)
	{
		return Register{api_type::bit_shift_left(value.native, count)};
	}

	/** @brief Shifts a complete 128-bit integral register right as one bit string. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register VECTORCALL bit_shift_right(
		this Register value,
		int count) noexcept
		requires(std::is_integral_v<element_type> && register_width == 128)
	{
		return Register{api_type::bit_shift_right(value.native, count)};
	}

	/** @brief Compile-time shifts a complete 128-bit integral register left as one bit string. */
	template <int count>
		requires(std::is_integral_v<element_type> && register_width == 128 && count >= 0)
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register VECTORCALL bit_shift_left(
		this Register value) noexcept
	{
		return Register{api_type::template bit_shift_left<count>(value.native)};
	}

	/** @brief Compile-time shifts a complete 128-bit integral register right as one bit string. */
	template <int count>
		requires(std::is_integral_v<element_type> && register_width == 128 && count >= 0)
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register VECTORCALL bit_shift_right(
		this Register value) noexcept
	{
		return Register{api_type::template bit_shift_right<count>(value.native)};
	}

#pragma endregion

#pragma region Comparison Operations

	/** @brief Compares corresponding lanes for ordered equality. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr mask_type VECTORCALL compare_equal(
		this Register lhs,
		Register rhs) noexcept
	{
		return mask_type{api_type::compare_equal(lhs.native, rhs.native)};
	}

	/** @brief Compares corresponding lanes for greater-than ordering. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr mask_type VECTORCALL compare_greater(
		this Register lhs,
		Register rhs) noexcept
	{
		return mask_type{api_type::compare_greater(lhs.native, rhs.native)};
	}

	/** @brief Compares corresponding lanes for greater-than-or-equal ordering. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr mask_type VECTORCALL compare_greater_equal(
		this Register lhs,
		Register rhs) noexcept
	{
		return mask_type{api_type::compare_greater_equal(lhs.native, rhs.native)};
	}

	/** @brief Compares corresponding lanes for less-than ordering. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr mask_type VECTORCALL compare_less(
		this Register lhs,
		Register rhs) noexcept
	{
		return mask_type{api_type::compare_less(lhs.native, rhs.native)};
	}

	/** @brief Compares corresponding lanes for less-than-or-equal ordering. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr mask_type VECTORCALL compare_less_equal(
		this Register lhs,
		Register rhs) noexcept
	{
		return mask_type{api_type::compare_less_equal(lhs.native, rhs.native)};
	}

	/** @brief Tests whether every corresponding lane compares equal. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr bool VECTORCALL operator==(
		this Register lhs,
		Register rhs) noexcept
	{
		return lhs.compare_equal(rhs).all();
	}

	/** @brief Tests whether at least one corresponding lane compares unequal. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr bool VECTORCALL operator!=(
		this Register lhs,
		Register rhs) noexcept
	{
		return !lhs.compare_equal(rhs).all();
	}

#pragma endregion

  private:
	/**
	 * @brief Implements compile-time lane observation through the portable array representation.
	 * @tparam index Logical lane index to observe.
	 * @param value Register containing the selected lane.
	 * @return Copy of lane `index`.
	 */
	template <std::size_t index>
	[[nodiscard]] constexpr static element_type lane_constexpr(Register value) noexcept
	{
		return value.to_array()[index];
	}

};

/** @brief Selects true or false register lanes according to this predicate. */
template <class element_t, std::size_t register_bits>
	requires RegisterAvailable<element_t, register_bits>
[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register<element_t, register_bits> VECTORCALL
	RegisterMask<element_t, register_bits>::select(
		this RegisterMask condition,
		register_type when_true,
		register_type when_false) noexcept
{
	return register_type{condition.select_native(when_true.native, when_false.native)};
}

/**
 * @brief Selects the widest complete register available for an element type.
 * @tparam element_t Scalar interpretation of each register lane.
 */
template <class element_t>
	requires RegisterAvailable<element_t, 128>
using NativeRegister = Register<element_t, is_register_available_v<element_t, 256> ? 256 : 128>;

} // namespace SimdLib
