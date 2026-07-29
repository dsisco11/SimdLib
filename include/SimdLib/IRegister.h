#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <span>
#include <type_traits>
#include <utility>

namespace SimdLib::IRegister
{

/** @brief Identifies an aggregate Register-shaped type with public SIMD metadata and native storage. */
template <class register_t>
concept Type = std::is_aggregate_v<register_t> && requires(register_t value, typename register_t::native_type native) {
	typename register_t::element_type;
	typename register_t::api_type;
	typename register_t::native_type;
	typename register_t::mask_type;
	{ register_t::register_width } -> std::convertible_to<const std::size_t &>;
	{ register_t::byte_count } -> std::convertible_to<const std::size_t &>;
	{ register_t::lane_count } -> std::convertible_to<const std::size_t &>;
	{ value.native } -> std::same_as<typename register_t::native_type &>;
	{ register_t{native} } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes zero initialization. */
template <class register_t>
concept Zero = Type<register_t> && requires {
	{ register_t::zero() } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes scalar broadcast construction. */
template <class register_t>
concept Broadcast = Type<register_t> && requires(typename register_t::element_type value) {
	{ register_t::broadcast(value) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type accepts the supplied complete logical lane list. */
template <class register_t, class... lane_types>
concept FromLanes = Type<register_t> && sizeof...(lane_types) == register_t::lane_count && requires(lane_types &&...lanes) {
	{ register_t::from_lanes(std::forward<lane_types>(lanes)...) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes fixed-size array construction. */
template <class register_t>
concept FromArray = Type<register_t> && requires(const std::array<typename register_t::element_type, register_t::lane_count> &source) {
	{ register_t::from_array(source) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes exact-width unaligned loading. */
template <class register_t>
concept Load = Type<register_t> && requires(std::span<const typename register_t::element_type, register_t::lane_count> source) {
	{ register_t::load(source) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes exact-width aligned loading. */
template <class register_t>
concept LoadAligned = Type<register_t> && requires(std::span<const typename register_t::element_type, register_t::lane_count> source) {
	{ register_t::load_aligned(source) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes exact-width raw-byte loading. */
template <class register_t>
concept LoadBytes = Type<register_t> && requires(std::span<const std::byte, register_t::byte_count> source) {
	{ register_t::load_bytes(source) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes exact-width unaligned storage. */
template <class register_t>
concept Store = Type<register_t> && requires(register_t value, std::span<typename register_t::element_type, register_t::lane_count> destination) {
	{ value.store(destination) } -> std::same_as<void>;
};

/** @brief Reports whether a Register type exposes exact-width aligned storage. */
template <class register_t>
concept StoreAligned = Type<register_t> && requires(register_t value, std::span<typename register_t::element_type, register_t::lane_count> destination) {
	{ value.store_aligned(destination) } -> std::same_as<void>;
};

/** @brief Reports whether a Register type exposes exact-width raw-byte storage. */
template <class register_t>
concept StoreBytes = Type<register_t> && requires(register_t value, std::span<std::byte, register_t::byte_count> destination) {
	{ value.store_bytes(destination) } -> std::same_as<void>;
};

/** @brief Reports whether a Register type exposes fixed-size array conversion. */
template <class register_t>
concept ToArray = Type<register_t> && requires(register_t value) {
	{ value.to_array() } -> std::same_as<std::array<typename register_t::element_type, register_t::lane_count>>;
};

/** @brief Reports whether a Register type exposes one compile-time-selected lane. */
template <class register_t, std::size_t index>
concept Lane = Type<register_t> && requires(register_t value) {
	{ value.template lane<index>() } -> std::same_as<typename register_t::element_type>;
};

/** @brief Reports whether a Register type can replace one compile-time-selected lane. */
template <class register_t, std::size_t index>
concept WithLane = Type<register_t> && requires(register_t value, typename register_t::element_type replacement) {
	{ value.template with_lane<index>(replacement) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes register addition. */
template <class register_t>
concept Add = Type<register_t> && requires(register_t lhs, register_t rhs) {
	{ lhs + rhs } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes register subtraction. */
template <class register_t>
concept Subtract = Type<register_t> && requires(register_t lhs, register_t rhs) {
	{ lhs - rhs } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes register multiplication. */
template <class register_t>
concept Multiply = Type<register_t> && requires(register_t lhs, register_t rhs) {
	{ lhs * rhs } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes register division. */
template <class register_t>
concept Divide = Type<register_t> && requires(register_t lhs, register_t rhs) {
	{ lhs / rhs } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes register remainder. */
template <class register_t>
concept Modulus = Type<register_t> && requires(register_t lhs, register_t rhs) {
	{ lhs % rhs } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes arithmetic negation. */
template <class register_t>
concept Negate = Type<register_t> && requires(register_t value) {
	{ -value } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes lane-wise minimum. */
template <class register_t>
concept Min = Type<register_t> && requires(register_t value) {
	{ value.min(value) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes lane-wise maximum. */
template <class register_t>
concept Max = Type<register_t> && requires(register_t value) {
	{ value.max(value) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes lane-wise absolute value. */
template <class register_t>
concept Absolute = Type<register_t> && requires(register_t value) {
	{ value.absolute() } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes lane-wise square root. */
template <class register_t>
concept Sqrt = Type<register_t> && requires(register_t value) {
	{ value.sqrt() } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes lane-wise average. */
template <class register_t>
concept Average = Type<register_t> && requires(register_t value) {
	{ value.average(value) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes multiply-add. */
template <class register_t>
concept MultiplyAdd = Type<register_t> && requires(register_t value) {
	{ value.multiply_add(value, value) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes magnitude. */
template <class register_t>
concept Magnitude = Type<register_t> && requires(register_t value) {
	{ value.magnitude() } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes checked magnitude. */
template <class register_t>
concept MagnitudeChecked = Type<register_t> && requires(register_t value) {
	{ value.magnitude_checked() } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes normalization. */
template <class register_t>
concept Normalize = Type<register_t> && requires(register_t value) {
	{ value.normalize() } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes horizontal addition. */
template <class register_t>
concept HorizontalAdd = Type<register_t> && requires(register_t value) {
	{ value.horizontal_add(value) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes horizontal subtraction. */
template <class register_t>
concept HorizontalSubtract = Type<register_t> && requires(register_t value) {
	{ value.horizontal_subtract(value) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes adjacent multiply-add for the requested source type. */
template <class register_t, class source_element_t = typename register_t::element_type>
concept MultiplyAddAdjacent = Type<register_t> && requires(register_t value) { value.template multiply_add_adjacent<source_element_t>(value); };

/** @brief Reports whether a Register type exposes unsigned/signed byte multiply-add for the requested source type. */
template <class register_t, class source_element_t = typename register_t::element_type>
concept MultiplyAddUnsignedSignedBytes =
	Type<register_t> && requires(register_t value) { value.template multiply_add_unsigned_signed_bytes<source_element_t>(value); };

/** @brief Reports whether a Register type exposes byte sum-of-absolute-differences for the requested source type. */
template <class register_t, class source_element_t = typename register_t::element_type>
concept SumAbsoluteByteDifferences = Type<register_t> && requires(register_t value) { value.template sum_absolute_byte_differences<source_element_t>(value); };

/** @brief Reports whether a Register type exposes immediate-controlled multi-SAD for the requested source type. */
template <class register_t, int immediate, class source_element_t = typename register_t::element_type>
concept MultiSumAbsoluteByteDifferences =
	Type<register_t> && requires(register_t value) { value.template multi_sum_absolute_byte_differences<immediate, source_element_t>(value); };

/** @brief Reports whether a Register type exposes minimum-position lookup. */
template <class register_t>
concept MinPosition = Type<register_t> && requires(register_t value) {
	{ value.min_position() } -> std::same_as<std::size_t>;
};

/** @brief Reports whether a Register type exposes maximum-position lookup. */
template <class register_t>
concept MaxPosition = Type<register_t> && requires(register_t value) {
	{ value.max_position() } -> std::same_as<std::size_t>;
};

/** @brief Reports whether a Register type exposes saturating addition. */
template <class register_t>
concept AddSaturated = Type<register_t> && requires(register_t value) {
	{ value.add_saturated(value) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes saturating subtraction. */
template <class register_t>
concept SubtractSaturated = Type<register_t> && requires(register_t value) {
	{ value.subtract_saturated(value) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes saturating horizontal addition. */
template <class register_t>
concept HorizontalAddSaturated = Type<register_t> && requires(register_t value) {
	{ value.horizontal_add_saturated(value) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes saturating horizontal subtraction. */
template <class register_t>
concept HorizontalSubtractSaturated = Type<register_t> && requires(register_t value) {
	{ value.horizontal_subtract_saturated(value) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes alternating add-subtract. */
template <class register_t>
concept AddSubtract = Type<register_t> && requires(register_t value) {
	{ value.add_subtract(value) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes an immediate-controlled dot product. */
template <class register_t, int immediate>
concept DotProduct = Type<register_t> && requires(register_t value) {
	{ value.template dot_product<immediate>(value) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes bitwise AND. */
template <class register_t>
concept BitwiseAnd = Type<register_t> && requires(register_t lhs, register_t rhs) {
	{ lhs & rhs } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes bitwise OR. */
template <class register_t>
concept BitwiseOr = Type<register_t> && requires(register_t lhs, register_t rhs) {
	{ lhs | rhs } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes bitwise XOR. */
template <class register_t>
concept BitwiseXor = Type<register_t> && requires(register_t lhs, register_t rhs) {
	{ lhs ^ rhs } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes bitwise complement. */
template <class register_t>
concept BitwiseNot = Type<register_t> && requires(register_t value) {
	{ ~value } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes bitwise AND-NOT. */
template <class register_t>
concept BitwiseAndNot = Type<register_t> && requires(register_t lhs, register_t rhs) {
	{ lhs.andnot(rhs) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes its native-granularity sign mask. */
template <class register_t>
concept Movemask = Type<register_t> && requires(register_t value) {
	{ value.movemask() } -> std::same_as<typename register_t::api_type::mask_t>;
};

/** @brief Reports whether a Register type exposes one sign bit per logical lane. */
template <class register_t>
concept LaneSignBits = Type<register_t> && requires(register_t value) {
	{ value.lane_sign_bits() } -> std::same_as<typename register_t::api_type::mask_t>;
};

/** @brief Reports whether a Register type exposes per-lane left shift. */
template <class register_t>
concept ShiftLeft = Type<register_t> && requires(register_t value) {
	{ value << 1 } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes logical per-lane right shift. */
template <class register_t>
concept LogicalShiftRight = Type<register_t> && requires(register_t value) {
	{ value.logical_shift_right(1) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes signedness-selected per-lane right shift. */
template <class register_t>
concept ShiftRight = Type<register_t> && requires(register_t value) {
	{ value >> 1 } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes explicit slow-path complete-register dynamic byte left shift. */
template <class register_t>
concept ByteShiftLeftSlow = Type<register_t> && requires(register_t value) {
	{ value.byte_shift_left_slow(1) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes explicit slow-path complete-register dynamic byte right shift. */
template <class register_t>
concept ByteShiftRightSlow = Type<register_t> && requires(register_t value) {
	{ value.byte_shift_right_slow(1) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes explicit slow-path complete-register dynamic bit left shift. */
template <class register_t>
concept BitShiftLeftSlow = Type<register_t> && requires(register_t value) {
	{ value.bit_shift_left_slow(1) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes explicit slow-path complete-register dynamic bit right shift. */
template <class register_t>
concept BitShiftRightSlow = Type<register_t> && requires(register_t value) {
	{ value.bit_shift_right_slow(1) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes complete-register compile-time bit left shift. */
template <class register_t, int count>
concept IndexedBitShiftLeft = Type<register_t> && requires(register_t value) {
	{ value.template bit_shift_left<count>() } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes complete-register compile-time bit right shift. */
template <class register_t, int count>
concept IndexedBitShiftRight = Type<register_t> && requires(register_t value) {
	{ value.template bit_shift_right<count>() } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register type exposes ordered equality comparison. */
template <class register_t>
concept CompareEqual = Type<register_t> && requires(register_t value) {
	{ value.compare_equal(value) } -> std::same_as<typename register_t::mask_type>;
};

/** @brief Reports whether a Register type exposes ordered greater-than comparison. */
template <class register_t>
concept CompareGreater = Type<register_t> && requires(register_t value) {
	{ value.compare_greater(value) } -> std::same_as<typename register_t::mask_type>;
};

/** @brief Reports whether a Register type exposes ordered greater-than-or-equal comparison. */
template <class register_t>
concept CompareGreaterEqual = Type<register_t> && requires(register_t value) {
	{ value.compare_greater_equal(value) } -> std::same_as<typename register_t::mask_type>;
};

/** @brief Reports whether a Register type exposes ordered less-than comparison. */
template <class register_t>
concept CompareLess = Type<register_t> && requires(register_t value) {
	{ value.compare_less(value) } -> std::same_as<typename register_t::mask_type>;
};

/** @brief Reports whether a Register type exposes ordered less-than-or-equal comparison. */
template <class register_t>
concept CompareLessEqual = Type<register_t> && requires(register_t value) {
	{ value.compare_less_equal(value) } -> std::same_as<typename register_t::mask_type>;
};

/** @brief Reports whether a Register type exposes whole-register equality. */
template <class register_t>
concept Equal = Type<register_t> && requires(register_t lhs, register_t rhs) {
	{ lhs == rhs } -> std::same_as<bool>;
};

/** @brief Reports whether a Register type exposes whole-register inequality. */
template <class register_t>
concept NotEqual = Type<register_t> && requires(register_t lhs, register_t rhs) {
	{ lhs != rhs } -> std::same_as<bool>;
};

/** @brief Identifies a Register-shaped result with the requested element type and width. */
template <class register_t, class element_t, std::size_t bits>
concept Shape = Type<register_t> && std::same_as<typename register_t::element_type, element_t> && register_t::register_width == bits;

/** @brief Reports whether a Register exposes its lower 128-bit half. */
template <class register_t>
concept LowerHalf = Type<register_t> && requires(register_t value) {
	{ value.lower_half() } -> Shape<typename register_t::element_type, 128>;
};

/** @brief Reports whether a Register exposes low-lane unpacking. */
template <class register_t>
concept UnpackLow = Type<register_t> && requires(register_t value) {
	{ value.unpack_low(value) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register exposes high-lane unpacking. */
template <class register_t>
concept UnpackHigh = Type<register_t> && requires(register_t value) {
	{ value.unpack_high(value) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register accepts one compile-time logical shuffle selector sequence. */
template <class register_t, std::size_t... indices>
concept Shuffle = Type<register_t> && requires(register_t value) {
	{ value.template shuffle<indices...>() } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register accepts one compile-time byte selector sequence. */
template <class register_t, std::size_t... indices>
concept ShuffleBytes = Type<register_t> && requires(register_t value) {
	{ value.template shuffle_bytes<indices...>() } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register exposes an immediate-controlled low-half shuffle. */
template <class register_t, int immediate>
concept ShuffleLow = Type<register_t> && requires(register_t value) {
	{ value.template shuffle_low<immediate>() } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register exposes an immediate-controlled high-half shuffle. */
template <class register_t, int immediate>
concept ShuffleHigh = Type<register_t> && requires(register_t value) {
	{ value.template shuffle_high<immediate>() } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register exposes an immediate-controlled two-register blend. */
template <class register_t, int immediate>
concept Blend = Type<register_t> && requires(register_t lhs, register_t rhs) {
	{ lhs.template blend<immediate>(rhs) } -> std::same_as<register_t>;
};

/** @brief Reports whether a Register can reinterpret its complete bit pattern as the requested element type. */
template <class register_t, class target_t>
concept BitCast = Type<register_t> && requires(register_t value) {
	{ value.template bit_cast<target_t>() } -> Shape<target_t, register_t::register_width>;
};

/** @brief Reports whether a Register can numerically convert every lane to the requested element type. */
template <class register_t, class target_t>
concept Convert = Type<register_t> && requires(register_t value) {
	{ value.template convert<target_t>() } -> Shape<target_t, register_t::register_width>;
};

/** @brief Reports whether a Register can widen its lowest lanes into the requested complete target register. */
template <class register_t, class target_t, std::size_t target_bits>
concept WidenLow = Type<register_t> && requires(register_t value) {
	{ value.template widen_low<target_t, target_bits>() } -> Shape<target_t, target_bits>;
};

} // namespace SimdLib::IRegister
