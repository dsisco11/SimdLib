#define SIMDLIB_METHOD_FLAGS_HAS_VECTORCALL 0
#define SIMDLIB_METHOD_FLAGS_HAS_SAFE_BUFFERS 1
#define SIMDLIB_METHOD_FLAGS_HAS_FORCE_INLINE 0
#define SIMDLIB_METHOD_FLAGS_HAS_FLATTEN 1
#define SIMDLIB_METHOD_FLAGS_VECTORCALL
#define SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS
#define SIMDLIB_METHOD_FLAGS_FORCE_INLINE inline
#define SIMDLIB_METHOD_FLAGS_FLATTEN
#include <SimdLib/Config.h>

static_assert(!SimdLib::Config::method_flags_has_vectorcall);
static_assert(SimdLib::Config::method_flags_has_safe_buffers);
static_assert(!SimdLib::Config::method_flags_has_force_inline);
static_assert(SimdLib::Config::method_flags_has_flatten);

/** @brief Exercises all caller-provided method-flags adapter definitions. */
[[nodiscard]] int SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) MethodFlagsConfigOverrideProbe(const int value) noexcept
{
	return value;
}
