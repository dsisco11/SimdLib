#include "MethodFlagsPrototype.h"

#define In downstream_object_macro

/// Declares a function whose boundary mode collides with an object-like macro.
SIMD_FLAGS(In)
int invalid_object_macro_collision();
