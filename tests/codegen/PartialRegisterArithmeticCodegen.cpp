#include <SimdLib/PartialRegister.h>

#include <cstddef>
#include <cstdint>

#ifndef SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH
#error "SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH must select the fixture register width"
#endif

namespace
{
constexpr std::size_t i32_lanes = SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH / 32 - 1;
constexpr std::size_t i16_lanes = SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH / 16 - 1;
constexpr std::size_t u8_lanes = SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH / 8 - 1;
constexpr std::size_t f32_lanes = SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH / 32 - 1;
} // namespace

using partial_i32 = SimdLib::PartialRegister<std::int32_t, SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH, i32_lanes>;
using partial_i16 = SimdLib::PartialRegister<std::int16_t, SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH, i16_lanes>;
using partial_u8 = SimdLib::PartialRegister<std::uint8_t, SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH, u8_lanes>;
using partial_f32 = SimdLib::PartialRegister<float, SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH, f32_lanes>;

/** @brief Emits partial-register addition. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_add(partial_i32 lhs, partial_i32 rhs) noexcept
{
	return (lhs + rhs).native;
}

/** @brief Emits partial-register subtraction. */
extern "C" [[nodiscard]] partial_f32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_subtract(partial_f32 lhs, partial_f32 rhs) noexcept
{
	return (lhs - rhs).native;
}

/** @brief Emits partial-register multiplication. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_multiply(partial_i32 lhs, partial_i32 rhs) noexcept
{
	return (lhs * rhs).native;
}

/** @brief Emits neutralized partial-register division. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_divide(partial_i32 lhs, partial_i32 rhs) noexcept
{
	return (lhs / rhs).native;
}

/** @brief Emits neutralized partial-register modulus. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_modulus(partial_i32 lhs, partial_i32 rhs) noexcept
{
	return (lhs % rhs).native;
}

/** @brief Emits projected partial-register negation. */
extern "C" [[nodiscard]] partial_f32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_negate(partial_f32 value) noexcept
{
	return (-value).native;
}

/** @brief Emits zero-closed partial-register minimum. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_min(partial_i32 lhs, partial_i32 rhs) noexcept
{
	return lhs.min(rhs).native;
}

/** @brief Emits zero-closed partial-register maximum. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_max(partial_i32 lhs, partial_i32 rhs) noexcept
{
	return lhs.max(rhs).native;
}

/** @brief Emits partial-register absolute value. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_absolute(partial_i32 value) noexcept
{
	return value.absolute().native;
}

/** @brief Emits partial-register square root. */
extern "C" [[nodiscard]] partial_f32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_sqrt(partial_f32 value) noexcept
{
	return value.sqrt().native;
}

/** @brief Emits partial-register average. */
extern "C" [[nodiscard]] partial_u8::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_average(partial_u8 lhs, partial_u8 rhs) noexcept
{
	return lhs.average(rhs).native;
}

/** @brief Emits partial-register multiply-add. */
extern "C" [[nodiscard]] partial_f32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_multiply_add(partial_f32 lhs, partial_f32 rhs, partial_f32 addend) noexcept
{
	return lhs.multiply_add(rhs, addend).native;
}

/** @brief Emits projected grouped magnitude. */
extern "C" [[nodiscard]] partial_i32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_magnitude(partial_i32 value) noexcept
{
	return value.magnitude().native;
}

/** @brief Emits checked grouped magnitude and its deliberate result mapping. */
extern "C" [[nodiscard]] partial_i16::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_magnitude_checked(partial_i16 value) noexcept
{
	return value.magnitude_checked().native;
}

/** @brief Emits projected grouped normalization. */
extern "C" [[nodiscard]] partial_f32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_normalize(partial_f32 value) noexcept
{
	return value.normalize().native;
}

/** @brief Emits projected horizontal addition. */
extern "C" [[nodiscard]] partial_i16::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_horizontal_add(partial_i16 lhs, partial_i16 rhs) noexcept
{
	return lhs.horizontal_add(rhs).native;
}

