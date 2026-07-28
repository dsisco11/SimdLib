#include "MethodFlagsPlacementFixture.h"

namespace SimdLibMethodFlagsPlacement
{
/// Defines a flagged declaration with the legacy spelling in another translation unit.
SIMDLIB_REGISTER_ONLY vector_type VECTORCALL flagged_abi(vector_type value) noexcept
{
	return value;
}

/// Defines a legacy declaration with the flagged pre-name spelling.
vector_type SIMD_FLAGS(InOut, RegisterOnly) legacy_abi(vector_type value) noexcept
{
	return value;
}
} // namespace SimdLibMethodFlagsPlacement
