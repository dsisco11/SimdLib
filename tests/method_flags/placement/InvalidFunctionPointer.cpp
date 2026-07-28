#include "MethodFlagsPlacementFixture.h"

/// Exercises source-audit rejection of flags inside an explicit pointer type.
using invalid_flagged_callback = SimdLibMethodFlagsPlacement::vector_type SIMD_FLAGS(InOut) (*)(SimdLibMethodFlagsPlacement::vector_type);
