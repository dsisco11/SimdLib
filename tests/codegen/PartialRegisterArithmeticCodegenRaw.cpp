#include <SimdLib/PartialRegister.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>

#ifndef SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH
#error "SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH must select the fixture register width"
#endif

namespace
{
constexpr std::size_t i32_lanes = SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH / 32 - 1;
constexpr std::size_t i16_lanes = SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH / 16 - 1;
constexpr std::size_t u8_lanes = SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH / 8 - 1;
constexpr std::size_t f32_lanes = SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH / 32 - 1;

/** @brief Clears every physical lane beyond a compile-time logical prefix. */
template <class element_t, class api_t, std::size_t active_lane_count>
[[nodiscard]] typename api_t::vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
	project(typename api_t::vector_t native) noexcept
{
	constexpr auto filter = []() constexpr {
		std::array<element_t, api_t::element_count> result{};
		std::array<std::byte, sizeof(element_t)> one_bytes{};
		one_bytes.fill(std::byte{0xff});
		const auto one = std::bit_cast<element_t>(one_bytes);
		for (std::size_t lane = 0; lane < active_lane_count; ++lane)
			result[lane] = one;
		return result;
	}();
	return api_t::bitwise_and(native, api_t::construct(filter));
}

/** @brief Replaces every inactive divisor lane with one. */
template <class element_t, class api_t, std::size_t active_lane_count>
[[nodiscard]] typename api_t::vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
	neutralize_divisors(typename api_t::vector_t native) noexcept
{
	constexpr auto identity = []() constexpr {
		std::array<element_t, api_t::element_count> result{};
		for (std::size_t lane = active_lane_count; lane < api_t::element_count; ++lane)
			result[lane] = element_t{1};
		return result;
	}();
	return api_t::bitwise_or(native, api_t::construct(identity));
}

/** @brief Replaces inactive lanes with an extrema-search sentinel. */
template <class element_t, class api_t, std::size_t active_lane_count, bool minimum_search>
[[nodiscard]] typename api_t::vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
	apply_position_sentinel(typename api_t::vector_t native) noexcept
{
	constexpr auto identity = []() constexpr {
		std::array<element_t, api_t::element_count> result{};
		for (std::size_t lane = active_lane_count; lane < api_t::element_count; ++lane)
			result[lane] = minimum_search ? std::numeric_limits<element_t>::max() : std::numeric_limits<element_t>::lowest();
		return result;
	}();
	return api_t::bitwise_or(native, api_t::construct(identity));
}
} // namespace

using partial_i32 = SimdLib::PartialRegister<std::int32_t, SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH, i32_lanes>;
using partial_i16 = SimdLib::PartialRegister<std::int16_t, SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH, i16_lanes>;
using partial_u8 = SimdLib::PartialRegister<std::uint8_t, SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH, u8_lanes>;
using partial_f32 = SimdLib::PartialRegister<float, SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH, f32_lanes>;

/** @brief Raw API mirror for partial-register addition. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_add(partial_i32 lhs, partial_i32 rhs) noexcept
{
	return partial_i32::api_type::add(lhs.native, rhs.native);
}

/** @brief Raw API mirror for projected partial-register subtraction. */
extern "C" [[nodiscard]] partial_f32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_subtract(partial_f32 lhs, partial_f32 rhs) noexcept
{
	using api_t = partial_f32::api_type;
	return project<float, api_t, f32_lanes>(api_t::subtract(lhs.native, rhs.native));
}

/** @brief Raw API mirror for partial-register multiplication. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_multiply(partial_i32 lhs, partial_i32 rhs) noexcept
{
	return partial_i32::api_type::multiply(lhs.native, rhs.native);
}

/** @brief Raw API mirror for neutralized partial-register division. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_divide(partial_i32 lhs, partial_i32 rhs) noexcept
{
	using api_t = partial_i32::api_type;
	return project<std::int32_t, api_t, i32_lanes>(
		api_t::divide(lhs.native, neutralize_divisors<std::int32_t, api_t, i32_lanes>(rhs.native)));
}

/** @brief Raw API mirror for neutralized partial-register modulus. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_modulus(partial_i32 lhs, partial_i32 rhs) noexcept
{
	using api_t = partial_i32::api_type;
	return project<std::int32_t, api_t, i32_lanes>(
		api_t::modulus(lhs.native, neutralize_divisors<std::int32_t, api_t, i32_lanes>(rhs.native)));
}

/** @brief Raw API mirror for projected partial-register negation. */
extern "C" [[nodiscard]] partial_f32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_negate(partial_f32 value) noexcept
{
	using api_t = partial_f32::api_type;
	return project<float, api_t, f32_lanes>(api_t::negate(value.native));
}

