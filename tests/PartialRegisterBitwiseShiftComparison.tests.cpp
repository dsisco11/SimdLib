#include <SimdLib/IRegister.h>
#include <SimdLib/PartialRegister.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#ifndef SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
#define SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256 SIMDLIB_HAS_AVX2
#endif

namespace
{

/** @brief Verifies bitwise, shift, and comparison surface parity for one partial geometry. */
template <class element_t, std::size_t bits, std::size_t active_count> [[nodiscard]] consteval bool has_operation_surface_parity() noexcept
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_count>;
	using api_t = typename value_t::api_type;
	static_assert(SimdLib::IRegister::BitwiseAnd<value_t> == SimdLib::IApi::BitwiseAnd<api_t>);
	static_assert(SimdLib::IRegister::BitwiseOr<value_t> == SimdLib::IApi::BitwiseOr<api_t>);
	static_assert(SimdLib::IRegister::BitwiseXor<value_t> == SimdLib::IApi::BitwiseXor<api_t>);
	static_assert(SimdLib::IRegister::BitwiseNot<value_t> == (SimdLib::IApi::BitwiseNot<api_t> && SimdLib::IApi::BitwiseAnd<api_t>));
	static_assert(SimdLib::IRegister::BitwiseAndNot<value_t> == SimdLib::IApi::BitwiseAndNot<api_t>);
	static_assert(SimdLib::IRegister::Movemask<value_t> == SimdLib::IApi::Movemask<api_t>);
	static_assert(SimdLib::IRegister::LaneSignBits<value_t> == SimdLib::IApi::MovemaskSlim<api_t>);
	static_assert(SimdLib::IRegister::ShiftLeft<value_t> == SimdLib::IApi::ShiftLeft<api_t>);
	static_assert(SimdLib::IRegister::LogicalShiftRight<value_t> == SimdLib::IApi::ShiftRight<api_t>);
	static_assert(SimdLib::IRegister::ShiftRight<value_t> == ((std::is_signed_v<element_t> && SimdLib::IApi::ArithmeticShiftRight<api_t>) ||
															  (std::is_unsigned_v<element_t> && SimdLib::IApi::ShiftRight<api_t>)));
	static_assert(SimdLib::IRegister::ShiftBytesLeftSlow<value_t> == (bits == 128 && SimdLib::IApi::ShiftBytesSlow<api_t> && SimdLib::IApi::BitwiseAnd<api_t>));
	static_assert(SimdLib::IRegister::ShiftBytesRightSlow<value_t> == (bits == 128 && SimdLib::IApi::ShiftBytesSlow<api_t>));
	static_assert(SimdLib::IRegister::ShiftBytesLeft<value_t, 1> == (SimdLib::IApi::ShiftBytesLeft<api_t, 1> && SimdLib::IApi::BitwiseAnd<api_t>));
	static_assert(SimdLib::IRegister::ShiftBytesRight<value_t, 1> == SimdLib::IApi::ShiftBytesRight<api_t, 1>);
	static_assert(SimdLib::IRegister::ShiftBitsLeftSlow<value_t> == (bits == 128 && SimdLib::IApi::ShiftBitsSlow<api_t> && SimdLib::IApi::BitwiseAnd<api_t>));
	static_assert(SimdLib::IRegister::ShiftBitsRightSlow<value_t> == (bits == 128 && SimdLib::IApi::ShiftBitsSlow<api_t>));
	static_assert(SimdLib::IRegister::ShiftBitsLeft<value_t, 1> == (bits == 128 && SimdLib::IApi::ShiftBits<api_t, 1> && SimdLib::IApi::BitwiseAnd<api_t>));
	static_assert(SimdLib::IRegister::ShiftBitsRight<value_t, 1> == (bits == 128 && SimdLib::IApi::ShiftBits<api_t, 1>));
	static_assert(SimdLib::IRegister::CompareEqual<value_t> == (SimdLib::IApi::CompareEqual<api_t> && SimdLib::IApi::BitwiseAnd<api_t>));
	static_assert(SimdLib::IRegister::CompareGreater<value_t> == (SimdLib::IApi::CompareGreater<api_t> && SimdLib::IApi::BitwiseAnd<api_t>));
	static_assert(SimdLib::IRegister::CompareGreaterEqual<value_t> == (SimdLib::IApi::CompareGreaterEqual<api_t> && SimdLib::IApi::BitwiseAnd<api_t>));
	static_assert(SimdLib::IRegister::CompareLess<value_t> == (SimdLib::IApi::CompareLess<api_t> && SimdLib::IApi::BitwiseAnd<api_t>));
	static_assert(SimdLib::IRegister::CompareLessEqual<value_t> == (SimdLib::IApi::CompareLessEqual<api_t> && SimdLib::IApi::BitwiseAnd<api_t>));
	static_assert(SimdLib::IRegister::Equal<value_t> ==
				  (SimdLib::IApi::CompareEqual<api_t> && SimdLib::IApi::BitwiseAnd<api_t> && SimdLib::IApi::MovemaskSlim<api_t>));
	static_assert(SimdLib::IRegister::NotEqual<value_t> == SimdLib::IRegister::Equal<value_t>);
	static_assert(!SimdLib::IRegister::ShiftBytesLeft<value_t, -1>);
	static_assert(!SimdLib::IRegister::ShiftBytesRight<value_t, -1>);
	static_assert(!SimdLib::IRegister::ShiftBitsLeft<value_t, -1>);
	static_assert(!SimdLib::IRegister::ShiftBitsRight<value_t, -1>);
	return true;
}

