#pragma once

#include <SimdLib/Api.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace SimdLib
{

/**
 * @brief Reports whether a complete SIMD register is available for an element type and width.
 * @tparam element_t Scalar interpretation of the register lanes.
 * @tparam bits Width of the native register in bits.
 */
template <class element_t, std::size_t bits> inline constexpr bool is_register_available_v = is_api_available_v<bits, element_t>;

/**
 * @brief Constrains a type and width to an available complete SIMD register.
 * @tparam element_t Scalar interpretation of the register lanes.
 * @tparam bits Width of the native register in bits.
 */
template <class element_t, std::size_t bits>
concept RegisterAvailable = is_register_available_v<element_t, bits>;

template <class element_t, std::size_t bits>
	requires RegisterAvailable<element_t, bits>
class Register;

template <class element_t, std::size_t bits>
	requires RegisterAvailable<element_t, bits>
class RegisterMask;

namespace Detail
{

/**
 * @brief Maps an integral lane type to the result lane produced by adjacent multiply-add.
 * @tparam element_t Source integral lane type.
 */
template <class element_t>
using multiply_add_adjacent_element_t = std::conditional_t<
	(sizeof(element_t) >= sizeof(std::int64_t)), element_t,
	std::conditional_t<std::is_signed_v<element_t>,
					   std::conditional_t<sizeof(element_t) == 1, std::int16_t, std::conditional_t<sizeof(element_t) == 2, std::int32_t, std::int64_t>>,
					   std::conditional_t<sizeof(element_t) == 1, std::uint16_t, std::conditional_t<sizeof(element_t) == 2, std::uint32_t, std::uint64_t>>>>;

} // namespace Detail

/**
 * @brief Result Register produced by adjacent integer multiply-add.
 * @tparam element_t Source integral lane type.
 * @tparam bits Register width in bits.
 */
template <class element_t, std::size_t bits>
	requires RegisterAvailable<element_t, bits> && std::is_integral_v<element_t> && IApi::MultiplyAddAdjacent<Api<bits, element_t>>
using multiply_add_adjacent_result_t = Register<Detail::multiply_add_adjacent_element_t<element_t>, bits>;

/**
 * @brief Signed 16-bit result Register produced by unsigned/signed byte multiply-add.
 * @tparam element_t Source lane type whose register bits are interpreted as bytes.
 * @tparam bits Register width in bits.
 */
template <class element_t, std::size_t bits>
	requires RegisterAvailable<element_t, bits> && std::is_integral_v<element_t> && IApi::ByteMultiplyAdd<Api<bits, element_t>>
using byte_multiply_add_result_t = Register<std::int16_t, bits>;

/**
 * @brief Unsigned 64-bit result Register produced by byte absolute-difference sums.
 * @tparam element_t Source lane type whose register bits are interpreted as bytes.
 * @tparam bits Register width in bits.
 */
template <class element_t, std::size_t bits>
	requires RegisterAvailable<element_t, bits> && std::is_integral_v<element_t> && IApi::Sad<Api<bits, element_t>>
using sad_result_t = Register<std::uint64_t, bits>;

/**
 * @brief Unsigned 16-bit result Register produced by immediate-controlled multi-SAD.
 * @tparam element_t Source lane type whose register bits are interpreted as bytes.
 * @tparam bits Register width in bits.
 */
template <class element_t, std::size_t bits>
	requires RegisterAvailable<element_t, bits> && std::is_integral_v<element_t> && IApi::MultiSad<Api<bits, element_t>, 0>
using multi_sad_result_t = Register<std::uint16_t, bits>;
} // namespace SimdLib
