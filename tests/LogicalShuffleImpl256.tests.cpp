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
 * @brief Invokes one 256-bit mapping-layer logical shuffle.
 * @tparam element_t Logical lane type.
 * @tparam selectors Logical source-lane selectors.
 * @tparam positions Output lane positions.
 * @param value Source native register.
 * @return Native register returned by the selected element specialization.
 */
template <class element_t, auto selectors, std::size_t... positions>
[[nodiscard]] auto invoke_mapping_shuffle(typename SimdLib::Api<256, element_t>::vector_t value, std::index_sequence<positions...>) noexcept
{
	using mapping_t = SimdLib::Detail::SimdMappings<256, element_t>;
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
 * @brief Reports whether one mapping accepts a complete selector array.
 * @tparam mapping_t Mapping specialization under test.
 * @tparam selectors Complete selector array.
 * @tparam positions Output lane positions.
 * @return True when the mapping accepts the expanded selector pack.
 */
template <class mapping_t, auto selectors, std::size_t... positions>
[[nodiscard]] consteval bool mapping_accepts_selectors(std::index_sequence<positions...>) noexcept
{
	return accepts_mapping_shuffle<mapping_t, selectors[positions]...>;
}

/**
 * @brief Compares one mapping shuffle result against the independent scalar oracle.
 * @tparam element_t Logical lane type.
 * @tparam selectors Logical source-lane selectors.
 */
template <class element_t, auto selectors> void require_mapping_shuffle() noexcept
{
	using api_t = SimdLib::Api<256, element_t>;
	constexpr auto source = distinct_lanes<element_t, 256>();
	const auto actual =
		api_t::to_array(invoke_mapping_shuffle<element_t, selectors>(api_t::construct(source), std::make_index_sequence<api_t::element_count>{}));
	constexpr auto expected = logical_shuffle_oracle<element_t, 256, selectors>(source);
	REQUIRE(same_object_representations(actual, expected));
}

/**
 * @brief Exercises local, cross-half, and mixed selector patterns for one 256-bit specialization.
 * @tparam element_t Logical lane type.
 */
template <class element_t> void require_mapping_shuffle_suite() noexcept
{
	require_mapping_shuffle<element_t, identity_selectors<element_t, 256>()>();
	require_mapping_shuffle<element_t, reverse_selectors<element_t, 256>()>();
	require_mapping_shuffle<element_t, first_lane_selectors<element_t, 256>()>();
	require_mapping_shuffle<element_t, last_lane_selectors<element_t, 256>()>();
	require_mapping_shuffle<element_t, repeated_selectors<element_t, 256>()>();
	require_mapping_shuffle<element_t, pair_swap_selectors<element_t, 256>()>();
	require_mapping_shuffle<element_t, rotation_selectors<element_t, 256>()>();
	require_mapping_shuffle<element_t, distinct_group_selectors<element_t>()>();
	require_mapping_shuffle<element_t, swap_half_selectors<element_t>()>();
	require_mapping_shuffle<element_t, mixed_half_selectors<element_t>()>();
	require_mapping_shuffle<element_t, full_reverse_selectors<element_t>()>();
}

using byte_mapping = SimdLib::Detail::SimdMappings<256, std::int8_t>;
using dword_mapping = SimdLib::Detail::SimdMappings<256, std::int32_t>;
constexpr auto byte_half_swap = swap_half_selectors<std::int8_t>();
static_assert(mapping_accepts_selectors<byte_mapping, byte_half_swap>(std::make_index_sequence<byte_mapping::element_count>{}));
static_assert(accepts_mapping_shuffle<dword_mapping, 4, 1, 2, 3, 0, 5, 6, 7>);
static_assert(!accepts_mapping_shuffle<dword_mapping, 0, 1, 2, 3, 4, 5, 6>);
static_assert(!accepts_mapping_shuffle<dword_mapping, 0, 1, 2, 3, 4, 5, 6, 8>);
static_assert(SimdLib::Detail::logical_shuffle_256_has_cross_half_selector<16, byte_half_swap>());
static_assert(!SimdLib::Detail::logical_shuffle_256_has_local_half_selector<16, byte_half_swap>());
static_assert(SimdLib::Detail::encode_logical_shuffle_256_byte<1, true, byte_half_swap, 0>() == 0);

TEST_CASE("256-bit mapping logical shuffle supports full-register lane selection", "[simdlib][logical-shuffle][backend]")
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
