#pragma once

#if SIMDLIB_CODEGEN_USE_WRAPPER
#include <SimdLib/Register.h>
#else
#include <SimdLib/Api.h>
#endif

#include <cstddef>
#include <cstdint>

#if SIMDLIB_COMPILER_MSVC
#define SIMDLIB_REARRANGEMENT_CODEGEN_NOINLINE __declspec(noinline)
#else
#define SIMDLIB_REARRANGEMENT_CODEGEN_NOINLINE __attribute__((noinline))
#endif

namespace SimdLibRearrangementCodegen
{

/** @brief Native register type for one code-generation element type and width. */
template <class element_t, std::size_t bits = SIMDLIB_REGISTER_TEST_WIDTH> using native_t = typename SimdLib::Api<bits, element_t>::vector_t;

} // namespace SimdLibRearrangementCodegen

#if SIMDLIB_CODEGEN_USE_WRAPPER
#define SIMDLIB_REARRANGE_UNARY(type, member, api, value) (SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{value}.member().native)
#define SIMDLIB_REARRANGE_BINARY(type, member, api, lhs, rhs)                                                                                                  \
	(SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{lhs}.member(SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{rhs}).native)
#define SIMDLIB_REARRANGE_INDEXED_UNARY(type, member, api, immediate, value)                                                                                   \
	(SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{value}.template member<immediate>().native)
#define SIMDLIB_REARRANGE_INDEXED_BINARY(type, member, api, immediate, lhs, rhs)                                                                               \
	(SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{lhs}.template member<immediate>(SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{rhs}).native)
#define SIMDLIB_REARRANGE_BIT_CAST(source_type, target_type, value)                                                                                            \
	(SimdLib::Register<source_type, SIMDLIB_REGISTER_TEST_WIDTH>{value}.template bit_cast<target_type>().native)
#define SIMDLIB_REARRANGE_CONVERT(source_type, target_type, value)                                                                                             \
	(SimdLib::Register<source_type, SIMDLIB_REGISTER_TEST_WIDTH>{value}.template convert<target_type>().native)
#define SIMDLIB_REARRANGE_LOWER(type, value) (SimdLib::Register<type, 256>{value}.lower_half().native)
#define SIMDLIB_REARRANGE_WIDEN(source_type, target_type, target_bits, value)                                                                                  \
	(SimdLib::Register<source_type, 128>{value}.template widen_low<target_type, target_bits>().native)
#define SIMDLIB_REARRANGE_LOGICAL_SHUFFLE(type, value, ...) (SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{value}.template shuffle<__VA_ARGS__>().native)
#else
#define SIMDLIB_REARRANGE_UNARY(type, member, api, value) (SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, type>::api(value))
#define SIMDLIB_REARRANGE_BINARY(type, member, api, lhs, rhs) (SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, type>::api(lhs, rhs))
#define SIMDLIB_REARRANGE_INDEXED_UNARY(type, member, api, immediate, value) (SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, type>::template api<immediate>(value))
#define SIMDLIB_REARRANGE_INDEXED_BINARY(type, member, api, immediate, lhs, rhs)                                                                               \
	(SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, type>::template api<immediate>(lhs, rhs))
#define SIMDLIB_REARRANGE_BIT_CAST(source_type, target_type, value)                                                                                            \
	(SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, source_type>::template bit_cast<target_type>(value))
#define SIMDLIB_REARRANGE_CONVERT(source_type, target_type, value)                                                                                             \
	(SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, source_type>::template convert<target_type>(value))
#define SIMDLIB_REARRANGE_LOWER(type, value) (SimdLib::Api<256, type>::lower_half(value))
#define SIMDLIB_REARRANGE_WIDEN(source_type, target_type, target_bits, value)                                                                                  \
	(SimdLib::Api<128, source_type>::template widen<SimdLib::Api<target_bits, target_type>>(value))
#define SIMDLIB_REARRANGE_LOGICAL_SHUFFLE(type, value, ...) (SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, type>::template shuffle<__VA_ARGS__>(value))
#endif

#define SIMDLIB_DEFINE_REARRANGE_UNARY(operation, token, type, member, api)                                                                                    \
	/** @brief Compares one unary rearrangement wrapper against its Api expression. */                                                                         \
	SIMDLIB_REGISTER_ONLY SIMDLIB_REARRANGEMENT_CODEGEN_NOINLINE SimdLibRearrangementCodegen::native_t<type> VECTORCALL                                        \
	simdlib_rearrangement_codegen_##operation##_##token(SimdLibRearrangementCodegen::native_t<type> value) noexcept                                            \
	{                                                                                                                                                          \
		return SIMDLIB_REARRANGE_UNARY(type, member, api, value);                                                                                              \
	}

