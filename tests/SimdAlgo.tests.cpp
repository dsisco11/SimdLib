#include <SimdLib/SimdAlgo.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

TEST_CASE("SimdAlgo fixed spans preserve equality and packed comparison semantics", "[simdlib][algo]")
{
	using Algo = SimdLib::SimdAlgo<8, 8>;
	const std::array<std::uint8_t, 8> eight{3, 7, 3, 0, 3, 9, 3, 3};
	REQUIRE(Algo::AnyEqual(std::span<const std::uint8_t, 8>(eight), std::uint8_t{7}));
	REQUIRE_FALSE(Algo::AnyEqual(std::span<const std::uint8_t, 8>(eight), std::uint8_t{8}));
	REQUIRE_FALSE(Algo::AllEqual(std::span<const std::uint8_t, 8>(eight), std::uint8_t{3}));

	std::array<std::uint8_t, 1> packed{};
	Algo::Compare(std::span<const std::uint8_t, 8>(eight), std::span<std::uint8_t, 1>(packed), std::uint8_t{3});
	REQUIRE(packed[0] == 0b11010101);

	const std::array<std::uint8_t, 32> uniform = []
	{
		std::array<std::uint8_t, 32> value{};
		value.fill(42);
		return value;
	}();
	REQUIRE(Algo::AnyEqual(std::span<const std::uint8_t, 32>(uniform), std::uint8_t{42}));
	REQUIRE(Algo::AllEqual(std::span<const std::uint8_t, 32>(uniform), std::uint8_t{42}));
}

TEST_CASE("SimdAlgo fixed bitwise operations match scalar references including tails", "[simdlib][algo]")
{
	using Algo = SimdLib::SimdAlgo<32, 8>;
	std::array<std::uint32_t, 9> lhs{};
	std::array<std::uint32_t, 9> rhs{};
	std::array<std::uint32_t, 9> output{};
	for (std::size_t index = 0; index < lhs.size(); ++index)
	{
		lhs[index] = 0xF0F00000u + static_cast<std::uint32_t>(index);
		rhs[index] = 0x0FF00FF0u ^ static_cast<std::uint32_t>(index * 17);
	}

	Algo::BitwiseAnd(std::span<const std::uint32_t, 9>(lhs), std::span<const std::uint32_t, 9>(rhs), std::span<std::uint32_t, 9>(output));
	for (std::size_t index = 0; index < lhs.size(); ++index)
		REQUIRE(output[index] == (lhs[index] & rhs[index]));
	Algo::BitwiseOr(std::span<const std::uint32_t, 9>(lhs), std::span<const std::uint32_t, 9>(rhs), std::span<std::uint32_t, 9>(output));
	for (std::size_t index = 0; index < lhs.size(); ++index)
		REQUIRE(output[index] == (lhs[index] | rhs[index]));
	Algo::BitwiseXor(std::span<const std::uint32_t, 9>(lhs), std::span<const std::uint32_t, 9>(rhs), std::span<std::uint32_t, 9>(output));
	for (std::size_t index = 0; index < lhs.size(); ++index)
		REQUIRE(output[index] == (lhs[index] ^ rhs[index]));
	Algo::BitwiseNot(std::span<const std::uint32_t, 9>(lhs), std::span<std::uint32_t, 9>(output));
	for (std::size_t index = 0; index < lhs.size(); ++index)
		REQUIRE(output[index] == ~lhs[index]);
	Algo::BitwiseAndNot(std::span<const std::uint32_t, 9>(lhs), std::span<const std::uint32_t, 9>(rhs), std::span<std::uint32_t, 9>(output));
	for (std::size_t index = 0; index < lhs.size(); ++index)
		REQUIRE(output[index] == ((~lhs[index]) & rhs[index]));
}

TEST_CASE("SimdAlgo dynamic spans select 128 and 256 bit execution without semantic drift", "[simdlib][algo]")
{
	using Algo = SimdLib::SimdAlgo<32, 8>;
	for (const std::size_t count : {std::size_t{3}, std::size_t{8}, std::size_t{9}, std::size_t{65}})
	{
		std::vector<std::uint32_t> lhs(count);
		std::vector<std::uint32_t> rhs(count);
		std::vector<std::uint32_t> output(count);
		for (std::size_t index = 0; index < count; ++index)
		{
			lhs[index] = static_cast<std::uint32_t>(index * 0x10203u);
			rhs[index] = static_cast<std::uint32_t>(0xFFFFFFFFu - index * 31u);
		}

		Algo::BitwiseAnd(std::span<const std::uint32_t>(lhs), std::span<const std::uint32_t>(rhs), std::span<std::uint32_t>(output));
		for (std::size_t index = 0; index < count; ++index)
			REQUIRE(output[index] == (lhs[index] & rhs[index]));
		Algo::BitwiseOr(std::span<const std::uint32_t>(lhs), std::span<const std::uint32_t>(rhs), std::span<std::uint32_t>(output));
		for (std::size_t index = 0; index < count; ++index)
			REQUIRE(output[index] == (lhs[index] | rhs[index]));
		Algo::BitwiseXor(std::span<const std::uint32_t>(lhs), std::span<const std::uint32_t>(rhs), std::span<std::uint32_t>(output));
		for (std::size_t index = 0; index < count; ++index)
			REQUIRE(output[index] == (lhs[index] ^ rhs[index]));
		Algo::BitwiseNot(std::span<const std::uint32_t>(lhs), std::span<std::uint32_t>(output));
		for (std::size_t index = 0; index < count; ++index)
			REQUIRE(output[index] == ~lhs[index]);
		Algo::BitwiseAndNot(std::span<const std::uint32_t>(lhs), std::span<const std::uint32_t>(rhs), std::span<std::uint32_t>(output));
		for (std::size_t index = 0; index < count; ++index)
			REQUIRE(output[index] == ((~lhs[index]) & rhs[index]));
	}
}
