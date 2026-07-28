#include "MethodFlagsPlacementFixture.h"

/// Links and executes both declaration spellings through their derived types.
int main()
{
	const auto input = _mm_set1_ps(7.0F);
	const auto flagged = SimdLibMethodFlagsPlacement::compatible_flagged_address(input);
	const auto legacy = SimdLibMethodFlagsPlacement::legacy_address(input);
	const auto flagged_in = SimdLibMethodFlagsPlacement::compatible_flagged_in_address(input);
	const auto legacy_in = SimdLibMethodFlagsPlacement::legacy_in_abi(input);
	const auto flagged_out = SimdLibMethodFlagsPlacement::compatible_flagged_out_address(7.0F);
	const auto legacy_out = SimdLibMethodFlagsPlacement::legacy_out_abi(7.0F);
	return _mm_cvtss_f32(flagged) == _mm_cvtss_f32(legacy) && flagged_in == legacy_in && _mm_cvtss_f32(flagged_out) == _mm_cvtss_f32(legacy_out) ? 0 : 1;
}
