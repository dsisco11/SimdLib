#include "TestSupport.h"
#include <SimdLib/Bmi.h>

#include <algorithm>
#include <array>
#include <cstdint>

using namespace SimdLib::Tests;

#if !SIMDLIB_HAS_AVX2
static_assert(SimdLib::NativeApi<float>::register_width == 128);
#endif

TEST_CASE("128-bit constexpr contracts match volatile runtime dispatch", "[simdlib][sse42][constexpr][runtime-parity]")
{
	require_constexpr_runtime_parity<128>();
}
TEST_CASE("128-bit Api specialization matrix", "[simdlib][sse42][availability]")
{
    require_supported_addition_matrix<128>();
}

TEST_CASE("128-bit aligned and unaligned transfer matrix", "[simdlib][sse42][transfer]")
{
    require_supported_transfer_matrix<128>();
}

TEST_CASE("128-bit partial loads accept unaligned prefixes and zero inactive lanes", "[simdlib][sse42][transfer][partial]")
{
    require_supported_partial_transfer_matrix<128>();
}

TEST_CASE("128-bit movemask contracts are byte and element granular", "[simdlib][sse42][movemask]")
{
    require_supported_movemask_matrix<128>();
}

TEST_CASE("128-bit transform_pack preserves packed lane order and exact tails", "[simdlib][sse42][transform-pack]")
{
	require_transform_pack_full_native_word_contract<128>();
    require_transform_pack_mask_contract<128, std::uint8_t, 24>();
    require_transform_pack_mask_contract<128, std::uint8_t, 80>();
    require_transform_pack_mask_contract<128, std::uint64_t, 8>();
    require_transform_pack_width_contract<128, std::uint32_t, 7, 3>();
    require_transform_pack_width_contract<128, std::uint32_t, 19, 9>();
    require_transform_pack_width_contract<128, std::uint64_t, 5, 32>();
    require_transform_pack_type_matrix<128>();
}

TEST_CASE("128-bit public transform overloads preserve exact spans", "[simdlib][sse42][transform]")
{
	require_transform_overload_contract<128>();
}

TEST_CASE("128-bit partial construction and float dot product use public Api entry points", "[simdlib][sse42][partial][dot]")
{
    using integers = SimdLib::Api<128, std::uint32_t>;
    const std::array<std::uint32_t, 2> prefix{3, 5};
    REQUIRE(integers::to_array(integers::setr_partial(3U, 5U)) == std::array<std::uint32_t, 4>{3, 5, 0, 0});
    REQUIRE(integers::to_array(integers::template load_partial<2>(prefix)) == std::array<std::uint32_t, 4>{3, 5, 0, 0});

    using floats = SimdLib::Api<128, float>;
    const auto dot = floats::template dot_product<0xFF>(floats::set1(1.0F), floats::set1(2.0F));
    REQUIRE(floats::to_array(dot) == std::array<float, 4>{8.0F, 8.0F, 8.0F, 8.0F});
    const auto partialDot = floats::template dot_product<0x11>(floats::set1(1.0F), floats::set1(2.0F));
    REQUIRE(floats::to_array(partialDot) == std::array<float, 4>{2.0F, 0.0F, 0.0F, 0.0F});
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
	require_supported_comparison_matrix<128>();

    using words = SimdLib::Api<128, std::int32_t>;
    const auto lhs = words::setr(1, 2, 3, 4);
    const auto rhs = words::setr(1, 0, 3, 9);
    REQUIRE(words::cmp_eq_mask(lhs, rhs) == 0x00000F0Fu);

    using bytes = SimdLib::Api<128, std::uint8_t>;
    REQUIRE(bytes::to_array(bytes::add_saturated(bytes::set1(250), bytes::set1(10))) ==
            std::array<std::uint8_t, 16>{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255});
    REQUIRE(bytes::to_array(bytes::subtract_saturated(bytes::set1(5), bytes::set1(10))) == std::array<std::uint8_t, 16>{});
}

