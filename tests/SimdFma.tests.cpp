#include <SimdLib/Api.h>

#include <catch2/catch_test_macros.hpp>

#include <array>

#ifndef SIMDLIB_EXPECT_FMA
#define SIMDLIB_EXPECT_FMA 0
#endif

static_assert((SIMDLIB_EXPECT_FMA != 0) == SimdLib::Config::has_fma);

TEST_CASE("FMA-specialized multiply-add matches scalar arithmetic", "[simdlib][fma]")
{
	using simd128 = SimdLib::Api<128, float>;
	const auto result128 = simd128::multiply_add(simd128::set1(2.0f), simd128::set1(3.0f), simd128::set1(4.0f));
	REQUIRE(simd128::to_array(result128) == std::array<float, 4>{10.0f, 10.0f, 10.0f, 10.0f});

	using simd256 = SimdLib::Api<256, double>;
	const auto result256 = simd256::multiply_add(simd256::set1(2.0), simd256::set1(3.0), simd256::set1(4.0));
	REQUIRE(simd256::to_array(result256) == std::array<double, 4>{10.0, 10.0, 10.0, 10.0});
}
