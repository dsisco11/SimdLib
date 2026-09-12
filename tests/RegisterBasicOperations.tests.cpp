#include <SimdLib/Register.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#ifndef SIMDLIB_REGISTER_TEST_ENABLE_256
#define SIMDLIB_REGISTER_TEST_ENABLE_256 SIMDLIB_HAS_AVX2
#endif

namespace
{

/** @brief Selects an unsigned integer capable of holding one element's complete bit pattern. */
template <class element_t, bool = std::is_integral_v<element_t>> struct bit_integer;

/** @brief Selects the corresponding unsigned representation for an integral element. */
template <class element_t> struct bit_integer<element_t, true>
{
	using type = std::make_unsigned_t<element_t>;
};

/** @brief Selects a 32-bit representation for a floating-point element. */
template <> struct bit_integer<float, false>
{
	using type = std::uint32_t;
};

/** @brief Selects a 64-bit representation for a double-precision element. */
template <> struct bit_integer<double, false>
{
	using type = std::uint64_t;
};

/** @brief Unsigned integer type that preserves one element's complete bit pattern. */
template <class element_t> using bit_integer_t = typename bit_integer<element_t>::type;

/** @brief Returns the object representation of one scalar value. */
template <class element_t> [[nodiscard]] constexpr bit_integer_t<element_t> scalar_bits(element_t value) noexcept
{
	return std::bit_cast<bit_integer_t<element_t>>(value);
}

/** @brief Constructs one scalar value from its complete object representation. */
template <class element_t> [[nodiscard]] constexpr element_t scalar_from_bits(bit_integer_t<element_t> value) noexcept
{
	return std::bit_cast<element_t>(value);
}

/** @brief Requires exact per-lane object-representation equality. */
template <class element_t, std::size_t count>
void require_bitwise_equal(const std::array<element_t, count> &actual, const std::array<element_t, count> &expected)
{
	for (std::size_t index = 0; index < count; ++index)
		REQUIRE(scalar_bits(actual[index]) == scalar_bits(expected[index]));
}

/** @brief Computes a wrapping scalar sum using unsigned representation arithmetic. */
template <class element_t> [[nodiscard]] constexpr element_t wrapping_add(element_t lhs, element_t rhs) noexcept
{
	using bits_type = bit_integer_t<element_t>;
	return scalar_from_bits<element_t>(static_cast<bits_type>(scalar_bits(lhs) + scalar_bits(rhs)));
}

/** @brief Computes a wrapping scalar difference using unsigned representation arithmetic. */
template <class element_t> [[nodiscard]] constexpr element_t wrapping_subtract(element_t lhs, element_t rhs) noexcept
{
	using bits_type = bit_integer_t<element_t>;
	return scalar_from_bits<element_t>(static_cast<bits_type>(scalar_bits(lhs) - scalar_bits(rhs)));
}

/** @brief Computes a wrapping scalar product using unsigned representation arithmetic. */
template <class element_t> [[nodiscard]] constexpr element_t wrapping_multiply(element_t lhs, element_t rhs) noexcept
{
	using bits_type = bit_integer_t<element_t>;
	return scalar_from_bits<element_t>(static_cast<bits_type>(scalar_bits(lhs) * scalar_bits(rhs)));
}

/** @brief Computes a wrapping scalar negation using unsigned representation arithmetic. */
template <class element_t> [[nodiscard]] constexpr element_t wrapping_negate(element_t value) noexcept
{
	using bits_type = bit_integer_t<element_t>;
	return scalar_from_bits<element_t>(static_cast<bits_type>(bits_type{} - scalar_bits(value)));
}

/** @brief Verifies integral arithmetic against both the Api path and independent scalar oracles. */
template <class element_t, std::size_t bits>
	requires std::is_integral_v<element_t>
void require_integral_arithmetic()
{
	using register_type = SimdLib::Register<element_t, bits>;
	using api_type = typename register_type::api_type;
	std::array<element_t, register_type::lane_count> left{};
	std::array<element_t, register_type::lane_count> right{};
	for (std::size_t index = 0; index < left.size(); ++index)
	{
		if constexpr (std::is_signed_v<element_t>)
		{
			switch (index % 6)
			{
			case 0:
				left[index] = std::numeric_limits<element_t>::lowest();
				right[index] = element_t{1};
				break;
			case 1:
				left[index] = std::numeric_limits<element_t>::max();
				right[index] = element_t{-1};
				break;
			case 2:
				left[index] = element_t{-17};
				right[index] = element_t{2};
				break;
			case 3:
				left[index] = element_t{17};
				right[index] = element_t{-3};
				break;
			case 4:
				left[index] = element_t{-1};
				right[index] = element_t{7};
				break;
			default:
				left[index] = element_t{};
				right[index] = element_t{5};
				break;
			}
		}
		else
		{
			using unsigned_type = bit_integer_t<element_t>;
			constexpr auto high_bit = static_cast<unsigned_type>(unsigned_type{1} << (std::numeric_limits<unsigned_type>::digits - 1));
			switch (index % 6)
			{
			case 0:
				left[index] = std::numeric_limits<element_t>::max();
				right[index] = element_t{1};
				break;
			case 1:
				left[index] = static_cast<element_t>(high_bit);
				right[index] = element_t{2};
				break;
			case 2:
				left[index] = static_cast<element_t>(high_bit | unsigned_type{7});
				right[index] = element_t{3};
				break;
			case 3:
				left[index] = element_t{17};
				right[index] = element_t{5};
				break;
			case 4:
				left[index] = element_t{1};
				right[index] = element_t{7};
				break;
			default:
				left[index] = element_t{};
				right[index] = element_t{11};
				break;
			}
		}
	}

	std::array<element_t, register_type::lane_count> sums{};
	std::array<element_t, register_type::lane_count> differences{};
	std::array<element_t, register_type::lane_count> products{};
	std::array<element_t, register_type::lane_count> quotients{};
	std::array<element_t, register_type::lane_count> remainders{};
	std::array<element_t, register_type::lane_count> negations{};
	for (std::size_t index = 0; index < left.size(); ++index)
	{
		sums[index] = wrapping_add(left[index], right[index]);
		differences[index] = wrapping_subtract(left[index], right[index]);
		products[index] = wrapping_multiply(left[index], right[index]);
		quotients[index] = static_cast<element_t>(left[index] / right[index]);
		remainders[index] = static_cast<element_t>(left[index] % right[index]);
		negations[index] = wrapping_negate(left[index]);
	}

	const register_type lhs = register_type::from_array(left);
	const register_type rhs = register_type::from_array(right);
	REQUIRE((lhs + rhs).to_array() == sums);
	REQUIRE((lhs - rhs).to_array() == differences);
	REQUIRE((lhs * rhs).to_array() == products);
	REQUIRE((lhs / rhs).to_array() == quotients);
	REQUIRE((lhs % rhs).to_array() == remainders);
	REQUIRE((-lhs).to_array() == negations);
	REQUIRE((lhs + rhs).to_array() == api_type::to_array(api_type::add(lhs.native, rhs.native)));
	REQUIRE((lhs - rhs).to_array() == api_type::to_array(api_type::subtract(lhs.native, rhs.native)));
	REQUIRE((lhs * rhs).to_array() == api_type::to_array(api_type::multiply(lhs.native, rhs.native)));
	REQUIRE((lhs / rhs).to_array() == api_type::to_array(api_type::divide(lhs.native, rhs.native)));
	REQUIRE((lhs % rhs).to_array() == api_type::to_array(api_type::modulus(lhs.native, rhs.native)));
	REQUIRE((-lhs).to_array() == api_type::to_array(api_type::negate(lhs.native)));

	auto reassigned = lhs;
	reassigned = reassigned + rhs;
	REQUIRE(reassigned.to_array() == sums);
	reassigned = lhs;
	reassigned = reassigned - rhs;
	REQUIRE(reassigned.to_array() == differences);
	reassigned = lhs;
	reassigned = reassigned * rhs;
	REQUIRE(reassigned.to_array() == products);
	reassigned = lhs;
	reassigned = reassigned / rhs;
	REQUIRE(reassigned.to_array() == quotients);
	reassigned = lhs;
	reassigned = reassigned % rhs;
	REQUIRE(reassigned.to_array() == remainders);
}

/** @brief Reports scalar floating equality while preserving NaN and signed-zero distinctions. */
template <class element_t> [[nodiscard]] bool equivalent_floating(element_t actual, element_t expected) noexcept
{
	if (std::isnan(expected))
		return std::isnan(actual);
	if (actual == element_t{} && expected == element_t{})
		return std::signbit(actual) == std::signbit(expected);
	return actual == expected;
}

/** @brief Requires floating arrays to match scalar-oracle values lane by lane. */
template <class element_t, std::size_t count>
void require_floating_equal(const std::array<element_t, count> &actual, const std::array<element_t, count> &expected)
{
	for (std::size_t index = 0; index < count; ++index)
		REQUIRE(equivalent_floating(actual[index], expected[index]));
}

/** @brief Verifies floating arithmetic, reassignment, infinities, NaNs, and signed zeros. */
template <class element_t, std::size_t bits>
	requires std::is_floating_point_v<element_t>
void require_floating_arithmetic()
{
	using register_type = SimdLib::Register<element_t, bits>;
	using api_type = typename register_type::api_type;
	std::array<element_t, register_type::lane_count> left{};
	std::array<element_t, register_type::lane_count> right{};
	for (std::size_t index = 0; index < left.size(); ++index)
	{
		switch (index % 8)
		{
		case 0:
			left[index] = element_t{0.0};
			right[index] = element_t{2.0};
			break;
		case 1:
			left[index] = element_t{-0.0};
			right[index] = element_t{-2.0};
			break;
		case 2:
			left[index] = std::numeric_limits<element_t>::infinity();
			right[index] = element_t{2.0};
			break;
		case 3:
			left[index] = -std::numeric_limits<element_t>::infinity();
			right[index] = element_t{2.0};
			break;
		case 4:
			left[index] = std::numeric_limits<element_t>::quiet_NaN();
			right[index] = element_t{1.0};
			break;
		case 5:
			left[index] = std::numeric_limits<element_t>::max() / element_t{2.0};
			right[index] = element_t{2.0};
			break;
		case 6:
			left[index] = element_t{-3.5};
			right[index] = element_t{-0.5};
			break;
		default:
			left[index] = element_t{7.25};
			right[index] = element_t{4.0};
			break;
		}
	}

	std::array<element_t, register_type::lane_count> sums{};
	std::array<element_t, register_type::lane_count> differences{};
	std::array<element_t, register_type::lane_count> products{};
	std::array<element_t, register_type::lane_count> quotients{};
	std::array<element_t, register_type::lane_count> negations{};
	for (std::size_t index = 0; index < left.size(); ++index)
	{
		sums[index] = left[index] + right[index];
		differences[index] = left[index] - right[index];
		products[index] = left[index] * right[index];
		quotients[index] = left[index] / right[index];
		negations[index] = element_t{} - left[index];
	}

	const register_type lhs = register_type::from_array(left);
	const register_type rhs = register_type::from_array(right);
	require_floating_equal((lhs + rhs).to_array(), sums);
	require_floating_equal((lhs - rhs).to_array(), differences);
	require_floating_equal((lhs * rhs).to_array(), products);
	require_floating_equal((lhs / rhs).to_array(), quotients);
	require_floating_equal((-lhs).to_array(), negations);
	require_floating_equal((lhs + rhs).to_array(), api_type::to_array(api_type::add(lhs.native, rhs.native)));
	require_floating_equal((lhs - rhs).to_array(), api_type::to_array(api_type::subtract(lhs.native, rhs.native)));
	require_floating_equal((lhs * rhs).to_array(), api_type::to_array(api_type::multiply(lhs.native, rhs.native)));
	require_floating_equal((lhs / rhs).to_array(), api_type::to_array(api_type::divide(lhs.native, rhs.native)));
	require_floating_equal((-lhs).to_array(), api_type::to_array(api_type::negate(lhs.native)));

	auto reassigned = lhs;
	reassigned = reassigned + rhs;
	require_floating_equal(reassigned.to_array(), sums);
	reassigned = lhs;
	reassigned = reassigned - rhs;
	require_floating_equal(reassigned.to_array(), differences);
	reassigned = lhs;
	reassigned = reassigned * rhs;
	require_floating_equal(reassigned.to_array(), products);
	reassigned = lhs;
	reassigned = reassigned / rhs;
	require_floating_equal(reassigned.to_array(), quotients);
}

/** @brief Verifies bitwise operations and both scalar mask granularities for one geometry. */
template <class element_t, std::size_t bits> void require_bitwise_operations()
{
	using register_type = SimdLib::Register<element_t, bits>;
	using api_type = typename register_type::api_type;
	using bits_type = bit_integer_t<element_t>;
	constexpr int element_bits = std::numeric_limits<bits_type>::digits;
	std::array<element_t, register_type::lane_count> left{};
	std::array<element_t, register_type::lane_count> right{};
	std::array<element_t, register_type::lane_count> intersection{};
	std::array<element_t, register_type::lane_count> union_values{};
	std::array<element_t, register_type::lane_count> exclusive{};
	std::array<element_t, register_type::lane_count> complement{};
	std::array<element_t, register_type::lane_count> andnot_values{};
	std::uint32_t expected_movemask = 0;
	std::uint32_t expected_lane_bits = 0;
	for (std::size_t index = 0; index < left.size(); ++index)
	{
		const auto high_bit = static_cast<bits_type>(bits_type{1} << (element_bits - 1));
		const auto left_bits = static_cast<bits_type>((index % 2 == 0 ? high_bit : bits_type{}) | static_cast<bits_type>(index * 37U + 0x15U));
		const auto right_bits = static_cast<bits_type>((index % 3 == 0 ? high_bit : bits_type{}) | static_cast<bits_type>(index * 19U + 0x2AU));
		left[index] = scalar_from_bits<element_t>(left_bits);
		right[index] = scalar_from_bits<element_t>(right_bits);
		intersection[index] = scalar_from_bits<element_t>(static_cast<bits_type>(left_bits & right_bits));
		union_values[index] = scalar_from_bits<element_t>(static_cast<bits_type>(left_bits | right_bits));
		exclusive[index] = scalar_from_bits<element_t>(static_cast<bits_type>(left_bits ^ right_bits));
		complement[index] = scalar_from_bits<element_t>(static_cast<bits_type>(~left_bits));
		andnot_values[index] = scalar_from_bits<element_t>(static_cast<bits_type>((~left_bits) & right_bits));
		if ((left_bits & high_bit) != 0)
			expected_lane_bits |= std::uint32_t{1} << index;
		const auto bytes = std::bit_cast<std::array<std::uint8_t, sizeof(element_t)>>(left[index]);
		for (std::size_t byte = 0; byte < bytes.size(); ++byte)
		{
			if ((bytes[byte] & 0x80U) != 0)
				expected_movemask |= std::uint32_t{1} << (index * sizeof(element_t) + byte);
		}
	}

	const register_type lhs = register_type::from_array(left);
	const register_type rhs = register_type::from_array(right);
	require_bitwise_equal((lhs & rhs).to_array(), intersection);
	require_bitwise_equal((lhs | rhs).to_array(), union_values);
	require_bitwise_equal((lhs ^ rhs).to_array(), exclusive);
	require_bitwise_equal((~lhs).to_array(), complement);
	require_bitwise_equal(lhs.andnot(rhs).to_array(), andnot_values);
	require_bitwise_equal((lhs & rhs).to_array(), api_type::to_array(api_type::bitwise_and(lhs.native, rhs.native)));
	require_bitwise_equal((lhs | rhs).to_array(), api_type::to_array(api_type::bitwise_or(lhs.native, rhs.native)));
	require_bitwise_equal((lhs ^ rhs).to_array(), api_type::to_array(api_type::bitwise_xor(lhs.native, rhs.native)));
	require_bitwise_equal((~lhs).to_array(), api_type::to_array(api_type::bitwise_not(lhs.native)));
	require_bitwise_equal(lhs.andnot(rhs).to_array(), api_type::to_array(api_type::bitwise_andnot(lhs.native, rhs.native)));
	REQUIRE(lhs.movemask() == expected_movemask);
	REQUIRE(lhs.lane_sign_bits() == expected_lane_bits);
	REQUIRE(lhs.movemask() == api_type::movemask(lhs.native));
	REQUIRE(lhs.lane_sign_bits() == api_type::movemask_slim(lhs.native));

	auto reassigned = lhs;
	reassigned = reassigned & rhs;
	require_bitwise_equal(reassigned.to_array(), intersection);
	reassigned = lhs;
	reassigned = reassigned | rhs;
	require_bitwise_equal(reassigned.to_array(), union_values);
	reassigned = lhs;
	reassigned = reassigned ^ rhs;
	require_bitwise_equal(reassigned.to_array(), exclusive);
}

/** @brief Computes one scalar per-lane logical left shift with backend boundary semantics. */
template <class element_t> [[nodiscard]] constexpr element_t logical_left(element_t value, int count) noexcept
{
	using bits_type = bit_integer_t<element_t>;
	constexpr int width = std::numeric_limits<bits_type>::digits;
	if (count >= width)
		return element_t{};
	return scalar_from_bits<element_t>(static_cast<bits_type>(scalar_bits(value) << count));
}

/** @brief Computes one scalar per-lane logical right shift with backend boundary semantics. */
template <class element_t> [[nodiscard]] constexpr element_t logical_right(element_t value, int count) noexcept
{
	using bits_type = bit_integer_t<element_t>;
	constexpr int width = std::numeric_limits<bits_type>::digits;
	if (count >= width)
		return element_t{};
	return scalar_from_bits<element_t>(static_cast<bits_type>(scalar_bits(value) >> count));
}

/** @brief Computes one scalar arithmetic right shift without relying on signed C++ shift behavior. */
template <class element_t> [[nodiscard]] constexpr element_t arithmetic_right(element_t value, int count) noexcept
{
	using bits_type = bit_integer_t<element_t>;
	constexpr int width = std::numeric_limits<bits_type>::digits;
	if (count >= width)
		count = width - 1;
	const bits_type input = scalar_bits(value);
	bits_type result = static_cast<bits_type>(input >> count);
	const bits_type sign = static_cast<bits_type>(bits_type{1} << (width - 1));
	if (count > 0 && (input & sign) != 0)
		result = static_cast<bits_type>(result | static_cast<bits_type>(~bits_type{}) << (width - count));
	return scalar_from_bits<element_t>(result);
}

/** @brief Verifies all per-lane shift boundaries and reassignment spellings for one integral geometry. */
template <class element_t, std::size_t bits>
	requires std::is_integral_v<element_t>
void require_lane_shifts()
{
	using register_type = SimdLib::Register<element_t, bits>;
	using api_type = typename register_type::api_type;
	using bits_type = bit_integer_t<element_t>;
	constexpr int width = std::numeric_limits<bits_type>::digits;
	std::array<element_t, register_type::lane_count> source{};
	for (std::size_t index = 0; index < source.size(); ++index)
	{
		const auto high = static_cast<bits_type>(bits_type{1} << (width - 1));
		source[index] = scalar_from_bits<element_t>(static_cast<bits_type>(high | static_cast<bits_type>(index * 17U + 3U)));
	}
	const register_type value = register_type::from_array(source);
	for (const int count : std::array<int, 4>{0, width - 1, width, width + 1})
	{
		std::array<element_t, register_type::lane_count> expected_left{};
		std::array<element_t, register_type::lane_count> expected_logical{};
		std::array<element_t, register_type::lane_count> expected_operator_right{};
		for (std::size_t index = 0; index < source.size(); ++index)
		{
			expected_left[index] = logical_left(source[index], count);
			expected_logical[index] = logical_right(source[index], count);
			if constexpr (std::is_signed_v<element_t>)
				expected_operator_right[index] = arithmetic_right(source[index], count);
			else
				expected_operator_right[index] = expected_logical[index];
		}
		REQUIRE((value << count).to_array() == expected_left);
		REQUIRE(value.logical_shift_right(count).to_array() == expected_logical);
		REQUIRE((value >> count).to_array() == expected_operator_right);
		REQUIRE((value << count).to_array() == api_type::to_array(api_type::shift_left(value.native, count)));
		REQUIRE(value.logical_shift_right(count).to_array() == api_type::to_array(api_type::shift_right(value.native, count)));
		if constexpr (std::is_signed_v<element_t>)
			REQUIRE((value >> count).to_array() == api_type::to_array(api_type::shift_right_arithmetic(value.native, count)));

		auto reassigned = value;
		reassigned = reassigned << count;
		REQUIRE(reassigned.to_array() == expected_left);
		reassigned = value;
		reassigned = reassigned >> count;
		REQUIRE(reassigned.to_array() == expected_operator_right);
	}
}

/** @brief Computes a complete-register left shift for two low-to-high 64-bit words. */
[[nodiscard]] constexpr std::array<std::uint64_t, 2> whole_left(std::array<std::uint64_t, 2> value, int count) noexcept
{
	if (count <= 0)
		return value;
	if (count >= 128)
		return {};
	if (count >= 64)
		return {0, value[0] << (count - 64)};
	return {value[0] << count, static_cast<std::uint64_t>((value[1] << count) | (value[0] >> (64 - count)))};
}

/** @brief Computes a complete-register right shift for two low-to-high 64-bit words. */
[[nodiscard]] constexpr std::array<std::uint64_t, 2> whole_right(std::array<std::uint64_t, 2> value, int count) noexcept
{
	if (count <= 0)
		return value;
	if (count >= 128)
		return {};
	if (count >= 64)
		return {value[1] >> (count - 64), 0};
	return {static_cast<std::uint64_t>((value[0] >> count) | (value[1] << (64 - count))), value[1] >> count};
}

/** @brief Verifies byte and whole-register shift boundaries for the supported 128-bit shape. */
void require_complete_register_shifts()
{
	using byte_register = SimdLib::Register<std::uint8_t, 128>;
	std::array<std::uint8_t, byte_register::lane_count> bytes{};
	for (std::size_t index = 0; index < bytes.size(); ++index)
		bytes[index] = static_cast<std::uint8_t>(index + 1);
	const byte_register byte_value = byte_register::from_array(bytes);
	constexpr std::array<int, 22> counts{std::numeric_limits<int>::lowest(), -17, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
										 std::numeric_limits<int>::max()};
	for (const int count : counts)
	{
		std::array<std::uint8_t, byte_register::lane_count> left{};
		std::array<std::uint8_t, byte_register::lane_count> right{};
		if (count <= 0)
		{
			left = bytes;
			right = bytes;
		}
		else if (count < 16)
		{
			for (std::size_t index = static_cast<std::size_t>(count); index < bytes.size(); ++index)
				left[index] = bytes[index - static_cast<std::size_t>(count)];
			for (std::size_t index = 0; index + static_cast<std::size_t>(count) < bytes.size(); ++index)
				right[index] = bytes[index + static_cast<std::size_t>(count)];
		}
		REQUIRE(byte_value.shift_bytes_left_slow(count).to_array() == left);
		REQUIRE(byte_value.shift_bytes_right_slow(count).to_array() == right);
	}

	using word_register = SimdLib::Register<std::uint64_t, 128>;
	constexpr std::array<std::uint64_t, 2> words{0x0123456789ABCDEFULL, 0xFEDCBA9876543210ULL};
	const word_register word_value = word_register::from_array(words);
	for (int count = 0; count < 128; ++count)
	{
		REQUIRE(word_value.shift_bits_left_slow(count).to_array() == whole_left(words, count));
		REQUIRE(word_value.shift_bits_right_slow(count).to_array() == whole_right(words, count));
	}
	constexpr std::array<int, 6> out_of_range_bit_counts{std::numeric_limits<int>::lowest(), -17, -1, 128, 129, std::numeric_limits<int>::max()};
	for (const int count : out_of_range_bit_counts)
	{
		REQUIRE(word_value.shift_bits_left_slow(count).to_array() == whole_left(words, count));
		REQUIRE(word_value.shift_bits_right_slow(count).to_array() == whole_right(words, count));
	}
	REQUIRE(word_value.template shift_bits_left<0>().to_array() == whole_left(words, 0));
	REQUIRE(word_value.template shift_bits_left<1>().to_array() == whole_left(words, 1));
	REQUIRE(word_value.template shift_bits_left<63>().to_array() == whole_left(words, 63));
	REQUIRE(word_value.template shift_bits_left<64>().to_array() == whole_left(words, 64));
	REQUIRE(word_value.template shift_bits_left<65>().to_array() == whole_left(words, 65));
	REQUIRE(word_value.template shift_bits_left<127>().to_array() == whole_left(words, 127));
	REQUIRE(word_value.template shift_bits_left<128>().to_array() == whole_left(words, 128));
	REQUIRE(word_value.template shift_bits_left<129>().to_array() == whole_left(words, 129));
	REQUIRE(word_value.template shift_bits_right<0>().to_array() == whole_right(words, 0));
	REQUIRE(word_value.template shift_bits_right<1>().to_array() == whole_right(words, 1));
	REQUIRE(word_value.template shift_bits_right<63>().to_array() == whole_right(words, 63));
	REQUIRE(word_value.template shift_bits_right<64>().to_array() == whole_right(words, 64));
	REQUIRE(word_value.template shift_bits_right<65>().to_array() == whole_right(words, 65));
	REQUIRE(word_value.template shift_bits_right<127>().to_array() == whole_right(words, 127));
	REQUIRE(word_value.template shift_bits_right<128>().to_array() == whole_right(words, 128));
	REQUIRE(word_value.template shift_bits_right<129>().to_array() == whole_right(words, 129));
}

/** @brief Runs arithmetic coverage at both supported register widths. */
template <class element_t> void require_arithmetic_type()
{
	if constexpr (std::is_integral_v<element_t>)
	{
		require_integral_arithmetic<element_t, 128>();
		if constexpr (SIMDLIB_REGISTER_TEST_ENABLE_256)
			require_integral_arithmetic<element_t, 256>();
	}
	else
	{
		require_floating_arithmetic<element_t, 128>();
		if constexpr (SIMDLIB_REGISTER_TEST_ENABLE_256)
			require_floating_arithmetic<element_t, 256>();
	}
}

/** @brief Runs bitwise coverage at both supported register widths. */
template <class element_t> void require_bitwise_type()
{
	require_bitwise_operations<element_t, 128>();
	if constexpr (SIMDLIB_REGISTER_TEST_ENABLE_256)
		require_bitwise_operations<element_t, 256>();
}

/** @brief Verifies every lane-sign combination while independently varying all non-sign payload bits. */
template <class element_t, std::size_t bits>
	requires(std::is_integral_v<element_t> && (sizeof(element_t) == 2 || sizeof(element_t) == 4 || sizeof(element_t) == 8))
void require_exhaustive_lane_sign_bits()
{
	using register_type = SimdLib::Register<element_t, bits>;
	using bits_type = bit_integer_t<element_t>;
	constexpr auto sign_bit = static_cast<bits_type>(bits_type{1} << (std::numeric_limits<bits_type>::digits - 1));
	constexpr auto payload_mask = static_cast<bits_type>(~sign_bit);
	constexpr std::uint32_t combination_count = std::uint32_t{1} << register_type::lane_count;

	for (std::uint32_t expected = 0; expected < combination_count; ++expected)
	{
		std::array<element_t, register_type::lane_count> sparse_payload{};
		std::array<element_t, register_type::lane_count> dense_payload{};
		for (std::size_t lane = 0; lane < register_type::lane_count; ++lane)
		{
			const auto lane_sign = static_cast<bits_type>(((expected >> lane) & 1U) != 0 ? sign_bit : bits_type{});
			const auto sparse_bits = static_cast<bits_type>((bits_type{1} << (lane % (std::numeric_limits<bits_type>::digits - 1))) | bits_type{0x15});
			const auto dense_bits = static_cast<bits_type>(~static_cast<bits_type>(lane * 0x10203U + expected * 0x101U));
			sparse_payload[lane] = scalar_from_bits<element_t>(static_cast<bits_type>(lane_sign | (sparse_bits & payload_mask)));
			dense_payload[lane] = scalar_from_bits<element_t>(static_cast<bits_type>(lane_sign | (dense_bits & payload_mask)));
		}

		REQUIRE(register_type::from_array(sparse_payload).lane_sign_bits() == expected);
		REQUIRE(register_type::from_array(dense_payload).lane_sign_bits() == expected);
	}
}

/** @brief Runs exhaustive 16-bit, 32-bit, and 64-bit integral sign-mask coverage at both supported widths. */
template <class element_t> void require_exhaustive_lane_sign_bits_type()
{
	require_exhaustive_lane_sign_bits<element_t, 128>();
	if constexpr (SIMDLIB_REGISTER_TEST_ENABLE_256)
		require_exhaustive_lane_sign_bits<element_t, 256>();
}

/** @brief Verifies representative byte-lane masks while independently varying every lane's non-sign payload. */
template <class element_t, std::size_t bits>
	requires(std::is_integral_v<element_t> && sizeof(element_t) == 1)
void require_byte_lane_sign_bits()
{
	using register_type = SimdLib::Register<element_t, bits>;
	constexpr std::uint32_t all_lane_bits = []() constexpr noexcept
	{
		if constexpr (register_type::lane_count == 32)
			return std::numeric_limits<std::uint32_t>::max();
		else
			return (std::uint32_t{1} << register_type::lane_count) - 1;
	}();
	constexpr std::array masks{std::uint32_t{}, all_lane_bits, std::uint32_t{1}, std::uint32_t{1} << (register_type::lane_count - 1),
		std::uint32_t{0xA5A5A5A5} & all_lane_bits, std::uint32_t{0x3CC35AA5} & all_lane_bits};

	for (const std::uint32_t expected : masks)
	{
		std::array<element_t, register_type::lane_count> sparse_payload{};
		std::array<element_t, register_type::lane_count> dense_payload{};
		for (std::size_t lane = 0; lane < register_type::lane_count; ++lane)
		{
			const auto sign = static_cast<std::uint8_t>(((expected >> lane) & 1U) != 0 ? 0x80U : 0U);
			sparse_payload[lane] = scalar_from_bits<element_t>(static_cast<std::uint8_t>(sign | ((lane * 5U + 1U) & 0x7FU)));
			dense_payload[lane] =
				scalar_from_bits<element_t>(static_cast<std::uint8_t>(sign | (0x7FU - static_cast<unsigned>((lane * 11U) & 0x7FU))));
		}

		REQUIRE(register_type::from_array(sparse_payload).lane_sign_bits() == expected);
		REQUIRE(register_type::from_array(dense_payload).lane_sign_bits() == expected);
	}
}

/** @brief Runs byte-lane sign-mask coverage for signed and unsigned elements at both supported widths. */
template <class element_t> void require_byte_lane_sign_bits_type()
{
	require_byte_lane_sign_bits<element_t, 128>();
	if constexpr (SIMDLIB_REGISTER_TEST_ENABLE_256)
		require_byte_lane_sign_bits<element_t, 256>();
}

/** @brief Runs per-lane shift coverage at both supported register widths. */
template <class element_t> void require_shift_type()
{
	require_lane_shifts<element_t, 128>();
	if constexpr (SIMDLIB_REGISTER_TEST_ENABLE_256)
		require_lane_shifts<element_t, 256>();
}

TEST_CASE("Register arithmetic matches Api and independent scalar edge-case oracles", "[simdlib][register][arithmetic]")
{
	require_arithmetic_type<std::int8_t>();
	require_arithmetic_type<std::uint8_t>();
	require_arithmetic_type<std::int16_t>();
	require_arithmetic_type<std::uint16_t>();
	require_arithmetic_type<std::int32_t>();
	require_arithmetic_type<std::uint32_t>();
	require_arithmetic_type<std::int64_t>();
	require_arithmetic_type<std::uint64_t>();
	require_arithmetic_type<float>();
	require_arithmetic_type<double>();
}

TEST_CASE("Register bitwise operations and sign masks preserve exact bits", "[simdlib][register][bitwise][movemask]")
{
	require_bitwise_type<std::int8_t>();
	require_bitwise_type<std::uint8_t>();
	require_bitwise_type<std::int16_t>();
	require_bitwise_type<std::uint16_t>();
	require_bitwise_type<std::int32_t>();
	require_bitwise_type<std::uint32_t>();
	require_bitwise_type<std::int64_t>();
	require_bitwise_type<std::uint64_t>();
	require_bitwise_type<float>();
	require_bitwise_type<double>();
}

TEST_CASE("Register lane sign masks exhaustively ignore non-sign integer payload bits", "[simdlib][register][movemask][exhaustive]")
{
	require_exhaustive_lane_sign_bits_type<std::int16_t>();
	require_exhaustive_lane_sign_bits_type<std::uint16_t>();
	require_exhaustive_lane_sign_bits_type<std::int32_t>();
	require_exhaustive_lane_sign_bits_type<std::uint32_t>();
	require_exhaustive_lane_sign_bits_type<std::int64_t>();
	require_exhaustive_lane_sign_bits_type<std::uint64_t>();
}

TEST_CASE("Register byte lane sign masks preserve lane order and ignore payload bits", "[simdlib][register][movemask][byte]")
{
	require_byte_lane_sign_bits_type<std::int8_t>();
	require_byte_lane_sign_bits_type<std::uint8_t>();
}

TEST_CASE("Register shifts match lane and complete-register boundary contracts", "[simdlib][register][shift]")
{
	require_shift_type<std::int8_t>();
	require_shift_type<std::uint8_t>();
	require_shift_type<std::int16_t>();
	require_shift_type<std::uint16_t>();
	require_shift_type<std::int32_t>();
	require_shift_type<std::uint32_t>();
	require_shift_type<std::int64_t>();
	require_shift_type<std::uint64_t>();
	require_complete_register_shifts();
}

} // namespace