#define SIMDLIB_DEFINE_REARRANGE_BINARY(operation, token, type, member, api)                                                                                   \
	/** @brief Compares one binary rearrangement wrapper against its Api expression. */                                                                        \
	SIMDLIB_REGISTER_ONLY SIMDLIB_REARRANGEMENT_CODEGEN_NOINLINE SimdLibRearrangementCodegen::native_t<type> VECTORCALL                                        \
	simdlib_rearrangement_codegen_##operation##_##token(SimdLibRearrangementCodegen::native_t<type> lhs,                                                       \
														SimdLibRearrangementCodegen::native_t<type> rhs) noexcept                                              \
	{                                                                                                                                                          \
		return SIMDLIB_REARRANGE_BINARY(type, member, api, lhs, rhs);                                                                                          \
	}

#define SIMDLIB_DEFINE_REARRANGE_INDEXED_UNARY(operation, token, type, member, api, immediate)                                                                 \
	/** @brief Compares one immediate unary rearrangement wrapper against its Api expression. */                                                               \
	SIMDLIB_REGISTER_ONLY SIMDLIB_REARRANGEMENT_CODEGEN_NOINLINE SimdLibRearrangementCodegen::native_t<type> VECTORCALL                                        \
	simdlib_rearrangement_codegen_##operation##_##token(SimdLibRearrangementCodegen::native_t<type> value) noexcept                                            \
	{                                                                                                                                                          \
		return SIMDLIB_REARRANGE_INDEXED_UNARY(type, member, api, immediate, value);                                                                           \
	}

#define SIMDLIB_DEFINE_REARRANGE_INDEXED_BINARY(operation, token, type, member, api, immediate)                                                                \
	/** @brief Compares one immediate binary rearrangement wrapper against its Api expression. */                                                              \
	SIMDLIB_REGISTER_ONLY SIMDLIB_REARRANGEMENT_CODEGEN_NOINLINE SimdLibRearrangementCodegen::native_t<type> VECTORCALL                                        \
	simdlib_rearrangement_codegen_##operation##_##token(SimdLibRearrangementCodegen::native_t<type> lhs,                                                       \
														SimdLibRearrangementCodegen::native_t<type> rhs) noexcept                                              \
	{                                                                                                                                                          \
		return SIMDLIB_REARRANGE_INDEXED_BINARY(type, member, api, immediate, lhs, rhs);                                                                       \
	}

#define SIMDLIB_FOR_EACH_REGISTER_TYPE(macro, operation, member, api)                                                                                          \
	macro(operation, i8, std::int8_t, member, api) macro(operation, u8, std::uint8_t, member, api) macro(operation, i16, std::int16_t, member, api)            \
		macro(operation, u16, std::uint16_t, member, api) macro(operation, i32, std::int32_t, member, api) macro(operation, u32, std::uint32_t, member, api)   \
			macro(operation, i64, std::int64_t, member, api) macro(operation, u64, std::uint64_t, member, api) macro(operation, f32, float, member, api)       \
				macro(operation, f64, double, member, api)

SIMDLIB_FOR_EACH_REGISTER_TYPE(SIMDLIB_DEFINE_REARRANGE_BINARY, unpack_low, unpack_low, unpack_lo)
SIMDLIB_FOR_EACH_REGISTER_TYPE(SIMDLIB_DEFINE_REARRANGE_BINARY, unpack_high, unpack_high, unpack_hi)

SIMDLIB_DEFINE_REARRANGE_INDEXED_UNARY(shuffle_low, i16, std::int16_t, shuffle_low, shuffle_lo, 0x1B)
SIMDLIB_DEFINE_REARRANGE_INDEXED_UNARY(shuffle_low, u16, std::uint16_t, shuffle_low, shuffle_lo, 0x1B)
SIMDLIB_DEFINE_REARRANGE_INDEXED_UNARY(shuffle_high, i16, std::int16_t, shuffle_high, shuffle_hi, 0x1B)
SIMDLIB_DEFINE_REARRANGE_INDEXED_UNARY(shuffle_high, u16, std::uint16_t, shuffle_high, shuffle_hi, 0x1B)
SIMDLIB_DEFINE_REARRANGE_INDEXED_BINARY(blend, i16, std::int16_t, blend, blend, 0xA5)
SIMDLIB_DEFINE_REARRANGE_INDEXED_BINARY(blend, u16, std::uint16_t, blend, blend, 0xA5)
SIMDLIB_DEFINE_REARRANGE_INDEXED_BINARY(blend, i32, std::int32_t, blend, blend, 0xA5)
SIMDLIB_DEFINE_REARRANGE_INDEXED_BINARY(blend, u32, std::uint32_t, blend, blend, 0xA5)
SIMDLIB_DEFINE_REARRANGE_INDEXED_BINARY(blend, f32, float, blend, blend, 0xA5)
SIMDLIB_DEFINE_REARRANGE_INDEXED_BINARY(blend, f64, double, blend, blend, 0xA5)

