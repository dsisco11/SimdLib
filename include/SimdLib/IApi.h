#pragma once

#include <SimdLib/Config.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace SimdLib
{

/** @brief Reports whether one scalar type and register width have a configured SIMD backend. */
template <std::size_t register_width, class element_t>
inline constexpr bool is_api_available_v =
	(std::same_as<element_t, std::int8_t> || std::same_as<element_t, std::uint8_t> || std::same_as<element_t, std::int16_t> ||
	 std::same_as<element_t, std::uint16_t> || std::same_as<element_t, std::int32_t> || std::same_as<element_t, std::uint32_t> ||
	 std::same_as<element_t, std::int64_t> || std::same_as<element_t, std::uint64_t> || std::same_as<element_t, float> || std::same_as<element_t, double>) &&
	Config::target_x86 && ((register_width == 128 && Config::has_sse42) || (register_width == 256 && Config::has_sse42 && Config::has_avx2));

/** @brief Constrains one scalar type and register width to a configured SIMD backend. */
template <std::size_t register_width, class element_t>
concept ApiAvailable = is_api_available_v<register_width, element_t>;

/** @brief Reports whether the native-width API alias is available for an element type. */
template <class element_t>
concept NativeApiAvailable = ApiAvailable<128, element_t>;

/** @brief Structural interface requirements exposed by an API layer. */
namespace IApi
{

/** @brief Identifies an API-shaped type with native vector and scalar metadata. */
template <class api_t>
concept Type = requires {
	typename api_t::element_type;
	typename api_t::vector_t;
};

/** @brief Identifies a valid widening destination API shape. */
template <class api_t>
concept WidenTarget = Type<api_t> && requires {
	{ api_t::register_width } -> std::convertible_to<const std::size_t &>;
};

/** @brief Reports whether an API exposes lane-wise addition. */
template <class api_t>
concept Add = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::add(lhs, rhs); };

/** @brief Reports whether an API exposes lane-wise subtraction. */
template <class api_t>
concept Subtract = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::subtract(lhs, rhs); };

/** @brief Reports whether an API exposes lane-wise multiplication. */
template <class api_t>
concept Multiply = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::multiply(lhs, rhs); };

/** @brief Reports whether an API exposes lane-wise division. */
template <class api_t>
concept Divide = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::divide(lhs, rhs); };

/** @brief Reports whether an API exposes lane-wise remainder. */
template <class api_t>
concept Modulus = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::modulus(lhs, rhs); };

/** @brief Reports whether an API exposes arithmetic negation. */
template <class api_t>
concept Negate = Type<api_t> && requires(typename api_t::vector_t value) { api_t::negate(value); };

/** @brief Reports whether an API exposes lane-wise minimum. */
template <class api_t>
concept Min = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::min(lhs, rhs); };

/** @brief Reports whether an API exposes lane-wise maximum. */
template <class api_t>
concept Max = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::max(lhs, rhs); };

/** @brief Reports whether an API exposes lane-wise absolute value. */
template <class api_t>
concept Absolute = Type<api_t> && requires(typename api_t::vector_t value) { api_t::absolute(value); };

/** @brief Reports whether an API exposes lane-wise square root. */
template <class api_t>
concept Sqrt = Type<api_t> && requires(typename api_t::vector_t value) { api_t::sqrt(value); };

/** @brief Reports whether an API exposes lane-wise average. */
template <class api_t>
concept Average = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::avg(lhs, rhs); };

/** @brief Reports whether an API exposes fused or emulated multiply-add. */
template <class api_t>
concept MultiplyAdd = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs, typename api_t::vector_t addend) {
	api_t::multiply_add(lhs, rhs, addend);
};

/** @brief Reports whether an API exposes register magnitude. */
template <class api_t>
concept Magnitude = Type<api_t> && requires(typename api_t::vector_t value) { api_t::magnitude(value); };

/** @brief Reports whether an API exposes checked integer magnitude. */
template <class api_t>
concept MagnitudeChecked = Type<api_t> && requires(typename api_t::vector_t value) { api_t::magnitude_checked(value); };

/** @brief Reports whether an API exposes floating-point normalization. */
template <class api_t>
concept Normalize = Type<api_t> && requires(typename api_t::vector_t value) { api_t::normalize(value); };

/** @brief Reports whether an API exposes adjacent horizontal addition. */
template <class api_t>
concept HorizontalAdd = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::add_horizontal(lhs, rhs); };

/** @brief Reports whether an API exposes adjacent horizontal subtraction. */
template <class api_t>
concept HorizontalSubtract = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::subtract_horizontal(lhs, rhs); };

/** @brief Reports whether an API exposes adjacent multiply-add. */
template <class api_t>
concept MultiplyAddAdjacent = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::multiply_add_adjacent(lhs, rhs); };

/** @brief Reports whether an API exposes unsigned-byte by signed-byte multiply-add. */
template <class api_t>
concept ByteMultiplyAdd =
	Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::multiply_add_unsigned_signed_bytes(lhs, rhs); };

