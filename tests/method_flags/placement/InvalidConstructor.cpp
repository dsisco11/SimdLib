#include "MethodFlagsPlacementFixture.h"

/// Exercises the prohibited constructor declaration category.
struct InvalidFlaggedConstructor final
{
	SIMD_FLAGS(Neither) InvalidFlaggedConstructor() noexcept;
};
