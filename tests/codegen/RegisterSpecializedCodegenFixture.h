#pragma once

#include <SimdLib/Register.h>

#include <cstddef>
#include <cstdint>

#if SIMDLIB_COMPILER_MSVC
#define SIMDLIB_SPECIALIZED_CODEGEN_NOINLINE __declspec(noinline)
#else
#define SIMDLIB_SPECIALIZED_CODEGEN_NOINLINE __attribute__((noinline))
#endif

namespace SimdLibSpecializedCodegen
{

/** @brief Native register type for one specialized-operation source type. */
template <class element_t> using native_t = typename SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, element_t>::vector_t;

} // namespace SimdLibSpecializedCodegen

#if SIMDLIB_CODEGEN_USE_WRAPPER
#define SIMDLIB_SPECIALIZED_UNARY_EXPRESSION(type, member, api, value) (SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{value}.member().native)
#define SIMDLIB_SPECIALIZED_BINARY_EXPRESSION(type, member, api, lhs, rhs)                                                                                     \
	(SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{lhs}.member(SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{rhs}).native)
#define SIMDLIB_SPECIALIZED_TERNARY_EXPRESSION(type, member, api, lhs, rhs, addend)                                                                            \
	(SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{lhs}                                                                                                 \
		 .member(SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{rhs}, SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{addend})                      \
		 .native)
#define SIMDLIB_SPECIALIZED_SCALAR_EXPRESSION(type, member, api, value) (SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{value}.member())
#define SIMDLIB_SPECIALIZED_PROMOTED_EXPRESSION(type, member, api, lhs, rhs)                                                                                   \
	(SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{lhs}.member(SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{rhs}).native)
#define SIMDLIB_SPECIALIZED_MULTI_SAD_EXPRESSION(type, lhs, rhs)                                                                                               \
	(SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{lhs}                                                                                                 \
		 .template multi_sum_absolute_byte_differences<0x1B>(SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{rhs})                                        \
		 .native)
#define SIMDLIB_SPECIALIZED_DOT_EXPRESSION(type, lhs, rhs)                                                                                                     \
	(SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{lhs}.template dot_product<0xD3>(SimdLib::Register<type, SIMDLIB_REGISTER_TEST_WIDTH>{rhs}).native)
#else
#define SIMDLIB_SPECIALIZED_UNARY_EXPRESSION(type, member, api, value) (SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, type>::api(value))
#define SIMDLIB_SPECIALIZED_BINARY_EXPRESSION(type, member, api, lhs, rhs) (SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, type>::api(lhs, rhs))
#define SIMDLIB_SPECIALIZED_TERNARY_EXPRESSION(type, member, api, lhs, rhs, addend) (SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, type>::api(lhs, rhs, addend))
#define SIMDLIB_SPECIALIZED_SCALAR_EXPRESSION(type, member, api, value) (SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, type>::api(value))
#define SIMDLIB_SPECIALIZED_PROMOTED_EXPRESSION(type, member, api, lhs, rhs) (SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, type>::api(lhs, rhs))
#define SIMDLIB_SPECIALIZED_MULTI_SAD_EXPRESSION(type, lhs, rhs)                                                                                               \
	(SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, type>::template multi_sum_absolute_byte_differences<0x1B>(lhs, rhs))
#define SIMDLIB_SPECIALIZED_DOT_EXPRESSION(type, lhs, rhs) (SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, type>::template dot_product<0xD3>(lhs, rhs))
#endif

#define SIMDLIB_DEFINE_SPECIALIZED_UNARY(operation, token, type, member, api)                                                                                  \
	/** @brief Compares one unary Register specialized operation against its raw Api expression. */                                                            \
	SIMDLIB_REGISTER_ONLY SIMDLIB_SPECIALIZED_CODEGEN_NOINLINE SimdLibSpecializedCodegen::native_t<type> VECTORCALL                                            \
	simdlib_specialized_codegen_##operation##_##token(SimdLibSpecializedCodegen::native_t<type> value) noexcept                                                \
	{                                                                                                                                                          \
		return SIMDLIB_SPECIALIZED_UNARY_EXPRESSION(type, member, api, value);                                                                                 \
	}

