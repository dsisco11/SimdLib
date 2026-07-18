#include "TestSupport.h"

#include <SimdLib/SimdVector.h>

#include <array>
#include <cstdint>

using namespace SimdLib::Tests;

static_assert(constexpr_movemask_contract<256, std::uint64_t>());
static_assert(constexpr_movemask_contract<256, double>());

TEST_CASE("256-bit Api specialization matrix", "[simdlib][avx2][availability]")
{
    require_supported_addition_matrix<256>();
}

TEST_CASE("256-bit aligned and unaligned transfer matrix", "[simdlib][avx2][transfer]")
{
    require_supported_transfer_matrix<256>();
}

TEST_CASE("256-bit movemask contracts are byte and element granular", "[simdlib][avx2][movemask]")
{
    require_supported_movemask_matrix<256>();
}

TEST_CASE("256-bit arithmetic, horizontal operations, shuffles, and blends match scalar references", "[simdlib][avx2][operations]")
{
    using simd = SimdLib::Api<256, std::int32_t>;
    const auto lhs = simd::setr(1, 2, 3, 4, 5, 6, 7, 8);
    const auto rhs = simd::setr(8, 7, 6, 5, 4, 3, 2, 1);
    REQUIRE(simd::to_array(simd::multiply(lhs, rhs)) == std::array<std::int32_t, 8>{8, 14, 18, 20, 20, 18, 14, 8});
    REQUIRE(simd::to_array(simd::add_horizontal(lhs, rhs)) == std::array<std::int32_t, 8>{3, 7, 15, 11, 11, 15, 7, 3});
    REQUIRE(simd::to_array(simd::shuffle_32(lhs, 0b00'01'10'11)) == std::array<std::int32_t, 8>{4, 3, 2, 1, 8, 7, 6, 5});
    REQUIRE(simd::to_array(simd::blend(lhs, rhs, 0b01010101)) == std::array<std::int32_t, 8>{8, 2, 6, 4, 4, 6, 2, 8});
}

TEST_CASE("256-bit SimdVector preserves arithmetic and storage", "[simdlib][avx2][vector]")
{
    using vector = SimdLib::SimdVector<std::int32_t, 8>;
    const vector lhs{std::array<std::int32_t, 8>{1, 2, 3, 4, 5, 6, 7, 8}};
    const vector rhs{2};
    REQUIRE(vector{lhs * rhs}.toArray() == std::array<std::int32_t, 8>{2, 4, 6, 8, 10, 12, 14, 16});
}
