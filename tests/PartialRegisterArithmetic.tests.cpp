#include <SimdLib/PartialRegister.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

namespace
{

/** @brief Verifies arithmetic availability parity with the selected API for one partial geometry. */
template <class element_t, std::size_t bits, std::size_t active_count> consteval bool has_arithmetic_surface_parity() noexcept
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_count>;
	using api_t = typename value_t::api_type;
	static_assert(SimdLib::IRegister::Add<value_t> == SimdLib::IApi::Add<api_t>);
	static_assert(SimdLib::IRegister::Subtract<value_t> == SimdLib::IApi::Subtract<api_t>);
	static_assert(SimdLib::IRegister::Multiply<value_t> == SimdLib::IApi::Multiply<api_t>);
	static_assert(SimdLib::IRegister::Divide<value_t> == SimdLib::IApi::Divide<api_t>);
	static_assert(SimdLib::IRegister::Modulus<value_t> == SimdLib::IApi::Modulus<api_t>);
	static_assert(SimdLib::IRegister::Negate<value_t> == SimdLib::IApi::Negate<api_t>);
	static_assert(SimdLib::IRegister::Min<value_t> == SimdLib::IApi::Min<api_t>);
	static_assert(SimdLib::IRegister::Max<value_t> == SimdLib::IApi::Max<api_t>);
	static_assert(SimdLib::IRegister::Absolute<value_t> == SimdLib::IApi::Absolute<api_t>);
	static_assert(SimdLib::IRegister::Sqrt<value_t> == SimdLib::IApi::Sqrt<api_t>);
	static_assert(SimdLib::IRegister::Average<value_t> == SimdLib::IApi::Average<api_t>);
	static_assert(SimdLib::IRegister::MultiplyAdd<value_t> == SimdLib::IApi::MultiplyAdd<api_t>);
	static_assert(SimdLib::IRegister::Magnitude<value_t> == SimdLib::IApi::Magnitude<api_t>);
	constexpr bool has_magnitude_checked = requires(value_t value) { value.magnitude_checked(); };
	static_assert(has_magnitude_checked == SimdLib::IApi::MagnitudeChecked<api_t>);
	if constexpr (SimdLib::IApi::MagnitudeChecked<api_t>)
		static_assert(
			std::same_as<decltype(std::declval<value_t>().magnitude_checked()), SimdLib::partial_magnitude_checked_result_t<element_t, bits, active_count>>);
	static_assert(SimdLib::IRegister::Normalize<value_t> == SimdLib::IApi::Normalize<api_t>);
	static_assert(SimdLib::IRegister::HorizontalAdd<value_t> == SimdLib::IApi::HorizontalAdd<api_t>);
	static_assert(SimdLib::IRegister::HorizontalSubtract<value_t> == SimdLib::IApi::HorizontalSubtract<api_t>);
	static_assert(SimdLib::IRegister::MinPosition<value_t> == SimdLib::IApi::MinPosition<api_t>);
	static_assert(SimdLib::IRegister::MaxPosition<value_t> == SimdLib::IApi::MaxPosition<api_t>);
	static_assert(SimdLib::IRegister::AddSaturated<value_t> == SimdLib::IApi::AddSaturated<api_t>);
	static_assert(SimdLib::IRegister::SubtractSaturated<value_t> == SimdLib::IApi::SubtractSaturated<api_t>);
	static_assert(SimdLib::IRegister::HorizontalAddSaturated<value_t> == SimdLib::IApi::HorizontalAddSaturated<api_t>);
	static_assert(SimdLib::IRegister::HorizontalSubtractSaturated<value_t> == SimdLib::IApi::HorizontalSubtractSaturated<api_t>);
	static_assert(SimdLib::IRegister::AddSubtract<value_t> == SimdLib::IApi::AddSubtract<api_t>);
	static_assert(SimdLib::IRegister::DotProduct<value_t, 0> == SimdLib::IApi::DotProduct<api_t, 0>);
	static_assert(SimdLib::IRegister::MultiplyAddAdjacent<value_t> == SimdLib::IApi::MultiplyAddAdjacent<api_t>);
	static_assert(SimdLib::IRegister::MultiplyAddUnsignedSignedBytes<value_t> == SimdLib::IApi::ByteMultiplyAdd<api_t>);
	static_assert(SimdLib::IRegister::SumAbsoluteByteDifferences<value_t> == SimdLib::IApi::Sad<api_t>);
	static_assert(SimdLib::IRegister::MultiSumAbsoluteByteDifferences<value_t, 0> == SimdLib::IApi::MultiSad<api_t, 0>);
	return true;
}

