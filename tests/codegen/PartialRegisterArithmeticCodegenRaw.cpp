#include <SimdLib/Api.h>

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

using partial_i32 = SimdLib::Api<SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH, std::int32_t>;
using partial_i16 = SimdLib::Api<SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH, std::int16_t>;
using partial_u8 = SimdLib::Api<SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH, std::uint8_t>;
using partial_f32 = SimdLib::Api<SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH, float>;

/** @brief Raw API mirror for partial-register addition. */
extern "C" [[nodiscard]] typename partial_i32::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_add(typename partial_i32::vector_t lhs, typename partial_i32::vector_t rhs) noexcept
{
	return partial_i32::add(rhs, lhs);
}

/** @brief Raw API mirror for projected partial-register subtraction. */
extern "C" [[nodiscard]] typename partial_f32::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_subtract(typename partial_f32::vector_t lhs, typename partial_f32::vector_t rhs) noexcept
{
	using api_t = partial_f32;
	return project<float, api_t, f32_lanes>(api_t::subtract(lhs, rhs));
}

/** @brief Raw API mirror for partial-register multiplication. */
extern "C" [[nodiscard]] typename partial_i32::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_multiply(typename partial_i32::vector_t lhs, typename partial_i32::vector_t rhs) noexcept
{
	return partial_i32::multiply(rhs, lhs);
}

/** @brief Raw API mirror for neutralized partial-register division. */
extern "C" [[nodiscard]] typename partial_i32::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_divide(typename partial_i32::vector_t lhs, typename partial_i32::vector_t rhs) noexcept
{
	using api_t = partial_i32;
	return project<std::int32_t, api_t, i32_lanes>(
		api_t::divide(lhs, neutralize_divisors<std::int32_t, api_t, i32_lanes>(rhs)));
}

/** @brief Raw API mirror for neutralized partial-register modulus. */
extern "C" [[nodiscard]] typename partial_i32::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_modulus(typename partial_i32::vector_t lhs, typename partial_i32::vector_t rhs) noexcept
{
	using api_t = partial_i32;
	return project<std::int32_t, api_t, i32_lanes>(
		api_t::modulus(lhs, neutralize_divisors<std::int32_t, api_t, i32_lanes>(rhs)));
}

/** @brief Raw API mirror for projected partial-register negation. */
extern "C" [[nodiscard]] typename partial_f32::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_negate(typename partial_f32::vector_t value) noexcept
{
	using api_t = partial_f32;
	return project<float, api_t, f32_lanes>(api_t::negate(value));
}

/** @brief Raw API mirror for zero-closed partial-register minimum. */
extern "C" [[nodiscard]] typename partial_i32::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_min(typename partial_i32::vector_t lhs, typename partial_i32::vector_t rhs) noexcept
{
	return partial_i32::min(lhs, rhs);
}

/** @brief Raw API mirror for zero-closed partial-register maximum. */
extern "C" [[nodiscard]] typename partial_i32::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_max(typename partial_i32::vector_t lhs, typename partial_i32::vector_t rhs) noexcept
{
	return partial_i32::max(lhs, rhs);
}

/** @brief Raw API mirror for partial-register absolute value. */
extern "C" [[nodiscard]] typename partial_i32::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_absolute(typename partial_i32::vector_t value) noexcept
{
	return partial_i32::absolute(value);
}

/** @brief Raw API mirror for partial-register square root. */
extern "C" [[nodiscard]] typename partial_f32::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_sqrt(typename partial_f32::vector_t value) noexcept
{
	return partial_f32::sqrt(value);
}

/** @brief Raw API mirror for partial-register average. */
extern "C" [[nodiscard]] typename partial_u8::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_average(typename partial_u8::vector_t lhs, typename partial_u8::vector_t rhs) noexcept
{
	return partial_u8::avg(lhs, rhs);
}

/** @brief Raw API mirror for partial-register multiply-add. */
extern "C" [[nodiscard]] typename partial_f32::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_multiply_add(typename partial_f32::vector_t lhs, typename partial_f32::vector_t rhs, typename partial_f32::vector_t addend) noexcept
{
	return partial_f32::multiply_add(rhs, lhs, addend);
}

/** @brief Raw API mirror for projected grouped magnitude. */
extern "C" [[nodiscard]] typename partial_i32::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_magnitude(typename partial_i32::vector_t value) noexcept
{
	using api_t = partial_i32;
	return project<std::int32_t, api_t, i32_lanes>(api_t::magnitude(value));
}

/** @brief Raw API mirror for checked grouped magnitude and result projection. */
extern "C" [[nodiscard]] typename partial_i16::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_magnitude_checked(typename partial_i16::vector_t value) noexcept
{
	using source_api_t = partial_i16;
	constexpr std::size_t group_lane_count = 128 / (sizeof(std::int16_t) * 8);
	constexpr std::size_t result_lane_count = ((i16_lanes - 1) / group_lane_count) * group_lane_count + 2;
	if constexpr (result_lane_count == source_api_t::element_count)
		return source_api_t::magnitude_checked(value);
	else
		return project<std::int16_t, source_api_t, result_lane_count>(source_api_t::magnitude_checked(value));
}

/** @brief Raw API mirror for projected grouped normalization. */
extern "C" [[nodiscard]] typename partial_f32::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_normalize(typename partial_f32::vector_t value) noexcept
{
	using api_t = partial_f32;
	return project<float, api_t, f32_lanes>(api_t::normalize(value));
}

