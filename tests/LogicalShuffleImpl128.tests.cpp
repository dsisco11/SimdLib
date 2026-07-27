#include "LogicalShuffleTestSupport.h"

#include <SimdLib/Api.h>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

namespace
{

using namespace SimdLib::Tests::LogicalShuffle;

/**
 * @brief Invokes one 128-bit mapping-layer logical shuffle.
 * @tparam element_t Logical lane type.
 * @tparam selectors Logical source-lane selectors.
 * @tparam positions Output lane positions.
 * @param value Source native register.
 * @return Native register returned by the selected element specialization.
 */
template <class element_t, auto selectors, std::size_t... positions>
[[nodiscard]] auto invoke_mapping_shuffle(typename SimdLib::Api<128, element_t>::vector_t value, std::index_sequence<positions...>) noexcept
{
	using mapping_t = SimdLib::Detail::SimdMappings<128, element_t>;
	return mapping_t::template shuffle<selectors[positions]...>(value);
}

/**
 * @brief Reports whether one mapping exposes the supplied logical selector sequence.
 * @tparam mapping_t Mapping specialization under test.
 * @tparam indices Logical source-lane selectors.
 */
template <class mapping_t, std::size_t... indices>
concept accepts_mapping_shuffle = requires(typename mapping_t::vector_t value) { mapping_t::template shuffle<indices...>(value); };

/**
 * @brief Compares one mapping shuffle result against the independent scalar oracle.
 * @tparam element_t Logical lane type.
 * @tparam selectors Logical source-lane selectors.
 */
template <class element_t, auto selectors> void require_mapping_shuffle() noexcept
{
	using api_t = SimdLib::Api<128, element_t>;
	constexpr auto source = distinct_lanes<element_t, 128>();
	const auto actual =
		api_t::to_array(invoke_mapping_shuffle<element_t, selectors>(api_t::construct(source), std::make_index_sequence<api_t::element_count>{}));
	constexpr auto expected = logical_shuffle_oracle<element_t, 128, selectors>(source);
	REQUIRE(same_object_representations(actual, expected));
}

/**
 * @brief Exercises every required selector pattern for one 128-bit element specialization.
 * @tparam element_t Logical lane type.
 */
template <class element_t> void require_mapping_shuffle_suite() noexcept
{
	require_mapping_shuffle<element_t, identity_selectors<element_t, 128>()>();
	require_mapping_shuffle<element_t, reverse_selectors<element_t, 128>()>();
	require_mapping_shuffle<element_t, first_lane_selectors<element_t, 128>()>();
	require_mapping_shuffle<element_t, last_lane_selectors<element_t, 128>()>();
	require_mapping_shuffle<element_t, repeated_selectors<element_t, 128>()>();
	require_mapping_shuffle<element_t, pair_swap_selectors<element_t, 128>()>();
	require_mapping_shuffle<element_t, rotation_selectors<element_t, 128>()>();
}

using int32_mapping = SimdLib::Detail::SimdMappings<128, std::int32_t>;
static_assert(accepts_mapping_shuffle<int32_mapping, 0, 1, 2, 3>);
static_assert(!accepts_mapping_shuffle<int32_mapping, 0, 1, 2>);
static_assert(!accepts_mapping_shuffle<int32_mapping, 0, 1, 2, 4>);
static_assert(SimdLib::Detail::encode_logical_shuffle_32_immediate<3, 2, 1, 0>() == 0x1B);
static_assert(SimdLib::Detail::encode_logical_shuffle_16_byte(3, 0) == 6);
static_assert(SimdLib::Detail::encode_logical_shuffle_16_byte(3, 1) == 7);
static_assert(SimdLib::Detail::encode_logical_shuffle_64_immediate<1, 0>() == 0x4E);
static_assert(SimdLib::Detail::encode_logical_shuffle_double_immediate<1, 0>() == 0x01);

TEST_CASE("128-bit mapping logical shuffle matches an independent object-representation oracle", "[simdlib][logical-shuffle][backend]")
{
	require_mapping_shuffle_suite<std::int8_t>();
	require_mapping_shuffle_suite<std::uint8_t>();
	require_mapping_shuffle_suite<std::int16_t>();
	require_mapping_shuffle_suite<std::uint16_t>();
	require_mapping_shuffle_suite<std::int32_t>();
	require_mapping_shuffle_suite<std::uint32_t>();
	require_mapping_shuffle_suite<std::int64_t>();
	require_mapping_shuffle_suite<std::uint64_t>();
	require_mapping_shuffle_suite<float>();
	require_mapping_shuffle_suite<double>();
}

} // namespace
