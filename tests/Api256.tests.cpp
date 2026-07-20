#include "TestSupport.h"

#include <SimdLib/SimdVector.h>

#include <array>
#include <cstdint>

using namespace SimdLib::Tests;


TEST_CASE("256-bit constexpr contracts match volatile runtime dispatch", "[simdlib][avx2][constexpr][runtime-parity]")
{
	require_constexpr_runtime_parity<256>();
}
TEST_CASE("256-bit Api specialization matrix", "[simdlib][avx2][availability]")
{
    require_supported_addition_matrix<256>();
}

TEST_CASE("256-bit aligned and unaligned transfer matrix", "[simdlib][avx2][transfer]")
{
    require_supported_transfer_matrix<256>();
}

TEST_CASE("256-bit partial loads accept unaligned prefixes and zero inactive lanes", "[simdlib][avx2][transfer][partial]")
{
    require_supported_partial_transfer_matrix<256>();
}

TEST_CASE("256-bit movemask contracts are byte and element granular", "[simdlib][avx2][movemask]")
{
    require_supported_movemask_matrix<256>();
}

TEST_CASE("256-bit transform_pack preserves packed lane order and exact tails", "[simdlib][avx2][transform-pack]")
{
	require_transform_pack_full_native_word_contract<256>();
	require_transform_pack_mask_contract<256, std::uint8_t, 40>();
	require_transform_pack_mask_contract<256, std::uint8_t, 80>();
	require_transform_pack_mask_contract<256, std::uint16_t, 24>();
	require_transform_pack_mask_contract<256, std::uint64_t, 8>();
	require_transform_pack_width_contract<256, std::uint32_t, 11, 3>();
	require_transform_pack_width_contract<256, std::uint64_t, 9, 9>();
	require_transform_pack_width_contract<256, std::uint64_t, 9, 16>();
	require_transform_pack_type_matrix<256>();
}

TEST_CASE("256-bit public transform overloads preserve exact spans", "[simdlib][avx2][transform]")
{
	require_transform_overload_contract<256>();
}

TEST_CASE("256-bit float and double dot products use public Api entry points", "[simdlib][avx2][dot]")
{
    using floats = SimdLib::Api<256, float>;
    const auto floatDot = floats::template dot_product<0xFF>(floats::set1(1.0F), floats::set1(2.0F));
    REQUIRE(floats::to_array(floatDot) == std::array<float, 8>{8.0F, 8.0F, 8.0F, 8.0F, 8.0F, 8.0F, 8.0F, 8.0F});

    using doubles = SimdLib::Api<256, double>;
    const auto doubleDot = doubles::template dot_product<0xFF>(doubles::set1(1.0), doubles::set1(2.0));
    REQUIRE(doubles::to_array(doubleDot) == std::array<double, 4>{4.0, 4.0, 4.0, 4.0});

    const auto floatPartialDot = floats::template dot_product<0x11>(floats::set1(1.0F), floats::set1(2.0F));
    REQUIRE(floats::to_array(floatPartialDot) == std::array<float, 8>{2.0F, 0.0F, 0.0F, 0.0F, 2.0F, 0.0F, 0.0F, 0.0F});
    const auto doublePartialDot = doubles::template dot_product<0x11>(doubles::set1(1.0), doubles::set1(2.0));
    REQUIRE(doubles::to_array(doublePartialDot) == std::array<double, 4>{2.0, 0.0, 2.0, 0.0});
}
TEST_CASE("256-bit byte function-pointer transforms use public Api entry points", "[simdlib][avx2][transform][byte]")
{
    using bytes = SimdLib::Api<256, std::uint8_t>;
    std::array<std::uint8_t, 33> lhs{};
    std::array<std::uint8_t, 33> rhs{};
    std::array<std::uint8_t, 33> output{};
    for (std::size_t index = 0; index < lhs.size(); ++index)
    {
        lhs[index] = static_cast<std::uint8_t>(index + 1);
        rhs[index] = static_cast<std::uint8_t>(0xA0U + index);
    }

    bytes::transform(std::span<const std::uint8_t>(lhs), std::span<std::uint8_t>(output), bytes::bitwise_not);
    for (std::size_t index = 0; index < output.size(); ++index)
        REQUIRE(output[index] == static_cast<std::uint8_t>(~lhs[index]));

    bytes::transform(std::span<const std::uint8_t>(lhs), std::span<const std::uint8_t>(rhs), std::span<std::uint8_t>(output), bytes::bitwise_xor);
    for (std::size_t index = 0; index < output.size(); ++index)
        REQUIRE(output[index] == static_cast<std::uint8_t>(lhs[index] ^ rhs[index]));
}
TEST_CASE("256-bit integer extrema and position matrix uses public Api entry points", "[simdlib][avx2][extrema][position]")
{
	require_integer_extrema_position_matrix<256>();
}

