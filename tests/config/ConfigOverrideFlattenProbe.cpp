#define SIMDLIB_FLATTEN
#include <SimdLib/Config.h>

/** @brief Exercises a caller-provided empty recursive-inlining annotation. */
SIMDLIB_FLATTEN int ConfigOverrideFlattenProbe() noexcept
{
	return 0;
}
