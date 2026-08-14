#include <SimdLib/PartialRegister.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <bit>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#ifndef SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
#define SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256 SIMDLIB_HAS_AVX2
#endif

namespace
{

/** @brief Returns the first permitted active extent for one partial-register geometry. */
template <class element_t, std::size_t bits> [[nodiscard]] consteval std::size_t first_active_lane_count() noexcept
{
	if constexpr (bits == 256)
		return 128 / (sizeof(element_t) * 8) + 1;
	else
		return 1;
}

/** @brief Exports every physical lane from a partial or complete result. */
template <class value_t> [[nodiscard]] auto native_lanes(value_t value) noexcept
{
	return value_t::api_type::to_array(value.native);
}

/** @brief Verifies the inactive physical suffix is represented by all-bits-zero lanes. */
template <class value_t> void require_zero_suffix(value_t value)
{
	const auto lanes = native_lanes(value);
	for (std::size_t lane = value_t::lane_count; lane < value_t::api_type::element_count; ++lane)
	{
		const auto bytes = std::bit_cast<std::array<std::byte, sizeof(typename value_t::element_type)>>(lanes[lane]);
		for (const auto byte : bytes)
			REQUIRE(std::to_integer<unsigned>(byte) == 0U);
	}
}

/** @brief Verifies grouped adjacent multiply-add against an independent physical-lane oracle. */
template <class element_t, std::size_t bits, std::size_t active_lane_count> void require_adjacent_oracle()
{
	using source_t = SimdLib::PartialRegister<element_t, bits, active_lane_count>;
	using result_t = SimdLib::partial_multiply_add_adjacent_result_t<element_t, bits, active_lane_count>;
	using result_element_t = typename result_t::element_type;
	constexpr std::size_t source_group_lanes = 128 / (sizeof(element_t) * 8);
	constexpr std::size_t result_group_lanes = 128 / (sizeof(result_element_t) * 8);
	std::array<element_t, active_lane_count> lhs{};
	std::array<element_t, active_lane_count> rhs{};
	for (std::size_t lane = 0; lane < active_lane_count; ++lane)
	{
		lhs[lane] = static_cast<element_t>(lane % 5 + 1);
		rhs[lane] = static_cast<element_t>(lane % 3 + 2);
	}
	const auto actual = native_lanes(source_t::from_array(lhs).multiply_add_adjacent(source_t::from_array(rhs)));
	std::array<result_element_t, result_t::api_type::element_count> expected{};
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		const std::size_t source_base = group * source_group_lanes;
		for (std::size_t pair = 0; pair < source_group_lanes / 2; ++pair)
		{
			const std::size_t low = source_base + pair * 2;
			const auto lhs_low = low < active_lane_count ? lhs[low] : element_t{};
			const auto rhs_low = low < active_lane_count ? rhs[low] : element_t{};
			const auto lhs_high = low + 1 < active_lane_count ? lhs[low + 1] : element_t{};
			const auto rhs_high = low + 1 < active_lane_count ? rhs[low + 1] : element_t{};
			expected[group * result_group_lanes + pair] =
				static_cast<result_element_t>(static_cast<result_element_t>(lhs_low) * static_cast<result_element_t>(rhs_low) +
											  static_cast<result_element_t>(lhs_high) * static_cast<result_element_t>(rhs_high));
		}
	}
	CAPTURE(sizeof(element_t), std::is_signed_v<element_t>, bits, active_lane_count);
	REQUIRE(actual == expected);
}

