#include <SimdLib/SimdResample.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <random>
#include <span>
#include <vector>

namespace
{
void reduce_any_reference(const std::span<const std::uint8_t> src, const std::span<std::uint8_t> dst)
{
	for (std::size_t group = 0; group < dst.size(); ++group)
	{
		std::uint8_t result = 0;
		for (std::size_t lane = 0; lane < 8; ++lane)
			result |= static_cast<std::uint8_t>((src[group * 8 + lane] != 0 ? 1u : 0u) << lane);
		dst[group] = result;
	}
}

void reduce_all_reference(const std::span<const std::uint8_t> src, const std::span<std::uint8_t> dst)
{
	for (std::size_t group = 0; group < dst.size(); ++group)
	{
		std::uint8_t result = 0;
		for (std::size_t lane = 0; lane < 8; ++lane)
			result |= static_cast<std::uint8_t>((src[group * 8 + lane] == 0xFF ? 1u : 0u) << lane);
		dst[group] = result;
	}
}

void reduce_parity_reference(const std::span<const std::uint8_t> src, const std::span<std::uint8_t> dst)
{
	for (std::size_t group = 0; group < dst.size(); ++group)
	{
		std::uint8_t result = 0;
		for (std::size_t lane = 0; lane < 8; ++lane)
			result |= static_cast<std::uint8_t>((std::popcount(static_cast<unsigned>(src[group * 8 + lane])) & 1) << lane);
		dst[group] = result;
	}
}

void expand_reference(const std::span<const std::uint8_t> src, const std::span<std::uint8_t> dst)
{
	for (std::size_t group = 0; group < src.size(); ++group)
		for (std::size_t lane = 0; lane < 8; ++lane)
			dst[group * 8 + lane] = (src[group] & (1u << lane)) != 0 ? 0xFF : 0;
}

void fill_random(const std::span<std::uint8_t> data, std::mt19937& random)
{
	std::uniform_int_distribution<int> distribution(0, 255);
	for (auto& value : data)
		value = static_cast<std::uint8_t>(distribution(random));
}
}

TEST_CASE("SimdResample preserves reduce bit ordering", "[simdlib][resample][ordering]")
{
	std::array<std::uint8_t, 8> src{};
	std::array<std::uint8_t, 1> dst{};
	for (std::size_t lane = 0; lane < 8; ++lane)
	{
		src.fill(0);
		src[lane] = 1;
		SimdLib::SimdResample::ReduceBytesToBitsBy8_Any(src, dst);
		REQUIRE(dst[0] == (1u << lane));
		SimdLib::SimdResample::ReduceBytesToBitsBy8_Parity(src, dst);
		REQUIRE(dst[0] == (1u << lane));
		src.fill(0);
		src[lane] = 0xFF;
		SimdLib::SimdResample::ReduceBytesToBitsBy8_All(src, dst);
		REQUIRE(dst[0] == (1u << lane));
		SimdLib::SimdResample::ReduceBytesToBitsBy8_Parity(src, dst);
		REQUIRE(dst[0] == 0);
	}
}

TEST_CASE("SimdResample expand is exhaustive for one packed byte", "[simdlib][resample][exhaustive]")
{
	std::array<std::uint8_t, 1> src{};
	std::array<std::uint8_t, 8> actual{};
	std::array<std::uint8_t, 8> expected{};
	for (unsigned bits = 0; bits <= 0xFF; ++bits)
	{
		src[0] = static_cast<std::uint8_t>(bits);
		SimdLib::SimdResample::ExpandBitsToBytesBy8(src, actual);
		expand_reference(src, expected);
		CAPTURE(bits);
		REQUIRE(actual == expected);
	}
}

TEST_CASE("SimdResample reductions preserve zero one and mixed edge cases", "[simdlib][resample][edge]")
{
	constexpr std::size_t groups = 3;
	std::array<std::uint8_t, groups * 8> src{};
	std::array<std::uint8_t, groups> actual{};
	std::array<std::uint8_t, groups> expected{};
	const auto verify = [&]
	{
		reduce_any_reference(src, expected);
		SimdLib::SimdResample::ReduceBytesToBitsBy8_Any(src, actual);
		REQUIRE(actual == expected);
		reduce_all_reference(src, expected);
		SimdLib::SimdResample::ReduceBytesToBitsBy8_All(src, actual);
		REQUIRE(actual == expected);
		reduce_parity_reference(src, expected);
		SimdLib::SimdResample::ReduceBytesToBitsBy8_Parity(src, actual);
		REQUIRE(actual == expected);
	};

	src.fill(0);
	verify();
	src.fill(0xFF);
	verify();
	for (std::size_t index = 0; index < src.size(); ++index)
		src[index] = (index & 1u) != 0 ? 0xFF : 0;
	verify();
	constexpr std::array<std::uint8_t, 8> pattern{0x01, 0x03, 0x07, 0x0F, 0x55, 0x7F, 0x80, 0x00};
	for (std::size_t group = 0; group < groups; ++group)
		std::ranges::copy(pattern, src.begin() + group * 8);
	verify();
}

