#include <SimdLib/Api.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#ifndef SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH
#error "SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH must select the fixture register width"
#endif

constexpr std::size_t active_lanes = SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH == 256 ? 5 : 3;
using api_u32 = SimdLib::Api<SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH, std::uint32_t>;
using native_u32 = typename api_u32::vector_t;
using api_f32 = SimdLib::Api<SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH, float>;
using native_f32 = typename api_f32::vector_t;
#if SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH == 128
using api_u16 = SimdLib::Api<128, std::uint16_t>;
using native_u16 = typename api_u16::vector_t;
#endif

/** @brief Compile-time active-lane filter shared by every raw normalization. */
alignas(SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH / 8) constexpr auto active_lane_filter = [] {
	std::array<std::uint32_t, api_u32::element_count> result{};
	for (std::size_t lane = 0; lane < active_lanes; ++lane)
		result[lane] = ~std::uint32_t{};
	return result;
}();

/** @brief Applies the required inactive-suffix projection to a raw result. */
[[nodiscard]] constexpr native_u32 normalize(native_u32 value) noexcept
{
	return api_u32::bitwise_and(value, api_u32::construct(active_lane_filter));
}

/** @brief Raw Api mirror for active-extent load, broadcast, addition, and store. */
extern "C" void simdlib_partial_general_codegen_transfer(const std::uint32_t *source, std::uint32_t *destination) noexcept
{
	const auto value = api_u32::template load_partial<active_lanes>(std::span<const std::uint32_t, active_lanes>{source, active_lanes});
	const auto one = api_u32::template broadcast_partial<active_lanes>(1U);
	api_u32::template store_partial<active_lanes>(api_u32::add(value, one), std::span<std::uint32_t, active_lanes>{destination, active_lanes});
}

/** @brief Raw Api mirror for unary bitwise work and per-lane shifting. */
extern "C" [[nodiscard]] native_u32 simdlib_partial_general_codegen_unary_shift(native_u32 value, int count) noexcept
{
	return api_u32::shift_left(normalize(api_u32::bitwise_not(value)), count);
}

/** @brief Raw Api mirror for projected logical-payload shifting. */
extern "C" [[nodiscard]] native_u32 simdlib_partial_general_codegen_payload_shift(native_u32 value) noexcept
{
	return normalize(api_u32::template shift_bytes_left<1>(value));
}

/** @brief Raw Api mirror for the opposite projected payload shift. */
extern "C" [[nodiscard]] native_u32 simdlib_partial_general_codegen_payload_shift_right(native_u32 value) noexcept
{
	return api_u32::template shift_bytes_right<1>(value);
}

/** @brief Raw Api mirror for projected active-prefix interleaving. */
extern "C" [[nodiscard]] native_u32 simdlib_partial_general_codegen_rearrange(native_u32 lhs, native_u32 rhs) noexcept
{
	return normalize(api_u32::unpack_lo(lhs, rhs));
}

/** @brief Raw Api mirror for immediate-lane blending. */
extern "C" [[nodiscard]] native_u32 simdlib_partial_general_codegen_blend(native_u32 lhs, native_u32 rhs) noexcept
{
	return api_u32::template blend<0b0101>(lhs, rhs);
}

/** @brief Raw Api mirror for a same-width type-changing result. */
extern "C" [[nodiscard]] native_u32 simdlib_partial_general_codegen_convert(native_u32 value) noexcept
{
	return value;
}

/** @brief Raw Api mirror for active-count-preserving numeric conversion. */
extern "C" [[nodiscard]] native_f32 simdlib_partial_general_codegen_numeric_convert(native_u32 value) noexcept
{
	return api_u32::template convert<float>(value);
}

#if SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH == 128
/** @brief Raw Api mirror for low-prefix integral widening. */
extern "C" [[nodiscard]] native_u32 simdlib_partial_general_codegen_widen_low(native_u16 value) noexcept
{
	return api_u16::template widen<api_u32>(value);
}
#endif

/** @brief Raw Api mirror for native import normalization and export. */
extern "C" [[nodiscard]] native_u32 simdlib_partial_general_codegen_native_transfer(native_u32 value) noexcept
{
	return normalize(value);
}

/** @brief Raw Api mirror for comparison, predicate selection, and composition. */
extern "C" [[nodiscard]] native_u32 simdlib_partial_general_codegen_composed(native_u32 lhs, native_u32 rhs) noexcept
{
	const auto predicate = normalize(api_u32::compare_greater(lhs, rhs));
	return api_u32::add(api_u32::select(predicate, lhs, rhs), rhs);
}
