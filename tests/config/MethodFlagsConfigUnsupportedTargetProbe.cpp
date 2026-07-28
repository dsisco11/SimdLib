#define SIMDLIB_COMPILER_CLANG 0
#define SIMDLIB_COMPILER_MSVC 0
#define SIMDLIB_COMPILER_GCC 0
#define SIMDLIB_TARGET_X86 0
#define SIMDLIB_TARGET_X64 0
#define SIMDLIB_VECTORCALL_ENABLED 0
#include <SimdLib/Config.h>

static_assert(!SimdLib::Config::target_x86);
static_assert(!SimdLib::Config::target_x64);
static_assert(!SimdLib::Config::method_flags_has_vectorcall);
static_assert(!SimdLib::Config::method_flags_has_safe_buffers);
static_assert(!SimdLib::Config::method_flags_has_force_inline);
static_assert(!SimdLib::Config::method_flags_has_flatten);

/** @brief Exercises every semantic flag when compiler mappings are unavailable. */
[[nodiscard]] int SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) MethodFlagsConfigUnsupportedTargetProbe(const int value) noexcept
{
	return value;
}
