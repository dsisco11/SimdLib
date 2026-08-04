#define SIMDLIB_PRECONDITION(condition, message)
#include <SimdLib/Config.h>

/// Declares a function whose invocation omits the required boundary mode.
int SIMD_FLAGS(RegisterOnly) invalid_missing_boundary();
