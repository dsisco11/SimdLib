#define SIMDLIB_VECTORCALL_ENABLED 0
#include <SimdLib/Config.h>

#include <immintrin.h>

static_assert(!SimdLib::Config::vectorcall_enabled);
static_assert(!SimdLib::Config::method_flags_has_vectorcall);

/** @brief Exercises Out while the vector calling-convention mapping is disabled. */
[[nodiscard]] __m128 SIMD_FLAGS(Out) MethodFlagsDisabledVectorcallOut() noexcept
{
	return _mm_setzero_ps();
}

/** @brief Exercises In while the vector calling-convention mapping is disabled. */
[[nodiscard]] float SIMD_FLAGS(In) MethodFlagsDisabledVectorcallIn(const __m128 value) noexcept
{
	return _mm_cvtss_f32(value);
}

/** @brief Exercises InOut while the vector calling-convention mapping is disabled. */
[[nodiscard]] __m128 SIMD_FLAGS(InOut) MethodFlagsDisabledVectorcallInOut(const __m128 value) noexcept
{
	return value;
}

/** @brief Instantiates every disabled-vectorcall boundary declaration. */
int MethodFlagsConfigDisabledVectorcallProbe() noexcept
{
	return static_cast<int>(MethodFlagsDisabledVectorcallIn(MethodFlagsDisabledVectorcallInOut(MethodFlagsDisabledVectorcallOut())));
}
