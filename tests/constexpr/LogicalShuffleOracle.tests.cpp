#include "../LogicalShuffleTestSupport.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace
{

using namespace SimdLib::Tests::LogicalShuffle;

/**
 * @brief Reports whether every selector remains inside its output's 128-bit group.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @tparam selectors Logical source-lane selectors.
 * @return True when the complete selector array obeys the group-local contract.
 */
template <class element_t, std::size_t bits, auto selectors> [[nodiscard]] consteval bool selectors_are_group_local() noexcept
{
	constexpr std::size_t lane_count = bits / (sizeof(element_t) * 8);
	constexpr std::size_t lanes_per_group = 128 / (sizeof(element_t) * 8);
	if (selectors.size() != lane_count)
		return false;
	for (std::size_t output = 0; output < lane_count; ++output)
		if (selectors[output] >= lane_count || selectors[output] / lanes_per_group != output / lanes_per_group)
			return false;
	return true;
}

/**
 * @brief Validates selector construction and the scalar oracle for one SIMD shape.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @return True when all required selector patterns produce their manually defined lanes.
 */
template <class element_t, std::size_t bits> [[nodiscard]] consteval bool oracle_contract() noexcept
{
	constexpr std::size_t lanes_per_group = 128 / (sizeof(element_t) * 8);
	constexpr auto source = distinct_lanes<element_t, bits>();
	constexpr auto identity = identity_selectors<element_t, bits>();
	constexpr auto reverse = reverse_selectors<element_t, bits>();
	constexpr auto first = first_lane_selectors<element_t, bits>();
	constexpr auto last = last_lane_selectors<element_t, bits>();
	constexpr auto repeated = repeated_selectors<element_t, bits>();
	constexpr auto pair_swap = pair_swap_selectors<element_t, bits>();
	constexpr auto rotation = rotation_selectors<element_t, bits>();
	static_assert(selectors_are_group_local<element_t, bits, identity>());
	static_assert(selectors_are_group_local<element_t, bits, reverse>());
	static_assert(selectors_are_group_local<element_t, bits, first>());
	static_assert(selectors_are_group_local<element_t, bits, last>());
	static_assert(selectors_are_group_local<element_t, bits, repeated>());
	static_assert(selectors_are_group_local<element_t, bits, pair_swap>());
	static_assert(selectors_are_group_local<element_t, bits, rotation>());

	constexpr auto identity_result = logical_shuffle_oracle<element_t, bits, identity>(source);
	constexpr auto reverse_result = logical_shuffle_oracle<element_t, bits, reverse>(source);
	constexpr auto first_result = logical_shuffle_oracle<element_t, bits, first>(source);
	constexpr auto last_result = logical_shuffle_oracle<element_t, bits, last>(source);
	constexpr auto repeated_result = logical_shuffle_oracle<element_t, bits, repeated>(source);
	constexpr auto pair_swap_result = logical_shuffle_oracle<element_t, bits, pair_swap>(source);
	constexpr auto rotation_result = logical_shuffle_oracle<element_t, bits, rotation>(source);
	for (std::size_t lane = 0; lane < source.size(); ++lane)
	{
		const std::size_t group = lane / lanes_per_group * lanes_per_group;
		const std::size_t local = lane % lanes_per_group;
		if (std::bit_cast<object_bits_t<element_t>>(identity_result[lane]) != std::bit_cast<object_bits_t<element_t>>(source[lane]) ||
			std::bit_cast<object_bits_t<element_t>>(reverse_result[lane]) !=
				std::bit_cast<object_bits_t<element_t>>(source[group + lanes_per_group - 1 - local]) ||
			std::bit_cast<object_bits_t<element_t>>(first_result[lane]) != std::bit_cast<object_bits_t<element_t>>(source[group]) ||
			std::bit_cast<object_bits_t<element_t>>(last_result[lane]) != std::bit_cast<object_bits_t<element_t>>(source[group + lanes_per_group - 1]) ||
			std::bit_cast<object_bits_t<element_t>>(repeated_result[lane]) != std::bit_cast<object_bits_t<element_t>>(source[group + local / 2]) ||
			std::bit_cast<object_bits_t<element_t>>(pair_swap_result[lane]) != std::bit_cast<object_bits_t<element_t>>(source[lane ^ std::size_t{1}]) ||
			std::bit_cast<object_bits_t<element_t>>(rotation_result[lane]) !=
				std::bit_cast<object_bits_t<element_t>>(source[group + (local + 1) % lanes_per_group]))
			return false;
	}
	if constexpr (bits == 256)
	{
		constexpr auto distinct_groups = distinct_group_selectors<element_t>();
		static_assert(selectors_are_group_local<element_t, bits, distinct_groups>());
		constexpr auto result = logical_shuffle_oracle<element_t, bits, distinct_groups>(source);
		for (std::size_t lane = 0; lane < lanes_per_group; ++lane)
		{
			if (std::bit_cast<object_bits_t<element_t>>(result[lane]) != std::bit_cast<object_bits_t<element_t>>(source[lane]) ||
				std::bit_cast<object_bits_t<element_t>>(result[lanes_per_group + lane]) !=
					std::bit_cast<object_bits_t<element_t>>(source[2 * lanes_per_group - 1 - lane]))
				return false;
		}
		constexpr auto swapped_halves = swap_half_selectors<element_t>();
		constexpr auto mixed_halves = mixed_half_selectors<element_t>();
		constexpr auto full_reverse = full_reverse_selectors<element_t>();
		static_assert(!selectors_are_group_local<element_t, bits, swapped_halves>());
		static_assert(!selectors_are_group_local<element_t, bits, mixed_halves>());
		static_assert(!selectors_are_group_local<element_t, bits, full_reverse>());
		constexpr auto swapped_result = logical_shuffle_oracle<element_t, bits, swapped_halves>(source);
		constexpr auto mixed_result = logical_shuffle_oracle<element_t, bits, mixed_halves>(source);
		constexpr auto full_reverse_result = logical_shuffle_oracle<element_t, bits, full_reverse>(source);
		for (std::size_t lane = 0; lane < source.size(); ++lane)
		{
			const std::size_t opposite = (lane + lanes_per_group) % source.size();
			const std::size_t mixed_source = lane == 0 ? lanes_per_group : (lane == lanes_per_group ? 0 : lane);
			if (std::bit_cast<object_bits_t<element_t>>(swapped_result[lane]) != std::bit_cast<object_bits_t<element_t>>(source[opposite]) ||
				std::bit_cast<object_bits_t<element_t>>(mixed_result[lane]) != std::bit_cast<object_bits_t<element_t>>(source[mixed_source]) ||
				std::bit_cast<object_bits_t<element_t>>(full_reverse_result[lane]) != std::bit_cast<object_bits_t<element_t>>(source[source.size() - 1 - lane]))
				return false;
		}
	}
	return true;
}

static_assert(oracle_contract<std::int8_t, 128>());
static_assert(oracle_contract<std::uint8_t, 128>());
static_assert(oracle_contract<std::int16_t, 128>());
static_assert(oracle_contract<std::uint16_t, 128>());
static_assert(oracle_contract<std::int32_t, 128>());
static_assert(oracle_contract<std::uint32_t, 128>());
static_assert(oracle_contract<std::int64_t, 128>());
static_assert(oracle_contract<std::uint64_t, 128>());
static_assert(oracle_contract<float, 128>());
static_assert(oracle_contract<double, 128>());
static_assert(oracle_contract<std::int8_t, 256>());
static_assert(oracle_contract<std::uint8_t, 256>());
static_assert(oracle_contract<std::int16_t, 256>());
static_assert(oracle_contract<std::uint16_t, 256>());
static_assert(oracle_contract<std::int32_t, 256>());
static_assert(oracle_contract<std::uint32_t, 256>());
static_assert(oracle_contract<std::int64_t, 256>());
static_assert(oracle_contract<std::uint64_t, 256>());
static_assert(oracle_contract<float, 256>());
static_assert(oracle_contract<double, 256>());

} // namespace
