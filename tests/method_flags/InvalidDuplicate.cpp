#define SIMDLIB_PRECONDITION(condition, message)
#include <SimdLib/Config.h>

/// Declares a function with a duplicate method-flags modifier.
int SIMD_FLAGS(InOut, RegisterOnly, RegisterOnly) invalid_duplicate();