static_assert(has_arithmetic_surface_parity<std::int8_t, 128, 7>());
static_assert(has_arithmetic_surface_parity<std::uint8_t, 128, 7>());
static_assert(has_arithmetic_surface_parity<std::int16_t, 128, 5>());
static_assert(has_arithmetic_surface_parity<std::uint16_t, 128, 5>());
static_assert(has_arithmetic_surface_parity<std::int32_t, 128, 3>());
static_assert(has_arithmetic_surface_parity<std::uint32_t, 128, 3>());
static_assert(has_arithmetic_surface_parity<std::int64_t, 128, 1>());
static_assert(has_arithmetic_surface_parity<std::uint64_t, 128, 1>());
static_assert(has_arithmetic_surface_parity<float, 128, 3>());
static_assert(has_arithmetic_surface_parity<double, 128, 1>());
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
static_assert(has_arithmetic_surface_parity<std::uint8_t, 256, 17>());
static_assert(has_arithmetic_surface_parity<std::int8_t, 256, 17>());
static_assert(has_arithmetic_surface_parity<std::int16_t, 256, 9>());
static_assert(has_arithmetic_surface_parity<std::uint16_t, 256, 9>());
static_assert(has_arithmetic_surface_parity<std::int32_t, 256, 5>());
static_assert(has_arithmetic_surface_parity<std::uint32_t, 256, 5>());
static_assert(has_arithmetic_surface_parity<std::int64_t, 256, 3>());
static_assert(has_arithmetic_surface_parity<std::uint64_t, 256, 3>());
static_assert(has_arithmetic_surface_parity<float, 256, 5>());
static_assert(has_arithmetic_surface_parity<double, 256, 3>());
#endif

/** @brief Reports whether one scalar has the all-bits-zero representation. */
template <class element_t> [[nodiscard]] bool has_zero_bits(element_t value) noexcept
{
	const auto bytes = std::bit_cast<std::array<std::byte, sizeof(element_t)>>(value);
	return std::ranges::all_of(bytes, [](std::byte byte) noexcept { return byte == std::byte{}; });
}

/** @brief Verifies that every physical lane beyond the logical prefix is bitwise zero. */
template <class value_t> void require_zero_suffix(value_t value)
{
	const auto native = [&]()
	{
		if constexpr (requires { value.to_native(); })
			return value.to_native();
		else
			return value.native;
	}();
	const auto lanes = value_t::api_type::to_array(native);
	for (std::size_t lane = value_t::lane_count; lane < value_t::api_type::element_count; ++lane)
		REQUIRE(has_zero_bits(lanes[lane]));
}

