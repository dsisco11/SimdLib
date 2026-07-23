#pragma once

#include <SimdLib/Api.h>

#include <concepts>
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

template <class element_t, std::size_t bits>
	requires RegisterAvailable<element_t, bits>
class Register;

template <class element_t, std::size_t bits>
	requires RegisterAvailable<element_t, bits>
class RegisterMask;

} // namespace SimdLib