/** @brief Raw API mirror for zero-closed partial-register minimum. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_min(partial_i32 lhs, partial_i32 rhs) noexcept
{
	return partial_i32::api_type::min(lhs.native, rhs.native);
}

/** @brief Raw API mirror for zero-closed partial-register maximum. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_max(partial_i32 lhs, partial_i32 rhs) noexcept
{
	return partial_i32::api_type::max(lhs.native, rhs.native);
}

/** @brief Raw API mirror for partial-register absolute value. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_absolute(partial_i32 value) noexcept
{
	return partial_i32::api_type::absolute(value.native);
}

/** @brief Raw API mirror for partial-register square root. */
extern "C" [[nodiscard]] partial_f32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_sqrt(partial_f32 value) noexcept
{
	return partial_f32::api_type::sqrt(value.native);
}

/** @brief Raw API mirror for partial-register average. */
extern "C" [[nodiscard]] partial_u8::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_average(partial_u8 lhs, partial_u8 rhs) noexcept
{
	return partial_u8::api_type::avg(lhs.native, rhs.native);
}

/** @brief Raw API mirror for partial-register multiply-add. */
extern "C" [[nodiscard]] partial_f32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_multiply_add(partial_f32 lhs, partial_f32 rhs, partial_f32 addend) noexcept
{
	return partial_f32::api_type::multiply_add(lhs.native, rhs.native, addend.native);
}

/** @brief Raw API mirror for projected grouped magnitude. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_magnitude(partial_i32 value) noexcept
{
	using api_t = partial_i32::api_type;
	return project<std::int32_t, api_t, i32_lanes>(api_t::magnitude(value.native));
}

/** @brief Raw API mirror for checked grouped magnitude and result projection. */
extern "C" [[nodiscard]] partial_i16::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_magnitude_checked(partial_i16 value) noexcept
{
	using source_api_t = partial_i16::api_type;
	using result_t = SimdLib::partial_magnitude_checked_result_t<std::int16_t, SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH, i16_lanes>;
	if constexpr (result_t::lane_count == result_t::api_type::element_count)
		return source_api_t::magnitude_checked(value.native);
	else
		return project<std::int16_t, typename result_t::api_type, result_t::lane_count>(
			source_api_t::magnitude_checked(value.native));
}

/** @brief Raw API mirror for projected grouped normalization. */
extern "C" [[nodiscard]] partial_f32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_normalize(partial_f32 value) noexcept
{
	using api_t = partial_f32::api_type;
	return project<float, api_t, f32_lanes>(api_t::normalize(value.native));
}

/** @brief Raw API mirror for projected horizontal addition. */
extern "C" [[nodiscard]] partial_i16::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_horizontal_add(partial_i16 lhs, partial_i16 rhs) noexcept
{
	using api_t = partial_i16::api_type;
	return project<std::int16_t, api_t, i16_lanes>(api_t::add_horizontal(lhs.native, rhs.native));
}

/** @brief Raw API mirror for projected horizontal subtraction. */
extern "C" [[nodiscard]] partial_i16::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_horizontal_subtract(partial_i16 lhs, partial_i16 rhs) noexcept
{
	using api_t = partial_i16::api_type;
	return project<std::int16_t, api_t, i16_lanes>(api_t::subtract_horizontal(lhs.native, rhs.native));
}

/** @brief Raw API mirror for adjacent multiply-add. */
extern "C" [[nodiscard]] partial_i16::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_multiply_add_adjacent(partial_i16 lhs, partial_i16 rhs) noexcept
{
	return partial_i16::api_type::multiply_add_adjacent(lhs.native, rhs.native);
}

