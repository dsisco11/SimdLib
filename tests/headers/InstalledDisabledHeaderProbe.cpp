#include <SimdLib/SimdLib.h>

/**
 * @brief Exercises method flags when every optional instruction family is disabled.
 * @param value Scalar value returned unchanged.
 * @return The supplied scalar value.
 */
int SIMD_FLAGS(Neither, RegisterOnly) installed_disabled_identity(const int value) noexcept
{
	return value;
}

static_assert(!SimdLib::is_api_available_v<128, unsigned>);
static_assert(!SimdLib::is_api_available_v<256, unsigned>);
