#pragma once

#include <SimdLib/Register.h>

#if SIMDLIB_COMPILER_MSVC
#define SIMDLIB_FMA_CODEGEN_NOINLINE __declspec(noinline)
#else
#define SIMDLIB_FMA_CODEGEN_NOINLINE __attribute__((noinline))
#endif

namespace SimdLibFmaCodegen
{

/** @brief Native single-precision register used by the isolated FMA fixture. */
using float_native_t = typename SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, float>::vector_t;

/** @brief Native double-precision register used by the isolated FMA fixture. */
using double_native_t = typename SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, double>::vector_t;

} // namespace SimdLibFmaCodegen

/**
 * @brief Compares single-precision Register multiply-add against the raw Api expression.
 * @param lhs Multiplicand register.
 * @param rhs Multiplier register.
 * @param addend Addend register.
 * @return Per-lane multiply-add result.
 */
SIMDLIB_FMA_CODEGEN_NOINLINE SimdLibFmaCodegen::float_native_t SIMD_FLAGS(Neither, RegisterOnly)
	simdlib_fma_codegen_multiply_add_f32(SimdLibFmaCodegen::float_native_t lhs, SimdLibFmaCodegen::float_native_t rhs,
										 SimdLibFmaCodegen::float_native_t addend) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLib::Register<float, SIMDLIB_REGISTER_TEST_WIDTH>{lhs}
		.multiply_add(SimdLib::Register<float, SIMDLIB_REGISTER_TEST_WIDTH>{rhs}, SimdLib::Register<float, SIMDLIB_REGISTER_TEST_WIDTH>{addend})
		.native;
#else
	return SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, float>::multiply_add(lhs, rhs, addend);
#endif
}

/**
 * @brief Compares double-precision Register multiply-add against the raw Api expression.
 * @param lhs Multiplicand register.
 * @param rhs Multiplier register.
 * @param addend Addend register.
 * @return Per-lane multiply-add result.
 */
SIMDLIB_FMA_CODEGEN_NOINLINE SimdLibFmaCodegen::double_native_t SIMD_FLAGS(Neither, RegisterOnly)
	simdlib_fma_codegen_multiply_add_f64(SimdLibFmaCodegen::double_native_t lhs, SimdLibFmaCodegen::double_native_t rhs,
										 SimdLibFmaCodegen::double_native_t addend) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLib::Register<double, SIMDLIB_REGISTER_TEST_WIDTH>{lhs}
		.multiply_add(SimdLib::Register<double, SIMDLIB_REGISTER_TEST_WIDTH>{rhs}, SimdLib::Register<double, SIMDLIB_REGISTER_TEST_WIDTH>{addend})
		.native;
#else
	return SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, double>::multiply_add(lhs, rhs, addend);
#endif
}

#undef SIMDLIB_FMA_CODEGEN_NOINLINE