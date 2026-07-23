#pragma once
#include <SimdLib/Config.h>
#include <SimdLib/TemplateTools.h>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#if SIMDLIB_TARGET_X86
#include <immintrin.h>
#endif
#if SIMDLIB_COMPILER_MSVC && SIMDLIB_TARGET_X86
#include <intrin.h>
#endif
#include <limits>
#include <type_traits>

namespace SimdLib::Detail
{
// This file contains SIMD extensions for 128-bit and 256-bit integer and floating-point types.
// SEE: http://www.alfredklomp.com/programming/sse-intrinsics/

/** Portable lane access for compiler-native x86 register types.
 * MSVC exposes intrinsic registers as unions with named arrays, while Clang
 * models them as
 * vector types. Keep that compiler difference inside Detail.
 */
template <class Element, class Vector>
	requires std::is_arithmetic_v<Element> && (sizeof(Vector) % sizeof(Element) == 0)
SIMDLIB_FORCE_INLINE constexpr Element register_get(const Vector value, const std::size_t index) noexcept
{
#if SIMDLIB_COMPILER_MSVC
	if constexpr (sizeof(Vector) == 16)
	{
		if constexpr (std::is_integral_v<Element> && sizeof(Element) == 1 && std::is_unsigned_v<Element>)
			return value.m128i_u8[index];
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 1)
			return value.m128i_i8[index];
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 2 && std::is_unsigned_v<Element>)
			return value.m128i_u16[index];
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 2)
			return value.m128i_i16[index];
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 4 && std::is_unsigned_v<Element>)
			return value.m128i_u32[index];
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 4)
			return value.m128i_i32[index];
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 8 && std::is_unsigned_v<Element>)
			return value.m128i_u64[index];
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 8)
			return value.m128i_i64[index];
		else if constexpr (std::same_as<Element, float>)
			return value.m128_f32[index];
		else
			return value.m128d_f64[index];
	}
	else
	{
		if constexpr (std::is_integral_v<Element> && sizeof(Element) == 1 && std::is_unsigned_v<Element>)
			return value.m256i_u8[index];
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 1)
			return value.m256i_i8[index];
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 2 && std::is_unsigned_v<Element>)
			return value.m256i_u16[index];
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 2)
			return value.m256i_i16[index];
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 4 && std::is_unsigned_v<Element>)
			return value.m256i_u32[index];
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 4)
			return value.m256i_i32[index];
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 8 && std::is_unsigned_v<Element>)
			return value.m256i_u64[index];
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 8)
			return value.m256i_i64[index];
		else if constexpr (std::same_as<Element, float>)
			return value.m256_f32[index];
		else
			return value.m256d_f64[index];
	}
#else
	return std::bit_cast<std::array<Element, sizeof(Vector) / sizeof(Element)>>(value)[index];
#endif
}

template <class Element, class Vector>
	requires std::is_arithmetic_v<Element> && (sizeof(Vector) % sizeof(Element) == 0)
SIMDLIB_FORCE_INLINE constexpr void register_set(Vector &value, const std::size_t index, const Element lane) noexcept
{
#if SIMDLIB_COMPILER_MSVC
	if constexpr (sizeof(Vector) == 16)
	{
		if constexpr (std::is_integral_v<Element> && sizeof(Element) == 1 && std::is_unsigned_v<Element>)
			value.m128i_u8[index] = lane;
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 1)
			value.m128i_i8[index] = lane;
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 2 && std::is_unsigned_v<Element>)
			value.m128i_u16[index] = lane;
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 2)
			value.m128i_i16[index] = lane;
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 4 && std::is_unsigned_v<Element>)
			value.m128i_u32[index] = lane;
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 4)
			value.m128i_i32[index] = lane;
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 8 && std::is_unsigned_v<Element>)
			value.m128i_u64[index] = lane;
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 8)
			value.m128i_i64[index] = lane;
		else if constexpr (std::same_as<Element, float>)
			value.m128_f32[index] = lane;
		else
			value.m128d_f64[index] = lane;
	}
	else
	{
		if constexpr (std::is_integral_v<Element> && sizeof(Element) == 1 && std::is_unsigned_v<Element>)
			value.m256i_u8[index] = lane;
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 1)
			value.m256i_i8[index] = lane;
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 2 && std::is_unsigned_v<Element>)
			value.m256i_u16[index] = lane;
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 2)
			value.m256i_i16[index] = lane;
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 4 && std::is_unsigned_v<Element>)
			value.m256i_u32[index] = lane;
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 4)
			value.m256i_i32[index] = lane;
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 8 && std::is_unsigned_v<Element>)
			value.m256i_u64[index] = lane;
		else if constexpr (std::is_integral_v<Element> && sizeof(Element) == 8)
			value.m256i_i64[index] = lane;
		else if constexpr (std::same_as<Element, float>)
			value.m256_f32[index] = lane;
		else
			value.m256d_f64[index] = lane;
	}
#else
	auto lanes = std::bit_cast<std::array<Element, sizeof(Vector) / sizeof(Element)>>(value);
	lanes[index] = lane;
	value = std::bit_cast<Vector>(lanes);
#endif
}

template <class Vector, class Element, std::size_t Count>
	requires(sizeof(Vector) == sizeof(Element) * Count)
SIMDLIB_FORCE_INLINE constexpr Vector register_from_array(const std::array<Element, Count> &lanes) noexcept
{
	Vector result{};
	for (std::size_t index = 0; index < Count; ++index)
	{
		register_set<Element>(result, index, lanes[index]);
	}
	return result;
}

template <class Vector, class Element, class... Args>
	requires(sizeof(Vector) == sizeof(Element) * sizeof...(Args)) && (std::convertible_to<Args, Element> && ...)
SIMDLIB_FORCE_INLINE constexpr Vector register_from_values(Args &&...values) noexcept
{
	return register_from_array<Vector>(std::array<Element, sizeof...(Args)>{static_cast<Element>(values)...});
}

/** @brief Constructs a constant-evaluated native register with every lane set to one value.
 *  @tparam Vector Native register representation.
 *  @tparam Element Scalar lane type.
 *  @param value Value copied into every lane.
 *  @return Native register containing the repeated value.
 */
template <class Vector, class Element>
	requires(sizeof(Vector) % sizeof(Element) == 0)
SIMDLIB_FORCE_INLINE constexpr Vector register_from_repeated_value(const Element value) noexcept
{
	std::array<Element, sizeof(Vector) / sizeof(Element)> lanes{};
	lanes.fill(value);
	return register_from_array<Vector>(lanes);
}

template <class Element, class Vector> SIMDLIB_FORCE_INLINE constexpr auto register_to_array(const Vector value) noexcept
{
	std::array<Element, sizeof(Vector) / sizeof(Element)> result{};
	for (std::size_t index = 0; index < result.size(); ++index)
	{
		result[index] = register_get<Element>(value, index);
	}
	return result;
}

template <class Element, class Vector> SIMDLIB_FORCE_INLINE Element *register_data(Vector &value) noexcept
{
	return reinterpret_cast<Element *>(&value);
}

template <class Element, class Vector> SIMDLIB_FORCE_INLINE const Element *register_data(const Vector &value) noexcept
{
	return reinterpret_cast<const Element *>(&value);
}

template <class Element, class Vector, class Value>
SIMDLIB_FORCE_INLINE constexpr Vector register_insert(Vector value, const Value lane, const std::size_t index) noexcept
{
	register_set<Element>(value, index, static_cast<Element>(lane));
	return value;
}

