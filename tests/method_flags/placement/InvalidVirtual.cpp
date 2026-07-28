#include <SimdLib/Config.h>

/// Supplies a prohibited virtual declaration shape.
struct InvalidVirtual
{
	/// Uses method flags on a virtual function.
	virtual int SIMD_FLAGS(Neither) value() const noexcept = 0;
};