#define SIMDLIB_PARTIAL_OPERATION_SURFACE(element_type, width, count) static_assert(has_operation_surface_parity<element_type, width, count>())
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::int8_t, 128, 13);
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::uint8_t, 128, 13);
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::int16_t, 128, 5);
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::uint16_t, 128, 5);
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::int32_t, 128, 3);
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::uint32_t, 128, 3);
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::int64_t, 128, 1);
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::uint64_t, 128, 1);
SIMDLIB_PARTIAL_OPERATION_SURFACE(float, 128, 3);
SIMDLIB_PARTIAL_OPERATION_SURFACE(double, 128, 1);
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::int8_t, 256, 19);
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::uint8_t, 256, 19);
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::int16_t, 256, 11);
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::uint16_t, 256, 11);
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::int32_t, 256, 5);
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::uint32_t, 256, 5);
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::int64_t, 256, 3);
SIMDLIB_PARTIAL_OPERATION_SURFACE(std::uint64_t, 256, 3);
SIMDLIB_PARTIAL_OPERATION_SURFACE(float, 256, 5);
SIMDLIB_PARTIAL_OPERATION_SURFACE(double, 256, 3);
#endif
#undef SIMDLIB_PARTIAL_OPERATION_SURFACE

/** @brief Reports whether one scalar has the all-bits-zero representation. */
template <class element_t> [[nodiscard]] bool has_zero_bits(element_t value) noexcept
{
	const auto bytes = std::bit_cast<std::array<std::byte, sizeof(element_t)>>(value);
	for (const auto byte : bytes)
		if (byte != std::byte{})
			return false;
	return true;
}

/** @brief Requires that every physical lane beyond the logical prefix remains bitwise zero. */
template <class value_t> void require_zero_suffix(value_t value)
{
	const auto lanes = value_t::api_type::to_array(value.to_native());
	for (std::size_t lane = value_t::lane_count; lane < value_t::native_lane_count; ++lane)
		REQUIRE(has_zero_bits(lanes[lane]));
}

/** @brief Verifies bitwise values, active-only masks, and suffix closure for one integral geometry. */
template <class element_t, std::size_t bits, std::size_t active_count> void require_bitwise_contract()
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_count>;
	std::array<element_t, active_count> lhs_values{};
	std::array<element_t, active_count> rhs_values{};
	for (std::size_t lane = 0; lane < active_count; ++lane)
	{
		lhs_values[lane] = static_cast<element_t>((lane & 1U) == 0 ? 0x55 : 0xaa);
		rhs_values[lane] = static_cast<element_t>((lane & 1U) == 0 ? 0x0f : 0xf0);
	}
	const auto lhs = value_t::from_array(lhs_values);
	const auto rhs = value_t::from_array(rhs_values);
	const auto intersection = lhs & rhs;
	const auto union_value = lhs | rhs;
	const auto exclusive = lhs ^ rhs;
	const auto complement = ~lhs;
	const auto difference = lhs.andnot(rhs);
	for (std::size_t lane = 0; lane < active_count; ++lane)
	{
		REQUIRE(intersection.to_array()[lane] == static_cast<element_t>(lhs_values[lane] & rhs_values[lane]));
		REQUIRE(union_value.to_array()[lane] == static_cast<element_t>(lhs_values[lane] | rhs_values[lane]));
		REQUIRE(exclusive.to_array()[lane] == static_cast<element_t>(lhs_values[lane] ^ rhs_values[lane]));
		REQUIRE(complement.to_array()[lane] == static_cast<element_t>(~lhs_values[lane]));
		REQUIRE(difference.to_array()[lane] == static_cast<element_t>((~lhs_values[lane]) & rhs_values[lane]));
	}
	require_zero_suffix(intersection);
	require_zero_suffix(union_value);
	require_zero_suffix(exclusive);
	require_zero_suffix(complement);
	require_zero_suffix(difference);
}

