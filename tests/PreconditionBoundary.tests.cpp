#include <SimdLib/Api.h>
#include <SimdLib/SimdAlgo.h>
#include <SimdLib/SimdResample.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace
{
/** @brief Verifies exact-capacity and minimum-count transfer contracts for one register width.
 *  @tparam RegisterWidth SIMD register width in bits.
 */
template <std::size_t RegisterWidth> void RequireApiTransferBoundaries()
{
	using Api = SimdLib::Api<RegisterWidth, std::uint32_t>;
	volatile std::uint32_t runtime_seed = 7;
	const std::uint32_t seed = runtime_seed;

	alignas(Api::byte_count) std::array<std::uint32_t, Api::element_count> input{};
	for (std::size_t index = 0; index < input.size(); ++index)
		input[index] = seed + static_cast<std::uint32_t>(index);

	const auto value = Api::load_aligned(input);
	alignas(Api::byte_count) std::array<std::uint32_t, Api::element_count> output{};
	Api::store_aligned(value, output);
	REQUIRE(output == input);

	std::array<std::byte, Api::byte_count> raw{};
	Api::store(value, std::span<std::byte>(raw));
	std::array<std::uint32_t, Api::element_count> recovered{};
	std::memcpy(recovered.data(), raw.data(), raw.size());
	REQUIRE(recovered == input);

	const auto empty = Api::template load_partial<0>(std::span<const std::uint32_t>{});
	REQUIRE(std::ranges::all_of(Api::to_array(empty), [](const std::uint32_t lane) { return lane == 0; }));

	const std::array<std::uint32_t, 1> single{seed};
	const auto partial = Api::template load_partial<1>(std::span<const std::uint32_t>(single));
	const auto lanes = Api::to_array(partial);
	REQUIRE(lanes.front() == seed);
	REQUIRE(std::ranges::all_of(lanes.begin() + 1, lanes.end(), [](const std::uint32_t lane) { return lane == 0; }));
}
} // namespace

TEST_CASE("Api transfer preconditions accept exact valid boundaries", "[simdlib][preconditions][boundary]")
{
	RequireApiTransferBoundaries<128>();
	RequireApiTransferBoundaries<256>();
}

TEST_CASE("SimdAlgo dynamic span preconditions accept matching minimum extents", "[simdlib][preconditions][boundary]")
{
	using Algo = SimdLib::SimdAlgo<8, 1>;
	const std::array<std::uint8_t, 1> lhs{0b1010'1010};
	const std::array<std::uint8_t, 1> rhs{0b1100'0011};
	std::array<std::uint8_t, 1> write{};

	Algo::BitwiseAnd(lhs, rhs, write);
	REQUIRE(write[0] == 0b1000'0010);
	Algo::BitwiseOr(lhs, rhs, write);
	REQUIRE(write[0] == 0b1110'1011);
	Algo::BitwiseXor(lhs, rhs, write);
	REQUIRE(write[0] == 0b0110'1001);
	Algo::BitwiseNot(lhs, write);
	REQUIRE(write[0] == 0b0101'0101);
	Algo::BitwiseAndNot(lhs, rhs, write);
	REQUIRE(write[0] == 0b0100'0001);

	const std::span<const std::uint8_t> empty_read{};
	const std::span<std::uint8_t> empty_write{};
	Algo::BitwiseAnd(empty_read, empty_read, empty_write);
	Algo::BitwiseOr(empty_read, empty_read, empty_write);
	Algo::BitwiseXor(empty_read, empty_read, empty_write);
	Algo::BitwiseNot(empty_read, empty_write);
	Algo::BitwiseAndNot(empty_read, empty_read, empty_write);
}

TEST_CASE("SimdResample preconditions accept empty and minimum valid extents", "[simdlib][preconditions][boundary]")
{
	const std::span<const std::uint8_t> empty_src{};
	const std::span<std::uint8_t> empty_dst{};
	SimdLib::SimdResample::ReduceBytesToBitsBy8_Any(empty_src, empty_dst);
	SimdLib::SimdResample::ReduceBytesToBitsBy8_All(empty_src, empty_dst);
	SimdLib::SimdResample::ReduceBytesToBitsBy8_Parity(empty_src, empty_dst);
	SimdLib::SimdResample::ExpandBitsToBytesBy8(empty_src, empty_dst);

	const std::array<std::uint8_t, 8> bytes{0, 1, 0xFF, 3, 0, 0x80, 0x55, 0xAA};
	std::array<std::uint8_t, 1> packed{};
	SimdLib::SimdResample::ReduceBytesToBitsBy8_Any(bytes, packed);
	REQUIRE(packed[0] == 0b1110'1110);
	SimdLib::SimdResample::ReduceBytesToBitsBy8_All(bytes, packed);
	REQUIRE(packed[0] == 0b0000'0100);
	SimdLib::SimdResample::ReduceBytesToBitsBy8_Parity(bytes, packed);
	REQUIRE(packed[0] == 0b0010'0010);

	const std::array<std::uint8_t, 1> bits{0b1010'0101};
	std::array<std::uint8_t, 8> expanded{};
	SimdLib::SimdResample::ExpandBitsToBytesBy8(bits, expanded);
	REQUIRE(expanded == std::array<std::uint8_t, 8>{0xFF, 0, 0xFF, 0, 0, 0xFF, 0, 0xFF});
}
