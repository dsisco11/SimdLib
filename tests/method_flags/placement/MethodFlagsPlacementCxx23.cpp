#include "MethodFlagsPlacementFixture.h"

namespace SimdLibMethodFlagsPlacement
{
/// Provides explicit-object member and operator declaration shapes.
struct ExplicitObject final
{
	vector_type value;

	/// Returns a native value through a by-value explicit object parameter.
	[[nodiscard]] vector_type SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) transform(this ExplicitObject self, vector_type rhs) noexcept
	{
		(void)rhs;
		return leaf_transform(self.value);
	}

	/// Adds an explicit-object operator declaration shape.
	[[nodiscard]] ExplicitObject SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator+(this ExplicitObject lhs, ExplicitObject rhs) noexcept
	{
		(void)rhs;
		return lhs;
	}
};

/// Instantiates the supported explicit-object declarations.
[[nodiscard]] vector_type SIMD_FLAGS(InOut, RegisterOnly) exercise_cxx23(vector_type value) noexcept
{
	const auto object = ExplicitObject{value} + ExplicitObject{value};
	return object.transform(value);
}
} // namespace SimdLibMethodFlagsPlacement
