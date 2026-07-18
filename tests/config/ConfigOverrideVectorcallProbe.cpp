#define VECTORCALL
#define SIMDLIB_VECTORCALL_ENABLED 0
#include <SimdLib/Config.h>

static_assert(!SimdLib::Config::vectorcall_enabled);

int VECTORCALL ConfigOverrideVectorcallProbe() noexcept
{
	return 0;
}
