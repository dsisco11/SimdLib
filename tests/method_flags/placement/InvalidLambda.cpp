#include "MethodFlagsPlacementFixture.h"

/// Exercises the prohibited lambda declaration category.
inline constexpr auto invalid_flagged_lambda = [] SIMD_FLAGS(Neither)() noexcept -> int { return 0; };