/** @brief Reports whether an API exposes byte sum-of-absolute-differences. */
template <class api_t>
concept Sad = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::sum_absolute_byte_differences(lhs, rhs); };

/** @brief Reports whether an API exposes immediate-controlled multi-SAD. */
template <class api_t, int immediate>
concept MultiSad = immediate >= 0 && immediate <= 255 && Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) {
	api_t::template multi_sum_absolute_byte_differences<immediate>(lhs, rhs);
};

/** @brief Reports whether an API exposes minimum-position lookup. */
template <class api_t>
concept MinPosition = Type<api_t> && requires(typename api_t::vector_t value) { api_t::min_position(value); };

/** @brief Reports whether an API exposes maximum-position lookup. */
template <class api_t>
concept MaxPosition = Type<api_t> && requires(typename api_t::vector_t value) { api_t::max_position(value); };

/** @brief Reports whether an API exposes saturating addition. */
template <class api_t>
concept AddSaturated = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::add_saturated(lhs, rhs); };

/** @brief Reports whether an API exposes saturating subtraction. */
template <class api_t>
concept SubtractSaturated = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::subtract_saturated(lhs, rhs); };

/** @brief Reports whether an API exposes saturating horizontal addition. */
template <class api_t>
concept HorizontalAddSaturated = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::hadd_saturated(lhs, rhs); };

/** @brief Reports whether an API exposes saturating horizontal subtraction. */
template <class api_t>
concept HorizontalSubtractSaturated =
	Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::hsubtract_saturated(lhs, rhs); };

/** @brief Reports whether an API exposes alternating add-subtract. */
template <class api_t>
concept AddSubtract = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::add_subtract(lhs, rhs); };

/** @brief Reports whether an API exposes an immediate-controlled dot product. */
template <class api_t, int immediate>
concept DotProduct = immediate >= 0 && immediate <= 255 && Type<api_t> &&
					 requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::template dot_product<immediate>(lhs, rhs); };

/** @brief Reports whether an API exposes bitwise intersection. */
template <class api_t>
concept BitwiseAnd = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::bitwise_and(lhs, rhs); };

/** @brief Reports whether an API exposes bitwise union. */
template <class api_t>
concept BitwiseOr = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::bitwise_or(lhs, rhs); };

/** @brief Reports whether an API exposes bitwise exclusive union. */
template <class api_t>
concept BitwiseXor = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::bitwise_xor(lhs, rhs); };

/** @brief Reports whether an API exposes bitwise complement. */
template <class api_t>
concept BitwiseNot = Type<api_t> && requires(typename api_t::vector_t value) { api_t::bitwise_not(value); };

/** @brief Reports whether an API exposes native predicate selection. */
template <class api_t>
concept Select = Type<api_t> && requires(typename api_t::vector_t condition, typename api_t::vector_t when_true, typename api_t::vector_t when_false) {
	api_t::select(condition, when_true, when_false);
};

/** @brief Reports whether an API exposes one sign bit per logical lane. */
template <class api_t>
concept MovemaskSlim = Type<api_t> && requires(typename api_t::vector_t value) { api_t::movemask_slim(value); };

/** @brief Reports whether an API exposes per-lane left shift. */
template <class api_t>
concept ShiftLeft = Type<api_t> && requires(typename api_t::vector_t value) { api_t::shift_left(value, 1); };

/** @brief Reports whether an API exposes per-lane logical right shift. */
template <class api_t>
concept ShiftRight = Type<api_t> && requires(typename api_t::vector_t value) { api_t::shift_right(value, 1); };

/** @brief Reports whether an API exposes per-lane arithmetic right shift. */
template <class api_t>
concept ArithmeticShiftRight = Type<api_t> && requires(typename api_t::vector_t value) { api_t::shift_right_arithmetic(value, 1); };

/** @brief Reports whether an API exposes explicit slow-path complete-register byte shifts. */
template <class api_t>
concept ShiftBytesSlow = Type<api_t> && requires(typename api_t::vector_t value) {
	api_t::shift_bytes_left_slow(value, 1);
	api_t::shift_bytes_right_slow(value, 1);
};

/** @brief Reports whether an API exposes an immediate complete-register byte left shift. */
template <class api_t, int count>
concept ShiftBytesLeft = count >= 0 && Type<api_t> && requires(typename api_t::int_vector_t value) { api_t::template shift_bytes_left<count>(value); };

/** @brief Reports whether an API exposes an immediate complete-register byte right shift. */
template <class api_t, int count>
concept ShiftBytesRight = count >= 0 && Type<api_t> && requires(typename api_t::int_vector_t value) { api_t::template shift_bytes_right<count>(value); };

/** @brief Reports whether an API exposes explicit slow-path complete-register bit shifts. */
template <class api_t>
concept ShiftBitsSlow = Type<api_t> && requires(typename api_t::vector_t value) {
	api_t::shift_bits_left_slow(value, 1);
	api_t::shift_bits_right_slow(value, 1);
};

