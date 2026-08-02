#include <SimdLib/PartialRegister.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#ifndef SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
#define SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256 SIMDLIB_HAS_AVX2
#endif

namespace
{

/** @brief Produces one nonzero value suitable for a PartialRegister test lane. */
template <class element_t> [[nodiscard]] constexpr element_t nonzero_lane_value() noexcept
{
	if constexpr (std::is_floating_point_v<element_t>)
		return static_cast<element_t>(7.25);
	else
		return static_cast<element_t>(7);
}

/** @brief Reports whether every byte in one scalar representation is zero. */
template <class element_t> [[nodiscard]] constexpr bool has_all_zero_bits(const element_t value) noexcept
{
	const auto bytes = std::bit_cast<std::array<std::byte, sizeof(element_t)>>(value);
	return std::all_of(bytes.begin(), bytes.end(), [](const std::byte byte) constexpr noexcept { return byte == std::byte{}; });
}

/** @brief Produces the canonical all-bits-one predicate representation for one lane geometry. */
template <class element_t> [[nodiscard]] constexpr element_t all_one_lane_value() noexcept
{
	std::array<std::byte, sizeof(element_t)> bytes{};
	for (auto &byte : bytes)
		byte = std::byte{0xff};
	return std::bit_cast<element_t>(bytes);
}

/** @brief Verifies native import sanitization and zero construction for one partial extent. */
template <class element_t, std::size_t bits, std::size_t active_lane_count> void require_native_boundary_contract()
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_lane_count>;
	using api_t = typename value_t::api_type;

	std::array<element_t, value_t::native_lane_count> source{};
	for (std::size_t lane = 0; lane < source.size(); ++lane)
		source[lane] = static_cast<element_t>(nonzero_lane_value<element_t>() + static_cast<element_t>(lane));

	const auto imported = value_t::from_native(api_t::construct(source));
	const auto imported_lanes = api_t::to_array(imported.to_native());
	for (std::size_t lane = 0; lane < value_t::lane_count; ++lane)
		REQUIRE(imported_lanes[lane] == source[lane]);
	for (std::size_t lane = value_t::lane_count; lane < value_t::native_lane_count; ++lane)
		REQUIRE(has_all_zero_bits(imported_lanes[lane]));

	const auto default_lanes = api_t::to_array(value_t{}.to_native());
	const auto zero_lanes = api_t::to_array(value_t::zero().to_native());
	for (std::size_t lane = 0; lane < value_t::native_lane_count; ++lane)
	{
		REQUIRE(has_all_zero_bits(default_lanes[lane]));
		REQUIRE(has_all_zero_bits(zero_lanes[lane]));
	}
}

/** @brief Verifies the native boundary for the smallest and largest partial extents of one geometry. */
template <class element_t, std::size_t bits> void require_native_boundary_extremes()
{
	using api_t = SimdLib::Api<bits, element_t>;
	require_native_boundary_contract<element_t, bits, 1>();
	require_native_boundary_contract<element_t, bits, api_t::element_count - 1>();
}

