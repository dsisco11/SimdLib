#include <SimdLib/Config.h>

/// Supplies a class template for a prohibited flagged deduction guide.
template <typename value_type> struct InvalidDeductionGuide
{
	value_type value;
};

/// Uses method flags on a deduction guide, which has no independent return type.
SIMD_FLAGS(Neither) InvalidDeductionGuide(int) -> InvalidDeductionGuide<int>;
