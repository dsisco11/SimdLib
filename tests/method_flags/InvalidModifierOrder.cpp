#include "MethodFlagsPrototype.h"

/// Declares a function whose modifiers use a noncanonical order.
SIMD_FLAGS(InOut, Flatten, ForceInline)
int invalid_modifier_order();
