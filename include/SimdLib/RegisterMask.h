#pragma once

#include <SimdLib/Config.h>

#if !SIMDLIB_REGISTER_INTERFACE_AVAILABLE && !SIMDLIB_REQUIRE_REGISTER_INTERFACE
#error "SIMDLIB_REGISTER_MASK_HEADER_REQUIRES_CXX23: <SimdLib/RegisterMask.h> requires C++23 explicit object parameter support"
#endif

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

	/** @brief Tests whether any predicate lane is true. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr bool VECTORCALL any(this RegisterMask value) noexcept
	{
		return value.bits() != 0;
	}

	/** @brief Tests whether every predicate lane is true. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr bool VECTORCALL all(this RegisterMask value) noexcept
	{
		return value.bits() == all_bits;
	}

	/** @brief Tests whether every predicate lane is false. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr bool VECTORCALL none(this RegisterMask value) noexcept
	{
		return value.bits() == 0;
	}

	/** @brief Returns one compact bit per logical predicate lane. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr bits_type VECTORCALL bits(this RegisterMask value) noexcept
	{
		return static_cast<bits_type>(api_type::movemask_slim(value.native));
	}

	/** @brief Selects true or false register lanes according to this predicate. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr register_type VECTORCALL select(this RegisterMask condition,
																													   register_type when_true,
																													   register_type when_false) noexcept;

	/** @brief Computes the intersection of two predicate registers. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr RegisterMask VECTORCALL operator&(this RegisterMask lhs,
																														 RegisterMask rhs) noexcept
	{
		return RegisterMask{bitwise_and(lhs.native, rhs.native)};
	}

	/** @brief Computes the union of two predicate registers. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr RegisterMask VECTORCALL operator|(this RegisterMask lhs,
																														 RegisterMask rhs) noexcept
	{
		return RegisterMask{bitwise_or(lhs.native, rhs.native)};
	}

	/** @brief Computes the exclusive union of two predicate registers. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr RegisterMask VECTORCALL operator^(this RegisterMask lhs,
																														 RegisterMask rhs) noexcept
	{
		return RegisterMask{bitwise_xor(lhs.native, rhs.native)};
	}

	/** @brief Inverts every predicate lane. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr RegisterMask VECTORCALL operator~(this RegisterMask value) noexcept
	{
		return RegisterMask{bitwise_not(value.native)};
	}

	/*
	 * Disabled compound assignment operators: their convenience does not justify the mutable-reference API surface,
	 * and MSVC 19.44 emits a redundant 32-byte stack-alignment frame for 256-bit wrapper mutation through references.
	 * Prefer `lhs = lhs & rhs`, `lhs = lhs | rhs`, or `lhs = lhs ^ rhs`.
	 *
	/// @brief Intersects this predicate with another predicate.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr RegisterMask &operator&=(this RegisterMask
	&lhs, RegisterMask rhs) noexcept
	{
		return lhs = lhs & rhs;
	}

	/// @brief Unites this predicate with another predicate.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr RegisterMask &operator|=(this RegisterMask &lhs,
	RegisterMask rhs) noexcept
	{
		return lhs = lhs | rhs;
	}

	/// @brief Exclusively combines this predicate with another predicate.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr RegisterMask &operator^=(this
	RegisterMask &lhs, RegisterMask rhs) noexcept
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

	/** @brief Computes the bitwise intersection of two native predicate registers. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static native_type VECTORCALL bitwise_and(const native_type lhs,
																																 const native_type rhs) noexcept
	{
		return api_type::bitwise_and(lhs, rhs);
	}

	/** @brief Computes the bitwise union of two native predicate registers. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static native_type VECTORCALL bitwise_or(const native_type lhs,
																																const native_type rhs) noexcept
	{
		return api_type::bitwise_or(lhs, rhs);
	}

	/** @brief Computes the bitwise exclusive union of two native predicate registers. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static native_type VECTORCALL bitwise_xor(const native_type lhs,
																																 const native_type rhs) noexcept
	{
		return api_type::bitwise_xor(lhs, rhs);
	}

	/** @brief Inverts every bit in a native predicate register. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static native_type VECTORCALL
	bitwise_not(const native_type value) noexcept
	{
		return api_type::bitwise_not(value);
	}

	/** @brief Selects native true or false lanes according to a canonical predicate register. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr native_type VECTORCALL
	select_native(this RegisterMask condition, const native_type when_true, const native_type when_false) noexcept
	{
		return api_type::select(condition.native, when_true, when_false);
	}
};

} // namespace SimdLib

#include <SimdLib/Register.h>
