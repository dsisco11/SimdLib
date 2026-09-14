#include <SimdLib/IRegister.h>
#include <SimdLib/PartialRegister.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>

#ifndef SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
#define SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256 SIMDLIB_HAS_AVX2
#endif

namespace
{

/** @brief Reports whether one scalar has the all-bits-zero representation. */
template <class element_t> [[nodiscard]] bool has_zero_bits(element_t value) noexcept
{
	const auto bytes = std::bit_cast<std::array<std::byte, sizeof(element_t)>>(value);
	for (const auto byte : bytes)
		if (byte != std::byte{})
			return false;
	return true;
}

/** @brief Requires every native lane beyond a result's logical prefix to remain bitwise zero. */
template <class value_t> void require_zero_suffix(value_t value)
{
	if constexpr (value_t::lane_count < value_t::api_type::element_count)
	{
		const auto lanes = value_t::api_type::to_array(value.to_native());
		for (std::size_t lane = value_t::lane_count; lane < value_t::api_type::element_count; ++lane)
			REQUIRE(has_zero_bits(lanes[lane]));
	}
}

/** @brief Builds distinctive active lanes for one PartialRegister specialization. */
template <class value_t> [[nodiscard]] constexpr std::array<typename value_t::element_type, value_t::lane_count> make_active_lanes() noexcept
{
	using element_t = typename value_t::element_type;
	std::array<element_t, value_t::lane_count> result{};
	for (std::size_t lane = 0; lane < result.size(); ++lane)
		result[lane] = static_cast<element_t>(lane * 5 + 1);
	return result;
}

/** @brief Reports whether one partial type accepts its exact active identity selector list. */
template <class value_t> [[nodiscard]] consteval bool has_active_shuffle() noexcept
{
	return []<std::size_t... indices>(std::index_sequence<indices...>) consteval
	{ return SimdLib::IRegister::Shuffle<value_t, indices...>; }(std::make_index_sequence<value_t::lane_count>{});
}

/** @brief Reports whether one partial type accepts its exact active-byte identity selector list. */
template <class value_t> [[nodiscard]] consteval bool has_active_byte_shuffle() noexcept
{
	return []<std::size_t... indices>(std::index_sequence<indices...>) consteval
	{ return SimdLib::IRegister::ShuffleBytes<value_t, indices...>; }(std::make_index_sequence<value_t::active_byte_count>{});
}

/** @brief Verifies rearrangement surface parity for one partial geometry. */
template <class element_t, std::size_t bits, std::size_t active_count> [[nodiscard]] consteval bool has_rearrangement_surface_parity() noexcept
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_count>;
	using api_t = typename value_t::api_type;
	static_assert(SimdLib::IRegister::LowerHalf<value_t> == (bits == 256 && SimdLib::IApi::LowerHalf<api_t>));
	static_assert(SimdLib::IRegister::UnpackLow<value_t> == (SimdLib::IApi::UnpackLow<api_t> && SimdLib::IApi::BitwiseAnd<api_t>));
	static_assert(SimdLib::IRegister::UnpackHigh<value_t> == SimdLib::IApi::UnpackHigh<api_t>);
	static_assert(has_active_shuffle<value_t>());
	static_assert(has_active_byte_shuffle<value_t>());
	static_assert(SimdLib::IRegister::ShuffleLow<value_t, 0x1b> == (SimdLib::IApi::ShuffleLow<api_t, 0x1b> && SimdLib::IApi::BitwiseAnd<api_t>));
	static_assert(SimdLib::IRegister::ShuffleHigh<value_t, 0x1b> == (SimdLib::IApi::ShuffleHigh<api_t, 0x1b> && SimdLib::IApi::BitwiseAnd<api_t>));
	static_assert(SimdLib::IRegister::Blend<value_t, 0xa5> == SimdLib::IApi::Blend<api_t, 0xa5>);
	return true;
}

#define SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(element_type, width, count) static_assert(has_rearrangement_surface_parity<element_type, width, count>())
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::int8_t, 128, 13);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::uint8_t, 128, 13);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::int16_t, 128, 5);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::uint16_t, 128, 5);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::int32_t, 128, 3);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::uint32_t, 128, 3);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::int64_t, 128, 1);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::uint64_t, 128, 1);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(float, 128, 3);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(double, 128, 1);
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::int8_t, 256, 19);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::uint8_t, 256, 19);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::int16_t, 256, 11);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::uint16_t, 256, 11);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::int32_t, 256, 5);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::uint32_t, 256, 5);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::int64_t, 256, 3);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(std::uint64_t, 256, 3);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(float, 256, 5);
SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE(double, 256, 3);
#endif
#undef SIMDLIB_PARTIAL_REARRANGEMENT_SURFACE

