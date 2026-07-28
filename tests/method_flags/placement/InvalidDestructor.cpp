#include <SimdLib/Config.h>

/// Supplies a prohibited destructor declaration shape.
struct InvalidDestructor
{
	/// Uses method flags on a destructor, which has no independent return type.
	SIMD_FLAGS(Neither) ~InvalidDestructor() noexcept;
};
