#include "MethodFlagsPlacementFixture.h"

/// Links and executes both declaration spellings through their derived types.
int main()
{
	const auto input = _mm_set1_ps(7.0F);
	const auto flagged = SimdLibMethodFlagsPlacement::compatible_flagged_address(input);
	const auto legacy = SimdLibMethodFlagsPlacement::legacy_address(input);
	return _mm_cvtss_f32(flagged) == _mm_cvtss_f32(legacy) ? 0 : 1;
}