template <class Element, class Vector> SIMDLIB_FORCE_INLINE constexpr Vector register_blend(Vector lhs, const Vector rhs, const unsigned int mask) noexcept
{
	constexpr std::size_t count = sizeof(Vector) / sizeof(Element);
	for (std::size_t index = 0; index < count; ++index)
	{
		if ((mask & (1u << (index % 8))) != 0)
			register_set<Element>(lhs, index, register_get<Element>(rhs, index));
	}
	return lhs;
}

template <class Vector> SIMDLIB_FORCE_INLINE constexpr Vector register_blend_bytes(Vector lhs, const Vector rhs, const Vector mask) noexcept
{
	constexpr std::size_t count = sizeof(Vector);
	for (std::size_t index = 0; index < count; ++index)
	{
		if ((register_get<std::uint8_t>(mask, index) & 0x80u) != 0)
			register_set<std::uint8_t>(lhs, index, register_get<std::uint8_t>(rhs, index));
	}
	return lhs;
}

template <class Vector> SIMDLIB_FORCE_INLINE constexpr Vector register_insert_float(Vector lhs, const Vector rhs, const unsigned int control) noexcept
{
	auto lanes = register_to_array<float>(lhs);
	const auto source = register_to_array<float>(rhs);
	lanes[(control >> 4) & 0x3u] = source[(control >> 6) & 0x3u];
	for (std::size_t index = 0; index < lanes.size(); ++index)
	{
		if ((control & (1u << index)) != 0)
			lanes[index] = 0.0f;
	}
	return register_from_array<Vector>(lanes);
}

template <class Vector> SIMDLIB_FORCE_INLINE constexpr Vector register_shuffle_float(const Vector lhs, const Vector rhs, const unsigned int control) noexcept
{
	const auto left = register_to_array<float>(lhs);
	const auto right = register_to_array<float>(rhs);
	std::array<float, sizeof(Vector) / sizeof(float)> result{};
	for (std::size_t lane = 0; lane < result.size(); lane += 4)
	{
		result[lane] = left[lane + (control & 0x3u)];
		result[lane + 1] = left[lane + ((control >> 2) & 0x3u)];
		result[lane + 2] = right[lane + ((control >> 4) & 0x3u)];
		result[lane + 3] = right[lane + ((control >> 6) & 0x3u)];
	}
	return register_from_array<Vector>(result);
}

template <class Vector> SIMDLIB_FORCE_INLINE constexpr Vector register_shuffle_double(const Vector lhs, const Vector rhs, const unsigned int control) noexcept
{
	const auto left = register_to_array<double>(lhs);
	const auto right = register_to_array<double>(rhs);
	std::array<double, sizeof(Vector) / sizeof(double)> result{};
	for (std::size_t lane = 0; lane < result.size(); lane += 2)
	{
		const unsigned int lane_control = control >> lane;
		result[lane] = left[lane + (lane_control & 0x1u)];
		result[lane + 1] = right[lane + ((lane_control >> 1) & 0x1u)];
	}
	return register_from_array<Vector>(result);
}

template <class Vector> SIMDLIB_FORCE_INLINE constexpr Vector register_shuffle_32(const Vector value, const unsigned int control) noexcept
{
	const auto source = register_to_array<std::uint32_t>(value);
	std::array<std::uint32_t, sizeof(Vector) / sizeof(std::uint32_t)> result{};
	for (std::size_t lane = 0; lane < result.size(); lane += 4)
	{
		for (std::size_t index = 0; index < 4; ++index)
			result[lane + index] = source[lane + ((control >> (index * 2)) & 0x3u)];
	}
	return register_from_array<Vector>(result);
}

template <class Vector>
SIMDLIB_FORCE_INLINE constexpr Vector register_shuffle_half_16(const Vector value, const unsigned int control, const bool high_half) noexcept
{
	const auto source = register_to_array<std::uint16_t>(value);
	auto result = source;
	for (std::size_t lane = 0; lane < result.size(); lane += 8)
	{
		const std::size_t base = lane + (high_half ? 4 : 0);
		for (std::size_t index = 0; index < 4; ++index)
			result[base + index] = source[base + ((control >> (index * 2)) & 0x3u)];
	}
	return register_from_array<Vector>(result);
}

template <class Vector> SIMDLIB_FORCE_INLINE constexpr Vector register_byte_shift_left(const Vector value, const int count) noexcept
{
	if (count <= 0)
		return value;
	constexpr std::size_t size = sizeof(Vector);
	if (static_cast<std::size_t>(count) >= size)
		return register_from_array<Vector>(std::array<std::uint8_t, size>{});
	const auto source = register_to_array<std::uint8_t>(value);
	std::array<std::uint8_t, size> result{};
	for (std::size_t index = static_cast<std::size_t>(count); index < size; ++index)
		result[index] = source[index - static_cast<std::size_t>(count)];
	return register_from_array<Vector>(result);
}

template <class Vector> SIMDLIB_FORCE_INLINE constexpr Vector register_byte_shift_right(const Vector value, const int count) noexcept
{
	if (count <= 0)
		return value;
	constexpr std::size_t size = sizeof(Vector);
	if (static_cast<std::size_t>(count) >= size)
		return register_from_array<Vector>(std::array<std::uint8_t, size>{});
	const auto source = register_to_array<std::uint8_t>(value);
	std::array<std::uint8_t, size> result{};
	for (std::size_t index = 0; index + static_cast<std::size_t>(count) < size; ++index)
		result[index] = source[index + static_cast<std::size_t>(count)];
	return register_from_array<Vector>(result);
}

template <class Element, class Vector, class Operation>
SIMDLIB_FORCE_INLINE constexpr Vector register_transform_binary(const Vector lhs, const Vector rhs, Operation &&operation) noexcept
{
	constexpr std::size_t count = sizeof(Vector) / sizeof(Element);
	std::array<Element, count> result{};
	for (std::size_t index = 0; index < count; ++index)
		result[index] = static_cast<Element>(operation(register_get<Element>(lhs, index), register_get<Element>(rhs, index)));
	return register_from_array<Vector>(result);
}

#if SIMDLIB_HAS_SSE42

#pragma region 128bit Integer Division Extensions

