#include "MethodFlagsPlacementFixture.h"

/// Exercises source-audit rejection of an immediate-only function.
[[nodiscard]] consteval int SIMD_FLAGS(Neither, RegisterOnly) invalid_consteval(int value) noexcept
{
	return value;
}
