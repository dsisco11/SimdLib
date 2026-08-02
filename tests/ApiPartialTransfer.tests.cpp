#include <SimdLib/Api.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#ifndef SIMDLIB_API_PARTIAL_TRANSFER_TEST_WIDTH
#error "SIMDLIB_API_PARTIAL_TRANSFER_TEST_WIDTH must select the tested register width"
#endif

namespace
{

/** @brief Produces one deterministic exactly representable lane value. */
template <class element_t> [[nodiscard]] constexpr element_t lane_value(std::size_t index) noexcept
{
	return static_cast<element_t>(index + 11);
}

/** @brief Reports whether one scalar has an all-bits-zero object representation. */
template <class element_t> [[nodiscard]] constexpr bool has_zero_bits(element_t value) noexcept
{
	const auto bytes = std::bit_cast<std::array<std::byte, sizeof(element_t)>>(value);
	return std::all_of(bytes.begin(), bytes.end(), [](std::byte byte) constexpr noexcept { return byte == std::byte{}; });
}

/** @brief Verifies every element-prefix load, store, and observation operation for one extent. */
template <class api_t, std::size_t active_count> [[nodiscard]] bool has_element_prefix_contract()
{
	using element_t = typename api_t::element_type;
	std::array<element_t, api_t::element_count> source{};
	for (std::size_t index = 0; index < source.size(); ++index)
		source[index] = lane_value<element_t>(index);

	std::array<element_t, api_t::element_count + 2> unaligned_source{};
	std::copy(source.begin(), source.end(), unaligned_source.begin() + 1);
	const auto loaded = api_t::template load_partial<active_count>(
		std::span<const element_t>{unaligned_source.data() + 1, active_count});
	const auto loaded_lanes = api_t::to_array(loaded);
	for (std::size_t index = 0; index < active_count; ++index)
		if (loaded_lanes[index] != source[index])
			return false;
	for (std::size_t index = active_count; index < api_t::element_count; ++index)
		if (!has_zero_bits(loaded_lanes[index]))
			return false;

	alignas(api_t::byte_count) std::array<element_t, api_t::element_count * 2> aligned_source{};
	std::copy(source.begin(), source.end(), aligned_source.begin() + api_t::element_count);
	const auto aligned_loaded = api_t::template load_partial_aligned<active_count>(
		std::span<const element_t>{aligned_source.data() + api_t::element_count, active_count});
	if (api_t::to_array(aligned_loaded) != loaded_lanes)
		return false;

	const auto observed = api_t::template to_array_partial<active_count>(loaded);
	if (!std::equal(observed.begin(), observed.end(), source.begin()))
		return false;

	const auto canary = lane_value<element_t>(api_t::element_count + 19);
	std::array<element_t, active_count + 2> destination{};
	destination.fill(canary);
	api_t::template store_partial<active_count>(loaded, std::span<element_t>{destination.data() + 1, active_count});
	if (destination.front() != canary || destination.back() != canary ||
		!std::equal(source.begin(), source.begin() + active_count, destination.begin() + 1))
		return false;

	alignas(api_t::byte_count) std::array<element_t, api_t::element_count * 2 + 1> aligned_destination{};
	aligned_destination.fill(canary);
	api_t::template store_partial_aligned<active_count>(
		loaded, std::span<element_t>{aligned_destination.data() + api_t::element_count, active_count});
	return std::all_of(aligned_destination.begin(), aligned_destination.begin() + api_t::element_count,
			[](element_t value) noexcept { return value == lane_value<element_t>(api_t::element_count + 19); }) &&
		   aligned_destination.back() == canary &&
		   std::equal(source.begin(), source.begin() + active_count, aligned_destination.begin() + api_t::element_count);
}

/** @brief Verifies partial scalar broadcast for one active-lane extent. */
template <class api_t, std::size_t active_count> [[nodiscard]] bool has_partial_broadcast_contract()
{
	using element_t = typename api_t::element_type;
	const auto expected = lane_value<element_t>(7);
	const auto lanes = api_t::to_array(api_t::template broadcast_partial<active_count>(expected));
	for (std::size_t index = 0; index < active_count; ++index)
		if (lanes[index] != expected)
			return false;
	for (std::size_t index = active_count; index < api_t::element_count; ++index)
		if (!has_zero_bits(lanes[index]))
			return false;
	return true;
}

/** @brief Verifies every byte-prefix load and store operation for one extent. */
template <class api_t, std::size_t active_byte_count> [[nodiscard]] bool has_byte_prefix_contract()
{
	std::array<std::byte, api_t::byte_count> source{};
	for (std::size_t index = 0; index < source.size(); ++index)
		source[index] = static_cast<std::byte>(index + 1);
	std::array<std::byte, api_t::byte_count + 2> unaligned_source{};
	std::copy(source.begin(), source.end(), unaligned_source.begin() + 1);
	const auto loaded = api_t::template load_bytes_partial<active_byte_count>(
		std::span<const std::byte>{unaligned_source.data() + 1, active_byte_count});
	std::array<std::byte, api_t::byte_count> loaded_bytes{};
	api_t::store(loaded, std::span<std::byte, api_t::byte_count>{loaded_bytes});
	for (std::size_t index = 0; index < active_byte_count; ++index)
		if (loaded_bytes[index] != source[index])
			return false;
	for (std::size_t index = active_byte_count; index < api_t::byte_count; ++index)
		if (loaded_bytes[index] != std::byte{})
			return false;

	constexpr std::byte canary{0xa5};
	std::array<std::byte, active_byte_count + 2> destination{};
	destination.fill(canary);
	api_t::template store_bytes_partial<active_byte_count>(
		loaded, std::span<std::byte>{destination.data() + 1, active_byte_count});
	return destination.front() == canary && destination.back() == canary &&
		   std::equal(source.begin(), source.begin() + active_byte_count, destination.begin() + 1);
}

/** @brief Verifies every supported element prefix for one API specialization. */
template <class api_t, std::size_t... counts> [[nodiscard]] bool has_all_element_prefixes(std::index_sequence<counts...>)
{
	return (has_element_prefix_contract<api_t, counts>() && ...);
}

/** @brief Verifies partial scalar broadcast for every supported active-lane extent. */
template <class api_t, std::size_t... counts> [[nodiscard]] bool has_all_partial_broadcasts(std::index_sequence<counts...>)
{
	return (has_partial_broadcast_contract<api_t, counts>() && ...);
}

/** @brief Verifies every supported byte prefix for one API specialization. */
template <class api_t, std::size_t... counts> [[nodiscard]] bool has_all_byte_prefixes(std::index_sequence<counts...>)
{
	return (has_byte_prefix_contract<api_t, counts>() && ...);
}

/** @brief Verifies the complete partial-transfer API for one scalar interpretation. */
template <class element_t> [[nodiscard]] bool has_partial_transfer_contract()
{
	using api_t = SimdLib::Api<SIMDLIB_API_PARTIAL_TRANSFER_TEST_WIDTH, element_t>;
	return has_all_element_prefixes<api_t>(std::make_index_sequence<api_t::element_count + 1>{}) &&
		   has_all_partial_broadcasts<api_t>(std::make_index_sequence<api_t::element_count + 1>{}) &&
		   has_all_byte_prefixes<api_t>(std::make_index_sequence<api_t::byte_count + 1>{});
}

} // namespace

/** @brief Verifies every partial API method across all scalar interpretations and extents. */
TEST_CASE("Api partial operations preserve exact prefixes, zero suffixes, and surrounding canaries", "[API][PARTIAL_TRANSFER]")
{
	REQUIRE(has_partial_transfer_contract<std::int8_t>());
	REQUIRE(has_partial_transfer_contract<std::uint8_t>());
	REQUIRE(has_partial_transfer_contract<std::int16_t>());
	REQUIRE(has_partial_transfer_contract<std::uint16_t>());
	REQUIRE(has_partial_transfer_contract<std::int32_t>());
	REQUIRE(has_partial_transfer_contract<std::uint32_t>());
	REQUIRE(has_partial_transfer_contract<std::int64_t>());
	REQUIRE(has_partial_transfer_contract<std::uint64_t>());
	REQUIRE(has_partial_transfer_contract<float>());
	REQUIRE(has_partial_transfer_contract<double>());
}