/** @brief Verifies the core lane-wise arithmetic surface against scalar arithmetic. */
template <class element_t, std::size_t bits, std::size_t active_count> void require_lane_arithmetic()
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_count>;
	std::array<element_t, active_count> lhs_values{};
	std::array<element_t, active_count> rhs_values{};
	for (std::size_t lane = 0; lane < active_count; ++lane)
	{
		lhs_values[lane] = static_cast<element_t>(lane + 6);
		rhs_values[lane] = static_cast<element_t>(2);
	}
	const auto lhs = value_t::from_array(lhs_values);
	const auto rhs = value_t::from_array(rhs_values);

	if constexpr (requires { lhs + rhs; })
	{
		const auto actual = (lhs + rhs).to_array();
		for (std::size_t lane = 0; lane < active_count; ++lane)
			REQUIRE(actual[lane] == static_cast<element_t>(lhs_values[lane] + rhs_values[lane]));
		require_zero_suffix(lhs + rhs);
	}
	if constexpr (requires { lhs - rhs; })
	{
		const auto actual = (lhs - rhs).to_array();
		for (std::size_t lane = 0; lane < active_count; ++lane)
			REQUIRE(actual[lane] == static_cast<element_t>(lhs_values[lane] - rhs_values[lane]));
		require_zero_suffix(lhs - rhs);
	}
	if constexpr (requires { lhs * rhs; })
	{
		const auto actual = (lhs * rhs).to_array();
		for (std::size_t lane = 0; lane < active_count; ++lane)
			REQUIRE(actual[lane] == static_cast<element_t>(lhs_values[lane] * rhs_values[lane]));
		require_zero_suffix(lhs * rhs);
	}
	if constexpr (requires { lhs / rhs; })
	{
		const auto result = lhs / rhs;
		const auto actual = result.to_array();
		for (std::size_t lane = 0; lane < active_count; ++lane)
			REQUIRE(actual[lane] == static_cast<element_t>(lhs_values[lane] / rhs_values[lane]));
		require_zero_suffix(result);
	}
	if constexpr (requires { lhs % rhs; })
	{
		const auto result = lhs % rhs;
		const auto actual = result.to_array();
		for (std::size_t lane = 0; lane < active_count; ++lane)
			REQUIRE(actual[lane] == static_cast<element_t>(lhs_values[lane] % rhs_values[lane]));
		require_zero_suffix(result);
	}
	if constexpr (requires { -lhs; })
	{
		const auto negated = -lhs;
		for (std::size_t lane = 0; lane < active_count; ++lane)
			REQUIRE(negated.to_array()[lane] == static_cast<element_t>(element_t{} - lhs_values[lane]));
		require_zero_suffix(negated);
	}
	if constexpr (requires { lhs.min(rhs); })
	{
		const auto minimum = lhs.min(rhs);
		const auto maximum = lhs.max(rhs);
		const auto absolute_source = [&]()
		{
			if constexpr (std::is_signed_v<element_t> || std::is_floating_point_v<element_t>)
				return -lhs;
			else
				return lhs;
		}();
		const auto absolute = absolute_source.absolute();
		for (std::size_t lane = 0; lane < active_count; ++lane)
		{
			REQUIRE(minimum.to_array()[lane] == std::min(lhs_values[lane], rhs_values[lane]));
			REQUIRE(maximum.to_array()[lane] == std::max(lhs_values[lane], rhs_values[lane]));
			REQUIRE(absolute.to_array()[lane] == lhs_values[lane]);
		}
		require_zero_suffix(minimum);
		require_zero_suffix(maximum);
		require_zero_suffix(absolute);
	}
	if constexpr (requires { lhs.sqrt(); })
	{
		std::array<element_t, active_count> squares{};
		for (std::size_t lane = 0; lane < active_count; ++lane)
		{
			const auto root = static_cast<element_t>(lane % 3 + 1);
			squares[lane] = static_cast<element_t>(root * root);
		}
		const auto result = value_t::from_array(squares).sqrt();
		for (std::size_t lane = 0; lane < active_count; ++lane)
			REQUIRE(result.to_array()[lane] == static_cast<element_t>(lane % 3 + 1));
		require_zero_suffix(result);
	}
	if constexpr (requires { lhs.average(rhs); })
	{
		const auto result = lhs.average(rhs);
		for (std::size_t lane = 0; lane < active_count; ++lane)
			REQUIRE(result.to_array()[lane] == static_cast<element_t>((lhs_values[lane] + rhs_values[lane] + 1) / 2));
		require_zero_suffix(result);
	}
	if constexpr (requires { lhs.multiply_add(rhs, rhs); })
	{
		const auto result = lhs.multiply_add(rhs, rhs);
		for (std::size_t lane = 0; lane < active_count; ++lane)
			REQUIRE(result.to_array()[lane] == static_cast<element_t>(lhs_values[lane] * rhs_values[lane] + rhs_values[lane]));
		require_zero_suffix(result);
	}
	if constexpr (requires { lhs.add_saturated(rhs); })
	{
		const auto added = lhs.add_saturated(rhs);
		const auto subtracted = lhs.subtract_saturated(rhs);
		for (std::size_t lane = 0; lane < active_count; ++lane)
		{
			REQUIRE(added.to_array()[lane] == static_cast<element_t>(lhs_values[lane] + rhs_values[lane]));
			REQUIRE(subtracted.to_array()[lane] == static_cast<element_t>(lhs_values[lane] - rhs_values[lane]));
		}
		require_zero_suffix(added);
		require_zero_suffix(subtracted);
	}
}