TEST_CASE("SimdResample reductions match scalar references for randomized unaligned spans", "[simdlib][resample][random]")
{
	constexpr std::array<std::size_t, 16> groupCounts{0, 1, 2, 3, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65};
	constexpr std::array<std::size_t, 3> srcOffsets{0, 1, 15};
	constexpr std::array<std::size_t, 3> dstOffsets{0, 3, 7};
	for (const std::size_t groups : groupCounts)
		for (const std::size_t srcOffset : srcOffsets)
			for (const std::size_t dstOffset : dstOffsets)
			{
				std::vector<std::uint8_t> srcStorage(srcOffset + groups * 8);
				std::vector<std::uint8_t> dstStorage(dstOffset + groups);
				std::vector<std::uint8_t> expected(groups);
				std::mt19937 random(static_cast<std::uint32_t>((groups << 16) ^ (srcOffset << 8) ^ dstOffset));
				fill_random(std::span<std::uint8_t>(srcStorage).subspan(srcOffset), random);
				const auto src = std::span<const std::uint8_t>(srcStorage).subspan(srcOffset);
				const auto dst = std::span<std::uint8_t>(dstStorage).subspan(dstOffset);

				reduce_any_reference(src, expected);
				SimdLib::SimdResample::ReduceBytesToBitsBy8_Any(src, dst);
				REQUIRE(std::ranges::equal(dst, expected));
				reduce_all_reference(src, expected);
				SimdLib::SimdResample::ReduceBytesToBitsBy8_All(src, dst);
				REQUIRE(std::ranges::equal(dst, expected));
				reduce_parity_reference(src, expected);
				SimdLib::SimdResample::ReduceBytesToBitsBy8_Parity(src, dst);
				REQUIRE(std::ranges::equal(dst, expected));
			}
}

TEST_CASE("SimdResample expansion matches scalar references for randomized unaligned spans", "[simdlib][resample][random]")
{
	constexpr std::array<std::size_t, 11> inputCounts{0, 1, 2, 3, 7, 8, 9, 15, 16, 17, 33};
	constexpr std::array<std::size_t, 3> srcOffsets{0, 1, 15};
	constexpr std::array<std::size_t, 3> dstOffsets{0, 3, 7};
	for (const std::size_t count : inputCounts)
		for (const std::size_t srcOffset : srcOffsets)
			for (const std::size_t dstOffset : dstOffsets)
			{
				std::vector<std::uint8_t> srcStorage(srcOffset + count);
				std::vector<std::uint8_t> dstStorage(dstOffset + count * 8);
				std::vector<std::uint8_t> expected(count * 8);
				std::mt19937 random(static_cast<std::uint32_t>((count << 16) ^ (srcOffset << 8) ^ dstOffset ^ 0x5A5A5A5A));
				fill_random(std::span<std::uint8_t>(srcStorage).subspan(srcOffset), random);
				const auto src = std::span<const std::uint8_t>(srcStorage).subspan(srcOffset);
				const auto dst = std::span<std::uint8_t>(dstStorage).subspan(dstOffset);
				SimdLib::SimdResample::ExpandBitsToBytesBy8(src, dst);
				expand_reference(src, expected);
				REQUIRE(std::ranges::equal(dst, expected));
			}
}

TEST_CASE("SimdResample documentation examples produce their documented results", "[simdlib][resample][documentation]")
{
	std::array<std::uint8_t, 8> expanded{};
	SimdLib::SimdResample::ExpandBitsToBytesBy8(std::array<std::uint8_t, 1>{0b0000'0101}, expanded);
	REQUIRE(expanded == std::array<std::uint8_t, 8>{0xFF, 0, 0xFF, 0, 0, 0, 0, 0});
	std::array<std::uint8_t, 1> result{};
	SimdLib::SimdResample::ReduceBytesToBitsBy8_All(std::array<std::uint8_t, 8>{0xFF, 0, 0xFF, 0, 0, 0, 0, 0}, result);
	REQUIRE(result[0] == 0b0000'0101);
	SimdLib::SimdResample::ReduceBytesToBitsBy8_Any(std::array<std::uint8_t, 8>{0, 4, 0, 2, 0, 0, 0, 0}, result);
	REQUIRE(result[0] == 0b0000'1010);
	SimdLib::SimdResample::ReduceBytesToBitsBy8_Parity(std::array<std::uint8_t, 8>{1, 3, 7, 0, 0, 0, 0, 0}, result);
	REQUIRE(result[0] == 0b0000'0101);
}