/** @brief Verifies byte multiply-add, SAD, and multi-SAD against independent byte-level oracles. */
template <class element_t, std::size_t bits, std::size_t active_lane_count, int multi_sad_control> void require_byte_specialized_oracles()
{
	using source_t = SimdLib::PartialRegister<element_t, bits, active_lane_count>;
	using source_api_t = typename source_t::api_type;
	using byte_api_t = SimdLib::Api<bits, std::uint8_t>;
	constexpr std::size_t active_byte_count = active_lane_count * sizeof(element_t);
	std::array<std::uint8_t, byte_api_t::element_count> lhs_bytes{};
	std::array<std::uint8_t, byte_api_t::element_count> rhs_bytes{};
	for (std::size_t byte = 0; byte < active_byte_count; ++byte)
	{
		lhs_bytes[byte] = static_cast<std::uint8_t>(byte % 7 + 1);
		rhs_bytes[byte] = static_cast<std::uint8_t>(byte % 5 + 1);
	}
	const auto lhs = source_t::from_native(byte_api_t::template bit_cast<element_t>(byte_api_t::construct(lhs_bytes)));
	const auto rhs = source_t::from_native(byte_api_t::template bit_cast<element_t>(byte_api_t::construct(rhs_bytes)));

	if constexpr (SimdLib::IApi::ByteMultiplyAdd<source_api_t>)
	{
		const auto actual = native_lanes(lhs.multiply_add_unsigned_signed_bytes(rhs));
		std::array<std::int16_t, bits / 16> expected{};
		for (std::size_t pair = 0; pair < (active_byte_count + 1) / 2; ++pair)
		{
			const std::size_t low = pair * 2;
			expected[pair] = static_cast<std::int16_t>(
				static_cast<unsigned>(lhs_bytes[low]) * static_cast<std::int8_t>(rhs_bytes[low]) +
				(low + 1 < active_byte_count ? static_cast<unsigned>(lhs_bytes[low + 1]) * static_cast<std::int8_t>(rhs_bytes[low + 1]) : 0));
		}
		REQUIRE(actual == expected);
	}

	if constexpr (SimdLib::IApi::Sad<source_api_t>)
	{
		const auto actual = native_lanes(lhs.sum_absolute_byte_differences(rhs));
		std::array<std::uint64_t, bits / 64> expected{};
		for (std::size_t group = 0; group < (active_byte_count + 7) / 8; ++group)
			for (std::size_t offset = 0; offset < 8 && group * 8 + offset < active_byte_count; ++offset)
				expected[group] +=
					static_cast<std::uint64_t>(std::abs(static_cast<int>(lhs_bytes[group * 8 + offset]) - static_cast<int>(rhs_bytes[group * 8 + offset])));
		REQUIRE(actual == expected);
	}

	if constexpr (SimdLib::IApi::MultiSad<source_api_t, multi_sad_control>)
	{
		const auto actual = lhs.template multi_sum_absolute_byte_differences<multi_sad_control>(rhs).to_array();
		std::array<std::uint16_t, bits / 16> expected{};
		for (std::size_t group = 0; group < bits / 128; ++group)
		{
			const unsigned control = (static_cast<unsigned>(multi_sad_control) >> (group * 3)) & 0x7U;
			const std::size_t group_base = group * 16;
			const std::size_t lhs_base = group_base + ((control >> 2) & 1U) * 4;
			const std::size_t rhs_base = group_base + (control & 3U) * 4;
			for (std::size_t output = 0; output < 8; ++output)
				for (std::size_t offset = 0; offset < 4; ++offset)
					expected[group * 8 + output] += static_cast<std::uint16_t>(
						std::abs(static_cast<int>(lhs_bytes[lhs_base + output + offset]) - static_cast<int>(rhs_bytes[rhs_base + offset])));
		}
		REQUIRE(actual == expected);
	}
}

/** @brief Verifies magnitude and checked-magnitude grouping with independent perfect-square inputs. */
template <class element_t, std::size_t bits, std::size_t active_lane_count> void require_magnitude_oracles()
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_lane_count>;
	constexpr std::size_t group_lane_count = 128 / (sizeof(element_t) * 8);
	std::array<element_t, active_lane_count> source{};
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		const std::size_t base = group * group_lane_count;
		if (base < active_lane_count)
			source[base] = static_cast<element_t>(3);
		if (base + 1 < active_lane_count)
			source[base + 1] = static_cast<element_t>(4);
	}
	const auto value = value_t::from_array(source);
	if constexpr (requires { value.magnitude(); })
	{
		const auto magnitude = value.magnitude();
		const auto physical = native_lanes(magnitude);
		for (std::size_t group = 0; group < bits / 128; ++group)
		{
			const std::size_t group_base = group * group_lane_count;
			if (group_base < active_lane_count)
				REQUIRE(physical[group_base] == (group_base + 1 < active_lane_count ? static_cast<element_t>(5) : static_cast<element_t>(3)));
		}
		require_zero_suffix(magnitude);
	}
	if constexpr (requires { value.magnitude_checked(); })
	{
		const auto checked = value.magnitude_checked();
		const auto physical = native_lanes(checked);
		for (std::size_t group = 0; group < bits / 128; ++group)
		{
			const std::size_t source_base = group * group_lane_count;
			if (source_base >= active_lane_count)
				continue;
			REQUIRE(physical[source_base] == (source_base + 1 < active_lane_count ? static_cast<element_t>(5) : static_cast<element_t>(3)));
			REQUIRE(physical[source_base + 1] == element_t{});
		}
		if constexpr (requires { checked.to_native(); })
			require_zero_suffix(checked);
	}
}

