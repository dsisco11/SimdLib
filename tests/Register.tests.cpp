#include <SimdLib/Register.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>

namespace
{

/** @brief Creates distinctive, exactly representable values for every lane. */
template <class register_t>
[[nodiscard]] constexpr auto lane_values() noexcept
{
	std::array<typename register_t::element_type, register_t::lane_count> result{};
	for (std::size_t index = 0; index < result.size(); ++index)
		result[index] = static_cast<typename register_t::element_type>(index + 1);
	return result;
}

/** @brief Constructs a register from an expanded low-to-high lane array. */
template <class register_t, std::size_t... indices>
[[nodiscard]] constexpr register_t from_lanes(
	const std::array<typename register_t::element_type, register_t::lane_count> &values,
	std::index_sequence<indices...>) noexcept
{
	return register_t::from_lanes(values[indices]...);
}

/** @brief Verifies construction, observation, and lane replacement for one register type. */
template <class element_t, std::size_t bits>
void require_value_contracts()
{
	using register_type = SimdLib::Register<element_t, bits>;
	const auto values = lane_values<register_type>();
	const std::array<element_t, register_type::lane_count> zeros{};

	REQUIRE(register_type{}.to_array() == zeros);
	REQUIRE(register_type::zero().to_array() == zeros);
	REQUIRE(register_type::broadcast(static_cast<element_t>(7)).to_array() ==
		[] {
			std::array<element_t, register_type::lane_count> result{};
			result.fill(static_cast<element_t>(7));
			return result;
		}());
	REQUIRE(register_type::from_array(values).to_array() == values);
	REQUIRE(from_lanes<register_type>(values, std::make_index_sequence<register_type::lane_count>{}).to_array() == values);

	const register_type wrapped(register_type::api_type::construct(values));
	REQUIRE(register_type::api_type::to_array(wrapped.native()) == values);
	REQUIRE(wrapped.template lane<0>() == values.front());
	REQUIRE(wrapped.template lane<register_type::lane_count - 1>() == values.back());

	const auto first_replaced = wrapped.template with_lane<0>(static_cast<element_t>(41)).to_array();
	const auto last_replaced = wrapped.template with_lane<register_type::lane_count - 1>(static_cast<element_t>(43)).to_array();
	for (std::size_t index = 0; index < values.size(); ++index)
	{
		REQUIRE(first_replaced[index] == (index == 0 ? static_cast<element_t>(41) : values[index]));
		REQUIRE(last_replaced[index] ==
			(index + 1 == values.size() ? static_cast<element_t>(43) : values[index]));
	}
}

/** @brief Verifies exact-width aligned, unaligned, and raw-byte transfers with canaries. */
template <class element_t, std::size_t bits>
void require_transfer_contracts()
{
	using register_type = SimdLib::Register<element_t, bits>;
	const auto values = lane_values<register_type>();

	alignas(register_type::byte_count) std::array<element_t, register_type::lane_count> aligned_source = values;
	alignas(register_type::byte_count) std::array<element_t, register_type::lane_count> aligned_destination{};
	register_type::load_aligned(std::span<const element_t, register_type::lane_count>{aligned_source})
		.store_aligned(std::span<element_t, register_type::lane_count>{aligned_destination});
	REQUIRE(aligned_destination == values);

	alignas(register_type::byte_count) std::array<element_t, register_type::lane_count + 2> unaligned_source{};
	alignas(register_type::byte_count) std::array<element_t, register_type::lane_count + 2> unaligned_destination{};
	unaligned_source.front() = static_cast<element_t>(91);
	unaligned_source.back() = static_cast<element_t>(93);
	unaligned_destination.front() = static_cast<element_t>(95);
	unaligned_destination.back() = static_cast<element_t>(97);
	for (std::size_t index = 0; index < values.size(); ++index)
		unaligned_source[index + 1] = values[index];
	const auto loaded = register_type::load(
		std::span<const element_t, register_type::lane_count>{unaligned_source.data() + 1, register_type::lane_count});
	loaded.store(std::span<element_t, register_type::lane_count>{
		unaligned_destination.data() + 1, register_type::lane_count});
	REQUIRE(unaligned_destination.front() == static_cast<element_t>(95));
	REQUIRE(unaligned_destination.back() == static_cast<element_t>(97));
	for (std::size_t index = 0; index < values.size(); ++index)
		REQUIRE(unaligned_destination[index + 1] == values[index]);

	std::array<std::byte, register_type::byte_count> source_bytes{};
	for (std::size_t index = 0; index < source_bytes.size(); ++index)
		source_bytes[index] = static_cast<std::byte>((index * 37U + 11U) & 0xFFU);
	std::array<std::byte, register_type::byte_count + 2> destination_bytes{};
	destination_bytes.front() = std::byte{0xA5};
	destination_bytes.back() = std::byte{0x5A};
	register_type::load_bytes(std::span<const std::byte, register_type::byte_count>{source_bytes})
		.store_bytes(std::span<std::byte, register_type::byte_count>{destination_bytes.data() + 1,
			register_type::byte_count});
	REQUIRE(std::to_integer<unsigned int>(destination_bytes.front()) == 0xA5U);
	REQUIRE(std::to_integer<unsigned int>(destination_bytes.back()) == 0x5AU);
	for (std::size_t index = 0; index < source_bytes.size(); ++index)
		REQUIRE(std::to_integer<unsigned int>(destination_bytes[index + 1]) ==
			std::to_integer<unsigned int>(source_bytes[index]));
}

/** @brief Runs all Register value and transfer contracts for one scalar type. */
template <class element_t>
void require_type_contracts()
{
	require_value_contracts<element_t, 128>();
	require_value_contracts<element_t, 256>();
	require_transfer_contracts<element_t, 128>();
	require_transfer_contracts<element_t, 256>();
}

TEST_CASE("Register construction and exact-width transfers preserve every lane and surrounding canaries",
	"[simdlib][register][avx2][transfer]")
{
	require_type_contracts<std::int8_t>();
	require_type_contracts<std::uint8_t>();
	require_type_contracts<std::int16_t>();
	require_type_contracts<std::uint16_t>();
	require_type_contracts<std::int32_t>();
	require_type_contracts<std::uint32_t>();
	require_type_contracts<std::int64_t>();
	require_type_contracts<std::uint64_t>();
	require_type_contracts<float>();
	require_type_contracts<double>();
}

} // namespace
