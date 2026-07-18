#include <SimdLib/SimdLib.h>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>

TEST_CASE("SimdLib representative extraction benchmarks", "[simdlib][benchmark][validation]")
{
	using simd128 = SimdLib::Api<128, std::uint32_t>;
	using simd256 = SimdLib::Api<256, std::uint32_t>;
	const auto lhs128 = simd128::set1(0x13579BDFu);
	const auto rhs128 = simd128::set1(0x01020304u);
	const auto lhs256 = simd256::set1(0x13579BDFu);
	const auto rhs256 = simd256::set1(0x01020304u);

	BENCHMARK("Api 128-bit add")
	{
		return simd128::add(lhs128, rhs128);
	};
	BENCHMARK("Api 256-bit add")
	{
		return simd256::add(lhs256, rhs256);
	};
	BENCHMARK("BMI2 pext_u64")
	{
		return SimdLib::Bmi::pext_u64(std::uint64_t{0xFEDCBA9876543210ULL}, std::uint64_t{0x0F0F33335555AAAAULL});
	};
	BENCHMARK("uint128_t add")
	{
		return SimdLib::uint128_t{0xFEDCBA9876543210ULL, 0x0123456789ABCDEFULL} + SimdLib::uint128_t{0x1111111111111111ULL, 0x2222222222222222ULL};
	};

	const std::array<std::uint8_t, 256> expanded_input = []
	{
		std::array<std::uint8_t, 256> values{};
		for (std::size_t index = 0; index < values.size(); ++index)
		{
			values[index] = static_cast<std::uint8_t>((index * 37u) | 1u);
		}
		return values;
	}();
	std::array<std::uint8_t, 32> packed_output{};
	BENCHMARK("bitmask resample reduce-by-8 any")
	{
		SimdLib::SimdResample::ReduceBytesToBitsBy8_Any(expanded_input, packed_output);
		return packed_output;
	};
}
