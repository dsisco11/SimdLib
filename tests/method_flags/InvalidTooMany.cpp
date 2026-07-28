#include "MethodFlagsPrototype.h"

/// Declares a function with too many method-flags arguments.
SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten, Extra)
int invalid_too_many();
