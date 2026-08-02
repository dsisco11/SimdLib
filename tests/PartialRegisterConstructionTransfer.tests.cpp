#include <SimdLib/PartialRegister.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>

#ifndef SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
#define SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256 SIMDLIB_HAS_AVX2
#endif

namespace
{

/** @brief Produces one exactly representable active-lane test value. */
template <class element_t> [[nodiscard]] constexpr element_t test_lane_value(std::size_t lane) noexcept
{
	return static_cast<element_t>(lane + 3);
}

/** @brief Reports whether one scalar value has an all-bits-zero representation. */
template <class element_t> [[nodiscard]] constexpr bool has_zero_bits(element_t value) noexcept
{
	const auto bytes = std::bit_cast<std::array<std::byte, sizeof(element_t)>>(value);
	return std::all_of(bytes.begin(), bytes.end(), [](std::byte byte) constexpr noexcept { return byte == std::byte{}; });
}

/** @brief Constructs one partial value through its exact lane-list factory. */
template <class value_t, std::size_t... indices>
[[nodiscard]] constexpr value_t from_test_lanes(std::index_sequence<indices...>) noexcept
{
	return value_t::from_lanes(test_lane_value<typename value_t::element_type>(indices)...);
}

/** @brief Reports whether every active lane matches and the inactive suffix is bitwise zero. */
template <class value_t>
[[nodiscard]] bool has_value_contract(value_t value, const std::array<typename value_t::element_type, value_t::lane_count> &expected)
{
	using api_t = typename value_t::api_type;
	const auto logical = value.to_array();
	const auto native = api_t::to_array(value.to_native());
	if (logical != expected)
		return false;
	for (std::size_t lane = 0; lane < value_t::lane_count; ++lane)
		if (native[lane] != expected[lane])
			return false;
	for (std::size_t lane = value_t::lane_count; lane < value_t::native_lane_count; ++lane)
		if (!has_zero_bits(native[lane]))
			return false;
	return true;
}

/** @brief Reports whether compile-time-selected active lane observation and replacement preserve the contract. */
template <class value_t, std::size_t... indices>
[[nodiscard]] bool has_lane_access(value_t value, std::index_sequence<indices...>)
{
	using element_t = typename value_t::element_type;
	if (!((value.template lane<indices>() == test_lane_value<element_t>(indices)) && ...))
		return false;
	((value = value.template with_lane<indices>(static_cast<element_t>(indices + 37))), ...);
	const auto replaced = value.to_array();
	if (!((replaced[indices] == static_cast<element_t>(indices + 37)) && ...))
		return false;
	const auto native = value_t::api_type::to_array(value.to_native());
	for (std::size_t lane = value_t::lane_count; lane < value_t::native_lane_count; ++lane)
		if (!has_zero_bits(native[lane]))
			return false;
	return true;
}

/** @brief Reports whether construction, exact-extent transfer, observation, and lane access satisfy one partial geometry. */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
[[nodiscard]] bool has_construction_transfer_contract()
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_lane_count>;
	std::array<element_t, value_t::lane_count> source{};
	for (std::size_t lane = 0; lane < source.size(); ++lane)
		source[lane] = test_lane_value<element_t>(lane);

	std::array<element_t, value_t::lane_count> zeros{};
	if (!has_value_contract(value_t::zero(), zeros))
		return false;
	std::array<element_t, value_t::lane_count> broadcasts{};
	broadcasts.fill(test_lane_value<element_t>(0));
	if (!has_value_contract(value_t::broadcast(test_lane_value<element_t>(0)), broadcasts) ||
		!has_value_contract(value_t::from_array(source), source))
		return false;
	const auto listed = from_test_lanes<value_t>(std::make_index_sequence<value_t::lane_count>{});
	if (!has_value_contract(listed, source) || !has_lane_access(listed, std::make_index_sequence<value_t::lane_count>{}))
		return false;

	std::array<element_t, value_t::lane_count + 2> unaligned_source{};
	std::copy(source.begin(), source.end(), unaligned_source.begin() + 1);
	if (!has_value_contract(value_t::load(std::span<const element_t, value_t::lane_count>{unaligned_source.data() + 1, value_t::lane_count}), source))
		return false;

	alignas(value_t::byte_count) std::array<element_t, value_t::native_lane_count + value_t::lane_count + 1> aligned_source{};
	std::copy(source.begin(), source.end(), aligned_source.begin() + value_t::native_lane_count);
	aligned_source[value_t::native_lane_count - 1] = static_cast<element_t>(89);
	aligned_source.back() = static_cast<element_t>(91);
	if (!has_value_contract(value_t::load_aligned(
			std::span<const element_t, value_t::lane_count>{aligned_source.data() + value_t::native_lane_count, value_t::lane_count}), source))
		return false;