/** @brief Emits projected horizontal subtraction. */
extern "C" [[nodiscard]] partial_i16::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_horizontal_subtract(partial_i16 lhs, partial_i16 rhs) noexcept
{
	return lhs.horizontal_subtract(rhs).native;
}

/** @brief Emits adjacent multiply-add with its deliberate result mapping. */
extern "C" [[nodiscard]] partial_i16::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_multiply_add_adjacent(partial_i16 lhs, partial_i16 rhs) noexcept
{
	return lhs.multiply_add_adjacent(rhs).native;
}

/** @brief Emits unsigned/signed byte multiply-add with its deliberate result mapping. */
extern "C" [[nodiscard]] partial_u8::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_byte_multiply_add(partial_u8 lhs, partial_u8 rhs) noexcept
{
	return lhs.multiply_add_unsigned_signed_bytes(rhs).native;
}

/** @brief Emits byte sum-of-absolute-differences with its deliberate result mapping. */
extern "C" [[nodiscard]] partial_u8::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_sad(partial_u8 lhs, partial_u8 rhs) noexcept
{
	return lhs.sum_absolute_byte_differences(rhs).native;
}

/** @brief Emits immediate-controlled multi-SAD. */
extern "C" [[nodiscard]] partial_u8::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_multi_sad(partial_u8 lhs, partial_u8 rhs) noexcept
{
	return lhs.template multi_sum_absolute_byte_differences<0x35>(rhs).native;
}

/** @brief Emits minimum-position reduction with inactive-lane exclusion. */
extern "C" [[nodiscard]] std::size_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_min_position(partial_i16 value) noexcept
{
	return value.min_position();
}

/** @brief Emits maximum-position reduction with inactive-lane exclusion. */
extern "C" [[nodiscard]] std::size_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_max_position(partial_i16 value) noexcept
{
	return value.max_position();
}

/** @brief Emits saturating partial-register addition. */
extern "C" [[nodiscard]] partial_u8::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_add_saturated(partial_u8 lhs, partial_u8 rhs) noexcept
{
	return lhs.add_saturated(rhs).native;
}

/** @brief Emits saturating partial-register subtraction. */
extern "C" [[nodiscard]] partial_u8::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_subtract_saturated(partial_u8 lhs, partial_u8 rhs) noexcept
{
	return lhs.subtract_saturated(rhs).native;
}

/** @brief Emits projected saturated horizontal addition. */
extern "C" [[nodiscard]] partial_i16::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_horizontal_add_saturated(partial_i16 lhs, partial_i16 rhs) noexcept
{
	return lhs.horizontal_add_saturated(rhs).native;
}

/** @brief Emits projected saturated horizontal subtraction. */
extern "C" [[nodiscard]] partial_i16::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_horizontal_subtract_saturated(partial_i16 lhs, partial_i16 rhs) noexcept
{
	return lhs.horizontal_subtract_saturated(rhs).native;
}

/** @brief Emits projected alternating subtraction and addition. */
extern "C" [[nodiscard]] partial_f32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_add_subtract(partial_f32 lhs, partial_f32 rhs) noexcept
{
	return lhs.add_subtract(rhs).native;
}

/** @brief Emits immediate-controlled dot product without redundant projection. */
extern "C" [[nodiscard]] partial_f32::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_dot_product(partial_f32 lhs, partial_f32 rhs) noexcept
{
	return lhs.template dot_product<0x11>(rhs).native;
}

#if SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH == 256
using partial_i64 = SimdLib::PartialRegister<std::int64_t, 256, 3>;

/** @brief Emits the sparse complete-register adjacent result for a three-lane 64-bit source. */
extern "C" [[nodiscard]] partial_i64::native_type SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_arithmetic_codegen_sparse_adjacent(partial_i64 lhs, partial_i64 rhs) noexcept
{
	return lhs.multiply_add_adjacent(rhs).native;
}
#endif
