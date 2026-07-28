#define SIMDLIB_PRECONDITION(condition, message)
#include <SimdLib/Config.h>

#define In downstream_object_macro

/// Declares a function whose boundary mode collides with an object-like macro.
int SIMD_FLAGS(In) invalid_object_macro_collision();