/** @brief Raw API mirror for projected horizontal addition. */
extern "C" [[nodiscard]] typename partial_i16::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_horizontal_add(typename partial_i16::vector_t lhs, typename partial_i16::vector_t rhs) noexcept
{
	using api_t = partial_i16;
	return project<std::int16_t, api_t, i16_lanes>(api_t::add_horizontal(lhs, rhs));
}

/** @brief Raw API mirror for projected horizontal subtraction. */
extern "C" [[nodiscard]] typename partial_i16::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_horizontal_subtract(typename partial_i16::vector_t lhs, typename partial_i16::vector_t rhs) noexcept
{
	using api_t = partial_i16;
	return project<std::int16_t, api_t, i16_lanes>(api_t::subtract_horizontal(lhs, rhs));
}

/** @brief Raw API mirror for adjacent multiply-add. */
extern "C" [[nodiscard]] typename partial_i16::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_multiply_add_adjacent(typename partial_i16::vector_t lhs, typename partial_i16::vector_t rhs) noexcept
{
	return partial_i16::multiply_add_adjacent(lhs, rhs);
}

/** @brief Raw API mirror for unsigned/signed byte multiply-add. */
extern "C" [[nodiscard]] typename partial_u8::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_byte_multiply_add(typename partial_u8::vector_t lhs, typename partial_u8::vector_t rhs) noexcept
{
	return partial_u8::multiply_add_unsigned_signed_bytes(lhs, rhs);
}

/** @brief Raw API mirror for byte sum-of-absolute-differences. */
extern "C" [[nodiscard]] typename partial_u8::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_sad(typename partial_u8::vector_t lhs, typename partial_u8::vector_t rhs) noexcept
{
	return partial_u8::sum_absolute_byte_differences(lhs, rhs);
}

/** @brief Raw API mirror for immediate-controlled multi-SAD. */
extern "C" [[nodiscard]] typename partial_u8::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_multi_sad(typename partial_u8::vector_t lhs, typename partial_u8::vector_t rhs) noexcept
{
	return partial_u8::template multi_sum_absolute_byte_differences<0x35>(lhs, rhs);
}

/** @brief Raw API mirror for minimum-position reduction with inactive-lane exclusion. */
extern "C" [[nodiscard]] std::size_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_min_position(typename partial_i16::vector_t value) noexcept
{
	using api_t = partial_i16;
	return api_t::min_position(apply_position_sentinel<std::int16_t, api_t, i16_lanes, true>(value));
}

/** @brief Raw API mirror for maximum-position reduction with inactive-lane exclusion. */
extern "C" [[nodiscard]] std::size_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_max_position(typename partial_i16::vector_t value) noexcept
{
	using api_t = partial_i16;
	return api_t::max_position(apply_position_sentinel<std::int16_t, api_t, i16_lanes, false>(value));
}

/** @brief Raw API mirror for saturating partial-register addition. */
extern "C" [[nodiscard]] typename partial_u8::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_add_saturated(typename partial_u8::vector_t lhs, typename partial_u8::vector_t rhs) noexcept
{
	return partial_u8::add_saturated(lhs, rhs);
}

/** @brief Raw API mirror for saturating partial-register subtraction. */
extern "C" [[nodiscard]] typename partial_u8::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_subtract_saturated(typename partial_u8::vector_t lhs, typename partial_u8::vector_t rhs) noexcept
{
	return partial_u8::subtract_saturated(lhs, rhs);
}

/** @brief Raw API mirror for projected saturated horizontal addition. */
extern "C" [[nodiscard]] typename partial_i16::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_horizontal_add_saturated(typename partial_i16::vector_t lhs, typename partial_i16::vector_t rhs) noexcept
{
	using api_t = partial_i16;
	return project<std::int16_t, api_t, i16_lanes>(api_t::hadd_saturated(lhs, rhs));
}

/** @brief Raw API mirror for projected saturated horizontal subtraction. */
extern "C" [[nodiscard]] typename partial_i16::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_horizontal_subtract_saturated(typename partial_i16::vector_t lhs, typename partial_i16::vector_t rhs) noexcept
{
	using api_t = partial_i16;
	return project<std::int16_t, api_t, i16_lanes>(api_t::hsubtract_saturated(lhs, rhs));
}

/** @brief Raw API mirror for projected alternating subtraction and addition. */
extern "C" [[nodiscard]] typename partial_f32::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_add_subtract(typename partial_f32::vector_t lhs, typename partial_f32::vector_t rhs) noexcept
{
	using api_t = partial_f32;
	return project<float, api_t, f32_lanes>(api_t::add_subtract(lhs, rhs));
}

/** @brief Raw API mirror for immediate-controlled dot product without projection. */
extern "C" [[nodiscard]] typename partial_f32::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_dot_product(typename partial_f32::vector_t lhs, typename partial_f32::vector_t rhs) noexcept
{
	return partial_f32::template dot_product<0x11>(lhs, rhs);
}

#if SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH == 256
using partial_i64 = SimdLib::Api<256, std::int64_t>;

/** @brief Raw API mirror for the sparse complete-register adjacent result. */
extern "C" [[nodiscard]] typename partial_i64::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_sparse_adjacent(typename partial_i64::vector_t lhs, typename partial_i64::vector_t rhs) noexcept
{
	return partial_i64::multiply_add_adjacent(rhs, lhs);
}
#endif
