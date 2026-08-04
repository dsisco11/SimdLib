#include "InstalledHeaderOdrFixture.h"

/**
 * @brief Defines the copied-header cross-translation-unit fixture.
 * @param value Scalar value transformed by the fixture.
 * @return The supplied value incremented by one.
 */
int SIMD_FLAGS(Neither) installed_header_odr_value(const int value) noexcept
{
	return value + 1;
}