#define SIMDLIB_DEFINE_SPECIALIZED_BINARY(operation, token, type, member, api)                                                                                 \
	/** @brief Compares one binary Register specialized operation against its raw Api expression. */                                                           \
	SIMDLIB_REGISTER_ONLY SIMDLIB_SPECIALIZED_CODEGEN_NOINLINE SimdLibSpecializedCodegen::native_t<type> VECTORCALL                                            \
	simdlib_specialized_codegen_##operation##_##token(SimdLibSpecializedCodegen::native_t<type> lhs, SimdLibSpecializedCodegen::native_t<type> rhs) noexcept   \
	{                                                                                                                                                          \
		return SIMDLIB_SPECIALIZED_BINARY_EXPRESSION(type, member, api, lhs, rhs);                                                                             \
	}

#define SIMDLIB_DEFINE_SPECIALIZED_TERNARY(operation, token, type, member, api)                                                                                \
	/** @brief Compares one ternary Register specialized operation against its raw Api expression. */                                                          \
	SIMDLIB_REGISTER_ONLY SIMDLIB_SPECIALIZED_CODEGEN_NOINLINE SimdLibSpecializedCodegen::native_t<type> VECTORCALL                                            \
	simdlib_specialized_codegen_##operation##_##token(SimdLibSpecializedCodegen::native_t<type> lhs, SimdLibSpecializedCodegen::native_t<type> rhs,            \
													  SimdLibSpecializedCodegen::native_t<type> addend) noexcept                                               \
	{                                                                                                                                                          \
		return SIMDLIB_SPECIALIZED_TERNARY_EXPRESSION(type, member, api, lhs, rhs, addend);                                                                    \
	}

#define SIMDLIB_DEFINE_SPECIALIZED_SCALAR(operation, token, type, member, api)                                                                                 \
	/** @brief Compares one scalar-result Register specialized operation against its raw Api expression. */                                                    \
	SIMDLIB_REGISTER_ONLY SIMDLIB_SPECIALIZED_CODEGEN_NOINLINE std::size_t VECTORCALL simdlib_specialized_codegen_##operation##_##token(                       \
		SimdLibSpecializedCodegen::native_t<type> value) noexcept                                                                                              \
	{                                                                                                                                                          \
		return SIMDLIB_SPECIALIZED_SCALAR_EXPRESSION(type, member, api, value);                                                                                \
	}

#define SIMDLIB_DEFINE_SPECIALIZED_PROMOTED(operation, token, type, member, api)                                                                               \
	/** @brief Compares one promoted-result Register specialized operation against its raw Api expression. */                                                  \
	SIMDLIB_REGISTER_ONLY SIMDLIB_SPECIALIZED_CODEGEN_NOINLINE SimdLibSpecializedCodegen::native_t<type> VECTORCALL                                            \
	simdlib_specialized_codegen_##operation##_##token(SimdLibSpecializedCodegen::native_t<type> lhs, SimdLibSpecializedCodegen::native_t<type> rhs) noexcept   \
	{                                                                                                                                                          \
		return SIMDLIB_SPECIALIZED_PROMOTED_EXPRESSION(type, member, api, lhs, rhs);                                                                           \
	}

#define SIMDLIB_DEFINE_SPECIALIZED_MULTI_SAD(token, type)                                                                                                      \
	/** @brief Compares immediate-controlled multi-SAD Register code against its raw Api expression. */                                                        \
	SIMDLIB_REGISTER_ONLY SIMDLIB_SPECIALIZED_CODEGEN_NOINLINE SimdLibSpecializedCodegen::native_t<type> VECTORCALL                                            \
	simdlib_specialized_codegen_multi_sad_##token(SimdLibSpecializedCodegen::native_t<type> lhs, SimdLibSpecializedCodegen::native_t<type> rhs) noexcept       \
	{                                                                                                                                                          \
		return SIMDLIB_SPECIALIZED_MULTI_SAD_EXPRESSION(type, lhs, rhs);                                                                                       \
	}

#define SIMDLIB_DEFINE_SPECIALIZED_DOT(token, type)                                                                                                            \
	/** @brief Compares immediate-controlled dot-product Register code against its raw Api expression. */                                                      \
	SIMDLIB_REGISTER_ONLY SIMDLIB_SPECIALIZED_CODEGEN_NOINLINE SimdLibSpecializedCodegen::native_t<type> VECTORCALL                                            \
	simdlib_specialized_codegen_dot_product_##token(SimdLibSpecializedCodegen::native_t<type> lhs, SimdLibSpecializedCodegen::native_t<type> rhs) noexcept     \
	{                                                                                                                                                          \
		return SIMDLIB_SPECIALIZED_DOT_EXPRESSION(type, lhs, rhs);                                                                                             \
	}