TEST_CASE("128-bit integer extrema and position matrix uses public Api entry points", "[simdlib][sse42][extrema][position]")
{
	require_integer_extrema_position_matrix<128>();
}

TEST_CASE("128-bit public integer operation matrix", "[simdlib][sse42][integer][operations]")
{
	require_integer_operation_matrix<128>();
}

TEST_CASE("128-bit public floating operation matrix", "[simdlib][sse42][floating][operations]")
{
	require_floating_operation_matrix<128>();
}

TEST_CASE("128-bit unsigned 32-bit conversion and division boundaries", "[simdlib][sse42][uint32][conversion][division]")
{
	require_unsigned_32bit_contract<128>();
}

TEST_CASE("128-bit uint64 adjacent multiply-add ordering and overflow", "[simdlib][sse42][uint64][multiply-add]")
{
	require_uint64_multiply_add_adjacent_contract<128>();
}

TEST_CASE("128-bit signed integer and float conversion gates preserve lane values", "[simdlib][sse42][conversion]")
{
	require_signed_32bit_conversion_contract<128>();
	using integers = SimdLib::Api<128, std::int32_t>;
	using floats = SimdLib::Api<128, float>;
	const auto integer_values = integers::setr(-7, 0, 42, 1'000'000);
	REQUIRE(floats::to_array(integers::convert_to_float(integer_values)) ==
			std::array<float, 4>{-7.0f, 0.0f, 42.0f, 1'000'000.0f});

	const auto float_values = floats::setr(-7.0f, 0.0f, 42.0f, 1'000'000.0f);
	REQUIRE(integers::to_array(floats::convert_to_int(float_values)) ==
			std::array<std::int32_t, 4>{-7, 0, 42, 1'000'000});
}