/** @brief Verifies per-lane shift semantics and suffix closure for one integral geometry. */
template <class element_t, std::size_t bits, std::size_t active_count> void require_lane_shift_contract()
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_count>;
	std::array<element_t, active_count> values{};
	for (std::size_t lane = 0; lane < active_count; ++lane)
		values[lane] = static_cast<element_t>(lane + 2);
	if constexpr (std::is_signed_v<element_t>)
		values[0] = static_cast<element_t>(-8);
	const auto value = value_t::from_array(values);
	const auto left = value << 2;
	const auto logical_right = value.logical_shift_right(1);
	const auto signedness_right = value >> 1;
	using unsigned_t = std::make_unsigned_t<element_t>;
	for (std::size_t lane = 0; lane < active_count; ++lane)
	{
		REQUIRE(left.to_array()[lane] == static_cast<element_t>(static_cast<unsigned_t>(values[lane]) << 2));
		REQUIRE(logical_right.to_array()[lane] == static_cast<element_t>(static_cast<unsigned_t>(values[lane]) >> 1));
		REQUIRE(signedness_right.to_array()[lane] == static_cast<element_t>(values[lane] >> 1));
	}
	require_zero_suffix(left);
	require_zero_suffix(logical_right);
	require_zero_suffix(signedness_right);
}

/** @brief Verifies logical whole-payload byte and bit shifts at a non-group-aligned active extent. */
template <std::size_t bits, std::size_t active_count> void require_payload_shift_contract()
{
	using value_t = SimdLib::PartialRegister<std::uint8_t, bits, active_count>;
	std::array<std::uint8_t, active_count> values{};
	for (std::size_t lane = 0; lane < active_count; ++lane)
		values[lane] = static_cast<std::uint8_t>(lane + 1);
	const auto value = value_t::from_array(values);
	const auto bytes_left = value.template shift_bytes_left<3>();
	const auto bytes_right = value.template shift_bytes_right<3>();
	for (std::size_t lane = 0; lane < active_count; ++lane)
	{
		REQUIRE(bytes_left.to_array()[lane] == (lane < 3 ? 0 : values[lane - 3]));
		REQUIRE(bytes_right.to_array()[lane] == (lane + 3 < active_count ? values[lane + 3] : 0));
	}
	require_zero_suffix(bytes_left);
	require_zero_suffix(bytes_right);

	if constexpr (bits == 128)
	{
		const auto slow_bytes_left = value.shift_bytes_left_slow(3);
		const auto slow_bytes_right = value.shift_bytes_right_slow(3);
		const auto bits_left = value.template shift_bits_left<4>();
		const auto bits_right = value.template shift_bits_right<4>();
		const auto slow_bits_left = value.shift_bits_left_slow(4);
		const auto slow_bits_right = value.shift_bits_right_slow(4);
		REQUIRE(slow_bytes_left.to_array() == bytes_left.to_array());
		REQUIRE(slow_bytes_right.to_array() == bytes_right.to_array());
		REQUIRE(slow_bits_left.to_array() == bits_left.to_array());
		REQUIRE(slow_bits_right.to_array() == bits_right.to_array());
		REQUIRE(value.template shift_bytes_left<active_count>().to_array() == std::array<std::uint8_t, active_count>{});
		REQUIRE(value.template shift_bytes_right<active_count>().to_array() == std::array<std::uint8_t, active_count>{});
		REQUIRE(value.template shift_bits_left<static_cast<int>(active_count * 8)>().to_array() == std::array<std::uint8_t, active_count>{});
		REQUIRE(value.template shift_bits_right<static_cast<int>(active_count * 8)>().to_array() == std::array<std::uint8_t, active_count>{});
		require_zero_suffix(bits_left);
		require_zero_suffix(bits_right);
		require_zero_suffix(slow_bytes_left);
		require_zero_suffix(slow_bytes_right);
		require_zero_suffix(slow_bits_left);
		require_zero_suffix(slow_bits_right);
	}
}

