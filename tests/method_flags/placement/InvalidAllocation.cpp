#include <SimdLib/Config.h>

#include <cstddef>

/// Supplies a prohibited allocation-function declaration shape.
struct InvalidAllocation
{
	/// Uses method flags on an allocation function.
	static void *SIMD_FLAGS(Neither) operator new(std::size_t size);
};
