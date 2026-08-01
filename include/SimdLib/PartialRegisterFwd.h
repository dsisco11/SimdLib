#pragma once

#include <SimdLib/RegisterFwd.h>

#include <cstddef>

namespace SimdLib
{

/**
 * @brief Owns one SIMD register with a compile-time low prefix of active lanes.
 * @tparam element_t Scalar interpretation of every lane.
 * @tparam bits Physical native-register width in bits.
 * @tparam active_lane_count Number of logical low lanes retained by the value.
 * @remarks Inactive high lanes always contain the all-bits-zero representation.
 */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
	requires RegisterAvailable<element_t, bits> && (active_lane_count > 0) && (active_lane_count < Api<bits, element_t>::element_count)
class PartialRegister;

/**
 * @brief Declares the predicate companion for a PartialRegister geometry.
 * @tparam element_t Scalar geometry represented by every predicate lane.
 * @tparam bits Physical native predicate-register width in bits.
 * @tparam active_lane_count Number of logical low predicate lanes.
 */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
	requires RegisterAvailable<element_t, bits> && (active_lane_count > 0) && (active_lane_count < Api<bits, element_t>::element_count)
class PartialRegisterMask;

/**
 * @brief Selects the widest available PartialRegister width for an element type.
 * @tparam element_t Scalar interpretation of every lane.
 * @tparam active_lane_count Number of logical low lanes retained by the value.
 * @remarks The selected width follows NativeRegister's 256-bit-then-128-bit rule.
 */
template <class element_t, std::size_t active_lane_count>
	requires RegisterAvailable<element_t, 128>
using NativePartialRegister = PartialRegister<element_t, (is_register_available_v<element_t, 256> ? 256 : 128), active_lane_count>;

} // namespace SimdLib