/**
 * @brief Divides 16 signed 8-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero and no dividend-minimum lane is divided by negative one.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m128i VECTORCALL _ext128_div_epi8(__m128i lhs, __m128i rhs) noexcept
{
	__m128i result = _mm_setzero_si128();
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 0)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 0)))), 0);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 1)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 1)))), 1);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 2)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 2)))), 2);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 3)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 3)))), 3);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 4)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 4)))), 4);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 5)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 5)))), 5);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 6)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 6)))), 6);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 7)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 7)))), 7);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 8)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 8)))), 8);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 9)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 9)))), 9);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 10)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 10)))),
		10);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 11)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 11)))),
		11);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 12)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 12)))),
		12);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 13)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 13)))),
		13);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 14)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 14)))),
		14);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm_extract_epi8(lhs, 15)) / static_cast<std::int8_t>(_mm_extract_epi8(rhs, 15)))),
		15);
	return result;
}

/**
 * @brief Divides 16 unsigned 8-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m128i VECTORCALL _ext128_div_epu8(__m128i lhs, __m128i rhs) noexcept
{
	__m128i result = _mm_setzero_si128();
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 0)) / static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 0)))),
		0);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 1)) / static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 1)))),
		1);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 2)) / static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 2)))),
		2);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 3)) / static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 3)))),
		3);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 4)) / static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 4)))),
		4);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 5)) / static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 5)))),
		5);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 6)) / static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 6)))),
		6);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 7)) / static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 7)))),
		7);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 8)) / static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 8)))),
		8);
	result = _mm_insert_epi8(
		result,
		static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 9)) / static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 9)))),
		9);
	result = _mm_insert_epi8(result,
							 static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 10)) /
																		static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 10)))),
							 10);
	result = _mm_insert_epi8(result,
							 static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 11)) /
																		static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 11)))),
							 11);
	result = _mm_insert_epi8(result,
							 static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 12)) /
																		static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 12)))),
							 12);
	result = _mm_insert_epi8(result,
							 static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 13)) /
																		static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 13)))),
							 13);
	result = _mm_insert_epi8(result,
							 static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 14)) /
																		static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 14)))),
							 14);
	result = _mm_insert_epi8(result,
							 static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm_extract_epi8(lhs, 15)) /
																		static_cast<std::uint8_t>(_mm_extract_epi8(rhs, 15)))),
							 15);
	return result;
}

/**
 * @brief Divides 8 signed 16-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero and no dividend-minimum lane is divided by negative one.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m128i VECTORCALL _ext128_div_epi16(__m128i lhs, __m128i rhs) noexcept
{
	__m128i result = _mm_setzero_si128();
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm_extract_epi16(lhs, 0)) /
																		 static_cast<std::int16_t>(_mm_extract_epi16(rhs, 0)))),
							  0);
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm_extract_epi16(lhs, 1)) /
																		 static_cast<std::int16_t>(_mm_extract_epi16(rhs, 1)))),
							  1);
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm_extract_epi16(lhs, 2)) /
																		 static_cast<std::int16_t>(_mm_extract_epi16(rhs, 2)))),
							  2);
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm_extract_epi16(lhs, 3)) /
																		 static_cast<std::int16_t>(_mm_extract_epi16(rhs, 3)))),
							  3);
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm_extract_epi16(lhs, 4)) /
																		 static_cast<std::int16_t>(_mm_extract_epi16(rhs, 4)))),
							  4);
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm_extract_epi16(lhs, 5)) /
																		 static_cast<std::int16_t>(_mm_extract_epi16(rhs, 5)))),
							  5);
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm_extract_epi16(lhs, 6)) /
																		 static_cast<std::int16_t>(_mm_extract_epi16(rhs, 6)))),
							  6);
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm_extract_epi16(lhs, 7)) /
																		 static_cast<std::int16_t>(_mm_extract_epi16(rhs, 7)))),
							  7);
	return result;
}

/**
 * @brief Divides 8 unsigned 16-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m128i VECTORCALL _ext128_div_epu16(__m128i lhs, __m128i rhs) noexcept
{
	__m128i result = _mm_setzero_si128();
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm_extract_epi16(lhs, 0)) /
																		  static_cast<std::uint16_t>(_mm_extract_epi16(rhs, 0)))),
							  0);
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm_extract_epi16(lhs, 1)) /
																		  static_cast<std::uint16_t>(_mm_extract_epi16(rhs, 1)))),
							  1);
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm_extract_epi16(lhs, 2)) /
																		  static_cast<std::uint16_t>(_mm_extract_epi16(rhs, 2)))),
							  2);
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm_extract_epi16(lhs, 3)) /
																		  static_cast<std::uint16_t>(_mm_extract_epi16(rhs, 3)))),
							  3);
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm_extract_epi16(lhs, 4)) /
																		  static_cast<std::uint16_t>(_mm_extract_epi16(rhs, 4)))),
							  4);
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm_extract_epi16(lhs, 5)) /
																		  static_cast<std::uint16_t>(_mm_extract_epi16(rhs, 5)))),
							  5);
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm_extract_epi16(lhs, 6)) /
																		  static_cast<std::uint16_t>(_mm_extract_epi16(rhs, 6)))),
							  6);
	result = _mm_insert_epi16(result,
							  static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm_extract_epi16(lhs, 7)) /
																		  static_cast<std::uint16_t>(_mm_extract_epi16(rhs, 7)))),
							  7);
	return result;
}

/**
 * @brief Divides 4 signed 32-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero and no dividend-minimum lane is divided by negative one.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m128i VECTORCALL _ext128_div_epi32(__m128i lhs, __m128i rhs) noexcept
{
	__m128i result = _mm_setzero_si128();
	result = _mm_insert_epi32(result, static_cast<std::int32_t>(_mm_extract_epi32(lhs, 0)) / static_cast<std::int32_t>(_mm_extract_epi32(rhs, 0)), 0);
	result = _mm_insert_epi32(result, static_cast<std::int32_t>(_mm_extract_epi32(lhs, 1)) / static_cast<std::int32_t>(_mm_extract_epi32(rhs, 1)), 1);
	result = _mm_insert_epi32(result, static_cast<std::int32_t>(_mm_extract_epi32(lhs, 2)) / static_cast<std::int32_t>(_mm_extract_epi32(rhs, 2)), 2);
	result = _mm_insert_epi32(result, static_cast<std::int32_t>(_mm_extract_epi32(lhs, 3)) / static_cast<std::int32_t>(_mm_extract_epi32(rhs, 3)), 3);
	return result;
}

/**
 * @brief Divides 4 unsigned 32-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m128i VECTORCALL _ext128_div_epu32(__m128i lhs, __m128i rhs) noexcept
{
	__m128i result = _mm_setzero_si128();
	result = _mm_insert_epi32(
		result, std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(_mm_extract_epi32(lhs, 0)) / static_cast<std::uint32_t>(_mm_extract_epi32(rhs, 0))), 0);
	result = _mm_insert_epi32(
		result, std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(_mm_extract_epi32(lhs, 1)) / static_cast<std::uint32_t>(_mm_extract_epi32(rhs, 1))), 1);
	result = _mm_insert_epi32(
		result, std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(_mm_extract_epi32(lhs, 2)) / static_cast<std::uint32_t>(_mm_extract_epi32(rhs, 2))), 2);
	result = _mm_insert_epi32(
		result, std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(_mm_extract_epi32(lhs, 3)) / static_cast<std::uint32_t>(_mm_extract_epi32(rhs, 3))), 3);
	return result;
}

/**
 * @brief Divides 2 signed 64-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero and no dividend-minimum lane is divided by negative one.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m128i VECTORCALL _ext128_div_epi64(__m128i lhs, __m128i rhs) noexcept
{
	__m128i result = _mm_setzero_si128();
	result = _mm_insert_epi64(result, static_cast<std::int64_t>(_mm_extract_epi64(lhs, 0)) / static_cast<std::int64_t>(_mm_extract_epi64(rhs, 0)), 0);
	result = _mm_insert_epi64(result, static_cast<std::int64_t>(_mm_extract_epi64(lhs, 1)) / static_cast<std::int64_t>(_mm_extract_epi64(rhs, 1)), 1);
	return result;
}

/**
 * @brief Divides 2 unsigned 64-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m128i VECTORCALL _ext128_div_epu64(__m128i lhs, __m128i rhs) noexcept
{
	__m128i result = _mm_setzero_si128();
	result = _mm_insert_epi64(
		result, std::bit_cast<std::int64_t>(static_cast<std::uint64_t>(_mm_extract_epi64(lhs, 0)) / static_cast<std::uint64_t>(_mm_extract_epi64(rhs, 0))), 0);
	result = _mm_insert_epi64(
		result, std::bit_cast<std::int64_t>(static_cast<std::uint64_t>(_mm_extract_epi64(lhs, 1)) / static_cast<std::uint64_t>(_mm_extract_epi64(rhs, 1))), 1);
	return result;
}

#pragma endregion

#pragma region 128bit int8_t Extensions

/**
 * @brief Multiplies corresponding 8-bit lanes modulo 256. [eg: 0x81 * 0x02 => 0x02]
 *
 * @param lhs The first byte-lane register.
 * @param rhs The second byte-lane register.
 * @return The low byte of each lane product.
 */
SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_mul_epi8(__m128i lhs, __m128i rhs) noexcept
{
	// unpack and multiply
	const __m128i dst_even = _mm_mullo_epi16(lhs, rhs);
	const __m128i dst_odd = _mm_slli_epi16(_mm_mullo_epi16(_mm_srli_epi16(lhs, 8), _mm_srli_epi16(rhs, 8)), 8);
	// repack
	const __m128i mask = _mm_set1_epi32(0x00FF00FF); // mask for even positions
	return _mm_blendv_epi8(dst_odd, dst_even, mask);
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_slli_epx8(__m128i lhs, const int count) noexcept
{
	const __m128i mask = _mm_set1_epi8(0xFF << count);
	return _mm_and_si128(_mm_slli_epi16(lhs, count), mask);
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_srli_epx8(__m128i lhs, const int count) noexcept
{
	const __m128i mask = _mm_set1_epi8(0xFF >> count);
	return _mm_and_si128(_mm_srli_epi16(lhs, count), mask);
}

/**
 * @brief Arithmetic-right-shifts each signed 8-bit lane. [eg: 0x82 >> 1 => 0xC1]
 *
 * @param lhs The signed byte-lane register.
 * @param count The per-lane shift count.
 * @return The arithmetic-right-shifted byte lanes.
 */
SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_srai_epx8(__m128i lhs, const int count) noexcept
{
	__m128i aeven = _mm_slli_epi16(lhs, 8);						 // even numbered elements get sign bit in position
	aeven = _mm_sra_epi16(aeven, _mm_cvtsi32_si128(count + 8));	 // shift arithmetic, back to position
	__m128i aodd = _mm_sra_epi16(lhs, _mm_cvtsi32_si128(count)); // shift odd numbered elements arithmetic
	__m128i mask = _mm_set1_epi32(0x00FF00FF);					 // mask for even positions
	__m128i res = _mm_blendv_epi8(aodd, aeven, mask);			 // interleave even and odd
	return res;
}

#pragma endregion

#pragma region 128bit uint8_t Extensions

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_mul_epu8(__m128i lhs, __m128i rhs) noexcept
{
	return _ext_mul_epi8(lhs, rhs);
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_cmpgt_epu8(__m128i lhs, __m128i rhs) noexcept
{
	// Returns 0xFF where x > y:
	return _mm_andnot_si128(_mm_cmpeq_epi8(lhs, rhs), _mm_cmpeq_epi8(_mm_max_epu8(lhs, rhs), lhs));
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_cmplt_epu8(__m128i lhs, __m128i rhs) noexcept
{
	// Returns 0xFF where x < y:
	return _ext_cmpgt_epu8(rhs, lhs);
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_set1_epu8(const std::uint8_t value) noexcept
{
	return _mm_set1_epi8(std::bit_cast<char>(value));
}

#pragma endregion

#pragma region 128bit uint16_t Extensions

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_cmple_epu16(__m128i x, __m128i y) noexcept
{
	// Returns 0xFFFF where x <= y:
	return _mm_cmpeq_epi16(_mm_subs_epu16(x, y), _mm_setzero_si128());
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_cmpgt_epu16(__m128i x, __m128i y) noexcept
{
	// Returns 0xFFFF where x > y:
	return _mm_andnot_si128(_mm_cmpeq_epi16(x, y), _ext_cmple_epu16(y, x));
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_cmplt_epu16(__m128i x, __m128i y) noexcept
{
	// Returns 0xFFFF where x < y:
	return _ext_cmpgt_epu16(y, x);
}

// Return x where x <= y, else y.
SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_min_epu16(__m128i x, __m128i y) noexcept
{
	return _mm_sub_epi16(x, _mm_subs_epu16(x, y));
}

// Return x where x >= y, else y.
SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_max_epu16(__m128i x, __m128i y) noexcept
{
	return _mm_add_epi16(x, _mm_subs_epu16(y, x));
}
#pragma endregion

#pragma region 128bit int32_t Extensions

#pragma endregion

#pragma region 128bit uint32_t Extensions

SIMDLIB_FORCE_INLINE __m128 VECTORCALL _ext_cvtepu32_ps(__m128i lhs) noexcept
{
	const __m128 signedFloats = _mm_cvtepi32_ps(lhs);
	const __m128i highBitMask = _mm_cmpgt_epi32(_mm_setzero_si128(), lhs);
	const __m128 correction = _mm_and_ps(_mm_castsi128_ps(highBitMask), _mm_set1_ps(4294967296.0f));
	return _mm_add_ps(signedFloats, correction);
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_cmpgt_epu32(__m128i lhs, __m128i rhs) noexcept
{
	// Returns 0xFFFFFFFF where x > y:
	return _mm_andnot_si128(_mm_cmpeq_epi32(lhs, rhs), _mm_cmpeq_epi32(_mm_max_epu32(lhs, rhs), lhs));
}

#pragma endregion

#endif // SIMDLIB_HAS_SSE42

#if SIMDLIB_HAS_AVX2 && SIMDLIB_HAS_SSE42

#pragma region 256bit Integer Division Extensions

/**
 * @brief Divides 32 signed 8-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero and no dividend-minimum lane is divided by negative one.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m256i VECTORCALL _ext256_div_epi8(__m256i lhs, __m256i rhs) noexcept
{
	__m256i result = _mm256_setzero_si256();
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 0)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 0)))),
								0);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 1)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 1)))),
								1);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 2)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 2)))),
								2);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 3)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 3)))),
								3);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 4)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 4)))),
								4);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 5)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 5)))),
								5);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 6)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 6)))),
								6);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 7)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 7)))),
								7);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 8)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 8)))),
								8);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 9)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 9)))),
								9);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 10)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 10)))),
								10);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 11)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 11)))),
								11);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 12)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 12)))),
								12);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 13)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 13)))),
								13);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 14)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 14)))),
								14);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 15)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 15)))),
								15);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 16)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 16)))),
								16);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 17)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 17)))),
								17);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 18)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 18)))),
								18);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 19)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 19)))),
								19);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 20)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 20)))),
								20);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 21)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 21)))),
								21);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 22)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 22)))),
								22);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 23)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 23)))),
								23);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 24)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 24)))),
								24);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 25)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 25)))),
								25);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 26)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 26)))),
								26);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 27)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 27)))),
								27);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 28)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 28)))),
								28);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 29)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 29)))),
								29);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 30)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 30)))),
								30);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::int8_t>(static_cast<std::int8_t>(_mm256_extract_epi8(lhs, 31)) /
																		  static_cast<std::int8_t>(_mm256_extract_epi8(rhs, 31)))),
								31);
	return result;
}

/**
 * @brief Divides 32 unsigned 8-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m256i VECTORCALL _ext256_div_epu8(__m256i lhs, __m256i rhs) noexcept
{
	__m256i result = _mm256_setzero_si256();
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 0)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 0)))),
								0);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 1)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 1)))),
								1);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 2)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 2)))),
								2);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 3)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 3)))),
								3);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 4)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 4)))),
								4);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 5)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 5)))),
								5);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 6)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 6)))),
								6);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 7)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 7)))),
								7);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 8)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 8)))),
								8);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 9)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 9)))),
								9);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 10)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 10)))),
								10);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 11)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 11)))),
								11);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 12)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 12)))),
								12);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 13)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 13)))),
								13);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 14)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 14)))),
								14);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 15)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 15)))),
								15);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 16)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 16)))),
								16);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 17)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 17)))),
								17);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 18)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 18)))),
								18);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 19)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 19)))),
								19);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 20)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 20)))),
								20);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 21)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 21)))),
								21);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 22)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 22)))),
								22);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 23)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 23)))),
								23);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 24)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 24)))),
								24);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 25)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 25)))),
								25);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 26)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 26)))),
								26);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 27)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 27)))),
								27);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 28)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 28)))),
								28);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 29)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 29)))),
								29);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 30)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 30)))),
								30);
	result = _mm256_insert_epi8(result,
								static_cast<int>(static_cast<std::uint8_t>(static_cast<std::uint8_t>(_mm256_extract_epi8(lhs, 31)) /
																		   static_cast<std::uint8_t>(_mm256_extract_epi8(rhs, 31)))),
								31);
	return result;
}

/**
 * @brief Divides 16 signed 16-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero and no dividend-minimum lane is divided by negative one.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m256i VECTORCALL _ext256_div_epi16(__m256i lhs, __m256i rhs) noexcept
{
	__m256i result = _mm256_setzero_si256();
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 0)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 0)))),
								 0);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 1)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 1)))),
								 1);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 2)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 2)))),
								 2);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 3)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 3)))),
								 3);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 4)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 4)))),
								 4);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 5)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 5)))),
								 5);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 6)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 6)))),
								 6);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 7)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 7)))),
								 7);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 8)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 8)))),
								 8);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 9)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 9)))),
								 9);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 10)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 10)))),
								 10);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 11)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 11)))),
								 11);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 12)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 12)))),
								 12);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 13)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 13)))),
								 13);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 14)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 14)))),
								 14);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::int16_t>(static_cast<std::int16_t>(_mm256_extract_epi16(lhs, 15)) /
																			static_cast<std::int16_t>(_mm256_extract_epi16(rhs, 15)))),
								 15);
	return result;
}

/**
 * @brief Divides 16 unsigned 16-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m256i VECTORCALL _ext256_div_epu16(__m256i lhs, __m256i rhs) noexcept
{
	__m256i result = _mm256_setzero_si256();
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 0)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 0)))),
								 0);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 1)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 1)))),
								 1);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 2)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 2)))),
								 2);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 3)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 3)))),
								 3);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 4)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 4)))),
								 4);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 5)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 5)))),
								 5);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 6)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 6)))),
								 6);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 7)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 7)))),
								 7);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 8)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 8)))),
								 8);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 9)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 9)))),
								 9);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 10)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 10)))),
								 10);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 11)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 11)))),
								 11);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 12)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 12)))),
								 12);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 13)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 13)))),
								 13);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 14)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 14)))),
								 14);
	result = _mm256_insert_epi16(result,
								 static_cast<int>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(_mm256_extract_epi16(lhs, 15)) /
																			 static_cast<std::uint16_t>(_mm256_extract_epi16(rhs, 15)))),
								 15);
	return result;
}

/**
 * @brief Divides 8 signed 32-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero and no dividend-minimum lane is divided by negative one.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m256i VECTORCALL _ext256_div_epi32(__m256i lhs, __m256i rhs) noexcept
{
	__m256i result = _mm256_setzero_si256();
	result = _mm256_insert_epi32(result, static_cast<std::int32_t>(_mm256_extract_epi32(lhs, 0)) / static_cast<std::int32_t>(_mm256_extract_epi32(rhs, 0)), 0);
	result = _mm256_insert_epi32(result, static_cast<std::int32_t>(_mm256_extract_epi32(lhs, 1)) / static_cast<std::int32_t>(_mm256_extract_epi32(rhs, 1)), 1);
	result = _mm256_insert_epi32(result, static_cast<std::int32_t>(_mm256_extract_epi32(lhs, 2)) / static_cast<std::int32_t>(_mm256_extract_epi32(rhs, 2)), 2);
	result = _mm256_insert_epi32(result, static_cast<std::int32_t>(_mm256_extract_epi32(lhs, 3)) / static_cast<std::int32_t>(_mm256_extract_epi32(rhs, 3)), 3);
	result = _mm256_insert_epi32(result, static_cast<std::int32_t>(_mm256_extract_epi32(lhs, 4)) / static_cast<std::int32_t>(_mm256_extract_epi32(rhs, 4)), 4);
	result = _mm256_insert_epi32(result, static_cast<std::int32_t>(_mm256_extract_epi32(lhs, 5)) / static_cast<std::int32_t>(_mm256_extract_epi32(rhs, 5)), 5);
	result = _mm256_insert_epi32(result, static_cast<std::int32_t>(_mm256_extract_epi32(lhs, 6)) / static_cast<std::int32_t>(_mm256_extract_epi32(rhs, 6)), 6);
	result = _mm256_insert_epi32(result, static_cast<std::int32_t>(_mm256_extract_epi32(lhs, 7)) / static_cast<std::int32_t>(_mm256_extract_epi32(rhs, 7)), 7);
	return result;
}

/**
 * @brief Divides 8 unsigned 32-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m256i VECTORCALL _ext256_div_epu32(__m256i lhs, __m256i rhs) noexcept
{
	__m256i result = _mm256_setzero_si256();
	result = _mm256_insert_epi32(
		result,
		std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(_mm256_extract_epi32(lhs, 0)) / static_cast<std::uint32_t>(_mm256_extract_epi32(rhs, 0))), 0);
	result = _mm256_insert_epi32(
		result,
		std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(_mm256_extract_epi32(lhs, 1)) / static_cast<std::uint32_t>(_mm256_extract_epi32(rhs, 1))), 1);
	result = _mm256_insert_epi32(
		result,
		std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(_mm256_extract_epi32(lhs, 2)) / static_cast<std::uint32_t>(_mm256_extract_epi32(rhs, 2))), 2);
	result = _mm256_insert_epi32(
		result,
		std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(_mm256_extract_epi32(lhs, 3)) / static_cast<std::uint32_t>(_mm256_extract_epi32(rhs, 3))), 3);
	result = _mm256_insert_epi32(
		result,
		std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(_mm256_extract_epi32(lhs, 4)) / static_cast<std::uint32_t>(_mm256_extract_epi32(rhs, 4))), 4);
	result = _mm256_insert_epi32(
		result,
		std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(_mm256_extract_epi32(lhs, 5)) / static_cast<std::uint32_t>(_mm256_extract_epi32(rhs, 5))), 5);
	result = _mm256_insert_epi32(
		result,
		std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(_mm256_extract_epi32(lhs, 6)) / static_cast<std::uint32_t>(_mm256_extract_epi32(rhs, 6))), 6);
	result = _mm256_insert_epi32(
		result,
		std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(_mm256_extract_epi32(lhs, 7)) / static_cast<std::uint32_t>(_mm256_extract_epi32(rhs, 7))), 7);
	return result;
}

/**
 * @brief Divides 4 signed 64-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero and no dividend-minimum lane is divided by negative one.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m256i VECTORCALL _ext256_div_epi64(__m256i lhs, __m256i rhs) noexcept
{
	__m256i result = _mm256_setzero_si256();
	result = _mm256_insert_epi64(result, static_cast<std::int64_t>(_mm256_extract_epi64(lhs, 0)) / static_cast<std::int64_t>(_mm256_extract_epi64(rhs, 0)), 0);
	result = _mm256_insert_epi64(result, static_cast<std::int64_t>(_mm256_extract_epi64(lhs, 1)) / static_cast<std::int64_t>(_mm256_extract_epi64(rhs, 1)), 1);
	result = _mm256_insert_epi64(result, static_cast<std::int64_t>(_mm256_extract_epi64(lhs, 2)) / static_cast<std::int64_t>(_mm256_extract_epi64(rhs, 2)), 2);
	result = _mm256_insert_epi64(result, static_cast<std::int64_t>(_mm256_extract_epi64(lhs, 3)) / static_cast<std::int64_t>(_mm256_extract_epi64(rhs, 3)), 3);
	return result;
}

/**
 * @brief Divides 4 unsigned 64-bit lanes using constant-index intrinsic extraction and insertion.
 * @param lhs Dividend lanes.
 * @param rhs Divisor lanes.
 * @pre Every lane in rhs is nonzero.
 * @return The truncating integer quotient for every lane.
 */
SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY __m256i VECTORCALL _ext256_div_epu64(__m256i lhs, __m256i rhs) noexcept
{
	__m256i result = _mm256_setzero_si256();
	result = _mm256_insert_epi64(
		result,
		std::bit_cast<std::int64_t>(static_cast<std::uint64_t>(_mm256_extract_epi64(lhs, 0)) / static_cast<std::uint64_t>(_mm256_extract_epi64(rhs, 0))), 0);
	result = _mm256_insert_epi64(
		result,
		std::bit_cast<std::int64_t>(static_cast<std::uint64_t>(_mm256_extract_epi64(lhs, 1)) / static_cast<std::uint64_t>(_mm256_extract_epi64(rhs, 1))), 1);
	result = _mm256_insert_epi64(
		result,
		std::bit_cast<std::int64_t>(static_cast<std::uint64_t>(_mm256_extract_epi64(lhs, 2)) / static_cast<std::uint64_t>(_mm256_extract_epi64(rhs, 2))), 2);
	result = _mm256_insert_epi64(
		result,
		std::bit_cast<std::int64_t>(static_cast<std::uint64_t>(_mm256_extract_epi64(lhs, 3)) / static_cast<std::uint64_t>(_mm256_extract_epi64(rhs, 3))), 3);
	return result;
}

#pragma endregion

#pragma region 256bit uint32_t Extensions

SIMDLIB_FORCE_INLINE __m256 VECTORCALL _ext256_cvtepu32_ps(__m256i lhs) noexcept
{
	const __m256 signedFloats = _mm256_cvtepi32_ps(lhs);
	const __m256i highBitMask = _mm256_cmpgt_epi32(_mm256_setzero_si256(), lhs);
	const __m256 correction = _mm256_and_ps(_mm256_castsi256_ps(highBitMask), _mm256_set1_ps(4294967296.0f));
	return _mm256_add_ps(signedFloats, correction);
}

#pragma endregion

#endif // SIMDLIB_HAS_AVX2 && SIMDLIB_HAS_SSE42

#if SIMDLIB_HAS_SSE42

#pragma region 128bit int64_t Extensions

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_cmpgt_epi64(__m128i lhs, __m128i rhs) noexcept
{
	return _mm_cmpgt_epi64(lhs, rhs);
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_mullo_epi64(__m128i lhs, __m128i rhs) noexcept
{
	const __m128i productLow = _mm_mul_epu32(lhs, rhs);
	const __m128i lhsHigh = _mm_srli_epi64(lhs, 32);
	const __m128i rhsHigh = _mm_srli_epi64(rhs, 32);
	const __m128i cross = _mm_add_epi64(_mm_mul_epu32(lhsHigh, rhs), _mm_mul_epu32(lhs, rhsHigh));
	return _mm_add_epi64(productLow, _mm_slli_epi64(cross, 32));
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_abs_epi64(__m128i lhs) noexcept
{
	const __m128i zero = _mm_setzero_si128();
	const __m128i sign = _mm_cmpgt_epi64(zero, lhs);
	return _mm_sub_epi64(_mm_xor_si128(lhs, sign), sign);
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_min_epi64(__m128i lhs, __m128i rhs) noexcept
{
	const __m128i mask = _mm_cmpgt_epi64(lhs, rhs);
	return _mm_blendv_epi8(lhs, rhs, mask);
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_max_epi64(__m128i lhs, __m128i rhs) noexcept
{
	const __m128i mask = _mm_cmpgt_epi64(lhs, rhs);
	return _mm_blendv_epi8(rhs, lhs, mask);
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_srai_epi64(__m128i lhs, const int count) noexcept
{
	if (count <= 0)
	{
		return lhs;
	}

	const __m128i zero = _mm_setzero_si128();
	const __m128i sign = _mm_cmpgt_epi64(zero, lhs);
	if (count >= 64)
	{
		return sign;
	}

	const __m128i shift = _mm_cvtsi32_si128(count);
	const __m128i fillShift = _mm_cvtsi32_si128(64 - count);
	const __m128i logical = _mm_srl_epi64(lhs, shift);
	const __m128i fill = _mm_sll_epi64(sign, fillShift);
	return _mm_or_si128(logical, fill);
}

// AVX2 has no efficient exact variable u64/s64 vector divide. For general-purpose
// per-lane divisors, unpacking to scalar hardware division is faster than a bit-serial
// SIMD long-division loop and preserves exact integer semantics.

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_rem_epu64(__m128i lhs, __m128i rhs) noexcept
{
	return register_from_values<__m128i, std::uint64_t>(register_get<std::uint64_t>(lhs, 0) % register_get<std::uint64_t>(rhs, 0),
														register_get<std::uint64_t>(lhs, 1) % register_get<std::uint64_t>(rhs, 1));
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_rem_epi64(__m128i lhs, __m128i rhs) noexcept
{
	return register_from_values<__m128i, std::int64_t>(register_get<std::int64_t>(lhs, 0) % register_get<std::int64_t>(rhs, 0),
													   register_get<std::int64_t>(lhs, 1) % register_get<std::int64_t>(rhs, 1));
}

#pragma endregion

#pragma region 128bit uint64_t Extensions

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_cmpgt_epu64(__m128i lhs, __m128i rhs) noexcept
{
	const __m128i signBit = _mm_set1_epi64x(std::numeric_limits<std::int64_t>::min());
	return _mm_cmpgt_epi64(_mm_xor_si128(lhs, signBit), _mm_xor_si128(rhs, signBit));
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_min_epu64(__m128i lhs, __m128i rhs) noexcept
{
	const __m128i mask = _ext_cmpgt_epu64(lhs, rhs);
	return _mm_blendv_epi8(lhs, rhs, mask);
}

SIMDLIB_FORCE_INLINE __m128i VECTORCALL _ext_max_epu64(__m128i lhs, __m128i rhs) noexcept
{
	const __m128i mask = _ext_cmpgt_epu64(lhs, rhs);
	return _mm_blendv_epi8(rhs, lhs, mask);
}

#pragma endregion

#pragma region 128bit uint128_t Extentions

SIMDLIB_FORCE_INLINE constexpr __m128i VECTORCALL _ext128_shift_left_bits_dynamic(__m128i lhs, int shift) noexcept
{
	if (shift <= 0)
		return lhs;
	if (shift >= 128)
		return register_from_values<__m128i, std::uint64_t>(0, 0);

	const auto lanes = register_to_array<std::uint64_t>(lhs);
	if (shift == 64)
		return register_from_values<__m128i, std::uint64_t>(0, lanes[0]);
	if (shift < 64)
		return register_from_values<__m128i, std::uint64_t>(lanes[0] << shift, (lanes[1] << shift) | (lanes[0] >> (64 - shift)));
	return register_from_values<__m128i, std::uint64_t>(0, lanes[0] << (shift - 64));
}

template <int shift> SIMDLIB_FORCE_INLINE constexpr __m128i VECTORCALL _ext128_shift_left_bits_static(__m128i lhs) noexcept
{
	static_assert(shift >= 0, "Whole-register shifts require a non-negative count.");
	return _ext128_shift_left_bits_dynamic(lhs, shift);
}

SIMDLIB_FORCE_INLINE constexpr __m128i VECTORCALL _ext128_shift_right_bits_dynamic(__m128i lhs, int shift) noexcept
{
	if (shift <= 0)
		return lhs;
	if (shift >= 128)
		return register_from_values<__m128i, std::uint64_t>(0, 0);

	const auto lanes = register_to_array<std::uint64_t>(lhs);
	if (shift == 64)
		return register_from_values<__m128i, std::uint64_t>(lanes[1], 0);
	if (shift < 64)
		return register_from_values<__m128i, std::uint64_t>((lanes[0] >> shift) | (lanes[1] << (64 - shift)), lanes[1] >> shift);
	return register_from_values<__m128i, std::uint64_t>(lanes[1] >> (shift - 64), 0);
}

template <int shift> SIMDLIB_FORCE_INLINE constexpr __m128i VECTORCALL _ext128_shift_right_bits_static(__m128i lhs) noexcept
{
	static_assert(shift >= 0, "Whole-register shifts require a non-negative count.");
	return _ext128_shift_right_bits_dynamic(lhs, shift);
}

#pragma endregion

#pragma region 128bit float Extensions

/**
 * @brief Clears the sign bit of each 32-bit floating-point lane.
 *
 * @param lhs The floating-point lanes.
 * @return The per-lane absolute values.
 */
SIMDLIB_FORCE_INLINE __m128 VECTORCALL _ext_abs_ps(const __m128 lhs) noexcept
{
	return _mm_and_ps(lhs, _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF)));
}

/**
 * @brief Clears the sign bit of each 64-bit floating-point lane.
 *
 * @param lhs The floating-point lanes.
 * @return The per-lane absolute values.
 */
SIMDLIB_FORCE_INLINE __m128d VECTORCALL _ext_abs_pd(const __m128d lhs) noexcept
{
	return _mm_and_pd(lhs, _mm_castsi128_pd(_mm_set1_epi64x(0x7FFF'FFFF'FFFF'FFFFLL)));
}
#pragma endregion

#endif // SIMDLIB_HAS_SSE42

#if SIMDLIB_HAS_AVX2 && SIMDLIB_HAS_SSE42

#pragma region 256bit int8_t Extensions

/**
 * @brief Multiplies corresponding 8-bit lanes modulo 256. [eg: 0x81 * 0x02 => 0x02]
 *
 * @param lhs The first byte-lane register.
 * @param rhs The second byte-lane register.
 * @return The low byte of each lane product.
 */
SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_mul_epi8(__m256i lhs, __m256i rhs) noexcept
{
	// unpack and multiply
	const auto dst_even = _mm256_mullo_epi16(lhs, rhs);
	const auto dst_odd = _mm256_slli_epi16(_mm256_mullo_epi16(_mm256_srli_epi16(lhs, 8), _mm256_srli_epi16(rhs, 8)), 8);
	// repack
	const auto mask = _mm256_set1_epi32(0x00FF00FF); // mask for even positions
	return _mm256_blendv_epi8(dst_odd, dst_even, mask);
}

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_cmplt_epi8(__m256i lhs, __m256i rhs) noexcept
{
	// Compare (b > a) which is effectively (a < b)
	return _mm256_cmpgt_epi8(rhs, lhs);
}

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_slli_epx8(__m256i lhs, const int count) noexcept
{
	const __m256i mask = _mm256_set1_epi8(0xFF << count);
	return _mm256_and_si256(_mm256_slli_epi16(lhs, count), mask);
}

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_srli_epx8(__m256i lhs, const int count) noexcept
{
	const __m256i mask = _mm256_set1_epi8(0xFF >> count);
	return _mm256_and_si256(_mm256_srli_epi16(lhs, count), mask);
}

/**
 * @brief Arithmetic-right-shifts each signed 8-bit lane. [eg: 0x82 >> 1 => 0xC1]
 *
 * @param lhs The signed byte-lane register.
 * @param count The per-lane shift count.
 * @return The arithmetic-right-shifted byte lanes.
 */
SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_srai_epx8(__m256i lhs, const int count) noexcept
{
	__m256i aeven = _mm256_slli_epi16(lhs, 8);						// even numbered elements get sign bit in position
	aeven = _mm256_sra_epi16(aeven, _mm_cvtsi32_si128(count + 8));	// shift arithmetic, back to position
	__m256i aodd = _mm256_sra_epi16(lhs, _mm_cvtsi32_si128(count)); // shift odd numbered elements arithmetic
	__m256i mask = _mm256_set1_epi32(0x00FF00FF);					// mask for even positions
	__m256i res = _mm256_blendv_epi8(aodd, aeven, mask);			// interleave even and odd
	return res;
}

#pragma endregion

#pragma region 256bit uint8_t Extensions

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_mul_epu8(__m256i lhs, __m256i rhs) noexcept
{
	return _ext256_mul_epi8(lhs, rhs);
}

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_set1_epu8(std::uint8_t value) noexcept
{
	return _mm256_set1_epi8(static_cast<char>(value));
}

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_cmpgt_epu8(__m256i lhs, __m256i rhs) noexcept
{
	// Returns 0xFF where x > y:
	return _mm256_andnot_si256(_mm256_cmpeq_epi8(lhs, rhs), _mm256_cmpeq_epi8(_mm256_max_epu8(lhs, rhs), lhs));
}

#pragma endregion

#pragma region 256bit uint16_t Extensions

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_cmpgt_epu16(__m256i lhs, __m256i rhs) noexcept
{
	// Returns 0xFF where x > y:
	return _mm256_andnot_si256(_mm256_cmpeq_epi16(lhs, rhs), _mm256_cmpeq_epi16(_mm256_max_epu16(lhs, rhs), lhs));
}

#pragma endregion

#pragma region 256bit uint32_t Extensions

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_cmpgt_epu32(__m256i lhs, __m256i rhs) noexcept
{
	// Returns 0xFF where x > y:
	return _mm256_andnot_si256(_mm256_cmpeq_epi32(lhs, rhs), _mm256_cmpeq_epi32(_mm256_max_epu32(lhs, rhs), lhs));
}

#pragma endregion

#pragma region 256bit uint64_t Extensions

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_cmpgt_epu64(__m256i lhs, __m256i rhs) noexcept
{
	const __m256i signBit = _mm256_set1_epi64x(std::numeric_limits<std::int64_t>::min());
	return _mm256_cmpgt_epi64(_mm256_xor_si256(lhs, signBit), _mm256_xor_si256(rhs, signBit));
}

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_mullo_epi64(__m256i lhs, __m256i rhs) noexcept
{
	const __m256i productLow = _mm256_mul_epu32(lhs, rhs);
	const __m256i lhsHigh = _mm256_srli_epi64(lhs, 32);
	const __m256i rhsHigh = _mm256_srli_epi64(rhs, 32);
	const __m256i cross = _mm256_add_epi64(_mm256_mul_epu32(lhsHigh, rhs), _mm256_mul_epu32(lhs, rhsHigh));
	return _mm256_add_epi64(productLow, _mm256_slli_epi64(cross, 32));
}

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_abs_epi64(__m256i lhs) noexcept
{
	const __m256i zero = _mm256_setzero_si256();
	const __m256i sign = _mm256_cmpgt_epi64(zero, lhs);
	return _mm256_sub_epi64(_mm256_xor_si256(lhs, sign), sign);
}

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_min_epi64(__m256i lhs, __m256i rhs) noexcept
{
	const __m256i mask = _mm256_cmpgt_epi64(lhs, rhs);
	return _mm256_blendv_epi8(lhs, rhs, mask);
}

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_max_epi64(__m256i lhs, __m256i rhs) noexcept
{
	const __m256i mask = _mm256_cmpgt_epi64(lhs, rhs);
	return _mm256_blendv_epi8(rhs, lhs, mask);
}

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_min_epu64(__m256i lhs, __m256i rhs) noexcept
{
	const __m256i mask = _ext256_cmpgt_epu64(lhs, rhs);
	return _mm256_blendv_epi8(lhs, rhs, mask);
}

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_max_epu64(__m256i lhs, __m256i rhs) noexcept
{
	const __m256i mask = _ext256_cmpgt_epu64(lhs, rhs);
	return _mm256_blendv_epi8(rhs, lhs, mask);
}

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_srai_epi64(__m256i lhs, const int count) noexcept
{
	if (count <= 0)
	{
		return lhs;
	}

	const __m256i zero = _mm256_setzero_si256();
	const __m256i sign = _mm256_cmpgt_epi64(zero, lhs);
	if (count >= 64)
	{
		return sign;
	}

	const __m128i shift = _mm_cvtsi32_si128(count);
	const __m128i fillShift = _mm_cvtsi32_si128(64 - count);
	const __m256i logical = _mm256_srl_epi64(lhs, shift);
	const __m256i fill = _mm256_sll_epi64(sign, fillShift);
	return _mm256_or_si256(logical, fill);
}

// AVX2 has no efficient exact variable u64/s64 vector divide. For general-purpose
// per-lane divisors, unpacking to scalar hardware division is faster than a bit-serial
// SIMD long-division loop and preserves exact integer semantics.

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_rem_epu64(__m256i lhs, __m256i rhs) noexcept
{
	return register_from_values<__m256i, std::uint64_t>(
		register_get<std::uint64_t>(lhs, 0) % register_get<std::uint64_t>(rhs, 0), register_get<std::uint64_t>(lhs, 1) % register_get<std::uint64_t>(rhs, 1),
		register_get<std::uint64_t>(lhs, 2) % register_get<std::uint64_t>(rhs, 2), register_get<std::uint64_t>(lhs, 3) % register_get<std::uint64_t>(rhs, 3));
}

SIMDLIB_FORCE_INLINE __m256i VECTORCALL _ext256_rem_epi64(__m256i lhs, __m256i rhs) noexcept
{
	return register_from_values<__m256i, std::int64_t>(
		register_get<std::int64_t>(lhs, 0) % register_get<std::int64_t>(rhs, 0), register_get<std::int64_t>(lhs, 1) % register_get<std::int64_t>(rhs, 1),
		register_get<std::int64_t>(lhs, 2) % register_get<std::int64_t>(rhs, 2), register_get<std::int64_t>(lhs, 3) % register_get<std::int64_t>(rhs, 3));
}

#pragma endregion

#pragma region 256bit float Extensions

/**
 * @brief Clears the sign bit of each 32-bit floating-point lane.
 *
 * @param lhs The floating-point lanes.
 * @return The per-lane absolute values.
 */
SIMDLIB_FORCE_INLINE __m256 VECTORCALL _ext256_abs_ps(const __m256 lhs) noexcept
{
	return _mm256_and_ps(lhs, _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF)));
}

/**
 * @brief Clears the sign bit of each 64-bit floating-point lane.
 *
 * @param lhs The floating-point lanes.
 * @return The per-lane absolute values.
 */
SIMDLIB_FORCE_INLINE __m256d VECTORCALL _ext256_abs_pd(const __m256d lhs) noexcept
{
	return _mm256_and_pd(lhs, _mm256_castsi256_pd(_mm256_set1_epi64x(0x7FFF'FFFF'FFFF'FFFFLL)));
}

SIMDLIB_FORCE_INLINE __m256 VECTORCALL _ext256_cmpeq_ps(__m256 lhs, __m256 rhs) noexcept
{
	return _mm256_cmp_ps(lhs, rhs, _CMP_EQ_OQ);
}

SIMDLIB_FORCE_INLINE __m256 VECTORCALL _ext256_cmpgt_ps(__m256 lhs, __m256 rhs) noexcept
{
	return _mm256_cmp_ps(lhs, rhs, _CMP_GT_OQ);
}
/**
 * @brief Compares 64-bit floating-point lanes for ordered equality.
 *
 * @param lhs The first floating-point register.
 * @param rhs The second floating-point register.
 * @return An all-ones lane mask where corresponding lanes are equal.
 */
SIMDLIB_FORCE_INLINE __m256d VECTORCALL _ext256_cmpeq_pd(const __m256d lhs, const __m256d rhs) noexcept
{
	return _mm256_cmp_pd(lhs, rhs, _CMP_EQ_OQ);
}

/**
 * @brief Compares 64-bit floating-point lanes for ordered greater-than.
 *
 * @param lhs The first floating-point register.
 * @param rhs The second floating-point register.
 * @return An all-ones lane mask where lhs is greater than rhs.
 */
SIMDLIB_FORCE_INLINE __m256d VECTORCALL _ext256_cmpgt_pd(const __m256d lhs, const __m256d rhs) noexcept
{
	return _mm256_cmp_pd(lhs, rhs, _CMP_GT_OQ);
}

// SIMDLIB_FORCE_INLINE VECTORCALL __m256 _ext256_insert_ps(__m256 lhs, __m128 rhs, const int imm8) noexcept
//{
//     return _mm256_insertf128_ps(lhs, rhs, imm8);
// }

#pragma endregion

#endif // SIMDLIB_HAS_AVX2 && SIMDLIB_HAS_SSE42

} // namespace SimdLib::Detail
