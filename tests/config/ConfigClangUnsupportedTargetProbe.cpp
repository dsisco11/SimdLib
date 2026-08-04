#define SIMDLIB_COMPILER_CLANG 1
#define SIMDLIB_COMPILER_MSVC 0
#define SIMDLIB_COMPILER_GCC 0
#define SIMDLIB_TARGET_X86 0
#include <SimdLib/Config.h>

static_assert(!SimdLib::Config::target_x86);
static_assert(!SimdLib::Config::vectorcall_enabled);

int SIMD_FLAGS(Neither) ConfigClangUnsupportedTargetProbe() noexcept
{
	return 0;
}