/** @brief Reports whether an API exposes compile-time complete-register bit shifts. */
template <class api_t, int count>
concept ShiftBits = Type<api_t> && requires(typename api_t::int_vector_t value) {
	api_t::template shift_bits_left<count>(value);
	api_t::template shift_bits_right<count>(value);
};

/** @brief Reports whether an API exposes explicit slow-path runtime-selected lane extraction. */
template <class api_t, class selector_t = int>
concept ExtractSlow = Type<api_t> && requires(typename api_t::vector_t value, selector_t selector) { api_t::extract_slow(value, selector); };

/** @brief Reports whether an API exposes explicit slow-path runtime-selected lane insertion. */
template <class api_t>
concept InsertSlow = Type<api_t> && requires(typename api_t::vector_t value, typename api_t::element_type lane) { api_t::insert_slow(value, lane, 0); };
/** @brief Reports whether an API exposes extraction of a 256-bit register's lower 128-bit half. */
template <class api_t>
concept LowerHalf = Type<api_t> && requires(typename api_t::vector_t value) { api_t::lower_half(value); };

/** @brief Reports whether an API exposes low-lane unpacking. */
template <class api_t>
concept UnpackLow = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::unpack_lo(lhs, rhs); };

/** @brief Reports whether an API exposes high-lane unpacking. */
template <class api_t>
concept UnpackHigh = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::unpack_hi(lhs, rhs); };

/** @brief Reports whether an API accepts one compile-time logical shuffle selector sequence. */
template <class api_t, std::size_t... indices>
concept Shuffle = Type<api_t> && requires(typename api_t::vector_t value) { api_t::template shuffle<indices...>(value); };

/** @brief Reports whether an API exposes an immediate-controlled low-half shuffle. */
template <class api_t, int immediate>
concept ShuffleLow = Type<api_t> && requires(typename api_t::vector_t value) { api_t::template shuffle_lo<immediate>(value); };

/** @brief Reports whether an API exposes an immediate-controlled high-half shuffle. */
template <class api_t, int immediate>
concept ShuffleHigh = Type<api_t> && requires(typename api_t::vector_t value) { api_t::template shuffle_hi<immediate>(value); };

/** @brief Reports whether an API exposes an immediate-controlled two-register blend. */
template <class api_t, int immediate>
concept Blend = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::template blend<immediate>(lhs, rhs); };

/** @brief Reports whether an API exposes a native register-selector shuffle. */
template <class api_t>
concept RegisterShuffle = Type<api_t> && requires(typename api_t::vector_t value) { api_t::shuffle(value, value); };

/** @brief Reports whether an API exposes an explicit slow-path scalar-controlled shuffle. */
template <class api_t>
concept ShuffleSlow = Type<api_t> && requires(typename api_t::vector_t value) { api_t::shuffle_slow(value, value, 0); };

/** @brief Reports whether an API exposes an explicit slow-path low-half shuffle. */
template <class api_t>
concept ShuffleLowSlow = Type<api_t> && requires(typename api_t::vector_t value) { api_t::shuffle_lo_slow(value, 0); };

/** @brief Reports whether an API exposes an explicit slow-path high-half shuffle. */
template <class api_t>
concept ShuffleHighSlow = Type<api_t> && requires(typename api_t::vector_t value) { api_t::shuffle_hi_slow(value, 0); };

/** @brief Reports whether an API exposes a native register-mask blend. */
template <class api_t>
concept RegisterBlend =
	Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs, typename api_t::vector_t mask) { api_t::blend(lhs, rhs, mask); };

/** @brief Reports whether an API exposes an explicit slow-path scalar-controlled blend. */
template <class api_t>
concept BlendSlow = Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::blend_slow(lhs, rhs, 0); };

/** @brief Reports whether an API exposes explicit slow-path 32-bit immediate-mask shuffling. */
template <class api_t>
concept Shuffle32Slow = Type<api_t> && requires(typename api_t::int_vector_t value) { api_t::shuffle_32_slow(value, std::uint32_t{}); };
/** @brief Reports whether an API can reinterpret a complete register as the requested target element type. */
template <class api_t, class target_t>
concept BitCast = Type<api_t> && requires(typename api_t::vector_t value) { api_t::template bit_cast<target_t>(value); };

/** @brief Reports whether an API can numerically convert a complete register to the requested target element type. */
template <class api_t, class target_t>
concept Convert = Type<api_t> && requires(typename api_t::vector_t value) { api_t::template convert<target_t>(value); };

/** @brief Reports whether an API can widen its lowest source lanes into one complete target API register. */
template <class api_t, class target_api_t>
concept Widen = Type<api_t> && WidenTarget<target_api_t> && requires(typename api_t::vector_t value) { api_t::template widen<target_api_t>(value); };

} // namespace IApi

} // namespace SimdLib
