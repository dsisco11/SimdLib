#define SIMDLIB_PRECONDITION(condition, message)
#include <SimdLib/Config.h>

/// Declares a function with too many method-flags arguments.
int SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten, Extra) invalid_too_many();
