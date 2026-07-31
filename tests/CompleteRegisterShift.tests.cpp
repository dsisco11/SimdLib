#include "TestSupport.h"

#include <SimdLib/Api.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

#ifndef SIMDLIB_COMPLETE_SHIFT_TEST_WIDTH
#error "SIMDLIB_COMPLETE_SHIFT_TEST_WIDTH must select the tested register width"
#endif

namespace
{

/**
 * @brief Produces the scalar reference for a complete-register left byte shift.
 * @tparam count Compile-time byte count.
 * @tparam byte_count Complete register width in bytes.
 * @param source Source bytes in low-to-high register order.
 * @return Shifted bytes with zero-filled low positions.
 */
template <std::size_t count, std::size_t byte_count>
[[nodiscard]] constexpr std::array<std::uint8_t, byte_count> shift_bytes_left_oracle(const std::array<std::uint8_t, byte_count> &source) noexcept
{
	std::array<std::uint8_t, byte_count> result{};
	if constexpr (count < byte_count)
		for (std::size_t index = count; index < byte_count; ++index)
			result[index] = source[index - count];
	return result;
}

/**
 * @brief Produces the scalar reference for a complete-register right byte shift.
 * @tparam count Compile-time byte count.
 * @tparam byte_count Complete register width in bytes.
 * @param source Source bytes in low-to-high register order.
 * @return Shifted bytes with zero-filled high positions.
 */
template <std::size_t count, std::size_t byte_count>
[[nodiscard]] constexpr std::array<std::uint8_t, byte_count> shift_bytes_right_oracle(const std::array<std::uint8_t, byte_count> &source) noexcept
{
	std::array<std::uint8_t, byte_count> result{};
	if constexpr (count < byte_count)
		for (std::size_t index = 0; index + count < byte_count; ++index)
			result[index] = source[index + count];
	return result;
}

/**
 * @brief Verifies one immediate byte count against scalar and whole-bit-string references.
 * @tparam count Compile-time byte count.
 * @tparam element_t Integral lane interpretation used to prove cross-element behavior.
 */
template <std::size_t count, class element_t> void require_immediate_byte_shift_count()
{
	using api = SimdLib::Api<SIMDLIB_COMPLETE_SHIFT_TEST_WIDTH, element_t>;
	constexpr std::size_t byte_count = api::byte_count;
	volatile std::uint8_t runtime_seed = 1;
	std::array<std::uint8_t, byte_count> source_bytes{};
	for (std::size_t index = 0; index < byte_count; ++index)
		source_bytes[index] = static_cast<std::uint8_t>(runtime_seed + index * 7);
	const auto source_lanes = std::bit_cast<std::array<element_t, api::element_count>>(source_bytes);
	const auto source = api::construct(source_lanes);
	const auto left = std::bit_cast<std::array<std::uint8_t, byte_count>>(api::to_array(api::template shift_bytes_left<static_cast<int>(count)>(source)));
	const auto right = std::bit_cast<std::array<std::uint8_t, byte_count>>(api::to_array(api::template shift_bytes_right<static_cast<int>(count)>(source)));
	REQUIRE(left == shift_bytes_left_oracle<count>(source_bytes));
	REQUIRE(right == shift_bytes_right_oracle<count>(source_bytes));
	if constexpr (SIMDLIB_COMPLETE_SHIFT_TEST_WIDTH == 128)
	{
		const auto bit_left =
			std::bit_cast<std::array<std::uint8_t, byte_count>>(api::to_array(api::template shift_bits_left<static_cast<int>(count * 8)>(source)));
		const auto bit_right =
			std::bit_cast<std::array<std::uint8_t, byte_count>>(api::to_array(api::template shift_bits_right<static_cast<int>(count * 8)>(source)));
		REQUIRE(left == bit_left);
		REQUIRE(right == bit_right);
	}
}

/** @brief Verifies every required immediate byte count for one lane interpretation. */
template <class element_t> void require_immediate_byte_shift_counts()
{
	require_immediate_byte_shift_count<0, element_t>();
	require_immediate_byte_shift_count<1, element_t>();
	require_immediate_byte_shift_count<7, element_t>();
	require_immediate_byte_shift_count<8, element_t>();
	require_immediate_byte_shift_count<15, element_t>();
	require_immediate_byte_shift_count<16, element_t>();
	require_immediate_byte_shift_count<17, element_t>();
	require_immediate_byte_shift_count<31, element_t>();
	require_immediate_byte_shift_count<32, element_t>();
	require_immediate_byte_shift_count<33, element_t>();
}

} // namespace

TEST_CASE("Immediate complete-register byte shifts match independent references", "[simdlib][shift][byte][immediate]")
{
	require_immediate_byte_shift_counts<std::uint8_t>();
	require_immediate_byte_shift_counts<std::uint16_t>();
	require_immediate_byte_shift_counts<std::uint32_t>();
	require_immediate_byte_shift_counts<std::uint64_t>();
}