#include <SimdLib/Config.h>

#include <immintrin.h>

/** @brief Exercises the default Out boundary on a SIMD load. */
[[nodiscard]] __m128 SIMD_FLAGS(Out) MethodFlagsDefaultLoad(const float *source) noexcept
{
	return _mm_loadu_ps(source);
}

/** @brief Exercises the default In boundary on a memory-writing SIMD store. */
void SIMD_FLAGS(In) MethodFlagsDefaultStore(const __m128 value, float *destination) noexcept
{
	_mm_storeu_ps(destination, value);
}

/** @brief Exercises an In reduction with the independent RegisterOnly promise. */
[[nodiscard]] float SIMD_FLAGS(In, RegisterOnly) MethodFlagsDefaultReduce(const __m128 value) noexcept
{
	return _mm_cvtss_f32(value);
}

/** @brief Exercises the complete default InOut modifier composition. */
[[nodiscard]] __m128 SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) MethodFlagsDefaultTransform(const __m128 value) noexcept
{
	return value;
}

/** @brief Exercises an attribute-free scalar boundary. */
[[nodiscard]] int SIMD_FLAGS(Neither) MethodFlagsDefaultScalar(const int value) noexcept
{
	return value;
}

static_assert(SimdLib::Config::method_flags_has_vectorcall == (SIMDLIB_METHOD_FLAGS_HAS_VECTORCALL != 0));
static_assert(SimdLib::Config::method_flags_has_safe_buffers == (SIMDLIB_METHOD_FLAGS_HAS_SAFE_BUFFERS != 0));
static_assert(SimdLib::Config::method_flags_has_force_inline == (SIMDLIB_METHOD_FLAGS_HAS_FORCE_INLINE != 0));
static_assert(SimdLib::Config::method_flags_has_flatten == (SIMDLIB_METHOD_FLAGS_HAS_FLATTEN != 0));

#if SIMDLIB_COMPILER_MSVC
static_assert(SimdLib::Config::method_flags_has_vectorcall);
static_assert(SimdLib::Config::method_flags_has_safe_buffers);
#endif

#if SIMDLIB_COMPILER_CLANG && defined(_WIN32)
static_assert(SimdLib::Config::method_flags_has_vectorcall);
static_assert(!SimdLib::Config::method_flags_has_safe_buffers);
#endif

#if SIMDLIB_COMPILER_GCC || (SIMDLIB_COMPILER_CLANG && !defined(_WIN32))
static_assert(!SimdLib::Config::method_flags_has_vectorcall);
static_assert(!SimdLib::Config::method_flags_has_safe_buffers);
#endif

#if SIMDLIB_COMPILER_MSVC || SIMDLIB_COMPILER_CLANG || SIMDLIB_COMPILER_GCC
static_assert(SimdLib::Config::method_flags_has_force_inline);
static_assert(SimdLib::Config::method_flags_has_flatten);
#endif

/** @brief Instantiates the default method-flags probe functions. */
int MethodFlagsConfigDefaultProbe() noexcept
{
	alignas(16) float values[4]{};
	const __m128 loaded = MethodFlagsDefaultLoad(values);
	MethodFlagsDefaultStore(MethodFlagsDefaultTransform(loaded), values);
	return MethodFlagsDefaultScalar(static_cast<int>(MethodFlagsDefaultReduce(loaded)));
}
