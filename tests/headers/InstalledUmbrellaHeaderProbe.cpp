#include <SimdLib/SimdLib.h>

/**
 * @brief Exercises the umbrella header and public declaration macro together.
 * @param value Scalar value returned unchanged.
 * @return The supplied scalar value.
 */
int SIMD_FLAGS(Neither) installed_umbrella_identity(const int value) noexcept
{
	return value;
}

#if SIMDLIB_HAS_SSE42
static_assert(SimdLib::Api<128, unsigned>::byte_count == 16);
#else
static_assert(!SimdLib::is_api_available_v<128, unsigned>);
#endif