using I8x13 = SimdLib::PartialRegister<std::int8_t, 128, 13>;
using U8x8 = SimdLib::PartialRegister<std::uint8_t, 128, 8>;
using I16x5 = SimdLib::PartialRegister<std::int16_t, 128, 5>;
using I32x1 = SimdLib::PartialRegister<std::int32_t, 128, 1>;
using I32x3 = SimdLib::PartialRegister<std::int32_t, 128, 3>;
using U32x3 = SimdLib::PartialRegister<std::uint32_t, 128, 3>;
using F32x3 = SimdLib::PartialRegister<float, 128, 3>;

static_assert(std::same_as<SimdLib::partial_bit_cast_result_t<std::uint8_t, std::uint32_t, 128, 8>, SimdLib::PartialRegister<std::uint32_t, 128, 2>>);
static_assert(SimdLib::IRegister::BitCast<U8x8, std::uint16_t> && SimdLib::IRegister::BitCast<U8x8, std::uint32_t> &&
			  SimdLib::IRegister::BitCast<U8x8, float> && SimdLib::IRegister::BitCast<U8x8, double>);
static_assert(!SimdLib::IRegister::BitCast<SimdLib::PartialRegister<std::uint8_t, 128, 13>, std::uint32_t>);
static_assert(SimdLib::IRegister::Convert<I32x3, float> && SimdLib::IRegister::Convert<U32x3, float> && SimdLib::IRegister::Convert<F32x3, std::int32_t>);
static_assert(!SimdLib::IRegister::Convert<I16x5, float> && !SimdLib::IRegister::Convert<F32x3, std::uint32_t>);
static_assert(std::same_as<SimdLib::partial_widen_low_result_t<std::int8_t, std::int16_t, 128, 128, 13>, SimdLib::Register<std::int16_t, 128>>);
static_assert(SimdLib::IRegister::WidenLow<I8x13, std::int16_t, 128>);
static_assert(SimdLib::IRegister::WidenLow<I32x1, std::int64_t, 128>);
static_assert(!SimdLib::IRegister::WidenLow<I32x3, std::int16_t, 128>);
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
static_assert(std::same_as<SimdLib::partial_lower_half_result_t<std::int32_t, 5>, SimdLib::Register<std::int32_t, 128>>);
static_assert(SimdLib::IRegister::WidenLow<I8x13, std::int16_t, 256>);
static_assert(!SimdLib::IRegister::WidenLow<I32x1, std::int64_t, 256>);
#endif

/** @brief Computes an integer power for compile-time exhaustive selector enumeration. */
[[nodiscard]] consteval std::size_t integer_power(std::size_t base, std::size_t exponent) noexcept
{
	std::size_t result = 1;
	for (std::size_t index = 0; index < exponent; ++index)
		result *= base;
	return result;
}

/** @brief Selects one base-N digit from an exhaustive selector-case index. */
template <std::size_t code, std::size_t base, std::size_t position> [[nodiscard]] consteval std::size_t selector_digit() noexcept
{
	return (code / integer_power(base, position)) % base;
}

/** @brief Verifies one logical-lane shuffle case against its independently decoded selector digits. */
template <class value_t, std::size_t code, std::size_t... positions>
void require_logical_shuffle_case(value_t value, const std::array<typename value_t::element_type, value_t::lane_count> &source,
								  std::index_sequence<positions...>)
{
	constexpr std::array<std::size_t, value_t::lane_count> selectors{selector_digit<code, value_t::lane_count, positions>()...};
	const auto actual = value.template shuffle<selector_digit<code, value_t::lane_count, positions>()...>().to_array();
	for (std::size_t lane = 0; lane < actual.size(); ++lane)
		REQUIRE(actual[lane] == source[selectors[lane]]);
	require_zero_suffix(value.template shuffle<selector_digit<code, value_t::lane_count, positions>()...>());
}

/** @brief Instantiates every logical-lane selector sequence for one small PartialRegister geometry. */
template <class value_t, std::size_t... codes> void require_all_logical_shuffle_cases(std::index_sequence<codes...>)
{
	const auto source = make_active_lanes<value_t>();
	const auto value = value_t::from_array(source);
	(require_logical_shuffle_case<value_t, codes>(value, source, std::make_index_sequence<value_t::lane_count>{}), ...);
}