/** @brief Verifies logical extrema positions cannot select zero-valued inactive lanes. */
template <class element_t, std::size_t bits> void require_positions()
{
	constexpr std::size_t active_count = bits == 256 ? 128 / (sizeof(element_t) * 8) + 1 : 3;
	using value_t = SimdLib::PartialRegister<element_t, bits, active_count>;
	std::array<element_t, active_count> lanes{};
	lanes.fill(element_t{5});
	lanes[1] = element_t{2};
	lanes[2] = element_t{9};
	const auto value = value_t::from_array(lanes);
	REQUIRE(value.min_position() == 1);
	REQUIRE(value.max_position() == 2);
	lanes.fill(element_t{2});
	const auto ties = value_t::from_array(lanes);
	REQUIRE(ties.min_position() == 0);
	REQUIRE(ties.max_position() == 0);
	const auto lower_sentinel_tie = value_t::broadcast(std::numeric_limits<element_t>::lowest());
	const auto upper_sentinel_tie = value_t::broadcast(std::numeric_limits<element_t>::max());
	REQUIRE(lower_sentinel_tie.max_position() == 0);
	REQUIRE(upper_sentinel_tie.min_position() == 0);
}

/** @brief Verifies grouped floating operations use only active inputs and clear generated suffix values. */
template <std::size_t bits, std::size_t active_count> void require_floating_group_operations()
{
	using value_t = SimdLib::PartialRegister<float, bits, active_count>;
	std::array<float, active_count> source{};
	source[0] = 3.0F;
	source[1] = 4.0F;
	for (std::size_t lane = 2; lane < active_count; ++lane)
		source[lane] = 0.0F;
	const auto value = value_t::from_array(source);
	const auto magnitude = value.magnitude();
	const auto normalized = value.normalize();
	REQUIRE(magnitude.to_array()[0] == Catch::Approx(5.0F));
	REQUIRE(magnitude.to_array()[1] == Catch::Approx(5.0F));
	REQUIRE(normalized.to_array()[0] == Catch::Approx(0.6F));
	REQUIRE(normalized.to_array()[1] == Catch::Approx(0.8F));
	require_zero_suffix(magnitude);
	require_zero_suffix(normalized);
	const auto dot = value.template dot_product<0x71>(value);
	REQUIRE(dot.to_array()[0] == Catch::Approx(25.0F));
	for (std::size_t lane = 1; lane < active_count; ++lane)
		REQUIRE(dot.to_array()[lane] == 0.0F);
	require_zero_suffix(dot);
}

/** @brief Verifies zero active magnitudes retain Register's exceptional normalization semantics without exposing the suffix. */
void require_zero_normalization_semantics()
{
	using value_t = SimdLib::PartialRegister<float, 128, 3>;
	const auto normalized = value_t::zero().normalize();
	for (const auto lane : normalized.to_array())
		REQUIRE(std::isnan(lane));
	require_zero_suffix(normalized);
}