/** @brief Verifies intrinsic-ordered horizontal operations against a grouped scalar oracle. */
template <class element_t, std::size_t bits, std::size_t active_lane_count> void require_horizontal_oracles()
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_lane_count>;
	constexpr std::size_t group_lane_count = 128 / (sizeof(element_t) * 8);
	std::array<element_t, active_lane_count> lhs_values{};
	std::array<element_t, active_lane_count> rhs_values{};
	for (std::size_t lane = 0; lane < active_lane_count; ++lane)
	{
		lhs_values[lane] = static_cast<element_t>(lane % 4 + 1);
		rhs_values[lane] = static_cast<element_t>(lane % 3 + 6);
	}
	const auto lhs = value_t::from_array(lhs_values);
	const auto rhs = value_t::from_array(rhs_values);
	if constexpr (requires { lhs.horizontal_add(rhs); })
	{
		std::array<element_t, active_lane_count> expected_add{};
		std::array<element_t, active_lane_count> expected_subtract{};
		for (std::size_t group = 0; group < bits / 128; ++group)
		{
			const std::size_t base = group * group_lane_count;
			for (std::size_t pair = 0; pair < group_lane_count / 2; ++pair)
			{
				const std::size_t low = base + pair * 2;
				const auto lhs_low = low < active_lane_count ? lhs_values[low] : element_t{};
				const auto lhs_high = low + 1 < active_lane_count ? lhs_values[low + 1] : element_t{};
				const auto rhs_low = low < active_lane_count ? rhs_values[low] : element_t{};
				const auto rhs_high = low + 1 < active_lane_count ? rhs_values[low + 1] : element_t{};
				if (base + pair < active_lane_count)
				{
					expected_add[base + pair] = static_cast<element_t>(lhs_low + lhs_high);
					expected_subtract[base + pair] = static_cast<element_t>(lhs_low - lhs_high);
				}
				if (base + group_lane_count / 2 + pair < active_lane_count)
				{
					expected_add[base + group_lane_count / 2 + pair] = static_cast<element_t>(rhs_low + rhs_high);
					expected_subtract[base + group_lane_count / 2 + pair] = static_cast<element_t>(rhs_low - rhs_high);
				}
			}
		}
		REQUIRE(lhs.horizontal_add(rhs).to_array() == expected_add);
		REQUIRE(lhs.horizontal_subtract(rhs).to_array() == expected_subtract);
		require_zero_suffix(lhs.horizontal_add(rhs));
		require_zero_suffix(lhs.horizontal_subtract(rhs));
	}
}

/** @brief Verifies one floating dot-product control against an independent grouped scalar oracle. */
template <int control, class element_t, std::size_t bits, std::size_t active_lane_count>
void require_dot_product_oracle(const SimdLib::PartialRegister<element_t, bits, active_lane_count> value)
{
	if constexpr (requires { value.template dot_product<control>(value); })
	{
		constexpr std::size_t group_lane_count = 128 / (sizeof(element_t) * 8);
		const auto source = value.to_array();
		std::array<element_t, active_lane_count> expected{};
		for (std::size_t group = 0; group < bits / 128; ++group)
		{
			const std::size_t base = group * group_lane_count;
			element_t sum{};
			for (std::size_t local = 0; local < group_lane_count; ++local)
				if ((control & (1 << (local + 4))) != 0 && base + local < active_lane_count)
					sum += source[base + local] * source[base + local];
			for (std::size_t local = 0; local < group_lane_count && base + local < active_lane_count; ++local)
				if ((control & (1 << local)) != 0)
					expected[base + local] = sum;
		}
		const auto actual = value.template dot_product<control>(value);
		for (std::size_t lane = 0; lane < active_lane_count; ++lane)
			REQUIRE(actual.to_array()[lane] == Catch::Approx(expected[lane]));
		require_zero_suffix(actual);
	}
}