	const auto source_bytes = std::bit_cast<std::array<std::byte, value_t::active_byte_count>>(source);
	std::array<std::byte, value_t::active_byte_count + 2> byte_source{};
	std::copy(source_bytes.begin(), source_bytes.end(), byte_source.begin() + 1);
	if (!has_value_contract(
			value_t::load_bytes(std::span<const std::byte, value_t::active_byte_count>{byte_source.data() + 1, value_t::active_byte_count}), source))
		return false;

	const auto element_canary = static_cast<element_t>(97);
	std::array<element_t, value_t::lane_count + 2> unaligned_destination{};
	unaligned_destination.fill(element_canary);
	listed.store(std::span<element_t, value_t::lane_count>{unaligned_destination.data() + 1, value_t::lane_count});
	if (unaligned_destination.front() != element_canary || unaligned_destination.back() != element_canary ||
		!std::equal(source.begin(), source.end(), unaligned_destination.begin() + 1))
		return false;

	alignas(value_t::byte_count) std::array<element_t, value_t::native_lane_count + value_t::lane_count + 1> aligned_destination{};
	aligned_destination.fill(element_canary);
	listed.store_aligned(
		std::span<element_t, value_t::lane_count>{aligned_destination.data() + value_t::native_lane_count, value_t::lane_count});
	if (!std::all_of(aligned_destination.begin(), aligned_destination.begin() + value_t::native_lane_count,
			[](element_t lane) noexcept { return lane == static_cast<element_t>(97); }) ||
		aligned_destination.back() != element_canary ||
		!std::equal(source.begin(), source.end(), aligned_destination.begin() + value_t::native_lane_count))
		return false;

	constexpr std::byte byte_canary{0xa5};
	std::array<std::byte, value_t::active_byte_count + 2> byte_destination{};
	byte_destination.fill(byte_canary);
	listed.store_bytes(std::span<std::byte, value_t::active_byte_count>{byte_destination.data() + 1, value_t::active_byte_count});
	return std::to_integer<unsigned int>(byte_destination.front()) == std::to_integer<unsigned int>(byte_canary) &&
		   std::to_integer<unsigned int>(byte_destination.back()) == std::to_integer<unsigned int>(byte_canary) &&
		   std::equal(source_bytes.begin(), source_bytes.end(), byte_destination.begin() + 1);
}

/** @brief Reports whether every non-complete active extent passes for one element type and native width. */
template <class element_t, std::size_t bits, std::size_t... active_lane_counts>
[[nodiscard]] bool has_all_active_counts(std::index_sequence<active_lane_counts...>)
{
	return (has_construction_transfer_contract<element_t, bits, active_lane_counts + 1>() && ...);
}

/** @brief Reports whether the full supported element-type matrix passes for one native width. */
template <std::size_t bits> [[nodiscard]] bool has_construction_transfer_matrix()
{
	#define SIMDLIB_HAS_PARTIAL_ELEMENT(element_type) \
		has_all_active_counts<element_type, bits>(std::make_index_sequence<SimdLib::Api<bits, element_type>::element_count - 1>{})
	return SIMDLIB_HAS_PARTIAL_ELEMENT(std::int8_t) && SIMDLIB_HAS_PARTIAL_ELEMENT(std::uint8_t) &&
		   SIMDLIB_HAS_PARTIAL_ELEMENT(std::int16_t) && SIMDLIB_HAS_PARTIAL_ELEMENT(std::uint16_t) &&
		   SIMDLIB_HAS_PARTIAL_ELEMENT(std::int32_t) && SIMDLIB_HAS_PARTIAL_ELEMENT(std::uint32_t) &&
		   SIMDLIB_HAS_PARTIAL_ELEMENT(std::int64_t) && SIMDLIB_HAS_PARTIAL_ELEMENT(std::uint64_t) &&
		   SIMDLIB_HAS_PARTIAL_ELEMENT(float) && SIMDLIB_HAS_PARTIAL_ELEMENT(double);
	#undef SIMDLIB_HAS_PARTIAL_ELEMENT
}

} // namespace

/** @brief Verifies exact-extent construction, transfer, and observation for every 128-bit partial geometry. */
TEST_CASE("PartialRegister construction and transfer preserve every 128-bit partial extent", "[PARTIAL_REGISTER][SSE42]")
{
	REQUIRE(has_construction_transfer_matrix<128>());
}

#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
/** @brief Verifies exact-extent construction, transfer, and observation for every 256-bit partial geometry. */
TEST_CASE("PartialRegister construction and transfer preserve every 256-bit partial extent", "[PARTIAL_REGISTER][AVX2]")
{
	REQUIRE(has_construction_transfer_matrix<256>());
}
#endif
