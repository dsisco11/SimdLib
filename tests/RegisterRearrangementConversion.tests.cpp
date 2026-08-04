#include <SimdLib/Register.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#ifndef SIMDLIB_REGISTER_TEST_ENABLE_256
#define SIMDLIB_REGISTER_TEST_ENABLE_256 SIMDLIB_HAS_AVX2
#endif

namespace
{

/** @brief Builds distinctive logical lanes for one Register specialization. */
template <class register_t> [[nodiscard]] constexpr std::array<typename register_t::element_type, register_t::lane_count> make_distinct_lanes() noexcept
{
	using element_t = typename register_t::element_type;
	std::array<element_t, register_t::lane_count> result{};
	for (std::size_t lane = 0; lane < result.size(); ++lane)
	{
		if constexpr (std::is_floating_point_v<element_t>)
			result[lane] = static_cast<element_t>(lane * 3 + 1) / static_cast<element_t>(2);
		else if constexpr (std::is_signed_v<element_t>)
			result[lane] = static_cast<element_t>(lane % 2 == 0 ? static_cast<std::int64_t>(lane + 1) : -static_cast<std::int64_t>(lane + 1));
		else
			result[lane] = static_cast<element_t>(lane * 7 + 3);
	}
	return result;
}

/** @brief Reports whether a Register accepts its complete logical identity selector list. */
template <class register_t> [[nodiscard]] consteval bool has_complete_shuffle() noexcept
{
	return []<std::size_t... indices>(std::index_sequence<indices...>) consteval
	{ return SimdLib::IRegister::Shuffle<register_t, indices...>; }(std::make_index_sequence<register_t::lane_count>{});
}

/** @brief Verifies low/high unpack lane order independently of the Api implementation. */
template <class element_t, std::size_t bits> void require_unpack_contract()
{
	using register_t = SimdLib::Register<element_t, bits>;
	constexpr std::size_t lanes_per_group = 128 / (sizeof(element_t) * 8);
	constexpr std::size_t lanes_per_half = lanes_per_group / 2;
	const auto left = make_distinct_lanes<register_t>();
	auto right = make_distinct_lanes<register_t>();
	for (std::size_t lane = 0; lane < right.size(); ++lane)
		right[lane] = static_cast<element_t>(right[lane] + static_cast<element_t>(37));

	std::array<element_t, register_t::lane_count> expected_low{};
	std::array<element_t, register_t::lane_count> expected_high{};
	for (std::size_t group = 0; group < register_t::lane_count; group += lanes_per_group)
	{
		for (std::size_t lane = 0; lane < lanes_per_half; ++lane)
		{
			expected_low[group + lane * 2] = left[group + lane];
			expected_low[group + lane * 2 + 1] = right[group + lane];
			expected_high[group + lane * 2] = left[group + lanes_per_half + lane];
			expected_high[group + lane * 2 + 1] = right[group + lanes_per_half + lane];
		}
	}

	const auto lhs = register_t::from_array(left);
	const auto rhs = register_t::from_array(right);
	REQUIRE(lhs.unpack_low(rhs).to_array() == expected_low);
	REQUIRE(lhs.unpack_high(rhs).to_array() == expected_high);
}

/** @brief Verifies intrinsic-compatible immediate blend selection for one Register type. */
template <class element_t, std::size_t bits, int immediate> void require_blend_contract()
{
	using register_t = SimdLib::Register<element_t, bits>;
	const auto left = make_distinct_lanes<register_t>();
	auto right = make_distinct_lanes<register_t>();
	for (std::size_t lane = 0; lane < right.size(); ++lane)
		right[lane] = static_cast<element_t>(right[lane] + static_cast<element_t>(53));
	std::array<element_t, register_t::lane_count> expected{};
	for (std::size_t lane = 0; lane < expected.size(); ++lane)
		expected[lane] = (static_cast<unsigned int>(immediate) & (1u << (lane % 8))) != 0 ? right[lane] : left[lane];

	const auto actual = register_t::from_array(left).template blend<immediate>(register_t::from_array(right));
	REQUIRE(actual.to_array() == expected);
}

/** @brief Verifies the explicitly consumed source prefix for one widening shape. */
template <class source_t, class target_t, std::size_t target_bits> void require_widen_low_contract()
{
	using source_register = SimdLib::Register<source_t, 128>;
	using target_register = SimdLib::Register<target_t, target_bits>;
	auto source = make_distinct_lanes<source_register>();
	source.front() = std::numeric_limits<source_t>::min();
	source.back() = std::numeric_limits<source_t>::max();
	std::array<target_t, target_register::lane_count> expected{};
	for (std::size_t lane = 0; lane < expected.size(); ++lane)
		expected[lane] = static_cast<target_t>(source[lane]);

	const auto widened = source_register::from_array(source).template widen_low<target_t, target_bits>();
	REQUIRE(widened.to_array() == expected);
}

/** @brief Verifies all supported widening destinations for one signedness family. */
template <class i8_t, class i16_t, class i32_t, class i64_t> void require_widening_family()
{
	require_widen_low_contract<i8_t, i16_t, 128>();
#if SIMDLIB_REGISTER_TEST_ENABLE_256
	require_widen_low_contract<i8_t, i16_t, 256>();
#endif
	require_widen_low_contract<i8_t, i32_t, 128>();
#if SIMDLIB_REGISTER_TEST_ENABLE_256
	require_widen_low_contract<i8_t, i32_t, 256>();
#endif
	require_widen_low_contract<i8_t, i64_t, 128>();
#if SIMDLIB_REGISTER_TEST_ENABLE_256
	require_widen_low_contract<i8_t, i64_t, 256>();
#endif
	require_widen_low_contract<i16_t, i32_t, 128>();
#if SIMDLIB_REGISTER_TEST_ENABLE_256
	require_widen_low_contract<i16_t, i32_t, 256>();
#endif
	require_widen_low_contract<i16_t, i64_t, 128>();
#if SIMDLIB_REGISTER_TEST_ENABLE_256
	require_widen_low_contract<i16_t, i64_t, 256>();
#endif
	require_widen_low_contract<i32_t, i64_t, 128>();
#if SIMDLIB_REGISTER_TEST_ENABLE_256
	require_widen_low_contract<i32_t, i64_t, 256>();
#endif
}

using I8x128 = SimdLib::Register<std::int8_t, 128>;
using U8x128 = SimdLib::Register<std::uint8_t, 128>;
using I16x128 = SimdLib::Register<std::int16_t, 128>;
using U16x128 = SimdLib::Register<std::uint16_t, 128>;
using I32x128 = SimdLib::Register<std::int32_t, 128>;
using U32x128 = SimdLib::Register<std::uint32_t, 128>;
using I64x128 = SimdLib::Register<std::int64_t, 128>;
using U64x128 = SimdLib::Register<std::uint64_t, 128>;
using F32x128 = SimdLib::Register<float, 128>;
using F64x128 = SimdLib::Register<double, 128>;
#if SIMDLIB_REGISTER_TEST_ENABLE_256
using U8x256 = SimdLib::Register<std::uint8_t, 256>;
#endif

static_assert(!SimdLib::IRegister::LowerHalf<I32x128>);
#if SIMDLIB_REGISTER_TEST_ENABLE_256
static_assert(SimdLib::IRegister::LowerHalf<SimdLib::Register<std::int32_t, 256>>);
#endif
static_assert(SimdLib::IRegister::UnpackLow<I8x128> && SimdLib::IRegister::UnpackHigh<F64x128>);
static_assert(has_complete_shuffle<I8x128>());
#if SIMDLIB_REGISTER_TEST_ENABLE_256
static_assert(has_complete_shuffle<U8x256>());
#endif
static_assert(has_complete_shuffle<I16x128>());
static_assert(SimdLib::IRegister::ShuffleLow<I16x128, 0> && SimdLib::IRegister::ShuffleHigh<U16x128, 255>);
static_assert(!SimdLib::IRegister::ShuffleLow<I32x128, 0>);
static_assert(SimdLib::IRegister::Blend<I16x128, 0> && SimdLib::IRegister::Blend<U32x128, 255> && SimdLib::IRegister::Blend<F32x128, 0> &&
			  SimdLib::IRegister::Blend<F64x128, 255>);
static_assert(!SimdLib::IRegister::Blend<I8x128, 0> && !SimdLib::IRegister::Blend<I64x128, 0>);
static_assert(SimdLib::IRegister::BitCast<F32x128, std::uint8_t> && SimdLib::IRegister::BitCast<U64x128, double>);
static_assert(SimdLib::IRegister::Convert<I32x128, float> && SimdLib::IRegister::Convert<U32x128, float> && SimdLib::IRegister::Convert<F32x128, std::int32_t>);
static_assert(!SimdLib::IRegister::Convert<F32x128, std::uint32_t> && !SimdLib::IRegister::Convert<I64x128, double>);
static_assert(SimdLib::IRegister::WidenLow<I8x128, std::int16_t, 128>);
static_assert(!SimdLib::IRegister::WidenLow<I8x128, std::uint16_t, 128>);
#if SIMDLIB_REGISTER_TEST_ENABLE_256
static_assert(SimdLib::IRegister::WidenLow<I8x128, std::int64_t, 256> && SimdLib::IRegister::WidenLow<U32x128, std::uint64_t, 256>);
static_assert(!SimdLib::IRegister::WidenLow<SimdLib::Register<std::int8_t, 256>, std::int16_t, 256>);
#endif

#if SIMDLIB_REGISTER_TEST_ENABLE_256
TEST_CASE("Register lower-half preserves the complete low 128-bit lane sequence", "[simdlib][register][rearrangement]")
{
	using register_t = SimdLib::Register<std::uint32_t, 256>;
	const auto lanes = make_distinct_lanes<register_t>();
	const auto actual = register_t::from_array(lanes).lower_half().to_array();
	REQUIRE(actual == std::array<std::uint32_t, 4>{lanes[0], lanes[1], lanes[2], lanes[3]});
}
#endif

TEST_CASE("Register unpack methods preserve intrinsic 128-bit grouping and lane order", "[simdlib][register][rearrangement]")
{
	require_unpack_contract<std::int8_t, 128>();
#if SIMDLIB_REGISTER_TEST_ENABLE_256
	require_unpack_contract<std::uint16_t, 256>();
	require_unpack_contract<std::int32_t, 256>();
#endif
	require_unpack_contract<std::uint64_t, 128>();
#if SIMDLIB_REGISTER_TEST_ENABLE_256
	require_unpack_contract<float, 256>();
	require_unpack_contract<double, 256>();
#endif
}

TEST_CASE("Register logical byte shuffle uses complete lane-local selector lists", "[simdlib][register][rearrangement]")
{
	using register128_t = SimdLib::Register<std::uint8_t, 128>;
	const auto source128 = make_distinct_lanes<register128_t>();
	const auto reversed128 = register128_t::from_array(source128).template shuffle<15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0>().to_array();
	for (std::size_t lane = 0; lane < 16; ++lane)
		REQUIRE(reversed128[lane] == source128[15 - lane]);
#if SIMDLIB_REGISTER_TEST_ENABLE_256
	using register256_t = SimdLib::Register<std::uint8_t, 256>;
	const auto source256 = make_distinct_lanes<register256_t>();
	const auto reversed256 =
		register256_t::from_array(source256)
			.template shuffle<15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16>()
			.to_array();
	for (std::size_t lane = 0; lane < 16; ++lane)
	{
		REQUIRE(reversed256[lane] == source256[15 - lane]);
		REQUIRE(reversed256[16 + lane] == source256[31 - lane]);
	}
#endif
}

#if SIMDLIB_REGISTER_TEST_ENABLE_256
TEST_CASE("Register 16-bit half shuffles preserve the unselected half in every 128-bit group", "[simdlib][register][rearrangement]")
{
	using register_t = SimdLib::Register<std::int16_t, 256>;
	const auto source = make_distinct_lanes<register_t>();
	const auto low = register_t::from_array(source).template shuffle_low<0x1B>().to_array();
	const auto high = register_t::from_array(source).template shuffle_high<0x1B>().to_array();
	for (std::size_t group = 0; group < source.size(); group += 8)
	{
		for (std::size_t lane = 0; lane < 4; ++lane)
		{
			REQUIRE(low[group + lane] == source[group + 3 - lane]);
			REQUIRE(low[group + 4 + lane] == source[group + 4 + lane]);
			REQUIRE(high[group + lane] == source[group + lane]);
			REQUIRE(high[group + 4 + lane] == source[group + 7 - lane]);
		}
	}
}
#endif

TEST_CASE("Register immediate blend retains operation-specific mask-bit behavior", "[simdlib][register][rearrangement]")
{
	require_blend_contract<std::int16_t, 128, 0xA5>();
#if SIMDLIB_REGISTER_TEST_ENABLE_256
	require_blend_contract<std::uint16_t, 256, 0xA5>();
#endif
	require_blend_contract<std::int32_t, 128, 0xF5>();
#if SIMDLIB_REGISTER_TEST_ENABLE_256
	require_blend_contract<std::uint32_t, 256, 0xA5>();
#endif
	require_blend_contract<float, 128, 0xF5>();
#if SIMDLIB_REGISTER_TEST_ENABLE_256
	require_blend_contract<float, 256, 0xA5>();
#endif
	require_blend_contract<double, 128, 0xFD>();
#if SIMDLIB_REGISTER_TEST_ENABLE_256
	require_blend_contract<double, 256, 0xF5>();
#endif
}

#if SIMDLIB_REGISTER_TEST_ENABLE_256
TEST_CASE("Register bit-cast preserves floating edge-value object representations", "[simdlib][register][conversion]")
{
	using bits_t = SimdLib::Register<std::uint32_t, 256>;
	constexpr std::array<std::uint32_t, 8> patterns{0x00000000u, 0x80000000u, 0x3F800000u, 0xBF800000u, 0x7F800000u, 0xFF800000u, 0x7FC12345u, 0xFFC54321u};
	const auto floating = bits_t::from_array(patterns).template bit_cast<float>();
	REQUIRE(floating.template bit_cast<std::uint32_t>().to_array() == patterns);
	REQUIRE(std::bit_cast<std::uint32_t>(floating.template lane<6>()) == patterns[6]);
}
#endif

TEST_CASE("Register numeric conversion is distinct from bit reinterpretation", "[simdlib][register][conversion]")
{
	using signed_t = SimdLib::Register<std::int32_t, 128>;
	using unsigned_t = SimdLib::Register<std::uint32_t, 128>;
	using float_register = SimdLib::Register<float, 128>;
	const auto signed_values =
		signed_t::from_lanes(std::numeric_limits<std::int32_t>::min(), -16'777'217, 16'777'217, std::numeric_limits<std::int32_t>::max());
	const auto unsigned_values = unsigned_t::from_lanes(0u, 16'777'217u, 0x80000000u, 0xFFFFFFFFu);
	const auto converted_signed = signed_values.template convert<float>().to_array();
	const auto converted_unsigned = unsigned_values.template convert<float>().to_array();
	for (std::size_t lane = 0; lane < 4; ++lane)
	{
		REQUIRE(converted_signed[lane] == static_cast<float>(signed_values.to_array()[lane]));
		REQUIRE(converted_unsigned[lane] == static_cast<float>(unsigned_values.to_array()[lane]));
	}

	const auto rounded = float_register::from_lanes(-2.5F, -1.5F, 2.5F, 3.5F).template convert<std::int32_t>().to_array();
	REQUIRE(rounded == std::array<std::int32_t, 4>{-2, -2, 2, 4});
	REQUIRE(signed_values.template bit_cast<float>().to_array() != converted_signed);
}

TEST_CASE("Register widening consumes exactly the documented low source lanes", "[simdlib][register][conversion]")
{
	require_widening_family<std::int8_t, std::int16_t, std::int32_t, std::int64_t>();
	require_widening_family<std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t>();
}

} // namespace
