#include "LogicalShuffleTestSupport.h"

#include <SimdLib/Api.h>
#include <SimdLib/IApi.h>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

#ifndef SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH
#error "SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH must select the Api test width"
#endif

namespace
{

using namespace SimdLib::Tests::LogicalShuffle;

/**
 * @brief Invokes one Api logical shuffle by expanding a selector array.
 * @tparam api_t Api specialization under test.
 * @tparam selectors Logical source-lane selectors.
 * @tparam positions Output lane positions.
 * @param value Source native register.
 * @return Native register returned by the logical shuffle.
 */
template <class api_t, auto selectors, std::size_t... positions>
[[nodiscard]] auto invoke_api_shuffle(typename api_t::vector_t value, std::index_sequence<positions...>) noexcept
{
	return api_t::template shuffle<selectors[positions]...>(value);
}

/**
 * @brief Reports whether one Api exposes a complete logical selector sequence.
 * @tparam api_t Api specialization under test.
 * @tparam selectors Logical source-lane selectors.
 * @tparam positions Output lane positions.
 * @return True when the selector-pack overload participates in overload resolution.
 */
template <class api_t, auto selectors, std::size_t... positions>
[[nodiscard]] consteval bool api_accepts_shuffle_impl(std::index_sequence<positions...>) noexcept
{
	return SimdLib::IApi::Shuffle<api_t, selectors[positions]...>;
}

/**
 * @brief Reports whether one Api exposes its complete identity logical shuffle.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @return True when the desired logical-shuffle interface is available.
 */
template <class element_t, std::size_t bits> [[nodiscard]] consteval bool api_accepts_identity_shuffle() noexcept
{
	using api_t = SimdLib::Api<bits, element_t>;
	constexpr auto selectors = identity_selectors<element_t, bits>();
	return api_accepts_shuffle_impl<api_t, selectors>(std::make_index_sequence<api_t::element_count>{});
}

/**
 * @brief Compares one Api shuffle result against the independent scalar oracle.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @tparam selectors Logical source-lane selectors.
 */
template <class element_t, std::size_t bits, auto selectors> void require_api_shuffle() noexcept
{
	using api_t = SimdLib::Api<bits, element_t>;
	constexpr auto source = distinct_lanes<element_t, bits>();
	const auto actual = api_t::to_array(invoke_api_shuffle<api_t, selectors>(api_t::construct(source), std::make_index_sequence<api_t::element_count>{}));
	constexpr auto expected = logical_shuffle_oracle<element_t, bits, selectors>(source);
	REQUIRE(same_object_representations(actual, expected));
}

/**
 * @brief Exercises every required logical selector pattern for one Api specialization.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 */
template <class element_t, std::size_t bits> void require_api_shuffle_suite() noexcept
{
	require_api_shuffle<element_t, bits, identity_selectors<element_t, bits>()>();
	require_api_shuffle<element_t, bits, reverse_selectors<element_t, bits>()>();
	require_api_shuffle<element_t, bits, first_lane_selectors<element_t, bits>()>();
	require_api_shuffle<element_t, bits, last_lane_selectors<element_t, bits>()>();
	require_api_shuffle<element_t, bits, repeated_selectors<element_t, bits>()>();
	require_api_shuffle<element_t, bits, pair_swap_selectors<element_t, bits>()>();
	require_api_shuffle<element_t, bits, rotation_selectors<element_t, bits>()>();
	if constexpr (bits == 256)
		require_api_shuffle<element_t, bits, distinct_group_selectors<element_t>()>();
}

static_assert(api_accepts_identity_shuffle<std::int8_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>());
static_assert(api_accepts_identity_shuffle<std::uint8_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>());
static_assert(api_accepts_identity_shuffle<std::int16_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>());
static_assert(api_accepts_identity_shuffle<std::uint16_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>());
static_assert(api_accepts_identity_shuffle<std::int32_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>());
static_assert(api_accepts_identity_shuffle<std::uint32_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>());
static_assert(api_accepts_identity_shuffle<std::int64_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>());
static_assert(api_accepts_identity_shuffle<std::uint64_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>());
static_assert(api_accepts_identity_shuffle<float, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>());
static_assert(api_accepts_identity_shuffle<double, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>());

TEST_CASE("Api logical shuffle matches an independent object-representation oracle", "[simdlib][logical-shuffle]")
{
	require_api_shuffle_suite<std::int8_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>();
	require_api_shuffle_suite<std::uint8_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>();
	require_api_shuffle_suite<std::int16_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>();
	require_api_shuffle_suite<std::uint16_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>();
	require_api_shuffle_suite<std::int32_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>();
	require_api_shuffle_suite<std::uint32_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>();
	require_api_shuffle_suite<std::int64_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>();
	require_api_shuffle_suite<std::uint64_t, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>();
	require_api_shuffle_suite<float, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>();
	require_api_shuffle_suite<double, SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH>();
}

} // namespace
