#include "TestSupport.h"

#include <algorithm>
#include <array>
#include <cstdint>

using namespace SimdLib::Tests;

static_assert(constexpr_movemask_contract<128, std::uint32_t>());
static_assert(constexpr_movemask_contract<128, float>());

TEST_CASE("128-bit Api specialization matrix", "[simdlib][sse42][availability]")
{
    require_supported_addition_matrix<128>();
}

TEST_CASE("128-bit aligned and unaligned transfer matrix", "[simdlib][sse42][transfer]")
{
    require_supported_transfer_matrix<128>();
}

TEST_CASE("128-bit movemask contracts are byte and element granular", "[simdlib][sse42][movemask]")
{
    require_supported_movemask_matrix<128>();
}

TEST_CASE("128-bit arithmetic and int8 division match scalar results", "[simdlib][sse42][arithmetic]")
{
    using integers = SimdLib::Api<128, std::int32_t>;
    const auto lhs = integers::setr(4, 8, 12, 16);
    const auto rhs = integers::setr(1, 2, 3, 4);
    REQUIRE(integers::to_array(integers::subtract(lhs, rhs)) == std::array<std::int32_t, 4>{3, 6, 9, 12});
    REQUIRE(integers::to_array(integers::multiply(lhs, rhs)) == std::array<std::int32_t, 4>{4, 16, 36, 64});

    using bytes = SimdLib::Api<128, std::int8_t>;
    const auto quotients = bytes::divide(bytes::set1(24), bytes::set1(6));
    REQUIRE(bytes::to_array(quotients) == std::array<std::int8_t, 16>{4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4});
}

TEST_CASE("128-bit comparisons and saturation match scalar semantics", "[simdlib][sse42][comparison][saturation]")
{
    using words = SimdLib::Api<128, std::int32_t>;
    const auto lhs = words::setr(1, 2, 3, 4);
    const auto rhs = words::setr(1, 0, 3, 9);
    REQUIRE(words::cmp_eq_mask(lhs, rhs) == 0x00000F0Fu);

    using bytes = SimdLib::Api<128, std::uint8_t>;
    REQUIRE(bytes::to_array(bytes::add_saturated(bytes::set1(250), bytes::set1(10))) ==
            std::array<std::uint8_t, 16>{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255});
    REQUIRE(bytes::to_array(bytes::subtract_saturated(bytes::set1(5), bytes::set1(10))) == std::array<std::uint8_t, 16>{});
}

TEST_CASE("128-bit widening and horizontal arithmetic match scalar references", "[simdlib][sse42][widen][horizontal]")
{
    using source = SimdLib::Api<128, std::int8_t>;
    using target = SimdLib::Api<128, std::int16_t>;
    const auto widened = source::template widen<target>(source::setr(-4, -3, -2, -1, 0, 1, 2, 3, 90, 91, 92, 93, 94, 95, 96, 97));
    REQUIRE(target::to_array(widened) == std::array<std::int16_t, 8>{-4, -3, -2, -1, 0, 1, 2, 3});

    using lanes = SimdLib::Api<128, std::int32_t>;
    const auto horizontal = lanes::add_horizontal(lanes::setr(1, 2, 3, 4), lanes::setr(5, 6, 7, 8));
    REQUIRE(lanes::to_array(horizontal) == std::array<std::int32_t, 4>{3, 7, 11, 15});
}

TEST_CASE("128-bit lane and whole-register shifts are distinct", "[simdlib][sse42][shift]")
{
    using simd = SimdLib::Api<128, std::uint64_t>;
    const auto input = simd::setr(0x0123456789ABCDEFULL, 0xFEDCBA9876543210ULL);
    REQUIRE(simd::to_array(simd::shift_left(input, 4)) ==
            std::array<std::uint64_t, 2>{0x123456789ABCDEF0ULL, 0xEDCBA98765432100ULL});

    const std::array<int, 9> counts{0, 1, 63, 64, 65, 127, 128, 129, 255};
    const auto source = simd::to_array(input);
    for (const int count : counts)
    {
        std::array<std::uint64_t, 2> left{};
        std::array<std::uint64_t, 2> right{};
        if (count == 0)
        {
            left = source;
            right = source;
        }
        else if (count < 64)
        {
            left = {source[0] << count, (source[1] << count) | (source[0] >> (64 - count))};
            right = {(source[0] >> count) | (source[1] << (64 - count)), source[1] >> count};
        }
        else if (count == 64)
        {
            left = {0, source[0]};
            right = {source[1], 0};
        }
        else if (count < 128)
        {
            left = {0, source[0] << (count - 64)};
            right = {source[1] >> (count - 64), 0};
        }
        REQUIRE(simd::to_array(simd::bit_shift_left(input, count)) == left);
        REQUIRE(simd::to_array(simd::bit_shift_right(input, count)) == right);
    }

    REQUIRE(simd::to_array(simd::template bit_shift_left<64>(input)) == std::array<std::uint64_t, 2>{0, source[0]});
    REQUIRE(simd::to_array(simd::template bit_shift_right<128>(input)) == std::array<std::uint64_t, 2>{});
}

TEST_CASE("128-bit shuffle, blend, and position helpers match scalar references", "[simdlib][sse42][shuffle][blend][position]")
{
    using words = SimdLib::Api<128, std::int32_t>;
    const auto lhs = words::setr(10, 20, 30, 40);
    const auto rhs = words::setr(1, 2, 3, 4);
    REQUIRE(words::to_array(words::shuffle_32(lhs, 0b00'01'10'11)) == std::array<std::int32_t, 4>{40, 30, 20, 10});
    REQUIRE(words::to_array(words::blend(lhs, rhs, 0b0101)) == std::array<std::int32_t, 4>{1, 20, 3, 40});

    using positions = SimdLib::Api<128, std::uint16_t>;
    const auto values = positions::setr(8, 4, 7, 1, 9, 2, 6, 3);
    REQUIRE(positions::min_position(values) == 3);
    REQUIRE(positions::max_position(values) == 4);
}
