#pragma once

#include <SimdLib/Config.h>

#include <concepts>
#include <immintrin.h>
#include <type_traits>

namespace SimdLibMethodFlagsPlacement
{
using vector_type = __m128;

/// Returns a native SIMD value through the fully composed declaration macro.
[[nodiscard]] vector_type SIMD_FLAGS(InOut, RegisterOnly, ForceInline) leaf_transform(vector_type value) noexcept
{
	return value;
}

/// Calls another flagged function so the flatten attribute has a real callee.
[[nodiscard]] vector_type SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) free_transform(vector_type value) noexcept
{
	return leaf_transform(value);
}

/// Exercises an ordinary inline specifier independently of ForceInline.
[[nodiscard]] inline constexpr int SIMD_FLAGS(Neither, RegisterOnly) inline_increment(int value) noexcept
{
	return value + 1;
}

/// Exercises constexpr, ForceInline, Flatten, and a leading requires clause.
template <typename value_type>
	requires std::integral<value_type>
[[nodiscard]] constexpr value_type SIMD_FLAGS(Neither, RegisterOnly, ForceInline, Flatten) constrained_increment(value_type value) noexcept
{
	return static_cast<value_type>(value + 1);
}

/// Exercises an independently selected trailing return type.
template <typename value_type>
[[nodiscard]] constexpr auto SIMD_FLAGS(Neither, RegisterOnly, ForceInline, Flatten) trailing_increment(value_type value) noexcept -> value_type
	requires std::integral<value_type>
{
	return static_cast<value_type>(value + 1);
}

/// Provides static and non-static member declaration shapes.
class MemberShapes final
{
  public:
	/// Returns a native value from a static member.
	[[nodiscard]] static vector_type SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) static_transform(vector_type value) noexcept
	{
		return leaf_transform(value);
	}

	/// Returns a native value from a non-static member.
	[[nodiscard]] vector_type SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) member_transform(vector_type value) const noexcept
	{
		return leaf_transform(value);
	}
};

/// Wraps a native SIMD value for friend-definition and operator coverage.
struct VectorBox final
{
	vector_type value;

	/// Selects the left operand through a friend operator definition.
	[[nodiscard]] friend VectorBox SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator+(VectorBox lhs, VectorBox rhs) noexcept
	{
		(void)rhs;
		return lhs;
	}
};

/// Declares the canonical InOut spelling for cross-TU ABI verification.
[[nodiscard]] vector_type SIMD_FLAGS(InOut, RegisterOnly) flagged_abi(vector_type value) noexcept;

/// Declares the legacy InOut calling-convention position for type comparison.
[[nodiscard]] SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS vector_type SIMDLIB_METHOD_FLAGS_VECTORCALL legacy_abi(vector_type value) noexcept;

/// Declares the canonical In spelling for cross-TU ABI verification.
[[nodiscard]] int SIMD_FLAGS(In, RegisterOnly) flagged_in_abi(vector_type value) noexcept;

/// Declares the legacy In calling-convention position for type comparison.
[[nodiscard]] SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS int SIMDLIB_METHOD_FLAGS_VECTORCALL legacy_in_abi(vector_type value) noexcept;

/// Declares the canonical Out spelling for cross-TU ABI verification.
[[nodiscard]] vector_type SIMD_FLAGS(Out, RegisterOnly) flagged_out_abi(float value) noexcept;

/// Declares the legacy Out calling-convention position for type comparison.
[[nodiscard]] SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS vector_type SIMDLIB_METHOD_FLAGS_VECTORCALL legacy_out_abi(float value) noexcept;

using flagged_callback = decltype(&flagged_abi);
using legacy_callback = decltype(&legacy_abi);
using flagged_in_callback = decltype(&flagged_in_abi);
using legacy_in_callback = decltype(&legacy_in_abi);
using flagged_out_callback = decltype(&flagged_out_abi);
using legacy_out_callback = decltype(&legacy_out_abi);

inline constexpr flagged_callback flagged_address = &flagged_abi;
inline constexpr legacy_callback legacy_address = &legacy_abi;
/// Proves the flagged declaration is directly assignable to the legacy callback type.
inline constexpr legacy_callback compatible_flagged_address = flagged_address;
/// Proves the flagged In declaration is directly assignable to the legacy callback type.
inline constexpr legacy_in_callback compatible_flagged_in_address = &flagged_in_abi;
/// Proves the flagged Out declaration is directly assignable to the legacy callback type.
inline constexpr legacy_out_callback compatible_flagged_out_address = &flagged_out_abi;
} // namespace SimdLibMethodFlagsPlacement
