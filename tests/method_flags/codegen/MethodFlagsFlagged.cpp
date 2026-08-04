#include <SimdLib/Config.h>

#include <immintrin.h>

#if defined(_MSC_VER)
#define SIMDLIB_METHOD_FLAGS_NOINLINE __declspec(noinline)
#else
#define SIMDLIB_METHOD_FLAGS_NOINLINE __attribute__((noinline))
#endif

namespace SimdLibMethodFlagsCodegen
{
/** Returns the square root of every input lane. */
SIMDLIB_METHOD_FLAGS_NOINLINE __m128 SIMD_FLAGS(InOut, RegisterOnly) simdlib_method_flags_codegen_unary(__m128 value) noexcept
{
	return _mm_sqrt_ps(value);
}

/** Adds corresponding lanes from two input registers. */
SIMDLIB_METHOD_FLAGS_NOINLINE __m128 SIMD_FLAGS(InOut, RegisterOnly) simdlib_method_flags_codegen_binary(__m128 lhs, __m128 rhs) noexcept
{
	return _mm_add_ps(lhs, rhs);
}

/** Multiplies two registers and adds a third register. */
SIMDLIB_METHOD_FLAGS_NOINLINE __m128 SIMD_FLAGS(InOut, RegisterOnly) simdlib_method_flags_codegen_ternary(__m128 lhs, __m128 rhs, __m128 addend) noexcept
{
	return _mm_add_ps(_mm_mul_ps(lhs, rhs), addend);
}

/** Extracts the low scalar lane from a register. */
SIMDLIB_METHOD_FLAGS_NOINLINE float SIMD_FLAGS(In, RegisterOnly) simdlib_method_flags_codegen_scalar_result(__m128 value) noexcept
{
	return _mm_cvtss_f32(value);
}

/** Broadcasts a scalar into a native register result. */
SIMDLIB_METHOD_FLAGS_NOINLINE __m128 SIMD_FLAGS(Out, RegisterOnly) simdlib_method_flags_codegen_register_result(float value) noexcept
{
	return _mm_set1_ps(value);
}

/** Loads an unaligned native register without writing through the source pointer. */
SIMDLIB_METHOD_FLAGS_NOINLINE __m128 SIMD_FLAGS(Out, RegisterOnly) simdlib_method_flags_codegen_load(const float *source) noexcept
{
	return _mm_loadu_ps(source);
}

/** Stores a native register through a caller-owned pointer. */
SIMDLIB_METHOD_FLAGS_NOINLINE void SIMD_FLAGS(In) simdlib_method_flags_codegen_store(float *destination, __m128 value) noexcept
{
	_mm_storeu_ps(destination, value);
}

/** Provides a small leaf for the force-inline-only fixture. */
__m128 SIMD_FLAGS(InOut, RegisterOnly, ForceInline) simdlib_method_flags_force_leaf(__m128 value) noexcept
{
	return _mm_add_ps(value, _mm_set1_ps(1.0F));
}

/** Exercises ForceInline independently of Flatten. */
SIMDLIB_METHOD_FLAGS_NOINLINE __m128 SIMD_FLAGS(InOut, RegisterOnly) simdlib_method_flags_codegen_forceinline(__m128 value) noexcept
{
	return simdlib_method_flags_force_leaf(value);
}

/** Provides a small leaf for the flatten-only fixture. */
inline __m128 SIMD_FLAGS(InOut, RegisterOnly) simdlib_method_flags_flatten_leaf(__m128 value) noexcept
{
	return _mm_mul_ps(value, value);
}

/** Exercises Flatten independently of ForceInline. */
SIMDLIB_METHOD_FLAGS_NOINLINE __m128 SIMD_FLAGS(InOut, RegisterOnly, Flatten) simdlib_method_flags_codegen_flatten(__m128 value) noexcept
{
	return simdlib_method_flags_flatten_leaf(simdlib_method_flags_flatten_leaf(value));
}
} // namespace SimdLibMethodFlagsCodegen