/** @brief Raw API mirror for unsigned/signed byte multiply-add. */
extern "C" [[nodiscard]] partial_u8::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_byte_multiply_add(partial_u8 lhs, partial_u8 rhs) noexcept
{
	return partial_u8::api_type::multiply_add_unsigned_signed_bytes(lhs.native, rhs.native);
}

/** @brief Raw API mirror for byte sum-of-absolute-differences. */
extern "C" [[nodiscard]] partial_u8::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_sad(partial_u8 lhs, partial_u8 rhs) noexcept
{
	return partial_u8::api_type::sum_absolute_byte_differences(lhs.native, rhs.native);
}

/** @brief Raw API mirror for immediate-controlled multi-SAD. */
extern "C" [[nodiscard]] partial_u8::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_multi_sad(partial_u8 lhs, partial_u8 rhs) noexcept
{
	return partial_u8::api_type::template multi_sum_absolute_byte_differences<0x35>(lhs.native, rhs.native);
}

/** @brief Raw API mirror for minimum-position reduction with inactive-lane exclusion. */
extern "C" [[nodiscard]] std::size_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_min_position(partial_i16 value) noexcept
{
	using api_t = partial_i16::api_type;
	return api_t::min_position(apply_position_sentinel<std::int16_t, api_t, i16_lanes, true>(value.native));
}

/** @brief Raw API mirror for maximum-position reduction with inactive-lane exclusion. */
extern "C" [[nodiscard]] std::size_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_max_position(partial_i16 value) noexcept
{
	using api_t = partial_i16::api_type;
	return api_t::max_position(apply_position_sentinel<std::int16_t, api_t, i16_lanes, false>(value.native));
}

/** @brief Raw API mirror for saturating partial-register addition. */
extern "C" [[nodiscard]] partial_u8::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_add_saturated(partial_u8 lhs, partial_u8 rhs) noexcept
{
	return partial_u8::api_type::add_saturated(lhs.native, rhs.native);
}

/** @brief Raw API mirror for saturating partial-register subtraction. */
extern "C" [[nodiscard]] partial_u8::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_subtract_saturated(partial_u8 lhs, partial_u8 rhs) noexcept
{
	return partial_u8::api_type::subtract_saturated(lhs.native, rhs.native);
}

/** @brief Raw API mirror for projected saturated horizontal addition. */
extern "C" [[nodiscard]] partial_i16::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_horizontal_add_saturated(partial_i16 lhs, partial_i16 rhs) noexcept
{
	using api_t = partial_i16::api_type;
	return project<std::int16_t, api_t, i16_lanes>(api_t::hadd_saturated(lhs.native, rhs.native));
}

/** @brief Raw API mirror for projected saturated horizontal subtraction. */
extern "C" [[nodiscard]] partial_i16::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_horizontal_subtract_saturated(partial_i16 lhs, partial_i16 rhs) noexcept
{
	using api_t = partial_i16::api_type;
	return project<std::int16_t, api_t, i16_lanes>(api_t::hsubtract_saturated(lhs.native, rhs.native));
}

/** @brief Raw API mirror for projected alternating subtraction and addition. */
extern "C" [[nodiscard]] partial_f32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_add_subtract(partial_f32 lhs, partial_f32 rhs) noexcept
{
	using api_t = partial_f32::api_type;
	return project<float, api_t, f32_lanes>(api_t::add_subtract(lhs.native, rhs.native));
}

/** @brief Raw API mirror for immediate-controlled dot product without projection. */
extern "C" [[nodiscard]] partial_f32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_dot_product(partial_f32 lhs, partial_f32 rhs) noexcept
{
	return partial_f32::api_type::template dot_product<0x11>(lhs.native, rhs.native);
}

#if SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH == 256
using partial_i64 = SimdLib::PartialRegister<std::int64_t, 256, 3>;

/** @brief Raw API mirror for the sparse complete-register adjacent result. */
extern "C" [[nodiscard]] partial_i64::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_sparse_adjacent(partial_i64 lhs, partial_i64 rhs) noexcept
{
	return partial_i64::api_type::multiply_add_adjacent(lhs.native, rhs.native);
}
#endif
