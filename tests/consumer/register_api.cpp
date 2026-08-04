#include "register_api.h"

namespace SimdLibConsumer
{
/** Defines the downstream Register boundary in a separate translation unit. */
Register SIMD_FLAGS(InOut, RegisterOnly) increment(Register value) noexcept
{
	return value + Register::broadcast(1);
}

/** Defines the downstream native-SIMD boundary in a separate translation unit. */
native_type SIMD_FLAGS(InOut, RegisterOnly) increment_native(native_type value) noexcept
{
	return _mm_add_epi32(value, _mm_set1_epi32(1));
}
} // namespace SimdLibConsumer