/** @brief Verifies one byte-shuffle case against its independently decoded selector digits. */
template <class value_t, std::size_t code, std::size_t... positions>
void require_byte_shuffle_case(value_t value, const std::array<std::uint8_t, value_t::active_byte_count> &source, std::index_sequence<positions...>)
{
	constexpr std::array<std::size_t, value_t::active_byte_count> selectors{selector_digit<code, value_t::active_byte_count, positions>()...};
	const auto shuffled = value.template shuffle_bytes<selector_digit<code, value_t::active_byte_count, positions>()...>();
	std::array<std::byte, value_t::active_byte_count> actual{};
	shuffled.store_bytes(actual);
	for (std::size_t byte = 0; byte < actual.size(); ++byte)
		REQUIRE(std::to_integer<std::uint8_t>(actual[byte]) == source[selectors[byte]]);
	require_zero_suffix(shuffled);
}

/** @brief Instantiates every byte selector sequence for one small PartialRegister geometry. */
template <class value_t, std::size_t... codes> void require_all_byte_shuffle_cases(std::index_sequence<codes...>)
{
	static_assert(std::same_as<typename value_t::element_type, std::uint8_t>);
	const auto source = make_active_lanes<value_t>();
	const auto value = value_t::from_array(source);
	(require_byte_shuffle_case<value_t, codes>(value, source, std::make_index_sequence<value_t::active_byte_count>{}), ...);
}

/** @brief Verifies projected unpack semantics for one odd or intrinsic-subgroup-sized logical prefix. */
template <class element_t, std::size_t bits, std::size_t active_count> void require_unpack_contract()
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_count>;
	constexpr std::size_t group_lane_count = 128 / (sizeof(element_t) * 8);
	constexpr std::size_t half_lane_count = group_lane_count / 2;
	const auto lhs_values = make_active_lanes<value_t>();
	auto rhs_values = make_active_lanes<value_t>();
	for (auto &lane : rhs_values)
		lane = static_cast<element_t>(lane + static_cast<element_t>(37));
	std::array<element_t, value_t::native_lane_count> lhs_native{};
	std::array<element_t, value_t::native_lane_count> rhs_native{};
	for (std::size_t lane = 0; lane < active_count; ++lane)
	{
		lhs_native[lane] = lhs_values[lane];
		rhs_native[lane] = rhs_values[lane];
	}
	std::array<element_t, value_t::native_lane_count> expected_low{};
	std::array<element_t, value_t::native_lane_count> expected_high{};
	for (std::size_t group = 0; group < value_t::native_lane_count; group += group_lane_count)
		for (std::size_t lane = 0; lane < half_lane_count; ++lane)
		{
			expected_low[group + lane * 2] = lhs_native[group + lane];
			expected_low[group + lane * 2 + 1] = rhs_native[group + lane];
			expected_high[group + lane * 2] = lhs_native[group + half_lane_count + lane];
			expected_high[group + lane * 2 + 1] = rhs_native[group + half_lane_count + lane];
		}
	const auto lhs = value_t::from_array(lhs_values);
	const auto rhs = value_t::from_array(rhs_values);
	const auto low = lhs.unpack_low(rhs);
	const auto high = lhs.unpack_high(rhs);
	for (std::size_t lane = 0; lane < active_count; ++lane)
	{
		REQUIRE(low.to_array()[lane] == expected_low[lane]);
		REQUIRE(high.to_array()[lane] == expected_high[lane]);
	}
	require_zero_suffix(low);
	require_zero_suffix(high);
}

/** @brief Verifies projected low/high four-lane shuffle semantics for one 16-bit logical prefix. */
template <std::size_t bits, std::size_t active_count> void require_half_shuffle_contract()
{
	using value_t = SimdLib::PartialRegister<std::int16_t, bits, active_count>;
	constexpr int immediate = 0x1b;
	constexpr std::size_t group_lane_count = 8;
	const auto source = make_active_lanes<value_t>();
	std::array<std::int16_t, value_t::native_lane_count> native_source{};
	for (std::size_t lane = 0; lane < source.size(); ++lane)
		native_source[lane] = source[lane];
	auto expected_low = native_source;
	auto expected_high = native_source;
	for (std::size_t group = 0; group < value_t::native_lane_count; group += group_lane_count)
		for (std::size_t lane = 0; lane < 4; ++lane)
		{
			expected_low[group + lane] = native_source[group + 3 - lane];
			expected_high[group + 4 + lane] = native_source[group + 7 - lane];
		}
	const auto value = value_t::from_array(source);
	const auto low = value.template shuffle_low<immediate>();
	const auto high = value.template shuffle_high<immediate>();
	for (std::size_t lane = 0; lane < active_count; ++lane)
	{
		REQUIRE(low.to_array()[lane] == expected_low[lane]);
		REQUIRE(high.to_array()[lane] == expected_high[lane]);
	}
	require_zero_suffix(low);
	require_zero_suffix(high);
}

