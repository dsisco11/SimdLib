#pragma once

#include <SimdLib/SimdLib.h>

#include <cstddef>
#include <cstdint>

#ifndef SIMDLIB_PARTIAL_REGISTER_ODR_BITS
#define SIMDLIB_PARTIAL_REGISTER_ODR_BITS 128
#endif

namespace SimdLibPartialRegisterOdr
{

constexpr inline std::size_t active_lane_count = SIMDLIB_PARTIAL_REGISTER_ODR_BITS == 256 ? 5 : 3;
using Register = SimdLib::PartialRegister<std::uint32_t, SIMDLIB_PARTIAL_REGISTER_ODR_BITS, active_lane_count>;
using RegisterMask = Register::mask_type;

/**
 * @brief Adds two partial registers in a separate translation unit.
 * @param lhs Left operand with a zero inactive suffix.
 * @param rhs Right operand with a zero inactive suffix.
 * @return Lane-wise active-prefix sum with a zero inactive suffix.
 */
[[nodiscard]] Register SIMD_FLAGS(InOut, RegisterOnly) add(Register lhs, Register rhs) noexcept;

/**
 * @brief Compares two partial registers in a separate translation unit.
 * @param lhs Left operand with a zero inactive suffix.
 * @param rhs Right operand with a zero inactive suffix.
 * @return Active-prefix equality predicate with false inactive lanes.
 */
[[nodiscard]] RegisterMask SIMD_FLAGS(InOut, RegisterOnly) equal(Register lhs, Register rhs) noexcept;

} // namespace SimdLibPartialRegisterOdr
