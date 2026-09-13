#include "fixture.h"

namespace SimdLibPartialRegisterOdr
{

/** Defines the cross-translation-unit active-prefix addition boundary. */
Register SIMD_FLAGS(InOut, RegisterOnly) add(Register lhs, Register rhs) noexcept
{
	return lhs + rhs;
}

/** Defines the cross-translation-unit active-prefix comparison boundary. */
RegisterMask SIMD_FLAGS(InOut, RegisterOnly) equal(Register lhs, Register rhs) noexcept
{
	return lhs.compare_equal(rhs);
}

} // namespace SimdLibPartialRegisterOdr