/** @brief Verifies active-only predicate reductions, composition, selection, and suffix sanitization. */
template <class element_t, std::size_t bits, std::size_t active_lane_count> void require_mask_contract()
{
	using mask_t = SimdLib::PartialRegisterMask<element_t, bits, active_lane_count>;
	using value_t = typename mask_t::register_type;
	using api_t = typename mask_t::api_type;
	std::array<element_t, mask_t::native_lane_count> true_lanes{};
	std::array<element_t, mask_t::native_lane_count> false_lanes{};
	std::array<element_t, mask_t::native_lane_count> mixed_lanes{};
	std::array<element_t, mask_t::native_lane_count> true_values{};
	std::array<element_t, mask_t::native_lane_count> false_values{};
	for (std::size_t lane = 0; lane < mask_t::native_lane_count; ++lane)
	{
		true_lanes[lane] = all_one_lane_value<element_t>();
		false_lanes[lane] = element_t{};
		mixed_lanes[lane] = lane % 2 == 0 ? all_one_lane_value<element_t>() : element_t{};
		true_values[lane] = static_cast<element_t>(10 + lane);
		false_values[lane] = static_cast<element_t>(20 + lane);
	}
	const auto all_true = mask_t::from_native(api_t::construct(true_lanes));
	const auto all_false = mask_t::from_native(api_t::construct(false_lanes));
	const auto mixed = mask_t::from_native(api_t::construct(mixed_lanes));
	REQUIRE(all_true.any());
	REQUIRE(all_true.all());
	REQUIRE_FALSE(all_true.none());
	REQUIRE(all_true.bits() == (typename mask_t::bits_type{1} << mask_t::lane_count) - 1);
	REQUIRE_FALSE(all_false.any());
	REQUIRE_FALSE(all_false.all());
	REQUIRE(all_false.none());
	REQUIRE((all_true & all_false).none());
	REQUIRE((all_true | all_false).all());
	REQUIRE((all_true ^ all_true).none());
	REQUIRE((~all_true).none());
	REQUIRE((~all_false).all());
	const auto imported_lanes = api_t::to_array(all_true.to_native());
	for (std::size_t lane = mask_t::lane_count; lane < mask_t::native_lane_count; ++lane)
		REQUIRE(has_all_zero_bits(imported_lanes[lane]));

	const auto selected = all_true.select(value_t::from_native(api_t::construct(true_values)), value_t::from_native(api_t::construct(false_values)));
	const auto mixed_selected = mixed.select(value_t::from_native(api_t::construct(true_values)), value_t::from_native(api_t::construct(false_values)));
	const auto selected_lanes = api_t::to_array(selected.to_native());
	const auto mixed_selected_lanes = api_t::to_array(mixed_selected.to_native());
	const auto predicate_lanes = api_t::to_array((~all_true).to_native());
	for (std::size_t lane = 0; lane < mask_t::lane_count; ++lane)
	{
		REQUIRE(selected_lanes[lane] == true_values[lane]);
		REQUIRE(mixed_selected_lanes[lane] == (lane % 2 == 0 ? true_values[lane] : false_values[lane]));
	}
	for (std::size_t lane = mask_t::lane_count; lane < mask_t::native_lane_count; ++lane)
	{
		REQUIRE(has_all_zero_bits(selected_lanes[lane]));
		REQUIRE(has_all_zero_bits(predicate_lanes[lane]));
	}
}

/** @brief Verifies mask behavior at the smallest and largest partial extents of one geometry. */
template <class element_t, std::size_t bits> void require_mask_contract_extremes()
{
	using api_t = SimdLib::Api<bits, element_t>;
	require_mask_contract<element_t, bits, 1>();
	require_mask_contract<element_t, bits, api_t::element_count - 1>();
}

/** @brief Verifies every non-complete predicate extent for one element and native width. */
template <class element_t, std::size_t bits, std::size_t... active_lane_counts>
void require_mask_contracts(std::index_sequence<active_lane_counts...>)
{
	(require_mask_contract<element_t, bits, active_lane_counts + 1>(), ...);
}

/** @brief Verifies all supported predicate extents for one element and native width. */
template <class element_t, std::size_t bits> void require_all_mask_contracts()
{
	require_mask_contracts<element_t, bits>(std::make_index_sequence<SimdLib::Api<bits, element_t>::element_count - 1>{});
}

/** @brief Runs the native-boundary matrix for all supported scalar interpretations at one width. */
template <std::size_t bits> void require_native_boundary_matrix()
{
	require_native_boundary_extremes<std::int8_t, bits>();
	require_native_boundary_extremes<std::uint8_t, bits>();
	require_native_boundary_extremes<std::int16_t, bits>();
	require_native_boundary_extremes<std::uint16_t, bits>();
	require_native_boundary_extremes<std::int32_t, bits>();
	require_native_boundary_extremes<std::uint32_t, bits>();
	require_native_boundary_extremes<std::int64_t, bits>();
	require_native_boundary_extremes<std::uint64_t, bits>();
	require_native_boundary_extremes<float, bits>();
	require_native_boundary_extremes<double, bits>();
	require_all_mask_contracts<std::int8_t, bits>();
	require_all_mask_contracts<std::uint8_t, bits>();
	require_all_mask_contracts<std::int16_t, bits>();
	require_all_mask_contracts<std::uint16_t, bits>();
	require_all_mask_contracts<std::int32_t, bits>();
	require_all_mask_contracts<std::uint32_t, bits>();
	require_all_mask_contracts<std::int64_t, bits>();
	require_all_mask_contracts<std::uint64_t, bits>();
	require_all_mask_contracts<float, bits>();
	require_all_mask_contracts<double, bits>();
}

} // namespace

/** @brief Verifies PartialRegister native boundaries for the 128-bit SIMD profile. */
TEST_CASE("PartialRegister native boundaries preserve the inactive suffix", "[PARTIAL_REGISTER][SSE42]")
{
	require_native_boundary_matrix<128>();
}

#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
/** @brief Verifies PartialRegister native boundaries for the 256-bit SIMD profile. */
TEST_CASE("PartialRegister native boundaries preserve the inactive suffix at 256 bits", "[PARTIAL_REGISTER][AVX2]")
{
	require_native_boundary_matrix<256>();
}
#endif