#define SIMDLIB_DEFINE_LOGICAL_SHUFFLE(token, type, ...)                                                                                                       \
	/** @brief Compares one complete logical shuffle wrapper against its Api expression. */                                                                    \
	SIMDLIB_REGISTER_ONLY SIMDLIB_REARRANGEMENT_CODEGEN_NOINLINE SimdLibRearrangementCodegen::native_t<type> VECTORCALL                                        \
	simdlib_rearrangement_codegen_logical_shuffle_##token(SimdLibRearrangementCodegen::native_t<type> value) noexcept                                          \
	{                                                                                                                                                          \
		return SIMDLIB_REARRANGE_LOGICAL_SHUFFLE(type, value, __VA_ARGS__);                                                                                    \
	}

#if SIMDLIB_REGISTER_TEST_WIDTH == 128
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(i8, std::int8_t, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(u8, std::uint8_t, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(i16, std::int16_t, 7, 6, 5, 4, 3, 2, 1, 0)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(u16, std::uint16_t, 7, 6, 5, 4, 3, 2, 1, 0)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(i32, std::int32_t, 3, 2, 1, 0)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(u32, std::uint32_t, 3, 2, 1, 0)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(i64, std::int64_t, 1, 0)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(u64, std::uint64_t, 1, 0)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(f32, float, 3, 2, 1, 0)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(f64, double, 1, 0)
#else
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(i8, std::int8_t, 0, 17, 2, 19, 4, 21, 6, 23, 8, 25, 10, 27, 12, 29, 14, 31, 16, 1, 18, 3, 20, 5, 22, 7, 24, 9, 26, 11, 28, 13,
							   30, 15)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(u8, std::uint8_t, 0, 17, 2, 19, 4, 21, 6, 23, 8, 25, 10, 27, 12, 29, 14, 31, 16, 1, 18, 3, 20, 5, 22, 7, 24, 9, 26, 11, 28, 13,
							   30, 15)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(i16, std::int16_t, 0, 9, 2, 11, 4, 13, 6, 15, 8, 1, 10, 3, 12, 5, 14, 7)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(u16, std::uint16_t, 0, 9, 2, 11, 4, 13, 6, 15, 8, 1, 10, 3, 12, 5, 14, 7)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(i32, std::int32_t, 7, 6, 5, 4, 3, 2, 1, 0)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(u32, std::uint32_t, 7, 6, 5, 4, 3, 2, 1, 0)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(i64, std::int64_t, 3, 2, 1, 0)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(u64, std::uint64_t, 3, 2, 1, 0)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(f32, float, 7, 6, 5, 4, 3, 2, 1, 0)
SIMDLIB_DEFINE_LOGICAL_SHUFFLE(f64, double, 3, 2, 1, 0)
#define SIMDLIB_DEFINE_LOWER(token, type)                                                                                                                      \
	/** @brief Compares one lower-half wrapper against its Api expression. */                                                                                  \
	SIMDLIB_REGISTER_ONLY SIMDLIB_REARRANGEMENT_CODEGEN_NOINLINE SimdLibRearrangementCodegen::native_t<type, 128> VECTORCALL                                   \
	simdlib_rearrangement_codegen_lower_half_##token(SimdLibRearrangementCodegen::native_t<type, 256> value) noexcept                                          \
	{                                                                                                                                                          \
		return SIMDLIB_REARRANGE_LOWER(type, value);                                                                                                           \
	}
SIMDLIB_DEFINE_LOWER(i8, std::int8_t)
SIMDLIB_DEFINE_LOWER(u8, std::uint8_t)
SIMDLIB_DEFINE_LOWER(i16, std::int16_t)
SIMDLIB_DEFINE_LOWER(u16, std::uint16_t)
SIMDLIB_DEFINE_LOWER(i32, std::int32_t)
SIMDLIB_DEFINE_LOWER(u32, std::uint32_t)
SIMDLIB_DEFINE_LOWER(i64, std::int64_t)
SIMDLIB_DEFINE_LOWER(u64, std::uint64_t)
SIMDLIB_DEFINE_LOWER(f32, float)
SIMDLIB_DEFINE_LOWER(f64, double)
#undef SIMDLIB_DEFINE_LOWER
#endif

#define SIMDLIB_DEFINE_BIT_CAST(source_token, source_type, target_token, target_type)                                                                          \
	/** @brief Compares one full-width bit reinterpretation wrapper against its Api expression. */                                                             \
	SIMDLIB_REGISTER_ONLY SIMDLIB_REARRANGEMENT_CODEGEN_NOINLINE SimdLibRearrangementCodegen::native_t<target_type> VECTORCALL                                 \
	simdlib_rearrangement_codegen_bit_cast_##source_token##_##target_token(SimdLibRearrangementCodegen::native_t<source_type> value) noexcept                  \
	{                                                                                                                                                          \
		return SIMDLIB_REARRANGE_BIT_CAST(source_type, target_type, value);                                                                                    \
	}

#define SIMDLIB_FOR_EACH_BIT_CAST_TARGET(macro, source_token, source_type)                                                                                     \
	macro(source_token, source_type, i8, std::int8_t) macro(source_token, source_type, u8, std::uint8_t) macro(source_token, source_type, i16, std::int16_t)   \
		macro(source_token, source_type, u16, std::uint16_t) macro(source_token, source_type, i32, std::int32_t)                                               \
			macro(source_token, source_type, u32, std::uint32_t) macro(source_token, source_type, i64, std::int64_t)                                           \
				macro(source_token, source_type, u64, std::uint64_t) macro(source_token, source_type, f32, float)                                              \
					macro(source_token, source_type, f64, double)

SIMDLIB_FOR_EACH_BIT_CAST_TARGET(SIMDLIB_DEFINE_BIT_CAST, i8, std::int8_t)
SIMDLIB_FOR_EACH_BIT_CAST_TARGET(SIMDLIB_DEFINE_BIT_CAST, u8, std::uint8_t)
SIMDLIB_FOR_EACH_BIT_CAST_TARGET(SIMDLIB_DEFINE_BIT_CAST, i16, std::int16_t)
SIMDLIB_FOR_EACH_BIT_CAST_TARGET(SIMDLIB_DEFINE_BIT_CAST, u16, std::uint16_t)
SIMDLIB_FOR_EACH_BIT_CAST_TARGET(SIMDLIB_DEFINE_BIT_CAST, i32, std::int32_t)
SIMDLIB_FOR_EACH_BIT_CAST_TARGET(SIMDLIB_DEFINE_BIT_CAST, u32, std::uint32_t)
SIMDLIB_FOR_EACH_BIT_CAST_TARGET(SIMDLIB_DEFINE_BIT_CAST, i64, std::int64_t)
SIMDLIB_FOR_EACH_BIT_CAST_TARGET(SIMDLIB_DEFINE_BIT_CAST, u64, std::uint64_t)
SIMDLIB_FOR_EACH_BIT_CAST_TARGET(SIMDLIB_DEFINE_BIT_CAST, f32, float)
SIMDLIB_FOR_EACH_BIT_CAST_TARGET(SIMDLIB_DEFINE_BIT_CAST, f64, double)

#define SIMDLIB_DEFINE_CONVERT(source_token, source_type, target_token, target_type)                                                                           \
	/** @brief Compares one complete numeric conversion wrapper against its Api expression. */                                                                 \
	SIMDLIB_REGISTER_ONLY SIMDLIB_REARRANGEMENT_CODEGEN_NOINLINE SimdLibRearrangementCodegen::native_t<target_type> VECTORCALL                                 \
	simdlib_rearrangement_codegen_convert_##source_token##_##target_token(SimdLibRearrangementCodegen::native_t<source_type> value) noexcept                   \
	{                                                                                                                                                          \
		return SIMDLIB_REARRANGE_CONVERT(source_type, target_type, value);                                                                                     \
	}
SIMDLIB_DEFINE_CONVERT(i32, std::int32_t, f32, float)
SIMDLIB_DEFINE_CONVERT(u32, std::uint32_t, f32, float)
SIMDLIB_DEFINE_CONVERT(f32, float, i32, std::int32_t)

#if SIMDLIB_REGISTER_TEST_WIDTH == 128
#define SIMDLIB_DEFINE_WIDEN(source_token, source_type, target_token, target_type, target_bits)                                                                \
	/** @brief Compares one explicit low-lane widening wrapper against its Api expression. */                                                                  \
	SIMDLIB_REGISTER_ONLY SIMDLIB_REARRANGEMENT_CODEGEN_NOINLINE SimdLibRearrangementCodegen::native_t<target_type, target_bits> VECTORCALL                    \
	simdlib_rearrangement_codegen_widen_##source_token##_##target_token##_##target_bits(                                                                       \
		SimdLibRearrangementCodegen::native_t<source_type, 128> value) noexcept                                                                                \
	{                                                                                                                                                          \
		return SIMDLIB_REARRANGE_WIDEN(source_type, target_type, target_bits, value);                                                                          \
	}
#if SIMDLIB_HAS_AVX2
#define SIMDLIB_DEFINE_WIDEN_WIDTHS(source_token, source_type, target_token, target_type)                                                                      \
	SIMDLIB_DEFINE_WIDEN(source_token, source_type, target_token, target_type, 128)                                                                            \
	SIMDLIB_DEFINE_WIDEN(source_token, source_type, target_token, target_type, 256)
#else
#define SIMDLIB_DEFINE_WIDEN_WIDTHS(source_token, source_type, target_token, target_type)                                                                      \
	SIMDLIB_DEFINE_WIDEN(source_token, source_type, target_token, target_type, 128)
#endif
SIMDLIB_DEFINE_WIDEN_WIDTHS(i8, std::int8_t, i16, std::int16_t)
SIMDLIB_DEFINE_WIDEN_WIDTHS(i8, std::int8_t, i32, std::int32_t)
SIMDLIB_DEFINE_WIDEN_WIDTHS(i8, std::int8_t, i64, std::int64_t)
SIMDLIB_DEFINE_WIDEN_WIDTHS(u8, std::uint8_t, u16, std::uint16_t)
SIMDLIB_DEFINE_WIDEN_WIDTHS(u8, std::uint8_t, u32, std::uint32_t)
SIMDLIB_DEFINE_WIDEN_WIDTHS(u8, std::uint8_t, u64, std::uint64_t)
SIMDLIB_DEFINE_WIDEN_WIDTHS(i16, std::int16_t, i32, std::int32_t)
SIMDLIB_DEFINE_WIDEN_WIDTHS(i16, std::int16_t, i64, std::int64_t)
SIMDLIB_DEFINE_WIDEN_WIDTHS(u16, std::uint16_t, u32, std::uint32_t)
SIMDLIB_DEFINE_WIDEN_WIDTHS(u16, std::uint16_t, u64, std::uint64_t)
SIMDLIB_DEFINE_WIDEN_WIDTHS(i32, std::int32_t, i64, std::int64_t)
SIMDLIB_DEFINE_WIDEN_WIDTHS(u32, std::uint32_t, u64, std::uint64_t)
#undef SIMDLIB_DEFINE_WIDEN_WIDTHS
#undef SIMDLIB_DEFINE_WIDEN
#endif

#undef SIMDLIB_DEFINE_CONVERT
#undef SIMDLIB_FOR_EACH_BIT_CAST_TARGET
#undef SIMDLIB_DEFINE_BIT_CAST
#undef SIMDLIB_FOR_EACH_REGISTER_TYPE
#undef SIMDLIB_DEFINE_REARRANGE_INDEXED_BINARY
#undef SIMDLIB_DEFINE_REARRANGE_INDEXED_UNARY
#undef SIMDLIB_DEFINE_REARRANGE_BINARY
#undef SIMDLIB_DEFINE_REARRANGE_UNARY
#undef SIMDLIB_DEFINE_LOGICAL_SHUFFLE
#undef SIMDLIB_REARRANGE_LOGICAL_SHUFFLE
#undef SIMDLIB_REARRANGE_WIDEN
#undef SIMDLIB_REARRANGE_LOWER
#undef SIMDLIB_REARRANGE_CONVERT
#undef SIMDLIB_REARRANGE_BIT_CAST
#undef SIMDLIB_REARRANGE_INDEXED_BINARY
#undef SIMDLIB_REARRANGE_INDEXED_UNARY
#undef SIMDLIB_REARRANGE_BINARY
#undef SIMDLIB_REARRANGE_UNARY
#undef SIMDLIB_REARRANGEMENT_CODEGEN_NOINLINE
