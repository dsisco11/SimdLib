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
 * @brief Stores one canonical Boolean predicate for every lane in a complete register.
 * @tparam element_t Scalar geometry associated with each predicate lane.
 * @tparam register_bits Width of the associated register in bits.
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

	/** @brief Constructs an all-false predicate register. */
	SIMDLIB_FORCE_INLINE SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS constexpr RegisterMask() noexcept
		: m_data(api_type::setzero())
	{
	}

	/** @brief Copies a predicate register value. */
	constexpr RegisterMask(const RegisterMask &) noexcept = default;

	/** @brief Moves a predicate register value. */
	constexpr RegisterMask(RegisterMask &&) noexcept = default;

	/** @brief Copies a predicate register value. */
	constexpr RegisterMask &operator=(const RegisterMask &) noexcept = default;

	/** @brief Moves a predicate register value. */
	constexpr RegisterMask &operator=(RegisterMask &&) noexcept = default;

	/** @brief Destroys the predicate register value. */
	~RegisterMask() = default;

	/** @brief Tests whether any predicate lane is true. */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL any(this RegisterMask value) noexcept
	{
		return value.bits() != 0;
	}

	/** @brief Tests whether every predicate lane is true. */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL all(this RegisterMask value) noexcept
	{
		return value.bits() == all_bits;
	}

	/** @brief Tests whether every predicate lane is false. */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL none(this RegisterMask value) noexcept
	{
		return value.bits() == 0;
	}

	/** @brief Returns one compact bit per logical predicate lane. */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr bits_type VECTORCALL bits(this RegisterMask value) noexcept
	{
		return static_cast<bits_type>(api_type::movemask_slim(value.m_data));
	}

	/** @brief Returns the native predicate register by value. */
	[[nodiscard]] SIMDLIB_FORCE_INLINE SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS constexpr native_type VECTORCALL
		native(this RegisterMask value) noexcept
	{
		return value.m_data;
	}

	/** @brief Selects true or false register lanes according to this predicate. */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr register_type VECTORCALL select(
		this RegisterMask condition,
		register_type when_true,
		register_type when_false) noexcept;

	/** @brief Computes the intersection of two predicate registers. */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr RegisterMask VECTORCALL operator&(
		this RegisterMask lhs,
		RegisterMask rhs) noexcept
	{
		return RegisterMask{bitwise_and(lhs.m_data, rhs.m_data)};
	}

	/** @brief Computes the union of two predicate registers. */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr RegisterMask VECTORCALL operator|(
		this RegisterMask lhs,
		RegisterMask rhs) noexcept
	{
		return RegisterMask{bitwise_or(lhs.m_data, rhs.m_data)};
	}

	/** @brief Computes the exclusive union of two predicate registers. */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr RegisterMask VECTORCALL operator^(
		this RegisterMask lhs,
		RegisterMask rhs) noexcept
	{
		return RegisterMask{bitwise_xor(lhs.m_data, rhs.m_data)};
	}

	/** @brief Inverts every predicate lane. */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr RegisterMask VECTORCALL operator~(this RegisterMask value) noexcept
	{
		return RegisterMask{bitwise_not(value.m_data)};
	}

	/** @brief Intersects this predicate with another predicate. */
	SIMDLIB_FORCE_INLINE constexpr RegisterMask &operator&=(this RegisterMask &lhs, RegisterMask rhs) noexcept
	{
		return lhs = lhs & rhs;
	}

	/** @brief Unites this predicate with another predicate. */
	SIMDLIB_FORCE_INLINE constexpr RegisterMask &operator|=(this RegisterMask &lhs, RegisterMask rhs) noexcept
	{
		return lhs = lhs | rhs;
	}

	/** @brief Exclusively combines this predicate with another predicate. */
	SIMDLIB_FORCE_INLINE constexpr RegisterMask &operator^=(this RegisterMask &lhs, RegisterMask rhs) noexcept
	{
		return lhs = lhs ^ rhs;
	}

  private:
	constexpr static inline bits_type all_bits = []() constexpr noexcept {
		if constexpr (lane_count == std::numeric_limits<bits_type>::digits)
			return std::numeric_limits<bits_type>::max();
		else
			return (bits_type{1} << lane_count) - 1;
	}();

	/** @brief Wraps native lanes already known to be canonical predicates. */
	SIMDLIB_FORCE_INLINE constexpr explicit RegisterMask(native_type value) noexcept
		: m_data(value)
	{
	}

	/** @brief Computes the bitwise intersection of two native predicate registers. */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static native_type VECTORCALL bitwise_and(
		const native_type lhs,
		const native_type rhs) noexcept
	{
		return api_type::bitwise_and(lhs, rhs);
	}

	/** @brief Computes the bitwise union of two native predicate registers. */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static native_type VECTORCALL bitwise_or(
		const native_type lhs,
		const native_type rhs) noexcept
	{
		return api_type::bitwise_or(lhs, rhs);
	}

	/** @brief Computes the bitwise exclusive union of two native predicate registers. */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static native_type VECTORCALL bitwise_xor(
		const native_type lhs,
		const native_type rhs) noexcept
	{
		return api_type::bitwise_xor(lhs, rhs);
	}

	/** @brief Inverts every bit in a native predicate register. */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static native_type VECTORCALL bitwise_not(
		const native_type value) noexcept
	{
		return api_type::bitwise_not(value);
	}

	/** @brief Selects native true or false lanes according to a canonical predicate register. */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr native_type VECTORCALL select_native(
		this RegisterMask condition,
		const native_type when_true,
		const native_type when_false) noexcept
	{
		return api_type::select(condition.m_data, when_true, when_false);
	}

	native_type m_data;

	friend class Register<element_type, register_bits>;
};

} // namespace SimdLib

#include <SimdLib/Register.h>
