#include "LogicalShuffleTestSupport.h"

#include <SimdLib/IRegister.h>
#include <SimdLib/Register.h>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

#ifndef SIMDLIB_REGISTER_TEST_ENABLE_256
#define SIMDLIB_REGISTER_TEST_ENABLE_256 SIMDLIB_HAS_AVX2
#endif

namespace
{

using namespace SimdLib::Tests::LogicalShuffle;

/**
 * @brief Invokes one Register logical shuffle by expanding a selector array.
 * @tparam register_t Register specialization under test.
 * @tparam selectors Logical source-lane selectors.
 * @tparam positions Output lane positions.
 * @param value Source Register.
 * @return Register returned by the logical shuffle.
 */
template <class register_t, auto selectors, std::size_t... positions>
[[nodiscard]] register_t invoke_register_shuffle(register_t value, std::index_sequence<positions...>) noexcept
{
	return value.template shuffle<selectors[positions]...>();
}

/**
 * @brief Reports whether one Register exposes a complete logical selector sequence.
 * @tparam register_t Register specialization under test.
 * @tparam selectors Logical source-lane selectors.
 * @tparam positions Output lane positions.
 * @return True when the selector-pack member participates in overload resolution.
 */
template <class register_t, auto selectors, std::size_t... positions>
[[nodiscard]] consteval bool register_accepts_shuffle_impl(std::index_sequence<positions...>) noexcept
{
	return SimdLib::IRegister::Shuffle<register_t, selectors[positions]...>;
}

/**
 * @brief Reports whether one Register exposes its complete identity logical shuffle.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @return True when the desired logical-shuffle interface is available.
 */
template <class element_t, std::size_t bits> [[nodiscard]] consteval bool register_accepts_identity_shuffle() noexcept
{
	using register_t = SimdLib::Register<element_t, bits>;
	constexpr auto selectors = identity_selectors<element_t, bits>();
	return register_accepts_shuffle_impl<register_t, selectors>(std::make_index_sequence<register_t::lane_count>{});
}

/**
 * @brief Compares one Register shuffle result against the independent scalar oracle.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @tparam selectors Logical source-lane selectors.
 */
template <class element_t, std::size_t bits, auto selectors> void require_register_shuffle() noexcept
{
	using register_t = SimdLib::Register<element_t, bits>;
	constexpr auto source = distinct_lanes<element_t, bits>();
	const auto actual =
		invoke_register_shuffle<register_t, selectors>(register_t::from_array(source), std::make_index_sequence<register_t::lane_count>{}).to_array();
	constexpr auto expected = logical_shuffle_oracle<element_t, bits, selectors>(source);
	REQUIRE(same_object_representations(actual, expected));
}

/**
 * @brief Exercises every required logical selector pattern for one Register specialization.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 */
template <class element_t, std::size_t bits> void require_register_shuffle_suite() noexcept
{
	require_register_shuffle<element_t, bits, identity_selectors<element_t, bits>()>();
	require_register_shuffle<element_t, bits, reverse_selectors<element_t, bits>()>();
	require_register_shuffle<element_t, bits, first_lane_selectors<element_t, bits>()>();
	require_register_shuffle<element_t, bits, last_lane_selectors<element_t, bits>()>();
	require_register_shuffle<element_t, bits, repeated_selectors<element_t, bits>()>();
	require_register_shuffle<element_t, bits, pair_swap_selectors<element_t, bits>()>();
	require_register_shuffle<element_t, bits, rotation_selectors<element_t, bits>()>();
	if constexpr (bits == 256)
		require_register_shuffle<element_t, bits, distinct_group_selectors<element_t>()>();
}

static_assert(register_accepts_identity_shuffle<std::int8_t, 128>());
static_assert(register_accepts_identity_shuffle<std::uint8_t, 128>());
static_assert(register_accepts_identity_shuffle<std::int16_t, 128>());
static_assert(register_accepts_identity_shuffle<std::uint16_t, 128>());
static_assert(register_accepts_identity_shuffle<std::int32_t, 128>());
static_assert(register_accepts_identity_shuffle<std::uint32_t, 128>());
static_assert(register_accepts_identity_shuffle<std::int64_t, 128>());
static_assert(register_accepts_identity_shuffle<std::uint64_t, 128>());
static_assert(register_accepts_identity_shuffle<float, 128>());
static_assert(register_accepts_identity_shuffle<double, 128>());

#if SIMDLIB_REGISTER_TEST_ENABLE_256
static_assert(register_accepts_identity_shuffle<std::int8_t, 256>());
static_assert(register_accepts_identity_shuffle<std::uint8_t, 256>());
static_assert(register_accepts_identity_shuffle<std::int16_t, 256>());
static_assert(register_accepts_identity_shuffle<std::uint16_t, 256>());
static_assert(register_accepts_identity_shuffle<std::int32_t, 256>());
static_assert(register_accepts_identity_shuffle<std::uint32_t, 256>());
static_assert(register_accepts_identity_shuffle<std::int64_t, 256>());
static_assert(register_accepts_identity_shuffle<std::uint64_t, 256>());
static_assert(register_accepts_identity_shuffle<float, 256>());
static_assert(register_accepts_identity_shuffle<double, 256>());
#endif

TEST_CASE("Register logical shuffle matches an independent object-representation oracle", "[simdlib][register][logical-shuffle]")
{
	require_register_shuffle_suite<std::int8_t, 128>();
	require_register_shuffle_suite<std::uint8_t, 128>();
	require_register_shuffle_suite<std::int16_t, 128>();
	require_register_shuffle_suite<std::uint16_t, 128>();
	require_register_shuffle_suite<std::int32_t, 128>();
	require_register_shuffle_suite<std::uint32_t, 128>();
	require_register_shuffle_suite<std::int64_t, 128>();
	require_register_shuffle_suite<std::uint64_t, 128>();
	require_register_shuffle_suite<float, 128>();
	require_register_shuffle_suite<double, 128>();
#if SIMDLIB_REGISTER_TEST_ENABLE_256
	require_register_shuffle_suite<std::int8_t, 256>();
	require_register_shuffle_suite<std::uint8_t, 256>();
	require_register_shuffle_suite<std::int16_t, 256>();
	require_register_shuffle_suite<std::uint16_t, 256>();
	require_register_shuffle_suite<std::int32_t, 256>();
	require_register_shuffle_suite<std::uint32_t, 256>();
	require_register_shuffle_suite<std::int64_t, 256>();
	require_register_shuffle_suite<std::uint64_t, 256>();
	require_register_shuffle_suite<float, 256>();
	require_register_shuffle_suite<double, 256>();
#endif
}

} // namespace