#define SIMDLIB_FOR_EACH_SPECIALIZED_TYPE(macro, operation, member, api)                                                                                       \
	macro(operation, i8, std::int8_t, member, api) macro(operation, u8, std::uint8_t, member, api) macro(operation, i16, std::int16_t, member, api)            \
		macro(operation, u16, std::uint16_t, member, api) macro(operation, i32, std::int32_t, member, api) macro(operation, u32, std::uint32_t, member, api)   \
			macro(operation, i64, std::int64_t, member, api) macro(operation, u64, std::uint64_t, member, api) macro(operation, f32, float, member, api)       \
				macro(operation, f64, double, member, api)

#define SIMDLIB_FOR_EACH_SPECIALIZED_INTEGER(macro, operation, member, api)                                                                                    \
	macro(operation, i8, std::int8_t, member, api) macro(operation, u8, std::uint8_t, member, api) macro(operation, i16, std::int16_t, member, api)            \
		macro(operation, u16, std::uint16_t, member, api) macro(operation, i32, std::int32_t, member, api) macro(operation, u32, std::uint32_t, member, api)   \
			macro(operation, i64, std::int64_t, member, api) macro(operation, u64, std::uint64_t, member, api)

SIMDLIB_FOR_EACH_SPECIALIZED_TYPE(SIMDLIB_DEFINE_SPECIALIZED_BINARY, min, min, min)
SIMDLIB_FOR_EACH_SPECIALIZED_TYPE(SIMDLIB_DEFINE_SPECIALIZED_BINARY, max, max, max)
SIMDLIB_FOR_EACH_SPECIALIZED_TYPE(SIMDLIB_DEFINE_SPECIALIZED_UNARY, absolute, absolute, absolute)
SIMDLIB_FOR_EACH_SPECIALIZED_TYPE(SIMDLIB_DEFINE_SPECIALIZED_UNARY, sqrt, sqrt, sqrt)
SIMDLIB_FOR_EACH_SPECIALIZED_TYPE(SIMDLIB_DEFINE_SPECIALIZED_UNARY, magnitude, magnitude, magnitude)
SIMDLIB_FOR_EACH_SPECIALIZED_INTEGER(SIMDLIB_DEFINE_SPECIALIZED_UNARY, magnitude_checked, magnitude_checked, magnitude_checked)

SIMDLIB_DEFINE_SPECIALIZED_UNARY(normalize, f32, float, normalize, normalize)
SIMDLIB_DEFINE_SPECIALIZED_UNARY(normalize, f64, double, normalize, normalize)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(average, u8, std::uint8_t, average, avg)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(average, u16, std::uint16_t, average, avg)
SIMDLIB_DEFINE_SPECIALIZED_TERNARY(multiply_add, f32, float, multiply_add, multiply_add)
SIMDLIB_DEFINE_SPECIALIZED_TERNARY(multiply_add, f64, double, multiply_add, multiply_add)

SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_add, i16, std::int16_t, horizontal_add, add_horizontal)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_add, u16, std::uint16_t, horizontal_add, add_horizontal)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_add, i32, std::int32_t, horizontal_add, add_horizontal)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_add, u32, std::uint32_t, horizontal_add, add_horizontal)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_add, f32, float, horizontal_add, add_horizontal)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_add, f64, double, horizontal_add, add_horizontal)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_subtract, i16, std::int16_t, horizontal_subtract, subtract_horizontal)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_subtract, u16, std::uint16_t, horizontal_subtract, subtract_horizontal)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_subtract, i32, std::int32_t, horizontal_subtract, subtract_horizontal)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_subtract, u32, std::uint32_t, horizontal_subtract, subtract_horizontal)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_subtract, f32, float, horizontal_subtract, subtract_horizontal)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_subtract, f64, double, horizontal_subtract, subtract_horizontal)

SIMDLIB_DEFINE_SPECIALIZED_BINARY(add_saturated, i8, std::int8_t, add_saturated, add_saturated)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(add_saturated, u8, std::uint8_t, add_saturated, add_saturated)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(add_saturated, i16, std::int16_t, add_saturated, add_saturated)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(add_saturated, u16, std::uint16_t, add_saturated, add_saturated)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(subtract_saturated, i8, std::int8_t, subtract_saturated, subtract_saturated)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(subtract_saturated, u8, std::uint8_t, subtract_saturated, subtract_saturated)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(subtract_saturated, i16, std::int16_t, subtract_saturated, subtract_saturated)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(subtract_saturated, u16, std::uint16_t, subtract_saturated, subtract_saturated)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_add_saturated, i16, std::int16_t, horizontal_add_saturated, hadd_saturated)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_add_saturated, u16, std::uint16_t, horizontal_add_saturated, hadd_saturated)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_subtract_saturated, i16, std::int16_t, horizontal_subtract_saturated, hsubtract_saturated)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(horizontal_subtract_saturated, u16, std::uint16_t, horizontal_subtract_saturated, hsubtract_saturated)