/** @brief Verifies lane-wise and horizontal saturation at both numeric limits. */
void require_saturation_boundaries()
{
	using unsigned_t = SimdLib::PartialRegister<std::uint8_t, 128, 3>;
	const auto unsigned_lhs = unsigned_t::from_lanes(250, 5, 0);
	const auto unsigned_rhs = unsigned_t::from_lanes(10, 10, 1);
	const auto unsigned_added = unsigned_lhs.add_saturated(unsigned_rhs);
	const auto unsigned_subtracted = unsigned_lhs.subtract_saturated(unsigned_rhs);
	REQUIRE(unsigned_added.to_array() == std::array<std::uint8_t, 3>{255, 15, 1});
	REQUIRE(unsigned_subtracted.to_array() == std::array<std::uint8_t, 3>{240, 0, 0});
	require_zero_suffix(unsigned_added);
	require_zero_suffix(unsigned_subtracted);

	using signed_t = SimdLib::PartialRegister<std::int8_t, 128, 3>;
	const auto signed_lhs = signed_t::from_lanes(120, -120, 5);
	const auto signed_rhs = signed_t::from_lanes(20, 20, -10);
	const auto signed_added = signed_lhs.add_saturated(signed_rhs);
	const auto signed_subtracted = signed_lhs.subtract_saturated(signed_rhs);
	REQUIRE(signed_added.to_array() == std::array<std::int8_t, 3>{127, -100, -5});
	REQUIRE(signed_subtracted.to_array() == std::array<std::int8_t, 3>{100, -128, 15});
	require_zero_suffix(signed_added);
	require_zero_suffix(signed_subtracted);

	using horizontal_t = SimdLib::PartialRegister<std::int16_t, 128, 5>;
	constexpr auto maximum = std::numeric_limits<std::int16_t>::max();
	constexpr auto minimum = std::numeric_limits<std::int16_t>::lowest();
	const auto add_input = horizontal_t::from_lanes(maximum, 1, minimum, -1, 5);
	const auto subtract_input = horizontal_t::from_lanes(maximum, -1, minimum, 1, 5);
	const auto horizontal_added = add_input.horizontal_add_saturated(add_input);
	const auto horizontal_subtracted = subtract_input.horizontal_subtract_saturated(subtract_input);
	REQUIRE(horizontal_added.to_array() == std::array<std::int16_t, 5>{maximum, minimum, 5, 0, maximum});
	REQUIRE(horizontal_subtracted.to_array() == std::array<std::int16_t, 5>{maximum, minimum, 5, 0, maximum});
	require_zero_suffix(horizontal_added);
	require_zero_suffix(horizontal_subtracted);
}

/** @brief Verifies result aliases preserve final unmatched and grouped output lanes when they fill a register. */
template <std::size_t bits> void require_complete_specialized_results()
{
	using source_t = SimdLib::PartialRegister<std::uint8_t, bits, SimdLib::Api<bits, std::uint8_t>::element_count - 1>;
	std::array<std::uint8_t, source_t::lane_count> source_lanes{};
	for (std::size_t lane = 0; lane < source_lanes.size(); ++lane)
		source_lanes[lane] = 1;
	const auto source = source_t::from_array(source_lanes);
	const auto adjacent = source.multiply_add_adjacent(source);
	const auto byte_products = source.multiply_add_unsigned_signed_bytes(source);
	const auto sad = source.sum_absolute_byte_differences(source_t::zero());
	static_assert(std::same_as<decltype(adjacent), const SimdLib::Register<std::uint16_t, bits>>);
	static_assert(std::same_as<decltype(byte_products), const SimdLib::Register<std::int16_t, bits>>);
	static_assert(std::same_as<decltype(sad), const SimdLib::Register<std::uint64_t, bits>>);
	const auto adjacent_lanes = adjacent.to_array();
	const auto byte_product_lanes = byte_products.to_array();
	for (std::size_t lane = 0; lane + 1 < adjacent_lanes.size(); ++lane)
	{
		REQUIRE(adjacent_lanes[lane] == 2);
		REQUIRE(byte_product_lanes[lane] == 2);
	}
	REQUIRE(adjacent_lanes.back() == 1);
	REQUIRE(byte_product_lanes.back() == 1);
	const auto sad_lanes = sad.to_array();
	for (std::size_t lane = 0; lane + 1 < sad_lanes.size(); ++lane)
		REQUIRE(sad_lanes[lane] == 8);
	REQUIRE(sad_lanes.back() == 7);
}

