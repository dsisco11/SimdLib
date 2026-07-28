#include "MethodFlagsPrototype.h"

/// Declares a function whose invocation omits the required boundary mode.
SIMD_FLAGS(RegisterOnly)
int invalid_missing_boundary();
