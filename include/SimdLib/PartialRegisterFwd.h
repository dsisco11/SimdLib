#pragma once

#include <SimdLib/RegisterFwd.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace SimdLib
{

/**
 * @brief Reports whether an element type, width, and active-lane count form a useful partial-register geometry.
 * @tparam element_t Scalar interpretation of every lane.
 * @tparam bits Physical native-register width in bits.
 * @tparam active_lane_count Number of logical low lanes retained by the value.
 * @remarks A 256-bit partial register must occupy at least one lane in its upper 128-bit group.
 */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
concept PartialRegisterAvailable = RegisterAvailable<element_t, bits> && (active_lane_count > 0) && (active_lane_count < Api<bits, element_t>::element_count) &&
								   ((bits != 256) || (active_lane_count * sizeof(element_t) * 8 > 128));

/** @brief Boolean form of PartialRegisterAvailable for metaprogramming contexts. */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
constexpr inline bool is_partial_register_available_v = PartialRegisterAvailable<element_t, bits, active_lane_count>;

/**
 * @brief Owns one SIMD register with a compile-time low prefix of active lanes.
 * @tparam element_t Scalar interpretation of every lane.
 * @tparam bits Physical native-register width in bits.
 * @tparam active_lane_count Number of logical low lanes retained by the value.
 * @remarks Inactive high lanes always contain the all-bits-zero representation.
 */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
	requires PartialRegisterAvailable<element_t, bits, active_lane_count>
class PartialRegister;

/**
 * @brief Declares the predicate companion for a PartialRegister geometry.
 * @tparam element_t Scalar geometry represented by every predicate lane.
 * @tparam bits Physical native predicate-register width in bits.
 * @tparam active_lane_count Number of logical low predicate lanes.
 */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
	requires PartialRegisterAvailable<element_t, bits, active_lane_count>
class PartialRegisterMask;

/**
 * @brief Selects the widest available PartialRegister width for an element type.
 * @tparam element_t Scalar interpretation of every lane.
 * @tparam active_lane_count Number of logical low lanes retained by the value.
 * @remarks Selects 256 bits only when that width is available and the active payload reaches its upper 128-bit group.
 */
template <class element_t, std::size_t active_lane_count>
	requires PartialRegisterAvailable<element_t, 128, active_lane_count> || PartialRegisterAvailable<element_t, 256, active_lane_count>
using NativePartialRegister = PartialRegister<element_t, (PartialRegisterAvailable<element_t, 256, active_lane_count> ? 256 : 128), active_lane_count>;

namespace Detail
{
/** @brief Selects a partial or complete result without forming a constraint-invalid unselected specialization. */
template <class element_t, std::size_t bits, std::size_t result_lane_count, bool use_complete_register> struct PartialOrCompleteRegisterSelector;

/** @brief Selects a complete Register when every target lane is meaningful. */
template <class element_t, std::size_t bits, std::size_t result_lane_count> struct PartialOrCompleteRegisterSelector<element_t, bits, result_lane_count, true>
{
	using type = Register<element_t, bits>;
};

/** @brief Selects a PartialRegister when the target retains an inactive suffix. */
template <class element_t, std::size_t bits, std::size_t result_lane_count> struct PartialOrCompleteRegisterSelector<element_t, bits, result_lane_count, false>
{
	using type = PartialRegister<element_t, bits, result_lane_count>;
};
} // namespace Detail

/**
 * @brief Selects a partial result unless it fills the target or cannot form a useful partial geometry.
 * @tparam element_t Scalar interpretation of the result lanes.
 * @tparam bits Physical result-register width in bits.
 * @tparam result_lane_count Number of meaningful low result lanes.
 */
template <class element_t, std::size_t bits, std::size_t result_lane_count>
	requires RegisterAvailable<element_t, bits> && (result_lane_count > 0) && (result_lane_count <= Api<bits, element_t>::element_count)
using partial_or_complete_register_t =
	typename Detail::PartialOrCompleteRegisterSelector<element_t, bits, result_lane_count,
													   result_lane_count == Api<bits, element_t>::element_count ||
														   !PartialRegisterAvailable<element_t, bits, result_lane_count>>::type;

/** @brief Scalar lane type produced by adjacent integer multiply-add. */
template <class element_t>
	requires std::is_integral_v<element_t>
using partial_multiply_add_adjacent_element_t = std::conditional_t<
	(sizeof(element_t) >= sizeof(std::int64_t)), element_t,
	std::conditional_t<std::is_signed_v<element_t>,
					   std::conditional_t<sizeof(element_t) == 1, std::int16_t, std::conditional_t<sizeof(element_t) == 2, std::int32_t, std::int64_t>>,
					   std::conditional_t<sizeof(element_t) == 1, std::uint16_t, std::conditional_t<sizeof(element_t) == 2, std::uint32_t, std::uint64_t>>>>;

/** @brief Result type produced by adjacent integer multiply-add over a partial logical prefix. */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
	requires PartialRegisterAvailable<element_t, bits, active_lane_count> && std::is_integral_v<element_t> && IApi::MultiplyAddAdjacent<Api<bits, element_t>>
using partial_multiply_add_adjacent_result_t =
	partial_or_complete_register_t<partial_multiply_add_adjacent_element_t<element_t>, bits, (active_lane_count + 1) / 2>;

/** @brief Result type produced by unsigned/signed byte multiply-add over active source bytes. */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
	requires PartialRegisterAvailable<element_t, bits, active_lane_count> && std::is_integral_v<element_t> && IApi::ByteMultiplyAdd<Api<bits, element_t>>
using partial_byte_multiply_add_result_t = partial_or_complete_register_t<std::int16_t, bits, (active_lane_count * sizeof(element_t) + 1) / 2>;

/** @brief Result type produced by byte absolute-difference sums over active source bytes. */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
	requires PartialRegisterAvailable<element_t, bits, active_lane_count> && std::is_integral_v<element_t> && IApi::Sad<Api<bits, element_t>>
using partial_sad_result_t = partial_or_complete_register_t<std::uint64_t, bits, (active_lane_count * sizeof(element_t) + 7) / 8>;

/**
 * @brief Result type produced by checked magnitude while retaining every occupied group's overflow lane.
 * @tparam element_t Scalar interpretation of the source and result lanes.
 * @tparam bits Physical source and result register width in bits.
 * @tparam active_lane_count Number of meaningful low source lanes.
 */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
	requires PartialRegisterAvailable<element_t, bits, active_lane_count> && std::is_integral_v<element_t> && IApi::MagnitudeChecked<Api<bits, element_t>>
constexpr inline std::size_t partial_magnitude_checked_lane_count = []() constexpr
{
	constexpr std::size_t group_lane_count = 128 / (sizeof(element_t) * 8);
	return ((active_lane_count - 1) / group_lane_count) * group_lane_count + 2;
}();

/** @brief Checked-magnitude result retaining each occupied group's magnitude and overflow lanes. */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
	requires PartialRegisterAvailable<element_t, bits, active_lane_count> && std::is_integral_v<element_t> && IApi::MagnitudeChecked<Api<bits, element_t>>
using partial_magnitude_checked_result_t =
	partial_or_complete_register_t<element_t, bits, partial_magnitude_checked_lane_count<element_t, bits, active_lane_count>>;

/** @brief Number of meaningful lanes retained when extracting the low 128-bit half of a partial register. */
template <class element_t, std::size_t active_lane_count>
	requires PartialRegisterAvailable<element_t, 256, active_lane_count> && IApi::LowerHalf<Api<256, element_t>>
constexpr inline std::size_t partial_lower_half_lane_count =
	active_lane_count < Api<128, element_t>::element_count ? active_lane_count : Api<128, element_t>::element_count;

/** @brief Partial or complete low-half result selected from the meaningful source-lane count. */
template <class element_t, std::size_t active_lane_count>
	requires PartialRegisterAvailable<element_t, 256, active_lane_count> && IApi::LowerHalf<Api<256, element_t>>
using partial_lower_half_result_t = partial_or_complete_register_t<element_t, 128, partial_lower_half_lane_count<element_t, active_lane_count>>;

/** @brief Number of complete target lanes represented by the active source-byte extent. */
template <class source_t, class target_t, std::size_t bits, std::size_t active_lane_count>
	requires PartialRegisterAvailable<source_t, bits, active_lane_count> && RegisterAvailable<target_t, bits> &&
				 ((active_lane_count * sizeof(source_t)) % sizeof(target_t) == 0) && IApi::BitCast<Api<bits, source_t>, target_t>
constexpr inline std::size_t partial_bit_cast_lane_count = active_lane_count * sizeof(source_t) / sizeof(target_t);

/** @brief Bit-cast result whose logical lane count exactly covers the active source bits. */
template <class source_t, class target_t, std::size_t bits, std::size_t active_lane_count>
	requires PartialRegisterAvailable<source_t, bits, active_lane_count> && RegisterAvailable<target_t, bits> &&
				 ((active_lane_count * sizeof(source_t)) % sizeof(target_t) == 0) && IApi::BitCast<Api<bits, source_t>, target_t>
using partial_bit_cast_result_t = partial_or_complete_register_t<target_t, bits, partial_bit_cast_lane_count<source_t, target_t, bits, active_lane_count>>;

/** @brief Numeric-conversion result retaining one meaningful target lane per active source lane. */
template <class source_t, class target_t, std::size_t bits, std::size_t active_lane_count>
	requires PartialRegisterAvailable<source_t, bits, active_lane_count> && RegisterAvailable<target_t, bits> &&
				 (active_lane_count <= Api<bits, target_t>::element_count) &&
				 (active_lane_count == Api<bits, target_t>::element_count || PartialRegisterAvailable<target_t, bits, active_lane_count>) &&
				 IApi::Convert<Api<bits, source_t>, target_t>
using partial_convert_result_t = partial_or_complete_register_t<target_t, bits, active_lane_count>;

/** @brief Number of low active source lanes consumed by one widening operation. */
template <class source_t, class target_t, std::size_t source_bits, std::size_t target_bits, std::size_t active_lane_count>
	requires PartialRegisterAvailable<source_t, source_bits, active_lane_count> && RegisterAvailable<target_t, target_bits> &&
				 IApi::Widen<Api<source_bits, source_t>, Api<target_bits, target_t>>
constexpr inline std::size_t partial_widen_low_lane_count =
	active_lane_count < Api<target_bits, target_t>::element_count ? active_lane_count : Api<target_bits, target_t>::element_count;

/** @brief Widening result whose type exposes exactly the consumed low source lanes. */
template <class source_t, class target_t, std::size_t source_bits, std::size_t target_bits, std::size_t active_lane_count>
	requires PartialRegisterAvailable<source_t, source_bits, active_lane_count> && RegisterAvailable<target_t, target_bits> &&
				 IApi::Widen<Api<source_bits, source_t>, Api<target_bits, target_t>> &&
				 (partial_widen_low_lane_count<source_t, target_t, source_bits, target_bits, active_lane_count> == Api<target_bits, target_t>::element_count ||
				  PartialRegisterAvailable<target_t, target_bits,
										   partial_widen_low_lane_count<source_t, target_t, source_bits, target_bits, active_lane_count>>)
using partial_widen_low_result_t =
	partial_or_complete_register_t<target_t, target_bits, partial_widen_low_lane_count<source_t, target_t, source_bits, target_bits, active_lane_count>>;

} // namespace SimdLib