/** @brief Verifies checked magnitude retains overflow metadata beyond a one-lane source prefix. */
template <std::size_t bits> void require_checked_magnitude_result_extent()
{
	using source_t = SimdLib::PartialRegister<std::int64_t, bits, (bits == 128 ? 1 : 3)>;
	const auto source = source_t::broadcast(std::numeric_limits<std::int64_t>::lowest());
	const auto checked = source.magnitude_checked();
	static_assert(std::same_as<decltype(checked), const SimdLib::Register<std::int64_t, bits>>);
	const auto lanes = checked.to_array();
	REQUIRE(lanes[0] == std::numeric_limits<std::int64_t>::max());
	REQUIRE(lanes[1] == -1);
	if constexpr (bits == 256)
	{
		REQUIRE(lanes[2] == std::numeric_limits<std::int64_t>::max());
		REQUIRE(lanes[3] == -1);
	}
}

/** @brief Verifies checked magnitude stops after the final occupied group's status lane. */
template <std::size_t bits> void require_partial_checked_magnitude_result_extent()
{
	using source_t = SimdLib::PartialRegister<std::int16_t, bits, (bits == 128 ? 5 : 15)>;
	using expected_t = SimdLib::PartialRegister<std::int16_t, bits, (bits == 128 ? 2 : 10)>;
	const auto source = source_t::from_array(std::array<std::int16_t, source_t::lane_count>{});
	const auto checked = source.magnitude_checked();
	static_assert(std::same_as<decltype(checked), const expected_t>);
	const auto lanes = checked.to_array();
	REQUIRE(lanes[0] == 0);
	REQUIRE(lanes[1] == 0);
	if constexpr (bits == 256)
	{
		REQUIRE(lanes[8] == 0);
		REQUIRE(lanes[9] == 0);
	}
	require_zero_suffix(checked);
}

/** @brief Verifies horizontal, saturated, alternating, and lane-combining adapters preserve their result contracts. */
void require_specialized_adapters()
{
	using horizontal_t = SimdLib::PartialRegister<std::int16_t, 128, 5>;
	const auto lhs = horizontal_t::from_lanes(1, 2, 3, 4, 5);
	const auto rhs = horizontal_t::from_lanes(10, 20, 30, 40, 50);
	REQUIRE(lhs.horizontal_add(rhs).to_array() == std::array<std::int16_t, 5>{3, 7, 5, 0, 30});
	REQUIRE(lhs.horizontal_subtract(rhs).to_array() == std::array<std::int16_t, 5>{-1, -1, 5, 0, -10});
	REQUIRE(lhs.horizontal_add_saturated(rhs).to_array() == lhs.horizontal_add(rhs).to_array());
	REQUIRE(lhs.horizontal_subtract_saturated(rhs).to_array() == lhs.horizontal_subtract(rhs).to_array());
	require_zero_suffix(lhs.horizontal_add_saturated(rhs));
	require_zero_suffix(lhs.horizontal_subtract_saturated(rhs));
	require_zero_suffix(lhs.add_saturated(rhs));
	require_zero_suffix(lhs.subtract_saturated(rhs));

	using byte_t = SimdLib::PartialRegister<std::uint8_t, 128, 7>;
	const auto bytes = byte_t::from_lanes(1, 2, 3, 4, 5, 6, 7);
	const auto adjacent = bytes.multiply_add_adjacent(bytes);
	REQUIRE(adjacent.to_array()[0] == 5);
	REQUIRE(adjacent.to_array()[1] == 25);
	REQUIRE(adjacent.to_array()[2] == 61);
	REQUIRE(adjacent.to_array()[3] == 49);
	require_zero_suffix(adjacent);
	const auto byte_products = bytes.multiply_add_unsigned_signed_bytes(bytes);
	REQUIRE(byte_products.to_array()[0] == 5);
	REQUIRE(byte_products.to_array()[1] == 25);
	REQUIRE(byte_products.to_array()[2] == 61);
	REQUIRE(byte_products.to_array()[3] == 49);
	require_zero_suffix(byte_products);
	const auto sad = bytes.sum_absolute_byte_differences(byte_t::zero());
	REQUIRE(sad.to_array()[0] == 28);
	require_zero_suffix(sad);
	const auto multi_sad = bytes.template multi_sum_absolute_byte_differences<0>(byte_t::zero());
	REQUIRE(multi_sad.to_array() == std::array<std::uint16_t, 8>{10, 14, 18, 22, 18, 13, 7, 0});

	using alternating_t = SimdLib::PartialRegister<float, 128, 3>;
	const auto floats = alternating_t::from_lanes(10.0F, 20.0F, 30.0F);
	const auto alternating = floats.add_subtract(alternating_t::broadcast(2.0F));
	REQUIRE(alternating.to_array() == std::array<float, 3>{8.0F, 22.0F, 28.0F});
	require_zero_suffix(alternating);

	using magnitude_t = SimdLib::PartialRegister<std::int16_t, 128, 5>;
	const auto magnitude_input = magnitude_t::from_lanes(3, 4, 0, 0, 0);
	const auto magnitude = magnitude_input.magnitude();
	const auto checked = magnitude_input.magnitude_checked();
	REQUIRE(magnitude.to_array()[0] == 5);
	REQUIRE(checked.to_array()[0] == 5);
	REQUIRE(checked.to_array()[1] == 0);
	require_zero_suffix(magnitude);
	require_zero_suffix(checked);
}