/** @brief Verifies immediate blend selection and suffix closure for one supported partial geometry. */
template <class element_t, std::size_t bits, std::size_t active_count> void require_blend_contract()
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_count>;
	constexpr int immediate = 0xa5;
	const auto lhs_values = make_active_lanes<value_t>();
	auto rhs_values = make_active_lanes<value_t>();
	for (auto &lane : rhs_values)
		lane = static_cast<element_t>(lane + static_cast<element_t>(53));
	const auto actual = value_t::from_array(lhs_values).template blend<immediate>(value_t::from_array(rhs_values));
	for (std::size_t lane = 0; lane < active_count; ++lane)
		REQUIRE(actual.to_array()[lane] == (((immediate >> (lane % 8)) & 1) != 0 ? rhs_values[lane] : lhs_values[lane]));
	require_zero_suffix(actual);
}

/** @brief Verifies one bit-cast matrix cell preserves exactly the active source bytes. */
template <std::size_t bits, std::size_t active_count, class target_t> void require_bit_cast_contract()
{
	using source_t = SimdLib::PartialRegister<std::uint8_t, bits, active_count>;
	const auto source_values = make_active_lanes<source_t>();
	const auto source = source_t::from_array(source_values);
	const auto result = source.template bit_cast<target_t>();
	using result_t = std::remove_cvref_t<decltype(result)>;
	static_assert(result_t::lane_count == active_count / sizeof(target_t));
	std::array<std::byte, active_count> actual{};
	result.store_bytes(actual);
	for (std::size_t byte = 0; byte < active_count; ++byte)
		REQUIRE(std::to_integer<std::uint8_t>(actual[byte]) == source_values[byte]);
	require_zero_suffix(result);
}

/** @brief Verifies one supported numeric conversion cell over active lanes only. */
template <class source_t, class target_t, std::size_t bits, std::size_t active_count> void require_conversion_contract()
{
	using value_t = SimdLib::PartialRegister<source_t, bits, active_count>;
	std::array<source_t, active_count> source{};
	for (std::size_t lane = 0; lane < active_count; ++lane)
	{
		if constexpr (std::is_unsigned_v<source_t>)
			source[lane] = static_cast<source_t>(lane + 1);
		else
			source[lane] = static_cast<source_t>(static_cast<int>(lane) - 2);
	}
	const auto result = value_t::from_array(source).template convert<target_t>();
	using result_t = std::remove_cvref_t<decltype(result)>;
	static_assert(result_t::lane_count == active_count);
	for (std::size_t lane = 0; lane < active_count; ++lane)
		REQUIRE(result.to_array()[lane] == static_cast<target_t>(source[lane]));
	require_zero_suffix(result);
}

/** @brief Verifies one widening cell consumes and exposes exactly its documented active source prefix. */
template <class source_t, class target_t, std::size_t target_bits, std::size_t active_count> void require_widen_contract()
{
	using value_t = SimdLib::PartialRegister<source_t, 128, active_count>;
	std::array<source_t, active_count> source{};
	for (std::size_t lane = 0; lane < active_count; ++lane)
		source[lane] = static_cast<source_t>(lane + 1);
	const auto result = value_t::from_array(source).template widen_low<target_t, target_bits>();
	using result_t = std::remove_cvref_t<decltype(result)>;
	constexpr std::size_t expected_count = active_count < result_t::api_type::element_count ? active_count : result_t::api_type::element_count;
	static_assert(result_t::lane_count == expected_count);
	for (std::size_t lane = 0; lane < expected_count; ++lane)
		REQUIRE(result.to_array()[lane] == static_cast<target_t>(source[lane]));
	require_zero_suffix(result);
}

TEST_CASE("PartialRegister exhaustively shuffles every selector sequence for three logical lanes", "[PartialRegister][Rearrangement]")
{
	using value_t = SimdLib::PartialRegister<std::uint32_t, 128, 3>;
	require_all_logical_shuffle_cases<value_t>(std::make_index_sequence<integer_power(3, 3)>{});
}

TEST_CASE("PartialRegister exhaustively shuffles every selector sequence for three active bytes", "[PartialRegister][Rearrangement]")
{
	using value_t = SimdLib::PartialRegister<std::uint8_t, 128, 3>;
	require_all_byte_shuffle_cases<value_t>(std::make_index_sequence<integer_power(3, 3)>{});
}

