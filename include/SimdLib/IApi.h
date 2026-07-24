#pragma once

#include <SimdLib/Config.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

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
concept HorizontalSubtract =
	Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::subtract_horizontal(lhs, rhs); };

/** @brief Reports whether an API exposes adjacent multiply-add. */
template <class api_t>
concept MultiplyAddAdjacent =
	Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::multiply_add_adjacent(lhs, rhs); };

/** @brief Reports whether an API exposes unsigned-byte by signed-byte multiply-add. */
template <class api_t>
concept ByteMultiplyAdd =
	Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::multiply_add_unsigned_signed_bytes(lhs, rhs); };

/** @brief Reports whether an API exposes byte sum-of-absolute-differences. */
template <class api_t>
concept Sad =
	Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::sum_absolute_byte_differences(lhs, rhs); };

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
concept SubtractSaturated =
	Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::subtract_saturated(lhs, rhs); };

/** @brief Reports whether an API exposes saturating horizontal addition. */
template <class api_t>
concept HorizontalAddSaturated =
	Type<api_t> && requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs) { api_t::hadd_saturated(lhs, rhs); };

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

/** @brief Reports whether an API exposes per-lane left shift. */
template <class api_t>
concept ShiftLeft = Type<api_t> && requires(typename api_t::vector_t value) { api_t::shift_left(value, 1); };

/** @brief Reports whether an API exposes per-lane logical right shift. */
template <class api_t>
concept ShiftRight = Type<api_t> && requires(typename api_t::vector_t value) { api_t::shift_right(value, 1); };

/** @brief Reports whether an API exposes per-lane arithmetic right shift. */
template <class api_t>
concept ArithmeticShiftRight = Type<api_t> && requires(typename api_t::vector_t value) { api_t::shift_right_arithmetic(value, 1); };

/** @brief Reports whether an API exposes complete-register byte shifts. */
template <class api_t>
concept ByteShift = Type<api_t> && requires(typename api_t::vector_t value) {
	api_t::byte_shift_left(value, 1);
	api_t::byte_shift_right(value, 1);
};

/** @brief Reports whether an API exposes complete-register bit shifts. */
template <class api_t>
concept BitShift = Type<api_t> && requires(typename api_t::vector_t value) {
	api_t::bit_shift_left(value, 1);
	api_t::bit_shift_right(value, 1);
};

} // namespace IApi

} // namespace SimdLib