/** @brief Reports whether a value type exposes addition assignment. */
template <class value_t>
concept has_add_assign = requires(value_t value) { value += value; };

/** @brief Reports whether a value type exposes subtraction assignment. */
template <class value_t>
concept has_subtract_assign = requires(value_t value) { value -= value; };

/** @brief Reports whether a value type exposes a dot-product control that writes inactive lanes. */
template <class value_t>
concept has_inactive_dot_output = requires(value_t value) { value.template dot_product<0x7f>(value); };

static_assert(!has_add_assign<SimdLib::PartialRegister<std::int32_t, 128, 3>>);
static_assert(!has_subtract_assign<SimdLib::PartialRegister<std::int32_t, 128, 3>>);
static_assert(!has_inactive_dot_output<SimdLib::PartialRegister<float, 128, 3>>);

TEST_CASE("PartialRegister lane arithmetic follows scalar active-lane semantics", "[simdlib][partial-register][arithmetic]")
{
	require_lane_arithmetic<std::int8_t, 128, 15>();
	require_lane_arithmetic<std::uint8_t, 128, 15>();
	require_lane_arithmetic<std::int16_t, 128, 7>();
	require_lane_arithmetic<std::uint16_t, 128, 7>();
	require_lane_arithmetic<std::int32_t, 128, 3>();
	require_lane_arithmetic<std::uint32_t, 128, 3>();
	require_lane_arithmetic<std::int64_t, 128, 1>();
	require_lane_arithmetic<std::uint64_t, 128, 1>();
	require_lane_arithmetic<float, 128, 3>();
	require_lane_arithmetic<double, 128, 1>();
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
	require_lane_arithmetic<std::int8_t, 256, 31>();
	require_lane_arithmetic<std::uint8_t, 256, 31>();
	require_lane_arithmetic<std::int16_t, 256, 15>();
	require_lane_arithmetic<std::uint16_t, 256, 15>();
	require_lane_arithmetic<std::int32_t, 256, 7>();
	require_lane_arithmetic<std::uint32_t, 256, 7>();
	require_lane_arithmetic<std::int64_t, 256, 3>();
	require_lane_arithmetic<std::uint64_t, 256, 3>();
	require_lane_arithmetic<float, 256, 7>();
	require_lane_arithmetic<double, 256, 3>();
#endif
}

TEST_CASE("PartialRegister reductions and specialized results honor the logical prefix", "[simdlib][partial-register][specialized]")
{
	require_positions<std::int16_t, 128>();
	require_positions<std::uint16_t, 128>();
	require_floating_group_operations<128, 3>();
	require_zero_normalization_semantics();
	require_specialized_adapters();
	require_saturation_boundaries();
	require_complete_specialized_results<128>();
	require_checked_magnitude_result_extent<128>();
	require_partial_checked_magnitude_result_extent<128>();
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
	require_positions<std::int16_t, 256>();
	require_floating_group_operations<256, 5>();
	require_complete_specialized_results<256>();
	require_checked_magnitude_result_extent<256>();
	require_partial_checked_magnitude_result_extent<256>();
#endif
}

} // namespace
