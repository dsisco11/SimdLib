#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace SimdLib::Tests::LogicalShuffle
{

/** @brief Maps an object size to an unsigned integer with the same representation size. */
template <std::size_t byte_count> struct unsigned_bits;

/** @brief Maps one-byte objects to an unsigned representation type. */
template <> struct unsigned_bits<1>
{
	using type = std::uint8_t;
};

/** @brief Maps two-byte objects to an unsigned representation type. */
template <> struct unsigned_bits<2>
{
	using type = std::uint16_t;
};

/** @brief Maps four-byte objects to an unsigned representation type. */
template <> struct unsigned_bits<4>
{
	using type = std::uint32_t;
};

/** @brief Maps eight-byte objects to an unsigned representation type. */
template <> struct unsigned_bits<8>
{
	using type = std::uint64_t;
};

/** @brief Unsigned integer type that preserves one element's complete object representation. */
template <class element_t> using object_bits_t = typename unsigned_bits<sizeof(element_t)>::type;

/**
 * @brief Creates one deterministic integer lane with nonuniform bytes.
 * @tparam element_t Integral lane type.
 * @param lane Logical lane index.
 * @return Element whose object representation is unique within every supported register width.
 */
template <class element_t> [[nodiscard]] constexpr element_t distinct_integer_lane(const std::size_t lane) noexcept
{
	using bits_t = object_bits_t<element_t>;
	bits_t bits{};
	if constexpr (sizeof(element_t) == 1)
		bits = static_cast<bits_t>(0xA5u + lane * 0x3Du);
	else if constexpr (sizeof(element_t) == 2)
		bits = static_cast<bits_t>(0xA55Au + lane * 0x1F3Du);
	else if constexpr (sizeof(element_t) == 4)
		bits = static_cast<bits_t>(0xA55AC33Cu + lane * 0x01020409u);
	else
		bits = static_cast<bits_t>(UINT64_C(0xA55AC33CF00F9669) + lane * UINT64_C(0x0102040810204081));
	return std::bit_cast<element_t>(bits);
}

/**
 * @brief Creates one floating lane with a deliberately observable object representation.
 * @tparam element_t Floating-point lane type.
 * @param lane Logical lane index.
 * @return Element selected from finite, signed-zero, infinity, subnormal, and NaN bit patterns.
 */
template <class element_t> [[nodiscard]] constexpr element_t distinct_floating_lane(const std::size_t lane) noexcept
{
	if constexpr (sizeof(element_t) == 4)
	{
		constexpr std::array<std::uint32_t, 8> patterns{0x00000000u, 0x80000000u, 0x7FC00001u, 0xFFC12345u, 0x3F800001u, 0xBF000003u, 0x00800005u, 0x7F800000u};
		return std::bit_cast<element_t>(patterns[lane]);
	}
	else
	{
		constexpr std::array<std::uint64_t, 4> patterns{UINT64_C(0x8000000000000000), UINT64_C(0x7FF8000000000001), UINT64_C(0x0000000000000000),
														UINT64_C(0xFFF8123456789ABC)};
		return std::bit_cast<element_t>(patterns[lane]);
	}
}

/**
 * @brief Creates unique lane representations for one supported SIMD shape.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @return Complete source lane array.
 */
template <class element_t, std::size_t bits> [[nodiscard]] constexpr std::array<element_t, bits / (sizeof(element_t) * 8)> distinct_lanes() noexcept
{
	std::array<element_t, bits / (sizeof(element_t) * 8)> result{};
	for (std::size_t lane = 0; lane < result.size(); ++lane)
	{
		if constexpr (std::is_floating_point_v<element_t>)
			result[lane] = distinct_floating_lane<element_t>(lane);
		else
			result[lane] = distinct_integer_lane<element_t>(lane);
	}
	return result;
}

/**
 * @brief Builds identity selectors for one logical SIMD shape.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @return One selector per output lane.
 */
template <class element_t, std::size_t bits> [[nodiscard]] consteval auto identity_selectors() noexcept
{
	std::array<std::size_t, bits / (sizeof(element_t) * 8)> result{};
	for (std::size_t lane = 0; lane < result.size(); ++lane)
		result[lane] = lane;
	return result;
}

/**
 * @brief Builds a complete reversal inside each 128-bit source group.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @return One group-local selector per output lane.
 */
template <class element_t, std::size_t bits> [[nodiscard]] consteval auto reverse_selectors() noexcept
{
	constexpr std::size_t lanes_per_group = 128 / (sizeof(element_t) * 8);
	auto result = identity_selectors<element_t, bits>();
	for (std::size_t lane = 0; lane < result.size(); ++lane)
		result[lane] = lane / lanes_per_group * lanes_per_group + lanes_per_group - 1 - lane % lanes_per_group;
	return result;
}

/**
 * @brief Builds selectors that broadcast the first lane of every 128-bit group.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @return One group-local selector per output lane.
 */
template <class element_t, std::size_t bits> [[nodiscard]] consteval auto first_lane_selectors() noexcept
{
	constexpr std::size_t lanes_per_group = 128 / (sizeof(element_t) * 8);
	auto result = identity_selectors<element_t, bits>();
	for (std::size_t lane = 0; lane < result.size(); ++lane)
		result[lane] = lane / lanes_per_group * lanes_per_group;
	return result;
}

/**
 * @brief Builds selectors that broadcast the last lane of every 128-bit group.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @return One group-local selector per output lane.
 */
template <class element_t, std::size_t bits> [[nodiscard]] consteval auto last_lane_selectors() noexcept
{
	constexpr std::size_t lanes_per_group = 128 / (sizeof(element_t) * 8);
	auto result = identity_selectors<element_t, bits>();
	for (std::size_t lane = 0; lane < result.size(); ++lane)
		result[lane] = lane / lanes_per_group * lanes_per_group + lanes_per_group - 1;
	return result;
}

/**
 * @brief Builds selectors containing repeated adjacent source lanes.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @return One group-local selector per output lane.
 */
template <class element_t, std::size_t bits> [[nodiscard]] consteval auto repeated_selectors() noexcept
{
	constexpr std::size_t lanes_per_group = 128 / (sizeof(element_t) * 8);
	auto result = identity_selectors<element_t, bits>();
	for (std::size_t lane = 0; lane < result.size(); ++lane)
		result[lane] = lane / lanes_per_group * lanes_per_group + (lane % lanes_per_group) / 2;
	return result;
}

/**
 * @brief Builds selectors that swap every adjacent logical lane pair.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @return One group-local selector per output lane.
 */
template <class element_t, std::size_t bits> [[nodiscard]] consteval auto pair_swap_selectors() noexcept
{
	auto result = identity_selectors<element_t, bits>();
	for (std::size_t lane = 0; lane < result.size(); ++lane)
		result[lane] = lane ^ std::size_t{1};
	return result;
}

/**
 * @brief Builds a one-lane left rotation inside each 128-bit group.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @return One group-local selector per output lane.
 */
template <class element_t, std::size_t bits> [[nodiscard]] consteval auto rotation_selectors() noexcept
{
	constexpr std::size_t lanes_per_group = 128 / (sizeof(element_t) * 8);
	auto result = identity_selectors<element_t, bits>();
	for (std::size_t lane = 0; lane < result.size(); ++lane)
		result[lane] = lane / lanes_per_group * lanes_per_group + (lane % lanes_per_group + 1) % lanes_per_group;
	return result;
}

/**
 * @brief Builds different lower- and upper-group permutations for a 256-bit shape.
 * @tparam element_t Logical lane type.
 * @return Identity selectors below bit 128 and reversed selectors above bit 128.
 */
template <class element_t> [[nodiscard]] consteval auto distinct_group_selectors() noexcept
{
	constexpr std::size_t lanes_per_group = 128 / (sizeof(element_t) * 8);
	auto result = identity_selectors<element_t, 256>();
	for (std::size_t lane = lanes_per_group; lane < result.size(); ++lane)
		result[lane] = lanes_per_group + lanes_per_group - 1 - lane % lanes_per_group;
	return result;
}

/**
 * @brief Expands one compile-time selector array into an independent scalar shuffle result.
 * @tparam selectors Logical source-lane selector array.
 * @tparam element_t Logical lane type.
 * @tparam lane_count Number of source and result lanes.
 * @tparam positions Output lane sequence.
 * @param source Source lane array.
 * @return Scalar-oracle result array.
 */
template <auto selectors, class element_t, std::size_t lane_count, std::size_t... positions>
[[nodiscard]] constexpr std::array<element_t, lane_count> logical_shuffle_oracle_impl(const std::array<element_t, lane_count> &source,
																					  std::index_sequence<positions...>) noexcept
{
	return {source[selectors[positions]]...};
}

/**
 * @brief Applies a compile-time logical selector sequence without using Api or Register code.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @tparam selectors One source-lane selector per output lane.
 * @param source Source lane array.
 * @return Independently computed scalar result.
 */
template <class element_t, std::size_t bits, auto selectors>
[[nodiscard]] constexpr auto logical_shuffle_oracle(const std::array<element_t, bits / (sizeof(element_t) * 8)> &source) noexcept
{
	constexpr std::size_t lane_count = bits / (sizeof(element_t) * 8);
	static_assert(selectors.size() == lane_count);
	return logical_shuffle_oracle_impl<selectors>(source, std::make_index_sequence<lane_count>{});
}

/**
 * @brief Compares lane arrays by object representation.
 * @tparam element_t Logical lane type.
 * @tparam lane_count Number of compared lanes.
 * @param lhs Left lane array.
 * @param rhs Right lane array.
 * @return True only when every corresponding lane contains identical bits.
 */
template <class element_t, std::size_t lane_count>
[[nodiscard]] constexpr bool same_object_representations(const std::array<element_t, lane_count> &lhs, const std::array<element_t, lane_count> &rhs) noexcept
{
	for (std::size_t lane = 0; lane < lane_count; ++lane)
		if (std::bit_cast<object_bits_t<element_t>>(lhs[lane]) != std::bit_cast<object_bits_t<element_t>>(rhs[lane]))
			return false;
	return true;
}

} // namespace SimdLib::Tests::LogicalShuffle
