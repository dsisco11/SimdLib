#include "MethodFlagsPrototype.h"

/// Declares a function with a duplicate method-flags modifier.
SIMD_FLAGS(InOut, RegisterOnly, RegisterOnly)
int invalid_duplicate();
