#include "MethodFlagsPlacementFixture.h"

namespace SimdLibMethodFlagsPlacement
{
/// Defines a flagged declaration with the legacy spelling in another translation unit.
SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS vector_type SIMDLIB_METHOD_FLAGS_VECTORCALL flagged_abi(vector_type value) noexcept
{
	return value;
}

/// Defines a legacy declaration with the flagged pre-name spelling.
vector_type SIMD_FLAGS(InOut, RegisterOnly) legacy_abi(vector_type value) noexcept
{
	return value;
}

/// Defines a flagged In declaration with the legacy spelling.
SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS int SIMDLIB_METHOD_FLAGS_VECTORCALL flagged_in_abi(vector_type value) noexcept
{
	return static_cast<int>(_mm_cvtss_f32(value));
}

/// Defines a legacy In declaration with the flagged pre-name spelling.
int SIMD_FLAGS(In, RegisterOnly) legacy_in_abi(vector_type value) noexcept
{
	return static_cast<int>(_mm_cvtss_f32(value));
}

/// Defines a flagged Out declaration with the legacy spelling.
SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS vector_type SIMDLIB_METHOD_FLAGS_VECTORCALL flagged_out_abi(float value) noexcept
{
	return _mm_set1_ps(value);
}

/// Defines a legacy Out declaration with the flagged pre-name spelling.
vector_type SIMD_FLAGS(Out, RegisterOnly) legacy_out_abi(float value) noexcept
{
	return _mm_set1_ps(value);
}
} // namespace SimdLibMethodFlagsPlacement
