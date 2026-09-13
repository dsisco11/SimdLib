#include <SimdLib/Api.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#ifndef SIMDLIB_IMPLEMENTATION_HALF_TRANSFER_TEST_WIDTH
#error "SIMDLIB_IMPLEMENTATION_HALF_TRANSFER_TEST_WIDTH must select the tested register width"
#endif

namespace
{

/** @brief Reports whether one mapping exposes aligned half-register transfer operations. */
template <class mapping_t, class element_t>
concept HasAlignedHalfTransfer = requires(const element_t *source, typename mapping_t::int_vector_t value, void *destination) {
	{ mapping_t::load_half_aligned(source) } -> std::same_as<typename mapping_t::int_vector_t>;
	mapping_t::store_half_aligned(value, destination);
};

/** @brief Reports whether one scalar value has an all-bits-zero object representation. */
template <class element_t> [[nodiscard]] constexpr bool has_zero_bits(element_t value) noexcept
{
	const auto bytes = std::bit_cast<std::array<std::byte, sizeof(element_t)>>(value);
	return std::all_of(bytes.begin(), bytes.end(), [](std::byte byte) constexpr noexcept { return byte == std::byte{}; });
}

/** @brief Verifies aligned half-register load and store behavior for one integral mapping. */
template <class element_t> [[nodiscard]] bool has_aligned_half_transfer_contract()
{
	using mapping_t = SimdLib::Detail::SimdMappings<SIMDLIB_IMPLEMENTATION_HALF_TRANSFER_TEST_WIDTH, element_t>;
	static_assert(HasAlignedHalfTransfer<mapping_t, element_t>);
	constexpr std::size_t half_count = mapping_t::element_count / 2;
	alignas(SIMDLIB_IMPLEMENTATION_HALF_TRANSFER_TEST_WIDTH / 8) std::array<element_t, mapping_t::element_count> source{};
	for (std::size_t index = 0; index < source.size(); ++index)
		source[index] = static_cast<element_t>(index + 17);

	const auto loaded = mapping_t::load_half_aligned(source.data());
	std::array<element_t, mapping_t::element_count> loaded_lanes{};
	mapping_t::store_unaligned(loaded, loaded_lanes.data());
	for (std::size_t index = 0; index < half_count; ++index)
		if (loaded_lanes[index] != source[index])
			return false;
	for (std::size_t index = half_count; index < loaded_lanes.size(); ++index)
		if (!has_zero_bits(loaded_lanes[index]))
			return false;

	const auto canary = static_cast<element_t>(91);
	alignas(SIMDLIB_IMPLEMENTATION_HALF_TRANSFER_TEST_WIDTH / 8) std::array<element_t, mapping_t::element_count> destination{};
	destination.fill(canary);
	mapping_t::store_half_aligned(loaded, destination.data());
	return std::equal(source.begin(), source.begin() + half_count, destination.begin()) &&
		   std::all_of(destination.begin() + half_count, destination.end(), [](element_t value) noexcept { return value == static_cast<element_t>(91); });
}

using signed_byte_mapping = SimdLib::Detail::SimdMappings<SIMDLIB_IMPLEMENTATION_HALF_TRANSFER_TEST_WIDTH, std::int8_t>;
using float_mapping = SimdLib::Detail::SimdMappings<SIMDLIB_IMPLEMENTATION_HALF_TRANSFER_TEST_WIDTH, float>;
static_assert(HasAlignedHalfTransfer<signed_byte_mapping, std::int8_t>);
static_assert(!HasAlignedHalfTransfer<float_mapping, float>);

} // namespace

/** @brief Verifies direct aligned half-register transfers for every supported integral mapping. */
TEST_CASE("Implementation aligned half transfers preserve exact bounds and clear loaded high lanes", "[IMPLEMENTATION][PARTIAL_TRANSFER]")
{
	REQUIRE(has_aligned_half_transfer_contract<std::int8_t>());
	REQUIRE(has_aligned_half_transfer_contract<std::uint8_t>());
	REQUIRE(has_aligned_half_transfer_contract<std::int16_t>());
	REQUIRE(has_aligned_half_transfer_contract<std::uint16_t>());
	REQUIRE(has_aligned_half_transfer_contract<std::int32_t>());
	REQUIRE(has_aligned_half_transfer_contract<std::uint32_t>());
	REQUIRE(has_aligned_half_transfer_contract<std::int64_t>());
	REQUIRE(has_aligned_half_transfer_contract<std::uint64_t>());
}