TEST_CASE("128-bit public 64-bit arithmetic contract", "[simdlib][sse42][int64][arithmetic]")
{
	require_64bit_arithmetic_contract<128>();
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

TEST_CASE("128-bit public byte operations cover lane shifts and byte-shift boundaries", "[simdlib][sse42][byte][shift]")
{
	using bytes = SimdLib::Api<128, std::uint8_t>;
	std::array<std::uint8_t, bytes::element_count> source{};
	for (std::size_t index = 0; index < source.size(); ++index)
		source[index] = static_cast<std::uint8_t>(index + 1);
	const auto input = bytes::construct(source);
	REQUIRE(bytes::to_array(bytes::set1(0x81)) == std::array<std::uint8_t, 16>{0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81});
	REQUIRE(bytes::to_array(bytes::multiply(bytes::set1(0x81), bytes::set1(2))) == std::array<std::uint8_t, 16>{2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2});
	REQUIRE(bytes::to_array(bytes::shift_left(bytes::set1(0x81), 1))[0] == 0x02);
	REQUIRE(bytes::to_array(bytes::shift_right(bytes::set1(0x81), 1))[0] == 0x40);
	using signed_bytes = SimdLib::Api<128, std::int8_t>;
	REQUIRE(signed_bytes::to_array(signed_bytes::shift_right_arithmetic(signed_bytes::set1(-126), 1))[0] == -63);

	for (const int count : std::array<int, 6>{-1, 0, 1, 15, 16, 17})
	{
		std::array<std::uint8_t, bytes::element_count> left{};
		std::array<std::uint8_t, bytes::element_count> right{};
		if (count <= 0) { left = source; right = source; }
		else if (count < static_cast<int>(bytes::byte_count))
		{
			for (std::size_t index = static_cast<std::size_t>(count); index < source.size(); ++index)
				left[index] = source[index - static_cast<std::size_t>(count)];
			for (std::size_t index = 0; index + static_cast<std::size_t>(count) < source.size(); ++index)
				right[index] = source[index + static_cast<std::size_t>(count)];
		}
		REQUIRE(bytes::to_array(bytes::byte_shift_left(input, count)) == left);
		REQUIRE(bytes::to_array(bytes::byte_shift_right(input, count)) == right);
	}
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

TEST_CASE("128-bit Api documentation examples produce their documented results", "[simdlib][sse42][documentation]")
{
	using ApiT = SimdLib::Api<128, float>;
	using I8 = SimdLib::Api<128, std::int8_t>;
	using I16 = SimdLib::Api<128, std::int16_t>;
	using I32 = SimdLib::Api<128, std::int32_t>;
	using U8 = SimdLib::Api<128, std::uint8_t>;
	using U16 = SimdLib::Api<128, std::uint16_t>;
	using U32 = SimdLib::Api<128, std::uint32_t>;
	STATIC_REQUIRE(SimdLib::is_api_available_v<128, float>);
	STATIC_REQUIRE(SimdLib::ApiAvailable<128, float>);
	require_documented_register<ApiT>(ApiT::absolute(ApiT::construct({-2.0F, 3.0F, 0.0F, 0.0F})), std::array{2.0F, 3.0F, 0.0F, 0.0F});
	require_documented_register<ApiT>(ApiT::add(ApiT::set1(2.0F), ApiT::set1(3.0F)), std::array{5.0F, 5.0F, 5.0F, 5.0F});
	require_documented_register<ApiT>(ApiT::add_horizontal(ApiT::setr(1.0F, 2.0F, 3.0F, 4.0F), ApiT::setr(5.0F, 6.0F, 7.0F, 8.0F)),
									  std::array{3.0F, 7.0F, 11.0F, 15.0F});
	require_documented_register<U8>(U8::add_saturated(U8::set1(250), U8::set1(10)),
									std::array<std::uint8_t, 16>{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255});
	require_documented_register<ApiT>(ApiT::add_subtract(ApiT::set1(10.0F), ApiT::setr(1.0F, 2.0F, 3.0F, 4.0F)), std::array{9.0F, 12.0F, 7.0F, 14.0F});
	require_documented_register<U8>(U8::avg(U8::set1(2), U8::set1(6)), std::array<std::uint8_t, 16>{4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4});
	require_documented_register<U32>(U32::bit_shift_left(U32::set1(3), 1), std::array{6U, 6U, 6U, 6U});
	require_documented_register<U32>(U32::bit_shift_right(U32::set1(8), 1), std::array{4U, 4U, 4U, 4U});
	require_documented_register<U32>(U32::bitwise_and(U32::set1(12), U32::set1(10)), std::array{8U, 8U, 8U, 8U});
	require_documented_register<U32>(U32::bitwise_andnot(U32::set1(12), U32::set1(10)), std::array{2U, 2U, 2U, 2U});
	require_documented_register<U32>(U32::bitwise_not(U32::setzero()), std::array{0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU});
	require_documented_register<U32>(U32::bitwise_or(U32::set1(12), U32::set1(10)), std::array{14U, 14U, 14U, 14U});
	require_documented_register<U32>(U32::bitwise_xor(U32::set1(12), U32::set1(10)), std::array{6U, 6U, 6U, 6U});
	require_documented_register<I32>(I32::blend(I32::setr(10, 20, 30, 40), I32::setr(1, 2, 3, 4), 0b0101), std::array{1, 20, 3, 40});
	require_documented_register<U8>(U8::byte_shift_left(U8::set1(7), 1), std::array<std::uint8_t, 16>{0, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7});
	require_documented_register<U8>(U8::byte_shift_right(U8::set1(7), 1), std::array<std::uint8_t, 16>{7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 0});
	REQUIRE(ApiT::cmp_eq(ApiT::set1(2.0F), ApiT::set1(2.0F)) == 0xFFFFU);
	REQUIRE(ApiT::cmp_eq_mask(ApiT::set1(2.0F), ApiT::set1(2.0F)) == 0xFFFFU);
	REQUIRE(ApiT::cmp_ge(ApiT::set1(2.0F), ApiT::set1(2.0F)) == 0xFFFFU);
	REQUIRE(ApiT::cmp_gt(ApiT::set1(3.0F), ApiT::set1(2.0F)) == 0xFFFFU);
	REQUIRE(ApiT::cmp_le(ApiT::set1(2.0F), ApiT::set1(2.0F)) == 0xFFFFU);
	REQUIRE(ApiT::cmp_lt(ApiT::set1(2.0F), ApiT::set1(3.0F)) == 0xFFFFU);
	require_documented_register<I8>(I16::compress(I16::set1(300), I16::set1(-300)),
									std::array<std::int8_t, 16>{127, 127, 127, 127, 127, 127, 127, 127, -128, -128, -128, -128, -128, -128, -128, -128});
	require_documented_register<ApiT>(ApiT::construct({1.0F, 2.0F, 0.0F, 0.0F}), std::array{1.0F, 2.0F, 0.0F, 0.0F});
	require_documented_register<I32>(ApiT::convert(ApiT::set1(3.6F)), std::array{4, 4, 4, 4});
	require_documented_register<ApiT>(I32::convert_to_float(I32::set1(16777217)), std::array{16777216.0F, 16777216.0F, 16777216.0F, 16777216.0F});
	require_documented_register<I32>(ApiT::convert_to_int(ApiT::set1(3.6F)), std::array{4, 4, 4, 4});
	require_documented_register<ApiT>(ApiT::divide(ApiT::set1(8.0F), ApiT::set1(2.0F)), std::array{4.0F, 4.0F, 4.0F, 4.0F});
	require_documented_register<ApiT>(ApiT::dot_product<0xFF>(ApiT::setr_partial(1.0F, 2.0F), ApiT::setr_partial(3.0F, 4.0F)),
									  std::array{11.0F, 11.0F, 11.0F, 11.0F});
	REQUIRE(I32::extract<0>(I32::setr(7, 8, 9, 10)) == 7);
	require_documented_register<I16>(I16::hadd_saturated(I16::set1(20000), I16::set1(10000)),
									 std::array<std::int16_t, 8>{32767, 32767, 32767, 32767, 20000, 20000, 20000, 20000});
	require_documented_register<I16>(I16::hsubtract_saturated(I16::setr_partial(30000, -10000, -30000, 10000), I16::setr_partial(20000, -20000, 10000, -10000)),
									 std::array<std::int16_t, 8>{32767, -32768, 0, 0, 32767, 20000, 0, 0});
	require_documented_register<I32>(I32::insert(I32::setzero(), 9, 0), std::array{9, 0, 0, 0});
	alignas(ApiT::byte_count) const std::array<float, ApiT::element_count> input{1.0F, 2.0F};
	require_documented_register<ApiT>(ApiT::load(input), std::array{1.0F, 2.0F, 0.0F, 0.0F});
	require_documented_register<ApiT>(ApiT::load_aligned(input), std::array{1.0F, 2.0F, 0.0F, 0.0F});
	require_documented_register<ApiT>(ApiT::load_partial<2>(std::span<const float>{input}), std::array{1.0F, 2.0F, 0.0F, 0.0F});
	require_documented_register<ApiT>(ApiT::load_unaligned(input), std::array{1.0F, 2.0F, 0.0F, 0.0F});
	require_documented_register<ApiT>(ApiT::load_unsafe(input), std::array{1.0F, 2.0F, 0.0F, 0.0F});
	require_documented_register<ApiT>(ApiT::magnitude(ApiT::setr_partial(3.0F, 4.0F)), std::array{5.0F, 5.0F, 5.0F, 5.0F});
	require_documented_register<ApiT>(ApiT::max(ApiT::setr(2.0F, 8.0F, 4.0F, 9.0F), ApiT::setr(5.0F, 3.0F, 7.0F, 1.0F)), std::array{5.0F, 8.0F, 7.0F, 9.0F});
	REQUIRE(U16::max_position(U16::insert(U16::set1(4), 9, 3)) == 3);
	require_documented_register<ApiT>(ApiT::min(ApiT::setr(2.0F, 8.0F, 4.0F, 9.0F), ApiT::setr(5.0F, 3.0F, 7.0F, 1.0F)), std::array{2.0F, 3.0F, 4.0F, 1.0F});
	REQUIRE(U16::min_position(U16::insert(U16::set1(4), 1, 3)) == 3);
	require_documented_register<U32>(U32::modulus(U32::set1(7), U32::set1(3)), std::array{1U, 1U, 1U, 1U});
	REQUIRE(ApiT::movemask(ApiT::set1(-0.0F)) == 0x8888U);
	REQUIRE(ApiT::movemask_slim(ApiT::set1(-0.0F)) == 0xFU);
	require_documented_register<SimdLib::Api<128, std::uint16_t>>(U8::multi_sum_absolute_byte_differences<0>(U8::set1(9), U8::set1(4)),
																  std::array<std::uint16_t, 8>{20, 20, 20, 20, 20, 20, 20, 20});
	require_documented_register<ApiT>(ApiT::multiply(ApiT::set1(3.0F), ApiT::set1(4.0F)), std::array{12.0F, 12.0F, 12.0F, 12.0F});
	require_documented_register<ApiT>(ApiT::multiply_add(ApiT::set1(2.0F), ApiT::set1(3.0F), ApiT::set1(4.0F)), std::array{10.0F, 10.0F, 10.0F, 10.0F});
	require_documented_register<I32>(I16::multiply_add_adjacent(I16::set1(2), I16::set1(3)), std::array{12, 12, 12, 12});
	require_documented_register<I16>(U8::multiply_add_unsigned_signed_bytes(U8::set1(2), U8::set1(3)),
									 std::array<std::int16_t, 8>{12, 12, 12, 12, 12, 12, 12, 12});
	require_documented_register<ApiT>(ApiT::negate(ApiT::setr_partial(2.0F, -3.0F)), std::array{-2.0F, 3.0F, 0.0F, 0.0F});
	const auto normalized = ApiT::to_array(ApiT::normalize(ApiT::setr_partial(3.0F, 4.0F)));
	REQUIRE(normalized[0] > 0.599F);
	REQUIRE(normalized[0] < 0.601F);
	REQUIRE(normalized[1] > 0.799F);
	REQUIRE(normalized[1] < 0.801F);
	require_documented_register<ApiT>(ApiT::set(4.0F, 3.0F, 2.0F, 1.0F), std::array{1.0F, 2.0F, 3.0F, 4.0F});
	require_documented_register<ApiT>(ApiT::set_partial(2.0F, 1.0F), std::array{0.0F, 0.0F, 1.0F, 2.0F});
	require_documented_register<ApiT>(ApiT::set1(2.5F), std::array{2.5F, 2.5F, 2.5F, 2.5F});
	require_documented_register<ApiT>(ApiT::setr(1.0F, 2.0F, 3.0F, 4.0F), std::array{1.0F, 2.0F, 3.0F, 4.0F});
	require_documented_register<ApiT>(ApiT::setr_partial(1.0F, 2.0F), std::array{1.0F, 2.0F, 0.0F, 0.0F});
	require_documented_register<ApiT>(ApiT::setzero(), std::array{0.0F, 0.0F, 0.0F, 0.0F});
	require_documented_register<I32>(I32::shift_left(I32::set1(3), 1), std::array{6, 6, 6, 6});
	require_documented_register<I32>(I32::shift_right(I32::set1(8), 1), std::array{4, 4, 4, 4});
	require_documented_register<I32>(I32::shift_right_arithmetic(I32::set1(-8), 1), std::array{-4, -4, -4, -4});
	require_documented_register<U8>(U8::shuffle(U8::set1(7), U8::set1(0x80)), std::array<std::uint8_t, 16>{});
	const auto high = I16::byte_shift_left(I16::setr_partial(1, 2, 3, 4), 8);
	require_documented_register<I16>(I16::shuffle_hi(high, 0b0001'1011), std::array<std::int16_t, 8>{0, 0, 0, 0, 4, 3, 2, 1});
	require_documented_register<I16>(I16::shuffle_lo(I16::setr_partial(1, 2, 3, 4), 0b0001'1011), std::array<std::int16_t, 8>{4, 3, 2, 1, 0, 0, 0, 0});
	require_documented_register<ApiT>(ApiT::sqrt(ApiT::setr_partial(4.0F, 9.0F)), std::array{2.0F, 3.0F, 0.0F, 0.0F});
	alignas(ApiT::byte_count) std::array<float, ApiT::element_count> stored{};
	ApiT::store(ApiT::setr_partial(1.0F, 2.0F), stored);
	REQUIRE(stored == std::array{1.0F, 2.0F, 0.0F, 0.0F});
	ApiT::store_aligned(ApiT::setr_partial(1.0F, 2.0F), stored);
	REQUIRE(stored == std::array{1.0F, 2.0F, 0.0F, 0.0F});
	ApiT::store_unaligned(ApiT::setr_partial(1.0F, 2.0F), stored);
	REQUIRE(stored == std::array{1.0F, 2.0F, 0.0F, 0.0F});
	require_documented_register<ApiT>(ApiT::subtract(ApiT::set1(7.0F), ApiT::set1(2.0F)), std::array{5.0F, 5.0F, 5.0F, 5.0F});
	require_documented_register<ApiT>(ApiT::subtract_horizontal(ApiT::setr(3.0F, 1.0F, 7.0F, 2.0F), ApiT::setr(9.0F, 4.0F, 8.0F, 2.0F)),
									  std::array{2.0F, 5.0F, 5.0F, 6.0F});
	require_documented_register<U8>(U8::subtract_saturated(U8::set1(5), U8::set1(10)), std::array<std::uint8_t, 16>{});
	require_documented_register<SimdLib::Api<128, std::uint64_t>>(U8::sum_absolute_byte_differences(U8::set1(9), U8::set1(4)),
																  std::array<std::uint64_t, 2>{40, 40});
	REQUIRE(ApiT::to_array(ApiT::setr_partial(1.0F, 2.0F)) == std::array{1.0F, 2.0F, 0.0F, 0.0F});
	std::array<float, 3> transformed{};
	ApiT::transform(std::array{1.0F, 2.0F, 3.0F}, transformed, [](auto lanes) { return ApiT::add(lanes, ApiT::set1(10.0F)); });
	REQUIRE(transformed == std::array{11.0F, 12.0F, 13.0F});
	std::array<std::uint8_t, 1> packed{};
	ApiT::transform_pack<1>(std::span<const float, 4>{std::array{1.0F, -2.0F, 3.0F, -4.0F}}, std::span<std::uint8_t, 1>{packed},
							[](auto lanes) { return ApiT::movemask_slim(lanes); });
	REQUIRE(packed[0] == 0b0000'1010);
	require_documented_register<ApiT>(ApiT::unpack_hi(ApiT::setr(1.0F, 2.0F, 3.0F, 4.0F), ApiT::setr(5.0F, 6.0F, 7.0F, 8.0F)),
									  std::array{3.0F, 7.0F, 4.0F, 8.0F});
	require_documented_register<ApiT>(ApiT::unpack_lo(ApiT::setr(1.0F, 2.0F, 3.0F, 4.0F), ApiT::setr(5.0F, 6.0F, 7.0F, 8.0F)),
									  std::array{1.0F, 5.0F, 2.0F, 6.0F});
}