/** @brief Runs all integral specialized-operation oracles for one element/width cell. */
template <class element_t, std::size_t bits> void require_integral_specialized_matrix_cell()
{
	constexpr std::size_t first = first_active_lane_count<element_t, bits>();
	constexpr std::size_t last = SimdLib::Api<bits, element_t>::element_count - 1;
	require_adjacent_oracle<element_t, bits, first>();
	if constexpr (first != last)
		require_adjacent_oracle<element_t, bits, last>();
	require_byte_specialized_oracles<element_t, bits, first, 0>();
	require_byte_specialized_oracles<element_t, bits, first, 0x35>();
	require_byte_specialized_oracles<element_t, bits, last, 255>();
	require_magnitude_oracles<element_t, bits, first>();
	require_horizontal_oracles<element_t, bits, first>();
}

/** @brief Verifies floating normalization and dot-product immediates for one element/width cell. */
template <class element_t, std::size_t bits> void require_floating_specialized_matrix_cell()
{
	constexpr std::size_t active_lane_count = SimdLib::Api<bits, element_t>::element_count - 1;
	using value_t = SimdLib::PartialRegister<element_t, bits, active_lane_count>;
	std::array<element_t, active_lane_count> source{};
	constexpr std::size_t group_lane_count = 128 / (sizeof(element_t) * 8);
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		const std::size_t base = group * group_lane_count;
		if (base < active_lane_count)
			source[base] = static_cast<element_t>(3);
		if (base + 1 < active_lane_count)
			source[base + 1] = static_cast<element_t>(4);
	}
	const auto value = value_t::from_array(source);
	const auto normalized = value.normalize();
	CAPTURE(sizeof(element_t), bits, active_lane_count);
	for (std::size_t lane = 0; lane < active_lane_count; ++lane)
	{
		const std::size_t group_base = (lane / group_lane_count) * group_lane_count;
		const std::size_t local = lane % group_lane_count;
		const auto expected = local == 0   ? (group_base + 1 < active_lane_count ? static_cast<element_t>(0.6) : static_cast<element_t>(1))
							  : local == 1 ? static_cast<element_t>(0.8)
										   : element_t{};
		REQUIRE(normalized.to_array()[lane] == Catch::Approx(expected));
	}
	require_zero_suffix(normalized);
	require_dot_product_oracle<0x00>(value);
	require_dot_product_oracle<0x11>(value);
	require_dot_product_oracle<0x53>(value);
	require_dot_product_oracle<0xf7>(value);
	require_horizontal_oracles<element_t, bits, active_lane_count>();
}

TEST_CASE("PartialRegister adjacent and byte-specialized operations match scalar oracles", "[simdlib][partial-register][specialized][oracle]")
{
#define SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(type, width) require_integral_specialized_matrix_cell<type, width>()
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::int8_t, 128);
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::uint8_t, 128);
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::int16_t, 128);
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::uint16_t, 128);
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::int32_t, 128);
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::uint32_t, 128);
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::int64_t, 128);
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::uint64_t, 128);
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::int8_t, 256);
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::uint8_t, 256);
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::int16_t, 256);
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::uint16_t, 256);
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::int32_t, 256);
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::uint32_t, 256);
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::int64_t, 256);
	SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED(std::uint64_t, 256);
#endif
#undef SIMDLIB_REQUIRE_INTEGRAL_SPECIALIZED
}

TEST_CASE("PartialRegister floating specialized operations match grouped scalar oracles", "[simdlib][partial-register][specialized][floating]")
{
	require_floating_specialized_matrix_cell<float, 128>();
	require_floating_specialized_matrix_cell<double, 128>();
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
	require_floating_specialized_matrix_cell<float, 256>();
	require_floating_specialized_matrix_cell<double, 256>();
#endif
}

} // namespace
