#include <SimdLib/Config.h>

/**
 * @brief Exercises the public method-flags parser from an isolated header image.
 * @param value Scalar value returned unchanged.
 * @return The supplied scalar value.
 */
int SIMD_FLAGS(Neither, ForceInline) installed_config_identity(const int value) noexcept
{
	return value;
}

static_assert(SimdLib::Config::version_major == SimdLib::version_major);
