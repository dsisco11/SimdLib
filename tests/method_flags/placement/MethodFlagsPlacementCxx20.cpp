#include "MethodFlagsPlacementFixture.h"

namespace SimdLibMethodFlagsPlacement
{
static_assert(inline_increment(1) == 2);
static_assert(constrained_increment(2) == 3);
static_assert(trailing_increment(3) == 4);

/// Instantiates every supported C++20 declaration shape.
[[nodiscard]] vector_type SIMD_FLAGS(InOut, RegisterOnly) exercise_cxx20(vector_type value) noexcept
{
	MemberShapes members;
	const auto member_result = members.member_transform(value);
	const auto static_result = MemberShapes::static_transform(member_result);
	const auto boxed_result = VectorBox{static_result} + VectorBox{value};
	return free_transform(boxed_result.value);
}
} // namespace SimdLibMethodFlagsPlacement
