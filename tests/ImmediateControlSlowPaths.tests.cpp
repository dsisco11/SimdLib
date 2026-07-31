#include "TestSupport.h"

#include <SimdLib/Api.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#ifndef SIMDLIB_IMMEDIATE_CONTROL_TEST_WIDTH
#error "SIMDLIB_IMMEDIATE_CONTROL_TEST_WIDTH must select the tested register width"
#endif

namespace SimdLib::Tests
{

/**
 * @brief Creates distinct lane values suitable for immediate-control reference comparisons.
 * @tparam api_t Api specialization whose lane array is produced.
 * @param offset Offset added to each logical lane index.
 * @return Array containing monotonically increasing, exactly representable lane values.
 */
template <class api_t> constexpr std::array<typename api_t::element_type, api_t::element_count> make_control_values(const int offset)
{
	std::array<typename api_t::element_type, api_t::element_count> result{};
	for (std::size_t index = 0; index < result.size(); ++index)
		result[index] = static_cast<typename api_t::element_type>(offset + static_cast<int>(index));
	return result;
}

/**
 * @brief Verifies every runtime blend control byte for one supported lane type and width.
 * @tparam Width SIMD register width in bits.
 * @tparam Element Lane type accepted by the immediate blend family.
 */
template <std::size_t Width, class Element> void require_blend_slow_controls()
{
	using api = SimdLib::Api<Width, Element>;
	const auto left = make_control_values<api>(1);
	const auto right = make_control_values<api>(65);
	const auto lhs = api::construct(left);
	const auto rhs = api::construct(right);
	for (unsigned int control = 0; control <= 0xFFu; ++control)
	{
		auto expected = left;
		for (std::size_t index = 0; index < expected.size(); ++index)
		{
			if ((control & (1u << (index % 8))) != 0)
				expected[index] = right[index];
		}
		const volatile int runtime_control = static_cast<int>(control);
		REQUIRE(api::to_array(api::blend_slow(lhs, rhs, runtime_control)) == expected);
	}
}

/**
 * @brief Verifies every runtime low- and high-half 16-bit shuffle control byte.
 * @tparam Width SIMD register width in bits.
 */
template <std::size_t Width> void require_half_shuffle_slow_controls()
{
	using api = SimdLib::Api<Width, std::uint16_t>;
	const auto source = make_control_values<api>(1);
	const auto value = api::construct(source);
	for (unsigned int control = 0; control <= 0xFFu; ++control)
	{
		auto low = source;
		auto high = source;
		for (std::size_t group = 0; group < source.size(); group += 8)
		{
			for (std::size_t index = 0; index < 4; ++index)
			{
				const std::size_t selected = (control >> (index * 2)) & 0x3u;
				low[group + index] = source[group + selected];
				high[group + 4 + index] = source[group + 4 + selected];
			}
		}
		const volatile int runtime_control = static_cast<int>(control);
		REQUIRE(api::to_array(api::shuffle_lo_slow(value, runtime_control)) == low);
		REQUIRE(api::to_array(api::shuffle_hi_slow(value, runtime_control)) == high);
	}
}

/**
 * @brief Verifies every runtime 32-bit shuffle control byte.
 * @tparam Width SIMD register width in bits.
 */
template <std::size_t Width> void require_shuffle_32_slow_controls()
{
	using api = SimdLib::Api<Width, std::uint32_t>;
	const auto source = make_control_values<api>(1);
	const auto value = api::construct(source);
	for (unsigned int control = 0; control <= 0xFFu; ++control)
	{
		std::array<typename api::element_type, api::element_count> expected{};
		for (std::size_t group = 0; group < source.size(); group += 4)
		{
			for (std::size_t index = 0; index < 4; ++index)
				expected[group + index] = source[group + ((control >> (index * 2)) & 0x3u)];
		}
		const volatile std::uint32_t runtime_control = control;
		REQUIRE(api::to_array(api::shuffle_32_slow(value, runtime_control)) == expected);
	}
}

/**
 * @brief Verifies every runtime floating-point shuffle control byte for one lane type.
 * @tparam Width SIMD register width in bits.
 * @tparam Element Floating-point lane type.
 */
template <std::size_t Width, class Element> void require_floating_shuffle_slow_controls()
{
	using api = SimdLib::Api<Width, Element>;
	const auto left = make_control_values<api>(1);
	const auto right = make_control_values<api>(65);
	const auto lhs = api::construct(left);
	const auto rhs = api::construct(right);
	for (unsigned int control = 0; control <= 0xFFu; ++control)
	{
		std::array<typename api::element_type, api::element_count> expected{};
		if constexpr (std::is_same_v<Element, float>)
		{
			for (std::size_t group = 0; group < expected.size(); group += 4)
			{
				expected[group] = left[group + (control & 0x3u)];
				expected[group + 1] = left[group + ((control >> 2) & 0x3u)];
				expected[group + 2] = right[group + ((control >> 4) & 0x3u)];
				expected[group + 3] = right[group + ((control >> 6) & 0x3u)];
			}
		}
		else
		{
			for (std::size_t group = 0; group < expected.size(); group += 2)
			{
				const unsigned int group_control = control >> group;
				expected[group] = left[group + (group_control & 0x1u)];
				expected[group + 1] = right[group + ((group_control >> 1) & 0x1u)];
			}
		}
		const volatile int runtime_control = static_cast<int>(control);
		REQUIRE(api::to_array(api::shuffle_slow(lhs, rhs, runtime_control)) == expected);
	}
}

/** @brief Verifies every immediate-control emulation family for one register width. */
template <std::size_t Width> void require_immediate_control_slow_matrix()
{
	require_blend_slow_controls<Width, std::int16_t>();
	require_blend_slow_controls<Width, std::uint16_t>();
	require_blend_slow_controls<Width, std::int32_t>();
	require_blend_slow_controls<Width, std::uint32_t>();
	require_blend_slow_controls<Width, float>();
	require_blend_slow_controls<Width, double>();
	require_half_shuffle_slow_controls<Width>();
	require_shuffle_32_slow_controls<Width>();
	require_floating_shuffle_slow_controls<Width, float>();
	require_floating_shuffle_slow_controls<Width, double>();
}

/** @brief Verifies every valid complete-register bit count and its documented boundaries. */
inline void require_complete_register_shift_slow_controls()
{
	using api = SimdLib::Api<128, std::uint64_t>;
	const std::array<typename api::element_type, api::element_count> source{0x0123456789ABCDEFULL, 0xFEDCBA9876543210ULL};
	const auto value = api::construct(source);
	for (int count = -1; count <= 129; ++count)
	{
		std::array<typename api::element_type, api::element_count> left{};
		std::array<typename api::element_type, api::element_count> right{};
		if (count <= 0)
		{
			left = source;
			right = source;
		}
		else if (count < 64)
		{
			left = {source[0] << count, (source[1] << count) | (source[0] >> (64 - count))};
			right = {(source[0] >> count) | (source[1] << (64 - count)), source[1] >> count};
		}
		else if (count == 64)
		{
			left = {0, source[0]};
			right = {source[1], 0};
		}
		else if (count < 128)
		{
			left = {0, source[0] << (count - 64)};
			right = {source[1] >> (count - 64), 0};
		}
		const volatile int runtime_count = count;
		REQUIRE(api::to_array(api::shift_bits_left_slow(value, runtime_count)) == left);
		REQUIRE(api::to_array(api::shift_bits_right_slow(value, runtime_count)) == right);
	}
	const volatile int minimum_count = std::numeric_limits<int>::lowest();
	const volatile int maximum_count = std::numeric_limits<int>::max();
	REQUIRE(api::to_array(api::shift_bits_left_slow(value, minimum_count)) == source);
	REQUIRE(api::to_array(api::shift_bits_right_slow(value, minimum_count)) == source);
	REQUIRE(api::to_array(api::shift_bits_left_slow(value, maximum_count)) == std::array<typename api::element_type, api::element_count>{});
	REQUIRE(api::to_array(api::shift_bits_right_slow(value, maximum_count)) == std::array<typename api::element_type, api::element_count>{});
}

} // namespace SimdLib::Tests

using namespace SimdLib::Tests;

TEST_CASE("Runtime immediate-control substitutes cover every control byte", "[simdlib][immediate-control][slow]")
{
	require_immediate_control_slow_matrix<SIMDLIB_IMMEDIATE_CONTROL_TEST_WIDTH>();
}

#if SIMDLIB_IMMEDIATE_CONTROL_TEST_WIDTH == 128
TEST_CASE("Complete-register slow shifts cover every valid count", "[simdlib][immediate-control][slow][shift]")
{
	require_complete_register_shift_slow_controls();
}
#endif