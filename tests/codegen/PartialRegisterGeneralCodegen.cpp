#include <SimdLib/PartialRegister.h>

#include <cstddef>
#include <cstdint>
#include <span>

#ifndef SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH
#error "SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH must select the fixture register width"
#endif

constexpr std::size_t active_lanes = SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH == 256 ? 5 : 3;
using partial_u32 = SimdLib::PartialRegister<std::uint32_t, SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH, active_lanes>;
using partial_f32 = SimdLib::PartialRegister<float, SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH, active_lanes>;
#if SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH == 128
using partial_u16 = SimdLib::PartialRegister<std::uint16_t, 128, active_lanes>;
#endif

/** @brief Loads, offsets, and stores exactly the active logical extent. */
extern "C" void simdlib_partial_general_codegen_transfer(const std::uint32_t *source, std::uint32_t *destination) noexcept
{
	const auto value = partial_u32::load(std::span<const std::uint32_t, active_lanes>{source, active_lanes});
	(value + partial_u32::broadcast(1U)).store(std::span<std::uint32_t, active_lanes>{destination, active_lanes});
}

/** @brief Covers unary bitwise work and per-lane shifting. */
extern "C" [[nodiscard]] partial_u32::native_type simdlib_partial_general_codegen_unary_shift(partial_u32 value, int count) noexcept
{
	return ((~value) << count).native;
}

/** @brief Covers logical payload shifting with required suffix projection. */
extern "C" [[nodiscard]] partial_u32::native_type simdlib_partial_general_codegen_payload_shift(partial_u32 value) noexcept
{
	return value.template shift_bytes_left<1>().native;
}

/** @brief Covers the opposite logical payload-shift direction. */
extern "C" [[nodiscard]] partial_u32::native_type simdlib_partial_general_codegen_payload_shift_right(partial_u32 value) noexcept
{
	return value.template shift_bytes_right<1>().native;
}

/** @brief Covers active-prefix interleaving and required result projection. */
extern "C" [[nodiscard]] partial_u32::native_type simdlib_partial_general_codegen_rearrange(partial_u32 lhs, partial_u32 rhs) noexcept
{
	return lhs.unpack_low(rhs).native;
}

/** @brief Covers immediate-lane blending as a second rearrangement shape. */
extern "C" [[nodiscard]] partial_u32::native_type simdlib_partial_general_codegen_blend(partial_u32 lhs, partial_u32 rhs) noexcept
{
	return lhs.template blend<0b0101>(rhs).native;
}

/** @brief Covers a same-width type-changing result. */
extern "C" [[nodiscard]] partial_u32::native_type simdlib_partial_general_codegen_convert(partial_u32 value) noexcept
{
	return value.template bit_cast<std::int32_t>().native;
}

/** @brief Covers numeric conversion while preserving the active logical count. */
extern "C" [[nodiscard]] partial_f32::native_type simdlib_partial_general_codegen_numeric_convert(partial_u32 value) noexcept
{
	return value.template convert<float>().native;
}

#if SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH == 128
/** @brief Covers low-prefix widening into a wider integral lane type. */
extern "C" [[nodiscard]] partial_u32::native_type simdlib_partial_general_codegen_widen_low(partial_u16 value) noexcept
{
	return value.template widen_low<std::uint32_t, 128>().native;
}
#endif

/** @brief Covers native import normalization and complete-register export. */
extern "C" [[nodiscard]] partial_u32::native_type simdlib_partial_general_codegen_native_transfer(partial_u32::native_type value) noexcept
{
	return partial_u32::from_native(value).to_register().native;
}

/** @brief Covers comparison, predicate use, and a composed expression. */
extern "C" [[nodiscard]] partial_u32::native_type simdlib_partial_general_codegen_composed(partial_u32 lhs, partial_u32 rhs) noexcept
{
	return (lhs.compare_greater(rhs).select(lhs, rhs) + rhs).native;
}
