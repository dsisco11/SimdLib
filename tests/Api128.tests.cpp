#include "TestSupport.h"

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