SIMDLIB_DEFINE_SPECIALIZED_BINARY(add_subtract, f32, float, add_subtract, add_subtract)
SIMDLIB_DEFINE_SPECIALIZED_BINARY(add_subtract, f64, double, add_subtract, add_subtract)
SIMDLIB_DEFINE_SPECIALIZED_DOT(f32, float)
SIMDLIB_DEFINE_SPECIALIZED_DOT(f64, double)

SIMDLIB_FOR_EACH_SPECIALIZED_INTEGER(SIMDLIB_DEFINE_SPECIALIZED_SCALAR, min_position, min_position, min_position)
SIMDLIB_FOR_EACH_SPECIALIZED_INTEGER(SIMDLIB_DEFINE_SPECIALIZED_SCALAR, max_position, max_position, max_position)
SIMDLIB_FOR_EACH_SPECIALIZED_INTEGER(SIMDLIB_DEFINE_SPECIALIZED_PROMOTED, multiply_add_adjacent, multiply_add_adjacent, multiply_add_adjacent)
SIMDLIB_FOR_EACH_SPECIALIZED_INTEGER(SIMDLIB_DEFINE_SPECIALIZED_PROMOTED, byte_multiply_add, multiply_add_unsigned_signed_bytes,
									 multiply_add_unsigned_signed_bytes)
SIMDLIB_FOR_EACH_SPECIALIZED_INTEGER(SIMDLIB_DEFINE_SPECIALIZED_PROMOTED, sum_absolute_byte_differences, sum_absolute_byte_differences,
									 sum_absolute_byte_differences)

SIMDLIB_DEFINE_SPECIALIZED_MULTI_SAD(i8, std::int8_t)
SIMDLIB_DEFINE_SPECIALIZED_MULTI_SAD(u8, std::uint8_t)
SIMDLIB_DEFINE_SPECIALIZED_MULTI_SAD(i16, std::int16_t)
SIMDLIB_DEFINE_SPECIALIZED_MULTI_SAD(u16, std::uint16_t)
SIMDLIB_DEFINE_SPECIALIZED_MULTI_SAD(i32, std::int32_t)
SIMDLIB_DEFINE_SPECIALIZED_MULTI_SAD(u32, std::uint32_t)
SIMDLIB_DEFINE_SPECIALIZED_MULTI_SAD(i64, std::int64_t)
SIMDLIB_DEFINE_SPECIALIZED_MULTI_SAD(u64, std::uint64_t)

#undef SIMDLIB_FOR_EACH_SPECIALIZED_INTEGER
#undef SIMDLIB_FOR_EACH_SPECIALIZED_TYPE
#undef SIMDLIB_DEFINE_SPECIALIZED_DOT
#undef SIMDLIB_DEFINE_SPECIALIZED_MULTI_SAD
#undef SIMDLIB_DEFINE_SPECIALIZED_PROMOTED
#undef SIMDLIB_DEFINE_SPECIALIZED_SCALAR
#undef SIMDLIB_DEFINE_SPECIALIZED_TERNARY
#undef SIMDLIB_DEFINE_SPECIALIZED_BINARY
#undef SIMDLIB_DEFINE_SPECIALIZED_UNARY
#undef SIMDLIB_SPECIALIZED_DOT_EXPRESSION
#undef SIMDLIB_SPECIALIZED_MULTI_SAD_EXPRESSION
#undef SIMDLIB_SPECIALIZED_PROMOTED_EXPRESSION
#undef SIMDLIB_SPECIALIZED_SCALAR_EXPRESSION
#undef SIMDLIB_SPECIALIZED_TERNARY_EXPRESSION
#undef SIMDLIB_SPECIALIZED_BINARY_EXPRESSION
#undef SIMDLIB_SPECIALIZED_UNARY_EXPRESSION
#undef SIMDLIB_SPECIALIZED_CODEGEN_NOINLINE