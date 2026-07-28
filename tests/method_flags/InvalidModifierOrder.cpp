#define SIMDLIB_PRECONDITION(condition, message)
#include <SimdLib/Config.h>

/// Declares a function whose modifiers use a noncanonical order.
int SIMD_FLAGS(InOut, Flatten, ForceInline) invalid_modifier_order();
