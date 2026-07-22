#pragma once

#include <SimdLib/Config.h>

#if !SIMDLIB_REGISTER_INTERFACE_AVAILABLE && !SIMDLIB_REQUIRE_REGISTER_INTERFACE
#error "SIMDLIB_REGISTER_HEADER_REQUIRES_CXX23: <SimdLib/Register.h> requires C++23 explicit object parameter support"
#endif

#include <SimdLib/Api.h>

#include <cstddef>

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