#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
TEST_CASE("PartialRegister logical shuffles cross the 128-bit boundary without exposing padding", "[PartialRegister][Rearrangement]")
{
	using lane_value_t = SimdLib::PartialRegister<std::uint32_t, 256, 5>;
	const auto lane_source = make_active_lanes<lane_value_t>();
	const auto lanes = lane_value_t::from_array(lane_source).template shuffle<4, 3, 2, 1, 0>();
	REQUIRE(lanes.to_array() == std::array<std::uint32_t, 5>{lane_source[4], lane_source[3], lane_source[2], lane_source[1], lane_source[0]});
	require_zero_suffix(lanes);
	using byte_value_t = SimdLib::PartialRegister<std::uint8_t, 256, 19>;
	const auto byte_source = make_active_lanes<byte_value_t>();
	const auto bytes = byte_value_t::from_array(byte_source).template shuffle_bytes<18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0>();
	for (std::size_t byte = 0; byte < byte_source.size(); ++byte)
		REQUIRE(bytes.to_array()[byte] == byte_source[18 - byte]);
	require_zero_suffix(bytes);
}

TEST_CASE("PartialRegister lower-half returns the complete meaningful low half", "[PartialRegister][Rearrangement]")
{
	using value_t = SimdLib::PartialRegister<std::uint32_t, 256, 5>;
	const auto source = make_active_lanes<value_t>();
	const auto result = value_t::from_array(source).lower_half();
	static_assert(std::same_as<std::remove_cvref_t<decltype(result)>, SimdLib::Register<std::uint32_t, 128>>);
	REQUIRE(result.to_array() == std::array<std::uint32_t, 4>{source[0], source[1], source[2], source[3]});
}
#endif

TEST_CASE("PartialRegister unpack operations project odd and undersized intrinsic groups", "[PartialRegister][Rearrangement]")
{
	require_unpack_contract<std::uint32_t, 128, 3>();
	require_unpack_contract<std::uint64_t, 128, 1>();
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
	require_unpack_contract<std::uint32_t, 256, 5>();
	require_unpack_contract<std::uint16_t, 256, 9>();
#endif
}

TEST_CASE("PartialRegister half-local shuffles project unavailable source lanes as zero", "[PartialRegister][Rearrangement]")
{
	require_half_shuffle_contract<128, 3>();
	require_half_shuffle_contract<128, 5>();
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
	require_half_shuffle_contract<256, 9>();
#endif
}

TEST_CASE("PartialRegister blends retain intrinsic mask semantics over active lanes", "[PartialRegister][Rearrangement]")
{
	require_blend_contract<std::int16_t, 128, 5>();
	require_blend_contract<std::int32_t, 128, 3>();
	require_blend_contract<float, 128, 3>();
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
	require_blend_contract<std::uint16_t, 256, 11>();
	require_blend_contract<double, 256, 3>();
#endif
}

TEST_CASE("PartialRegister bit casts preserve the exact active bit extent", "[PartialRegister][Conversion]")
{
	require_bit_cast_contract<128, 8, std::uint16_t>();
	require_bit_cast_contract<128, 8, std::uint32_t>();
	require_bit_cast_contract<128, 8, float>();
	require_bit_cast_contract<128, 8, double>();
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
	require_bit_cast_contract<256, 24, std::uint16_t>();
	require_bit_cast_contract<256, 24, std::uint32_t>();
	require_bit_cast_contract<256, 24, float>();
	require_bit_cast_contract<256, 24, double>();
#endif
}

TEST_CASE("PartialRegister numeric conversion retains exactly one output per active input", "[PartialRegister][Conversion]")
{
	require_conversion_contract<std::int32_t, float, 128, 3>();
	require_conversion_contract<std::uint32_t, float, 128, 3>();
	require_conversion_contract<float, std::int32_t, 128, 3>();
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
	require_conversion_contract<std::int32_t, float, 256, 5>();
	require_conversion_contract<std::uint32_t, float, 256, 5>();
	require_conversion_contract<float, std::int32_t, 256, 5>();
#endif
}

TEST_CASE("PartialRegister widening exposes exactly the consumed source prefix", "[PartialRegister][Conversion]")
{
	require_widen_contract<std::int8_t, std::int16_t, 128, 13>();
	require_widen_contract<std::uint16_t, std::uint32_t, 128, 5>();
	require_widen_contract<std::int32_t, std::int64_t, 128, 1>();
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
	require_widen_contract<std::int8_t, std::int16_t, 256, 13>();
	require_widen_contract<std::uint16_t, std::uint32_t, 256, 5>();
	require_widen_contract<std::int32_t, std::int64_t, 256, 3>();
#endif
}

} // namespace
