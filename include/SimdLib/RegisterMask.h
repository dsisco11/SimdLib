#pragma once

#include <SimdLib/Config.h>

#if !SIMDLIB_REGISTER_INTERFACE_AVAILABLE && !SIMDLIB_REQUIRE_REGISTER_INTERFACE
#error "SIMDLIB_REGISTER_MASK_HEADER_REQUIRES_CXX23: <SimdLib/RegisterMask.h> requires C++23 explicit object parameter support"
#endif

#include <SimdLib/IRegisterMask.h>
#include <SimdLib/RegisterFwd.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace SimdLib
{

/**
 * @brief Wraps one native Boolean predicate register for a complete register.
 * @tparam element_t Scalar geometry associated with each predicate lane.
 *
 * @tparam register_bits Width of the associated register in bits.
 * @invariant Every logical predicate lane is all-zero or all-one for Boolean mask
 * operations.
 */
template <class element_t, std::size_t register_bits>
	requires RegisterAvailable<element_t, register_bits>
class RegisterMask final
{
  public:
	using element_type = element_t;
	using api_type = Api<register_bits, element_type>;
	using native_type = typename api_type::vector_t;
	using register_type = Register<element_type, register_bits>;
	using bits_type = std::conditional_t<(api_type::element_count <= 32), std::uint32_t, std::uint64_t>;

	constexpr static inline std::size_t register_width = register_bits;
	constexpr static inline std::size_t byte_count = api_type::byte_count;
	constexpr static inline std::size_t lane_count = api_type::element_count;

	/**
	 * @brief Owns the complete native predicate value represented by this aggregate.
	 * @pre Every logical lane is either all-zero or all-one when
	 * initialized directly.
	 */
	native_type native = api_type::setzero();

	/**
	 * @brief Tests whether any predicate lane is true.
	 * @param value Canonical predicate register to reduce.
	 * @return `true` when at least
	 * one logical predicate lane is all-one.
	 */
	[[nodiscard]] constexpr bool SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) any(this RegisterMask value) noexcept
	{
		if (std::is_constant_evaluated())
			return value.bits() != 0;

		using byte_api_type = Api<register_bits, std::uint8_t>;
		const auto bits = api_type::template bit_cast<std::uint8_t>(value.native);
		// Canonical predicates are all-zero or all-one per lane, so any set bit proves that a lane is true.
		return byte_api_type::testz(bits, bits) == 0;
	}

	/**
	 * @brief Tests whether every predicate lane is true.
	 * @param value Canonical predicate register to reduce.
	 * @return `true` when every
	 * logical predicate lane is all-one.
	 */
	[[nodiscard]] constexpr bool SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) all(this RegisterMask value) noexcept
	{
		return value.bits() == all_bits;
	}

	/**
	 * @brief Tests whether every predicate lane is false.
	 * @param value Canonical predicate register to reduce.
	 * @return `true` when every
	 * logical predicate lane is all-zero.
	 */
	[[nodiscard]] constexpr bool SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) none(this RegisterMask value) noexcept
	{
		if (std::is_constant_evaluated())
			return value.bits() == 0;

		using byte_api_type = Api<register_bits, std::uint8_t>;
		const auto bits = api_type::template bit_cast<std::uint8_t>(value.native);
		// Testing the complete canonical predicate avoids materializing a compact scalar lane mask.
		return byte_api_type::testz(bits, bits) != 0;
	}

	/**
	 * @brief Returns one compact bit per logical predicate lane.
	 * @param value Canonical predicate register to reduce.
	 * @return Scalar whose
	 * bit `i` reports logical predicate lane `i`; all unused high bits are zero.
	 */
	[[nodiscard]] constexpr bits_type SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) bits(this RegisterMask value) noexcept
	{
		return static_cast<bits_type>(api_type::movemask_slim(value.native));
	}

	/**
	 * @brief Selects corresponding true or false Register lanes according to this predicate.
	 * @param condition Canonical predicate lanes; all-one
	 * selects `when_true` and all-zero selects `when_false`.
	 * @param when_true Register supplying lanes selected by true predicates.
	 * @param when_false
	 * Register supplying lanes selected by false predicates.
	 * @return Register containing the intrinsic-backed per-lane selection in logical lane order.

	 */
	[[nodiscard]] constexpr register_type SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		select(this RegisterMask condition, register_type when_true, register_type when_false) noexcept;

	/**
	 * @brief Computes the intersection of two predicate registers.
	 * @param lhs Left canonical predicate register.
	 * @param rhs Right
	 * canonical predicate register.
	 * @return Canonical predicate register whose lane is true only where both input lanes are true.
	 */
	[[nodiscard]] constexpr RegisterMask SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator&(this RegisterMask lhs, RegisterMask rhs) noexcept
	{
		return RegisterMask{bitwise_and(lhs.native, rhs.native)};
	}

	/**
	 * @brief Computes the union of two predicate registers.
	 * @param lhs Left canonical predicate register.
	 * @param rhs Right canonical
	 * predicate register.
	 * @return Canonical predicate register whose lane is true where either input lane is true.
	 */
	[[nodiscard]] constexpr RegisterMask SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator|(this RegisterMask lhs, RegisterMask rhs) noexcept
	{
		return RegisterMask{bitwise_or(lhs.native, rhs.native)};
	}

	/**
	 * @brief Computes the exclusive union of two predicate registers.
	 * @param lhs Left canonical predicate register.
	 * @param rhs Right
	 * canonical predicate register.
	 * @return Canonical predicate register whose lane is true where exactly one input lane is true.
	 */
	[[nodiscard]] constexpr RegisterMask SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator^(this RegisterMask lhs, RegisterMask rhs) noexcept
	{
		return RegisterMask{bitwise_xor(lhs.native, rhs.native)};
	}

	/**
	 * @brief Inverts every predicate lane.
	 * @param value Canonical predicate register.
	 * @return Canonical predicate register with true and
	 * false lanes exchanged.
	 */
	[[nodiscard]] constexpr RegisterMask SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator~(this RegisterMask value) noexcept
	{
		return RegisterMask{bitwise_not(value.native)};
	}

	/*
	 * Disabled compound assignment operators: their convenience does not justify the mutable-reference API surface,
	 * and MSVC 19.44 emits a redundant 32-byte stack-alignment frame for 256-bit wrapper mutation through references.
	 * Prefer `lhs = lhs & rhs`, `lhs = lhs | rhs`, or `lhs = lhs ^ rhs`.
	 *
	/// @brief Intersects this predicate with another predicate.
	constexpr auto SIMD_FLAGS(In, ForceInline, Flatten) operator&=(
		this RegisterMask &lhs,
		RegisterMask rhs) noexcept -> RegisterMask &
	{
		return lhs = lhs & rhs;
	}

	/// @brief Unites this predicate with another predicate.
	constexpr auto SIMD_FLAGS(In, ForceInline, Flatten) operator|=(
		this RegisterMask &lhs,
		RegisterMask rhs) noexcept -> RegisterMask &
	{
		return lhs = lhs | rhs;
	}

	/// @brief Exclusively combines this predicate with another predicate.
	constexpr auto SIMD_FLAGS(In, ForceInline, Flatten) operator^=(
		this RegisterMask &lhs,
		RegisterMask rhs) noexcept -> RegisterMask &
	{
		return lhs = lhs ^ rhs;
	}
	 */
  private:
	constexpr static inline bits_type all_bits = []() constexpr noexcept
	{
		if constexpr (lane_count == std::numeric_limits<bits_type>::digits)
			return std::numeric_limits<bits_type>::max();
		else
			return (bits_type{1} << lane_count) - 1;
	}();

	/**
	 * @brief Computes the bitwise intersection of two native predicate registers.
	 * @param lhs Left canonical native predicate.
	 * @param rhs Right
	 * canonical native predicate.
	 * @return Canonical native predicate containing `lhs & rhs`.
	 */
	[[nodiscard]] constexpr static native_type SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		bitwise_and(const native_type lhs, const native_type rhs) noexcept
	{
		return api_type::bitwise_and(lhs, rhs);
	}

	/**
	 * @brief Computes the bitwise union of two native predicate registers.
	 * @param lhs Left canonical native predicate.
	 * @param rhs Right
	 * canonical native predicate.
	 * @return Canonical native predicate containing `lhs | rhs`.
	 */
	[[nodiscard]] constexpr static native_type SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		bitwise_or(const native_type lhs, const native_type rhs) noexcept
	{
		return api_type::bitwise_or(lhs, rhs);
	}

	/**
	 * @brief Computes the bitwise exclusive union of two native predicate registers.
	 * @param lhs Left canonical native predicate.
	 * @param rhs
	 * Right canonical native predicate.
	 * @return Canonical native predicate containing `lhs ^ rhs`.
	 */
	[[nodiscard]] constexpr static native_type SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		bitwise_xor(const native_type lhs, const native_type rhs) noexcept
	{
		return api_type::bitwise_xor(lhs, rhs);
	}

	/**
	 * @brief Inverts every bit in a native predicate register.
	 * @param value Canonical native predicate.
	 * @return Canonical native
	 * predicate containing the complemented lanes.
	 */
	[[nodiscard]] constexpr static native_type SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_not(const native_type value) noexcept
	{
		return api_type::bitwise_not(value);
	}

	/**
	 * @brief Selects native true or false lanes according to a canonical predicate register.
	 * @param condition Canonical predicate selecting the
	 * source of every logical lane.
	 * @param when_true Native register selected by all-one predicate lanes.
	 * @param when_false Native register
	 * selected by all-zero predicate lanes.
	 * @return Intrinsic-backed native register containing the selected lane values.
	 */
	[[nodiscard]] constexpr native_type SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		select_native(this RegisterMask condition, const native_type when_true, const native_type when_false) noexcept
	{
		return api_type::select(condition.native, when_true, when_false);
	}
};

} // namespace SimdLib

#include <SimdLib/Register.h>
