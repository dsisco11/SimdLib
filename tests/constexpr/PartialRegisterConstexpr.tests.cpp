#include <SimdLib/PartialRegister.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace
{

/** @brief Reports whether one scalar value has an all-bits-zero representation. */
template <class element_t> [[nodiscard]] consteval bool has_zero_bits(element_t value) noexcept
{
	const auto bytes = std::bit_cast<std::array<std::byte, sizeof(element_t)>>(value);
	for (const auto byte : bytes)
		if (byte != std::byte{})
			return false;
	return true;
}

/** @brief Verifies constant-evaluated partial construction and active-only observation. */
consteval bool has_constexpr_partial_construction() noexcept
{
	using value_t = SimdLib::PartialRegister<std::uint32_t, 128, 3>;
	using api_t = typename value_t::api_type;
	constexpr std::array<std::uint32_t, 3> source{1U, 2U, 3U};
	const auto zero = value_t::zero();
	const auto broadcast = value_t::broadcast(7U);
	const auto listed = value_t::from_lanes(1U, 2U, 3U);
	const auto array_value = value_t::from_array(source);
	const auto listed_native = api_t::to_array(listed.to_native());
	return zero.to_array() == std::array<std::uint32_t, 3>{} && broadcast.to_array() == std::array<std::uint32_t, 3>{7U, 7U, 7U} &&
		   listed.to_array() == source && array_value.to_array() == source && listed_native[0] == 1U && listed_native[1] == 2U && listed_native[2] == 3U &&
		   has_zero_bits(listed_native[3]);
}

/** @brief Verifies constant-evaluated active lane extraction and replacement. */
consteval bool has_constexpr_partial_lane_access() noexcept
{
	using value_t = SimdLib::PartialRegister<std::uint64_t, 128, 1>;
	using api_t = typename value_t::api_type;
	const auto original = value_t::from_lanes(11U);
	const auto replaced = original.template with_lane<0>(29U);
	const auto native = api_t::to_array(replaced.to_native());
	return original.template lane<0>() == 11U && replaced.template lane<0>() == 29U && native[0] == 29U && has_zero_bits(native[1]);
}

/** @brief Verifies constant-evaluated extrema positions ignore the inactive zero suffix. */
consteval bool has_constexpr_partial_positions() noexcept
{
	using value_t = SimdLib::PartialRegister<std::int32_t, 128, 3>;
	const auto value = value_t::from_lanes(5, 2, 9);
	const auto ties = value_t::from_lanes(4, 4, 4);
	return value.min_position() == 1 && value.max_position() == 2 && ties.min_position() == 0 && ties.max_position() == 0;
}

/** @brief Reports whether a partial register or predicate has a bitwise-zero inactive native suffix. */
template <class value_t> [[nodiscard]] consteval bool has_zero_native_suffix(value_t value) noexcept
{
	using api_t = typename value_t::api_type;
	const auto native_lanes = api_t::to_array(value.to_native());
	for (std::size_t lane = value_t::lane_count; lane < value_t::native_lane_count; ++lane)
		if (!has_zero_bits(native_lanes[lane]))
			return false;
	return true;
}

/** @brief Verifies every constant-evaluated bitwise and sign-mask operation over the logical prefix. */
consteval bool has_constexpr_partial_bitwise_operations() noexcept
{
	using value_t = SimdLib::PartialRegister<std::uint32_t, 128, 3>;
	const auto lhs = value_t::from_lanes(1U, 2U, 4U);
	const auto rhs = value_t::from_lanes(3U, 2U, 1U);
	const auto intersection = lhs & rhs;
	const auto union_value = lhs | rhs;
	const auto exclusive = lhs ^ rhs;
	const auto complement = ~lhs;
	const auto difference = lhs.andnot(rhs);
	using signed_value_t = SimdLib::PartialRegister<std::int32_t, 128, 3>;
	const auto signed_value = signed_value_t::from_lanes(-1, 2, -3);
	return intersection.to_array() == std::array<std::uint32_t, 3>{1U, 2U, 0U} && union_value.to_array() == std::array<std::uint32_t, 3>{3U, 2U, 5U} &&
		   exclusive.to_array() == std::array<std::uint32_t, 3>{2U, 0U, 5U} && complement.to_array() == std::array<std::uint32_t, 3>{~1U, ~2U, ~4U} &&
		   difference.to_array() == std::array<std::uint32_t, 3>{2U, 0U, 1U} && signed_value.movemask() == 0x0f0f && signed_value.lane_sign_bits() == 0b101 &&
		   has_zero_native_suffix(intersection) && has_zero_native_suffix(union_value) && has_zero_native_suffix(exclusive) &&
		   has_zero_native_suffix(complement) && has_zero_native_suffix(difference);
}

/** @brief Verifies every constant-evaluated per-lane shift form and its inactive suffix. */
consteval bool has_constexpr_partial_lane_shifts() noexcept
{
	using unsigned_value_t = SimdLib::PartialRegister<std::uint32_t, 128, 3>;
	const auto unsigned_value = unsigned_value_t::from_lanes(1U, 2U, 4U);
	const auto left = unsigned_value << 1;
	const auto logical_right = unsigned_value.logical_shift_right(1);
	const auto unsigned_right = unsigned_value >> 1;
	using signed_value_t = SimdLib::PartialRegister<std::int32_t, 128, 3>;
	const auto signed_value = signed_value_t::from_lanes(-8, 8, -1);
	const auto signed_logical_right = signed_value.logical_shift_right(1);
	const auto signed_right = signed_value >> 1;
	return left.to_array() == std::array<std::uint32_t, 3>{2U, 4U, 8U} && logical_right.to_array() == std::array<std::uint32_t, 3>{0U, 1U, 2U} &&
		   unsigned_right.to_array() == logical_right.to_array() && signed_logical_right.to_array() == std::array<std::int32_t, 3>{0x7ffffffc, 4, 0x7fffffff} &&
		   signed_right.to_array() == std::array<std::int32_t, 3>{-4, 4, -1} && has_zero_native_suffix(left) && has_zero_native_suffix(logical_right) &&
		   has_zero_native_suffix(unsigned_right) && has_zero_native_suffix(signed_logical_right) && has_zero_native_suffix(signed_right);
}

/** @brief Verifies every runtime-count and immediate-count whole-payload shift form over an awkward active byte extent. */
consteval bool has_constexpr_partial_payload_shifts() noexcept
{
	using value_t = SimdLib::PartialRegister<std::uint8_t, 128, 13>;
	std::array<std::uint8_t, value_t::lane_count> source{};
	for (std::size_t lane = 0; lane < source.size(); ++lane)
		source[lane] = static_cast<std::uint8_t>(lane + 1);
	source.back() = 0xf1U;
	const auto value = value_t::from_array(source);
	std::array<std::uint8_t, value_t::lane_count> expected_bytes_left{};
	std::array<std::uint8_t, value_t::lane_count> expected_bytes_right{};
	for (std::size_t lane = 3; lane < source.size(); ++lane)
		expected_bytes_left[lane] = source[lane - 3];
	for (std::size_t lane = 0; lane + 3 < source.size(); ++lane)
		expected_bytes_right[lane] = source[lane + 3];
	std::array<std::uint8_t, value_t::lane_count> expected_bits_left{};
	std::array<std::uint8_t, value_t::lane_count> expected_bits_right{};
	for (std::size_t lane = 0; lane < source.size(); ++lane)
	{
		expected_bits_left[lane] = static_cast<std::uint8_t>(source[lane] << 4);
		if (lane != 0)
			expected_bits_left[lane] = static_cast<std::uint8_t>(expected_bits_left[lane] | (source[lane - 1] >> 4));
		expected_bits_right[lane] = static_cast<std::uint8_t>(source[lane] >> 4);
		if (lane + 1 < source.size())
			expected_bits_right[lane] = static_cast<std::uint8_t>(expected_bits_right[lane] | (source[lane + 1] << 4));
	}
	const auto bytes_left_slow = value.shift_bytes_left_slow(3);
	const auto bytes_right_slow = value.shift_bytes_right_slow(3);
	const auto bytes_left = value.template shift_bytes_left<3>();
	const auto bytes_right = value.template shift_bytes_right<3>();
	const auto bits_left_slow = value.shift_bits_left_slow(4);
	const auto bits_right_slow = value.shift_bits_right_slow(4);
	const auto bits_left = value.template shift_bits_left<4>();
	const auto bits_right = value.template shift_bits_right<4>();
	return bytes_left_slow.to_array() == expected_bytes_left && bytes_right_slow.to_array() == expected_bytes_right &&
		   bytes_left.to_array() == expected_bytes_left && bytes_right.to_array() == expected_bytes_right && bits_left_slow.to_array() == expected_bits_left &&
		   bits_right_slow.to_array() == expected_bits_right && bits_left.to_array() == expected_bits_left && bits_right.to_array() == expected_bits_right &&
		   has_zero_native_suffix(bytes_left_slow) && has_zero_native_suffix(bytes_right_slow) && has_zero_native_suffix(bytes_left) &&
		   has_zero_native_suffix(bytes_right) && has_zero_native_suffix(bits_left_slow) && has_zero_native_suffix(bits_right_slow) &&
		   has_zero_native_suffix(bits_left) && has_zero_native_suffix(bits_right);
}

/** @brief Verifies every constant-evaluated active-only comparison and false inactive predicate suffix. */
consteval bool has_constexpr_partial_comparisons() noexcept
{
	using value_t = SimdLib::PartialRegister<std::uint32_t, 128, 3>;
	const auto lhs = value_t::from_lanes(1U, 2U, 4U);
	const auto rhs = value_t::from_lanes(3U, 2U, 1U);
	const auto equal = lhs.compare_equal(rhs);
	const auto greater = lhs.compare_greater(rhs);
	const auto greater_equal = lhs.compare_greater_equal(rhs);
	const auto less = lhs.compare_less(rhs);
	const auto less_equal = lhs.compare_less_equal(rhs);
	return equal.bits() == 0b010U && greater.bits() == 0b100U && greater_equal.bits() == 0b110U && less.bits() == 0b001U && less_equal.bits() == 0b011U &&
		   lhs != rhs && lhs == lhs && has_zero_native_suffix(equal) && has_zero_native_suffix(greater) && has_zero_native_suffix(greater_equal) &&
		   has_zero_native_suffix(less) && has_zero_native_suffix(less_equal);
}

/** @brief Verifies constant-evaluated scalar equality preserves floating NaN and signed-zero semantics. */
consteval bool has_constexpr_partial_floating_equality() noexcept
{
	using value_t = SimdLib::PartialRegister<float, 128, 3>;
	const auto positive_zero = value_t::from_lanes(0.0F, 2.0F, 3.0F);
	const auto negative_zero = value_t::from_lanes(-0.0F, 2.0F, 3.0F);
	const auto nan_value = value_t::from_lanes(std::numeric_limits<float>::quiet_NaN(), 2.0F, 3.0F);
	return positive_zero == negative_zero && !(nan_value == nan_value) && nan_value != nan_value;
}

static_assert(has_constexpr_partial_construction());
static_assert(has_constexpr_partial_lane_access());
static_assert(has_constexpr_partial_positions());
// MSVC 19.44 ICEs while inspecting new explicit-object PartialRegister operation results in constant evaluation.
#if !defined(_MSC_VER) || defined(__clang__)
static_assert(has_constexpr_partial_bitwise_operations());
static_assert(has_constexpr_partial_lane_shifts());
static_assert(has_constexpr_partial_payload_shifts());
static_assert(has_constexpr_partial_comparisons());
static_assert(has_constexpr_partial_floating_equality());
#endif

} // namespace
