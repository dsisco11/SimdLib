#define SIMDLIB_FORCE_INLINE inline
#include <SimdLib/Config.h>

SIMDLIB_FORCE_INLINE int ConfigOverrideForceInlineProbe() noexcept
{
	return 0;
}