/** @brief Verifies ordered comparisons, scalar equality, and false inactive predicate lanes. */
template <class element_t, std::size_t bits, std::size_t active_count> void require_comparison_contract()
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_count>;
	std::array<element_t, active_count> lhs_values{};
	std::array<element_t, active_count> rhs_values{};
	for (std::size_t lane = 0; lane < active_count; ++lane)
	{
		lhs_values[lane] = static_cast<element_t>(lane + 1);
		rhs_values[lane] = static_cast<element_t>(lane + 1);
	}
	rhs_values[active_count - 1] = static_cast<element_t>(rhs_values[active_count - 1] + 1);
	const auto lhs = value_t::from_array(lhs_values);
	const auto rhs = value_t::from_array(rhs_values);
	const auto equal = lhs.compare_equal(rhs);
	const auto greater = lhs.compare_greater(rhs);
	const auto greater_equal = lhs.compare_greater_equal(rhs);
	const auto less = lhs.compare_less(rhs);
	const auto less_equal = lhs.compare_less_equal(rhs);
	const auto expected_equal_bits = (typename value_t::mask_type::bits_type{1} << (active_count - 1)) - 1;
	REQUIRE(equal.bits() == expected_equal_bits);
	REQUIRE(greater.none());
	REQUIRE(greater_equal.bits() == expected_equal_bits);
	REQUIRE(less.bits() == (typename value_t::mask_type::bits_type{1} << (active_count - 1)));
	REQUIRE(less_equal.all());
	REQUIRE_FALSE(lhs == rhs);
	REQUIRE(lhs != rhs);
	REQUIRE(lhs == lhs);
	const auto require_false_suffix = []<class mask_t>(mask_t mask)
	{
		const auto native = mask_t::api_type::to_array(mask.to_native());
		for (std::size_t lane = mask_t::lane_count; lane < mask_t::native_lane_count; ++lane)
			REQUIRE(has_zero_bits(native[lane]));
	};
	require_false_suffix(equal);
	require_false_suffix(greater);
	require_false_suffix(greater_equal);
	require_false_suffix(less);
	require_false_suffix(less_equal);
}

TEST_CASE("PartialRegister bitwise operations preserve active values and inactive zeros", "[PartialRegister][Bitwise]")
{
	require_bitwise_contract<std::uint8_t, 128, 13>();
	require_bitwise_contract<std::uint32_t, 128, 3>();
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
	require_bitwise_contract<std::uint8_t, 256, 19>();
#endif
}

TEST_CASE("PartialRegister sign masks expose active lanes only", "[PartialRegister][Bitwise]")
{
	using value_t = SimdLib::PartialRegister<std::int32_t, 128, 3>;
	const auto value = value_t::from_lanes(-1, 2, -3);
	REQUIRE(value.lane_sign_bits() == 0b101);
	REQUIRE(value.movemask() == 0x0f0f);
}

TEST_CASE("PartialRegister per-lane shifts preserve logical geometry", "[PartialRegister][Shift]")
{
	require_lane_shift_contract<std::int32_t, 128, 3>();
	require_lane_shift_contract<std::uint16_t, 128, 5>();
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
	require_lane_shift_contract<std::int32_t, 256, 5>();
#endif
}

TEST_CASE("PartialRegister whole-payload shifts use the active extent", "[PartialRegister][Shift]")
{
	require_payload_shift_contract<128, 13>();
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
	require_payload_shift_contract<256, 19>();
#endif
}

TEST_CASE("PartialRegister comparisons ignore inactive zero equality", "[PartialRegister][Comparison]")
{
	require_comparison_contract<std::int32_t, 128, 3>();
	require_comparison_contract<float, 128, 3>();
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
	require_comparison_contract<std::int32_t, 256, 5>();
#endif
}

TEST_CASE("PartialRegister floating equality preserves NaN and signed-zero behavior", "[PartialRegister][Comparison]")
{
	using value_t = SimdLib::PartialRegister<float, 128, 3>;
	const auto positive_zero = value_t::from_lanes(0.0F, 2.0F, 3.0F);
	const auto negative_zero = value_t::from_lanes(-0.0F, 2.0F, 3.0F);
	const auto nan_value = value_t::from_lanes(std::numeric_limits<float>::quiet_NaN(), 2.0F, 3.0F);
	REQUIRE(positive_zero == negative_zero);
	REQUIRE_FALSE(nan_value == nan_value);
	REQUIRE(nan_value != nan_value);
}

} // namespace
