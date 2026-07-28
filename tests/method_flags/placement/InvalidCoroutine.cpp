#include <SimdLib/Config.h>

/// Uses method flags on a coroutine-shaped definition.
int SIMD_FLAGS(Neither) invalid_coroutine()
{
	co_return 0;
}