TEST_CASE("256-bit public integer operation matrix", "[simdlib][avx2][integer][operations]")
{
	require_integer_operation_matrix<256>();
}

TEST_CASE("256-bit public floating operation matrix", "[simdlib][avx2][floating][operations]")
{
	require_floating_operation_matrix<256>();
}

TEST_CASE("256-bit unsigned 32-bit conversion and division boundaries", "[simdlib][avx2][uint32][conversion][division]")
{
	require_unsigned_32bit_contract<256>();
}

TEST_CASE("256-bit signed 32-bit conversion boundaries", "[simdlib][avx2][int32][conversion]")
{
	require_signed_32bit_conversion_contract<256>();
}

TEST_CASE("256-bit uint64 adjacent multiply-add ordering and overflow", "[simdlib][avx2][uint64][multiply-add]")
{
	require_uint64_multiply_add_adjacent_contract<256>();
}

TEST_CASE("256-bit arithmetic, horizontal operations, shuffles, and blends match scalar references", "[simdlib][avx2][operations]")
{
	require_supported_comparison_matrix<256>();

    using simd = SimdLib::Api<256, std::int32_t>;
    const auto lhs = simd::setr(1, 2, 3, 4, 5, 6, 7, 8);
    const auto rhs = simd::setr(8, 7, 6, 5, 4, 3, 2, 1);
    REQUIRE(simd::to_array(simd::multiply(lhs, rhs)) == std::array<std::int32_t, 8>{8, 14, 18, 20, 20, 18, 14, 8});
    REQUIRE(simd::to_array(simd::add_horizontal(lhs, rhs)) == std::array<std::int32_t, 8>{3, 7, 15, 11, 11, 15, 7, 3});
    REQUIRE(simd::to_array(simd::shuffle_32(lhs, 0b00'01'10'11)) == std::array<std::int32_t, 8>{4, 3, 2, 1, 8, 7, 6, 5});
    REQUIRE(simd::to_array(simd::blend(lhs, rhs, 0b01010101)) == std::array<std::int32_t, 8>{8, 2, 6, 4, 4, 6, 2, 8});
}

TEST_CASE("256-bit public 64-bit arithmetic contract", "[simdlib][avx2][int64][arithmetic]")
{
	require_64bit_arithmetic_contract<256>();
}

TEST_CASE("256-bit public byte operations cover multiplication and lane shifts", "[simdlib][avx2][byte][shift]")
{
	using bytes = SimdLib::Api<256, std::uint8_t>;
	REQUIRE(bytes::to_array(bytes::set1(0x81)) == std::array<std::uint8_t, 32>{0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81});
	REQUIRE(bytes::to_array(bytes::multiply(bytes::set1(0x81), bytes::set1(2))) == std::array<std::uint8_t, 32>{2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2});
	REQUIRE(bytes::to_array(bytes::shift_left(bytes::set1(0x81), 1))[0] == 0x02);
	REQUIRE(bytes::to_array(bytes::shift_right(bytes::set1(0x81), 1))[0] == 0x40);
	using signed_bytes = SimdLib::Api<256, std::int8_t>;
	REQUIRE(signed_bytes::to_array(signed_bytes::shift_right_arithmetic(signed_bytes::set1(-126), 1))[0] == -63);
}

TEST_CASE("256-bit SimdVector preserves arithmetic and storage", "[simdlib][avx2][vector]")
{
    using vector = SimdLib::SimdVector<std::int32_t, 8>;
    const vector lhs{std::array<std::int32_t, 8>{1, 2, 3, 4, 5, 6, 7, 8}};
    const vector rhs{2};
    REQUIRE(vector{lhs * rhs}.toArray() == std::array<std::int32_t, 8>{2, 4, 6, 8, 10, 12, 14, 16});
}
