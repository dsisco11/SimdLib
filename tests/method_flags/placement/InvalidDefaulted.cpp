#include <SimdLib/Config.h>

/// Supplies a prohibited defaulted-function declaration shape.
struct InvalidDefaulted
{
	/// Uses method flags on a defaulted comparison function.
	bool SIMD_FLAGS(Neither) operator==(const InvalidDefaulted &) const = default;
};
