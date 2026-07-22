#pragma once

#include <SimdLib/Config.h>

#if !SIMDLIB_REGISTER_INTERFACE_AVAILABLE && !SIMDLIB_REQUIRE_REGISTER_INTERFACE
#error "SIMDLIB_REGISTER_HEADER_REQUIRES_CXX23: <SimdLib/Register.h> requires C++23 explicit object parameter support"
#endif

#include <SimdLib/Api.h>

#include <array>
#include <concepts>
#include <cstddef>
#include <span>
#include <utility>

namespace SimdLib
{

/**
 * @brief Reports whether a complete SIMD register is available for an element type and width.
 * @tparam element_t Scalar interpretation of the register lanes.
 * @tparam bits Width of the native register in bits.
 */
template <class element_t, std::size_t bits>
inline constexpr bool is_register_available_v = is_api_available_v<bits, element_t>;

/**
 * @brief Constrains a type and width to an available complete SIMD register.
 * @tparam element_t Scalar interpretation of the register lanes.
 * @tparam bits Width of the native register in bits.
 */
template <class element_t, std::size_t bits>
concept RegisterAvailable = is_register_available_v<element_t, bits>;

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
	using element_type = element_t;
	using api_type = Api<bits, element_type>;
	using native_type = typename api_type::vector_t;

	constexpr static inline std::size_t register_width = bits;
	constexpr static inline std::size_t byte_count = api_type::byte_count;
	constexpr static inline std::size_t lane_count = api_type::element_count;

	/** @brief Constructs an all-false predicate through the native zero-register operation. */
	SIMDLIB_FORCE_INLINE SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS constexpr RegisterMask() noexcept
		: m_data(api_type::setzero())
	{
	}

	/** @brief Copies one complete predicate register. */
	constexpr RegisterMask(const RegisterMask &) noexcept = default;

	/** @brief Moves one complete predicate register. */
	constexpr RegisterMask(RegisterMask &&) noexcept = default;

	/** @brief Replaces this predicate with a copied complete predicate register. */
	constexpr RegisterMask &operator=(const RegisterMask &) noexcept = default;

	/** @brief Replaces this predicate with a moved complete predicate register. */
	constexpr RegisterMask &operator=(RegisterMask &&) noexcept = default;

	/** @brief Destroys the predicate register value. */
	~RegisterMask() = default;

