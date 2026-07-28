#include "MethodFlagsPlacementFixture.h"

/// Exercises the prohibited conversion-operator declaration category.
struct InvalidFlaggedConversion final
{
	SIMD_FLAGS(Out) operator SimdLibMethodFlagsPlacement::vector_type() const noexcept;
};