  private:
	native_type m_data;
};

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

	/** @brief Constructs a register with every active lane set to zero through the native zero-register operation. */
	SIMDLIB_FORCE_INLINE SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS constexpr Register() noexcept
		: m_data(api_type::setzero())
	{
	}

	/**
	 * @brief Wraps one complete native register without changing its bits.
	 * @param value Complete native register value.
	 */
	SIMDLIB_FORCE_INLINE SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS constexpr explicit Register(native_type value) noexcept
		: m_data(value)
	{
	}

	/**
	 * @brief Returns a register with every active lane set to zero.
	 * @return Fully initialized zero register.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS constexpr static Register zero() noexcept
	{
		return Register{api_type::setzero()};
	}

	/**
	 * @brief Broadcasts one scalar value to every active lane.
	 * @param value Scalar value to broadcast.
	 * @return Register containing `value` in every lane.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS constexpr static Register broadcast(
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
	[[nodiscard]] SIMDLIB_FORCE_INLINE SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS constexpr static Register from_lanes(
		lane_types &&...lanes) noexcept
	{
		return Register{api_type::setr(static_cast<element_type>(std::forward<lane_types>(lanes))...)};
	}

	/**
	 * @brief Constructs a register from one complete fixed-size lane array.
	 * @param source Source containing every active lane in logical order.
	 * @return Register containing all source lane values.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static Register from_array(
		const std::array<element_type, lane_count> &source) noexcept
	{
		return Register{api_type::construct(source)};
	}

	/**
	 * @brief Loads a complete register from potentially unaligned storage.
	 * @param source Source containing exactly one register of elements.
	 * @return Register loaded from `source`.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE static Register load(
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
	[[nodiscard]] SIMDLIB_FORCE_INLINE static Register load_aligned(
		std::span<const element_type, lane_count> source) noexcept
	{
		return Register{api_type::load_aligned(source)};
	}

	/**
	 * @brief Loads one complete register bit pattern from raw bytes.
	 * @param source Source containing exactly one register of bytes.
	 * @return Register containing the source bit pattern.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE static Register load_bytes(
		std::span<const std::byte, byte_count> source) noexcept
	{
		return Register{api_type::load(source)};
	}

	/** @brief Copies one complete register. */
	constexpr Register(const Register &) noexcept = default;

	/** @brief Moves one complete register. */
	constexpr Register(Register &&) noexcept = default;

	/** @brief Replaces this value with a copied complete register. */
	constexpr Register &operator=(const Register &) noexcept = default;

	/** @brief Replaces this value with a moved complete register. */
	constexpr Register &operator=(Register &&) noexcept = default;

	/** @brief Destroys the register value. */
	~Register() = default;

	/**
	 * @brief Stores every active lane to potentially unaligned storage.
	 * @param value Register to store.
	 * @param destination Destination for exactly one register of elements.
	 */
	SIMDLIB_FORCE_INLINE void VECTORCALL store(
		this Register value,
		std::span<element_type, lane_count> destination) noexcept
	{
		api_type::store(value.m_data, destination);
	}

	/**
	 * @brief Stores every active lane to register-aligned storage.
	 * @param value Register to store.
	 * @param destination Aligned destination for one complete register.
	 * @pre `destination.data()` is aligned to `byte_count` bytes.
	 */
	SIMDLIB_FORCE_INLINE void VECTORCALL store_aligned(
		this Register value,
		std::span<element_type, lane_count> destination) noexcept
	{
		api_type::store_aligned(value.m_data, destination);
	}

	/**
	 * @brief Stores the complete register bit pattern to raw bytes.
	 * @param value Register to store.
	 * @param destination Destination containing exactly one register of bytes.
	 */
	SIMDLIB_FORCE_INLINE void VECTORCALL store_bytes(
		this Register value,
		std::span<std::byte, byte_count> destination) noexcept
	{
		api_type::store(value.m_data, destination);
	}

	/**
	 * @brief Copies every active lane into a fixed-size array.
	 * @param value Register to copy.
	 * @return Array containing all lanes in low-to-high logical order.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr std::array<element_type, lane_count> VECTORCALL to_array(
		this Register value) noexcept
	{
		return api_type::to_array(value.m_data);
	}

	/**
	 * @brief Returns one compile-time-selected lane.
	 * @tparam index Logical lane index.
	 * @param value Register containing the selected lane.
	 * @return Copy of the selected lane.
	 */
	template <std::size_t index>
		requires(index < lane_count)
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr element_type VECTORCALL lane(
		this Register value) noexcept
	{
		if consteval
		{
			return lane_constexpr<index>(value);
		}
		else
		{
			return api_type::template extract<static_cast<int>(index)>(value.m_data);
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
	[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr Register VECTORCALL with_lane(
		this Register value,
		element_type replacement) noexcept
	{
		if consteval
		{
			return with_lane_constexpr<index>(value, replacement);
		}
		else
		{
			value.m_data = api_type::template insert<index>(value.m_data, replacement);
			return value;
		}
	}

	/**
	 * @brief Returns the wrapped native register by value.
	 * @param value Register to unwrap.
	 * @return Complete native register value.
	 */
	[[nodiscard]] SIMDLIB_FORCE_INLINE SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS constexpr native_type VECTORCALL
		native(this Register value) noexcept
	{
		return value.m_data;
	}


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

	/**
	 * @brief Implements compile-time lane replacement through the portable array representation.
	 * @tparam index Logical lane index to replace.
	 * @param value Register containing the lanes to copy.
	 * @param replacement Replacement value for lane `index`.
	 * @return Register with lane `index` replaced.
	 */
	template <std::size_t index>
	[[nodiscard]] constexpr static Register with_lane_constexpr(
		Register value,
		element_type replacement) noexcept
	{
		auto lanes = value.to_array();
		lanes[index] = replacement;
		return from_array(lanes);
	}

	native_type m_data;
};

/**
 * @brief Selects the widest complete register available for an element type.
 * @tparam element_t Scalar interpretation of each register lane.
 */
template <class element_t>
	requires RegisterAvailable<element_t, 128>
using NativeRegister = Register<element_t, is_register_available_v<element_t, 256> ? 256 : 128>;

} // namespace SimdLib
