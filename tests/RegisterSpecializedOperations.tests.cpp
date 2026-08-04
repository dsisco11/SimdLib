#include <SimdLib/IRegister.h>
#include <SimdLib/Register.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#ifndef SIMDLIB_REGISTER_TEST_ENABLE_256
#define SIMDLIB_REGISTER_TEST_ENABLE_256 SIMDLIB_HAS_AVX2
#endif
#if SIMDLIB_REGISTER_TEST_ENABLE_256
#define SIMDLIB_REGISTER_IF_256(...) __VA_ARGS__
#else
#define SIMDLIB_REGISTER_IF_256(...)
#endif

namespace
{

/** @brief Reports whether one constrained promoted-result alias is available. */
template <class element_t, std::size_t bits>
concept has_multiply_add_adjacent_alias = requires { typename SimdLib::multiply_add_adjacent_result_t<element_t, bits>; };
template <class element_t, std::size_t bits>
concept has_byte_multiply_add_alias = requires { typename SimdLib::byte_multiply_add_result_t<element_t, bits>; };
template <class element_t, std::size_t bits>
concept has_sad_alias = requires { typename SimdLib::sad_result_t<element_t, bits>; };
template <class element_t, std::size_t bits>
concept has_multi_sad_alias = requires { typename SimdLib::multi_sad_result_t<element_t, bits>; };

/** @brief Mirrors the proposal's adjacent multiply-add lane promotion mapping. */
template <class element_t>
using expected_adjacent_element_t = std::conditional_t<
	(sizeof(element_t) >= sizeof(std::int64_t)), element_t,
	std::conditional_t<std::is_signed_v<element_t>,
					   std::conditional_t<sizeof(element_t) == 1, std::int16_t, std::conditional_t<sizeof(element_t) == 2, std::int32_t, std::int64_t>>,
					   std::conditional_t<sizeof(element_t) == 1, std::uint16_t, std::conditional_t<sizeof(element_t) == 2, std::uint32_t, std::uint64_t>>>>;

/** @brief Verifies availability parity and exact promoted result mappings for one source shape. */
template <class element_t, std::size_t bits> consteval bool validate_specialized_surface()
{
	using register_t = SimdLib::Register<element_t, bits>;
	using api_t = SimdLib::Api<bits, element_t>;
	using other_element_t = std::conditional_t<std::same_as<element_t, std::int8_t>, std::uint8_t, std::int8_t>;

	static_assert(SimdLib::IRegister::Add<register_t> == SimdLib::IApi::Add<api_t>);
	static_assert(SimdLib::IRegister::Subtract<register_t> == SimdLib::IApi::Subtract<api_t>);
	static_assert(SimdLib::IRegister::Multiply<register_t> == SimdLib::IApi::Multiply<api_t>);
	static_assert(SimdLib::IRegister::Divide<register_t> == SimdLib::IApi::Divide<api_t>);
	static_assert(SimdLib::IRegister::Modulus<register_t> == SimdLib::IApi::Modulus<api_t>);
	static_assert(SimdLib::IRegister::Negate<register_t> == SimdLib::IApi::Negate<api_t>);
	static_assert(SimdLib::IRegister::Min<register_t> == SimdLib::IApi::Min<api_t>);
	static_assert(SimdLib::IRegister::Max<register_t> == SimdLib::IApi::Max<api_t>);
	static_assert(SimdLib::IRegister::Absolute<register_t> == SimdLib::IApi::Absolute<api_t>);
	static_assert(SimdLib::IRegister::Sqrt<register_t> == SimdLib::IApi::Sqrt<api_t>);
	static_assert(SimdLib::IRegister::Average<register_t> == SimdLib::IApi::Average<api_t>);
	static_assert(SimdLib::IRegister::MultiplyAdd<register_t> == SimdLib::IApi::MultiplyAdd<api_t>);
	static_assert(SimdLib::IRegister::Magnitude<register_t> == SimdLib::IApi::Magnitude<api_t>);
	static_assert(SimdLib::IRegister::MagnitudeChecked<register_t> == SimdLib::IApi::MagnitudeChecked<api_t>);
	static_assert(SimdLib::IRegister::Normalize<register_t> == SimdLib::IApi::Normalize<api_t>);
	static_assert(SimdLib::IRegister::HorizontalAdd<register_t> == SimdLib::IApi::HorizontalAdd<api_t>);
	static_assert(SimdLib::IRegister::HorizontalSubtract<register_t> == SimdLib::IApi::HorizontalSubtract<api_t>);
	static_assert(SimdLib::IRegister::MinPosition<register_t> == SimdLib::IApi::MinPosition<api_t>);
	static_assert(SimdLib::IRegister::MaxPosition<register_t> == SimdLib::IApi::MaxPosition<api_t>);
	static_assert(SimdLib::IRegister::AddSaturated<register_t> == SimdLib::IApi::AddSaturated<api_t>);
	static_assert(SimdLib::IRegister::SubtractSaturated<register_t> == SimdLib::IApi::SubtractSaturated<api_t>);
	static_assert(SimdLib::IRegister::HorizontalAddSaturated<register_t> == SimdLib::IApi::HorizontalAddSaturated<api_t>);
	static_assert(SimdLib::IRegister::HorizontalSubtractSaturated<register_t> == SimdLib::IApi::HorizontalSubtractSaturated<api_t>);
	static_assert(SimdLib::IRegister::AddSubtract<register_t> == SimdLib::IApi::AddSubtract<api_t>);
	static_assert(SimdLib::IRegister::DotProduct<register_t, 0x11> == SimdLib::IApi::DotProduct<api_t, 0x11>);
	static_assert(SimdLib::IRegister::DotProduct<register_t, 0> == SimdLib::IApi::DotProduct<api_t, 0>);
	static_assert(SimdLib::IRegister::DotProduct<register_t, 255> == SimdLib::IApi::DotProduct<api_t, 255>);
	static_assert(SimdLib::IRegister::MultiSumAbsoluteByteDifferences<register_t, 0> == SimdLib::IApi::MultiSad<api_t, 0>);
	static_assert(SimdLib::IRegister::MultiSumAbsoluteByteDifferences<register_t, 255> == SimdLib::IApi::MultiSad<api_t, 255>);
	static_assert(!SimdLib::IRegister::DotProduct<register_t, -1>);
	static_assert(!SimdLib::IRegister::DotProduct<register_t, 256>);
	static_assert(!SimdLib::IRegister::MultiSumAbsoluteByteDifferences<register_t, -1>);
	static_assert(!SimdLib::IRegister::MultiSumAbsoluteByteDifferences<register_t, 256>);
	static_assert(!SimdLib::IRegister::MultiplyAddAdjacent<register_t, other_element_t>);
	static_assert(!SimdLib::IRegister::MultiplyAddUnsignedSignedBytes<register_t, other_element_t>);
	static_assert(!SimdLib::IRegister::SumAbsoluteByteDifferences<register_t, other_element_t>);
	static_assert(!SimdLib::IRegister::MultiSumAbsoluteByteDifferences<register_t, 0, other_element_t>);

	static_assert(has_multiply_add_adjacent_alias<element_t, bits> == (std::is_integral_v<element_t> && SimdLib::IApi::MultiplyAddAdjacent<api_t>));
	static_assert(has_byte_multiply_add_alias<element_t, bits> == (std::is_integral_v<element_t> && SimdLib::IApi::ByteMultiplyAdd<api_t>));
	static_assert(has_sad_alias<element_t, bits> == (std::is_integral_v<element_t> && SimdLib::IApi::Sad<api_t>));
	static_assert(has_multi_sad_alias<element_t, bits> == (std::is_integral_v<element_t> && SimdLib::IApi::MultiSad<api_t, 0>));
	static_assert(SimdLib::IRegister::MultiplyAddAdjacent<register_t> == SimdLib::IApi::MultiplyAddAdjacent<api_t>);
	static_assert(SimdLib::IRegister::MultiplyAddUnsignedSignedBytes<register_t> == SimdLib::IApi::ByteMultiplyAdd<api_t>);
	static_assert(SimdLib::IRegister::SumAbsoluteByteDifferences<register_t> == SimdLib::IApi::Sad<api_t>);
	static_assert(SimdLib::IRegister::MultiSumAbsoluteByteDifferences<register_t, 0> == SimdLib::IApi::MultiSad<api_t, 0>);

	if constexpr (has_multiply_add_adjacent_alias<element_t, bits>)
	{
		using result_t = SimdLib::multiply_add_adjacent_result_t<element_t, bits>;
		static_assert(std::same_as<result_t, SimdLib::Register<expected_adjacent_element_t<element_t>, bits>>);
		static_assert(std::same_as<decltype(std::declval<register_t>().multiply_add_adjacent(std::declval<register_t>())), result_t>);
	}
	if constexpr (has_byte_multiply_add_alias<element_t, bits>)
	{
		using result_t = SimdLib::byte_multiply_add_result_t<element_t, bits>;
		static_assert(std::same_as<result_t, SimdLib::Register<std::int16_t, bits>>);
		static_assert(std::same_as<decltype(std::declval<register_t>().multiply_add_unsigned_signed_bytes(std::declval<register_t>())), result_t>);
	}
	if constexpr (has_sad_alias<element_t, bits>)
	{
		using result_t = SimdLib::sad_result_t<element_t, bits>;
		static_assert(std::same_as<result_t, SimdLib::Register<std::uint64_t, bits>>);
		static_assert(std::same_as<decltype(std::declval<register_t>().sum_absolute_byte_differences(std::declval<register_t>())), result_t>);
	}
	if constexpr (has_multi_sad_alias<element_t, bits>)
	{
		using result_t = SimdLib::multi_sad_result_t<element_t, bits>;
		static_assert(std::same_as<result_t, SimdLib::Register<std::uint16_t, bits>>);
		static_assert(std::same_as<decltype(std::declval<register_t>().template multi_sum_absolute_byte_differences<0>(std::declval<register_t>())), result_t>);
		static_assert(
			std::same_as<decltype(std::declval<register_t>().template multi_sum_absolute_byte_differences<255>(std::declval<register_t>())), result_t>);
	}
	return true;
}

#define SIMDLIB_VALIDATE_SPECIALIZED_TYPE(type)                                                                                                                \
	static_assert(validate_specialized_surface<type, 128>());                                                                                                  \
	SIMDLIB_REGISTER_IF_256(static_assert(validate_specialized_surface<type, 256>());)
SIMDLIB_VALIDATE_SPECIALIZED_TYPE(std::int8_t);
SIMDLIB_VALIDATE_SPECIALIZED_TYPE(std::uint8_t);
SIMDLIB_VALIDATE_SPECIALIZED_TYPE(std::int16_t);
SIMDLIB_VALIDATE_SPECIALIZED_TYPE(std::uint16_t);
SIMDLIB_VALIDATE_SPECIALIZED_TYPE(std::int32_t);
SIMDLIB_VALIDATE_SPECIALIZED_TYPE(std::uint32_t);
SIMDLIB_VALIDATE_SPECIALIZED_TYPE(std::int64_t);
SIMDLIB_VALIDATE_SPECIALIZED_TYPE(std::uint64_t);
SIMDLIB_VALIDATE_SPECIALIZED_TYPE(float);
SIMDLIB_VALIDATE_SPECIALIZED_TYPE(double);
#undef SIMDLIB_VALIDATE_SPECIALIZED_TYPE

/** @brief Returns the exact nearest integer square root of a bounded unsigned square sum. */
constexpr std::uint64_t rounded_integer_sqrt(const std::uint64_t total, const std::uint64_t maximum) noexcept
{
	std::uint64_t low = 0;
	std::uint64_t high = maximum;
	while (low < high)
	{
		const std::uint64_t middle = low + (high - low + 1) / 2;
		if (middle <= total / middle)
			low = middle;
		else
			high = middle - 1;
	}
	return low < maximum && total > low * low + low ? low + 1 : low;
}

/** @brief Converts one signed or unsigned integer lane to its exact unsigned magnitude. */
template <class element_t> constexpr std::uint64_t unsigned_lane_magnitude(const element_t value) noexcept
{
	using unsigned_t = std::make_unsigned_t<element_t>;
	const unsigned_t bits = static_cast<unsigned_t>(value);
	if constexpr (std::is_signed_v<element_t>)
		return value < 0 ? static_cast<std::uint64_t>(static_cast<unsigned_t>(unsigned_t{0} - bits)) : static_cast<std::uint64_t>(bits);
	else
		return static_cast<std::uint64_t>(bits);
}

/** @brief Compares checked Register magnitudes with an independent threshold-clamped scalar oracle. */
template <class element_t, std::size_t bits>
void require_checked_magnitude_oracle(const std::array<element_t, SimdLib::Register<element_t, bits>::lane_count> &input)
{
	using register_t = SimdLib::Register<element_t, bits>;
	using unsigned_t = std::make_unsigned_t<element_t>;
	constexpr std::size_t groupLanes = 128 / (sizeof(element_t) * 8);
	constexpr std::uint64_t maximum = static_cast<std::uint64_t>(std::numeric_limits<element_t>::max());
	constexpr std::uint64_t threshold = maximum * maximum + maximum + 1;
	constexpr element_t overflowMask = std::bit_cast<element_t>(static_cast<unsigned_t>(~unsigned_t{0}));
	const auto actual = register_t::from_array(input).magnitude_checked().to_array();
	std::array<std::int64_t, register_t::lane_count> diagnosticInput{};
	std::transform(input.begin(), input.end(), diagnosticInput.begin(), [](const element_t value) { return static_cast<std::int64_t>(value); });
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		const std::size_t base = group * groupLanes;
		std::uint64_t total = 0;
		bool overflow = false;
		for (std::size_t lane = 0; lane < groupLanes; ++lane)
		{
			const std::uint64_t magnitude = unsigned_lane_magnitude(input[base + lane]);
			const std::uint64_t square = magnitude * magnitude;
			if (square >= threshold - total)
			{
				overflow = true;
				break;
			}
			total += square;
		}
		const std::uint64_t expectedMagnitude = overflow ? maximum : rounded_integer_sqrt(total, maximum);
		CAPTURE(sizeof(element_t), bits, group, total, overflow);
		CAPTURE(diagnosticInput);
		REQUIRE(actual[base] == static_cast<element_t>(expectedMagnitude));
		REQUIRE(actual[base + 1] == (overflow ? overflowMask : element_t{0}));
	}
}

/** @brief Verifies integer roots plus sparse fast and checked magnitude contracts for one Register shape. */
template <class element_t, std::size_t bits> void require_integer_roots_and_magnitude()
{
	using register_t = SimdLib::Register<element_t, bits>;
	using unsigned_t = std::make_unsigned_t<element_t>;
	constexpr std::size_t groupLanes = 128 / (sizeof(element_t) * 8);
	constexpr element_t maximum = std::numeric_limits<element_t>::max();
	constexpr element_t overflowMask = std::bit_cast<element_t>(static_cast<unsigned_t>(~unsigned_t{0}));
	std::array<element_t, register_t::lane_count> roots{};
	std::array<element_t, register_t::lane_count> squares{};
	std::array<element_t, register_t::lane_count> safeInput{};
	std::array<element_t, register_t::lane_count> boundaryInput{};
	std::array<element_t, register_t::lane_count> nearBoundaryInput{};
	std::array<element_t, register_t::lane_count> overflowInput{};
	std::array<element_t, register_t::lane_count> roundingDownInput{};
	std::array<element_t, register_t::lane_count> roundingUpInput{};
	for (std::size_t index = 0; index < roots.size(); ++index)
	{
		roots[index] = static_cast<element_t>(index % 10);
		squares[index] = static_cast<element_t>(roots[index] * roots[index]);
		overflowInput[index] = maximum;
	}
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		const std::size_t base = group * groupLanes;
		safeInput[base] = element_t{3};
		safeInput[base + 1] = element_t{4};
		boundaryInput[base] = maximum;
		nearBoundaryInput[base] = maximum;
		nearBoundaryInput[base + 1] = element_t{1};
		roundingDownInput[base] = element_t{1};
		roundingDownInput[base + 1] = element_t{1};
		roundingUpInput[base] = element_t{2};
		roundingUpInput[base + 1] = element_t{2};
	}

	REQUIRE(register_t::from_array(squares).sqrt().to_array() == roots);
	const auto fast = register_t::from_array(safeInput).magnitude().to_array();
	const auto checkedSafe = register_t::from_array(safeInput).magnitude_checked().to_array();
	const auto checkedBoundary = register_t::from_array(boundaryInput).magnitude_checked().to_array();
	const auto checkedOverflow = register_t::from_array(overflowInput).magnitude_checked().to_array();
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		const std::size_t base = group * groupLanes;
		REQUIRE(fast[base] == element_t{5});
		REQUIRE(checkedSafe[base] == element_t{5});
		REQUIRE(checkedSafe[base + 1] == element_t{0});
		REQUIRE(checkedBoundary[base] == maximum);
		REQUIRE(checkedBoundary[base + 1] == element_t{0});
		REQUIRE(checkedOverflow[base] == maximum);
		REQUIRE(checkedOverflow[base + 1] == overflowMask);
	}

	if constexpr (sizeof(element_t) <= 4)
	{
		require_checked_magnitude_oracle<element_t, bits>(safeInput);
		require_checked_magnitude_oracle<element_t, bits>(boundaryInput);
		require_checked_magnitude_oracle<element_t, bits>(nearBoundaryInput);
		require_checked_magnitude_oracle<element_t, bits>(overflowInput);
		require_checked_magnitude_oracle<element_t, bits>(roundingDownInput);
		require_checked_magnitude_oracle<element_t, bits>(roundingUpInput);
		for (std::uint64_t caseIndex = 0; caseIndex < 8; ++caseIndex)
		{
			std::array<element_t, register_t::lane_count> generated{};
			std::uint64_t state = 0x9E37'79B9'7F4A'7C15ULL ^ (caseIndex * 0xD1B5'4A32'D192'ED03ULL);
			for (std::size_t lane = 0; lane < generated.size(); ++lane)
			{
				state ^= state >> 12;
				state ^= state << 25;
				state ^= state >> 27;
				unsigned_t laneBits = static_cast<unsigned_t>(state * 0x2545'F491'4F6C'DD1DULL);
				if ((caseIndex & 1U) == 0)
				{
					const unsigned_t safeMaximum = static_cast<unsigned_t>(static_cast<unsigned_t>(maximum) / groupLanes);
					laneBits = static_cast<unsigned_t>(laneBits % (safeMaximum + unsigned_t{1}));
					if constexpr (std::is_signed_v<element_t>)
						if ((lane & 1U) != 0)
							laneBits = unsigned_t{0} - laneBits;
				}
				generated[lane] = std::bit_cast<element_t>(laneBits);
			}
			require_checked_magnitude_oracle<element_t, bits>(generated);
		}
	}

	if constexpr (std::is_signed_v<element_t>)
	{
		std::array<element_t, register_t::lane_count> minimumInput{};
		for (std::size_t group = 0; group < bits / 128; ++group)
			minimumInput[group * groupLanes] = std::numeric_limits<element_t>::min();
		const auto checkedMinimum = register_t::from_array(minimumInput).magnitude_checked().to_array();
		for (std::size_t group = 0; group < bits / 128; ++group)
		{
			const std::size_t base = group * groupLanes;
			REQUIRE(checkedMinimum[base] == maximum);
			REQUIRE(checkedMinimum[base + 1] == overflowMask);
		}
		if constexpr (sizeof(element_t) <= 4)
			require_checked_magnitude_oracle<element_t, bits>(minimumInput);
	}
}
/** @brief Returns one source value represented modulo the adjacent-result lane width. */
template <class result_t, class element_t> constexpr std::make_unsigned_t<result_t> adjacent_operand_bits(const element_t value) noexcept
{
	return static_cast<std::make_unsigned_t<result_t>>(static_cast<result_t>(value));
}

/** @brief Verifies promoted adjacent multiply-add lane order, signedness, padding, and modular overflow. */
template <class element_t, std::size_t bits> void require_adjacent_multiply_add_contract()
{
	using source_register = SimdLib::Register<element_t, bits>;
	using result_register = SimdLib::multiply_add_adjacent_result_t<element_t, bits>;
	using result_t = typename result_register::element_type;
	using unsigned_result_t = std::make_unsigned_t<result_t>;
	constexpr std::size_t sourceGroupLanes = 128 / (sizeof(element_t) * 8);
	constexpr std::size_t resultGroupLanes = 128 / (sizeof(result_t) * 8);
	std::array<element_t, source_register::lane_count> lhs{};
	std::array<element_t, source_register::lane_count> rhs{};
	std::array<unsigned_result_t, result_register::lane_count> expectedBits{};
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		const std::size_t sourceBase = group * sourceGroupLanes;
		const element_t overflowValue = []
		{
			if constexpr (std::is_signed_v<element_t>)
				return std::numeric_limits<element_t>::lowest();
			else
				return std::numeric_limits<element_t>::max();
		}();
		std::fill_n(lhs.begin() + static_cast<std::ptrdiff_t>(sourceBase), sourceGroupLanes, overflowValue);
		std::fill_n(rhs.begin() + static_cast<std::ptrdiff_t>(sourceBase), sourceGroupLanes, overflowValue);
		if constexpr (std::is_signed_v<element_t> && sourceGroupLanes >= 4)
		{
			lhs[sourceBase + 2] = element_t{-3};
			lhs[sourceBase + 3] = element_t{4};
			rhs[sourceBase + 2] = element_t{5};
			rhs[sourceBase + 3] = element_t{-6};
		}
		for (std::size_t pair = 0; pair < sourceGroupLanes / 2; ++pair)
		{
			const std::size_t sourceIndex = sourceBase + pair * 2;
			const std::size_t resultIndex = group * resultGroupLanes + pair;
			const std::uint64_t lowProduct = static_cast<std::uint64_t>(adjacent_operand_bits<result_t>(lhs[sourceIndex])) *
											 static_cast<std::uint64_t>(adjacent_operand_bits<result_t>(rhs[sourceIndex]));
			const std::uint64_t highProduct = static_cast<std::uint64_t>(adjacent_operand_bits<result_t>(lhs[sourceIndex + 1])) *
											  static_cast<std::uint64_t>(adjacent_operand_bits<result_t>(rhs[sourceIndex + 1]));
			expectedBits[resultIndex] = static_cast<unsigned_result_t>(lowProduct + highProduct);
		}
	}
	const auto actual = source_register::from_array(lhs).multiply_add_adjacent(source_register::from_array(rhs)).to_array();
	for (std::size_t index = 0; index < actual.size(); ++index)
		REQUIRE(std::bit_cast<unsigned_result_t>(actual[index]) == expectedBits[index]);
}

/** @brief Computes an independent MPSADBW oracle for one immediate and Register width. */
template <int imm8, std::size_t bits>
std::array<std::uint16_t, bits / 16> multi_sad_oracle(const std::array<std::uint8_t, bits / 8> &lhs, const std::array<std::uint8_t, bits / 8> &rhs)
{
	std::array<std::uint16_t, bits / 16> expected{};
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		const unsigned control = (static_cast<unsigned>(imm8) >> (group * 3)) & 0x7U;
		const std::size_t groupBase = group * 16;
		const std::size_t lhsBase = groupBase + ((control >> 2) & 0x1U) * 4;
		const std::size_t rhsBase = groupBase + (control & 0x3U) * 4;
		for (std::size_t output = 0; output < 8; ++output)
		{
			for (std::size_t offset = 0; offset < 4; ++offset)
			{
				expected[group * 8 + output] +=
					static_cast<std::uint16_t>(std::abs(static_cast<int>(lhs[lhsBase + output + offset]) - static_cast<int>(rhs[rhsBase + offset])));
			}
		}
	}
	return expected;
}

/** @brief Verifies one MPSADBW immediate against the independent byte-window oracle. */
template <int imm8, std::size_t bits> void require_multi_sad_immediate()
{
	using bytes = SimdLib::Register<std::uint8_t, bits>;
	std::array<std::uint8_t, bytes::lane_count> lhs{};
	std::array<std::uint8_t, bytes::lane_count> rhs{};
	for (std::size_t index = 0; index < lhs.size(); ++index)
	{
		lhs[index] = static_cast<std::uint8_t>((index * 17 + 3) % 251);
		rhs[index] = static_cast<std::uint8_t>((index * 29 + 11) % 253);
	}
	const auto actual = bytes::from_array(lhs).template multi_sum_absolute_byte_differences<imm8>(bytes::from_array(rhs)).to_array();
	REQUIRE(actual == multi_sad_oracle<imm8, bits>(lhs, rhs));
}

/** @brief Computes the intrinsic-selected dot-product result independently for one immediate. */
template <int imm8, class element_t, std::size_t bits>
std::array<element_t, SimdLib::Register<element_t, bits>::lane_count> dot_product_oracle(
	const std::array<element_t, SimdLib::Register<element_t, bits>::lane_count> &lhs,
	const std::array<element_t, SimdLib::Register<element_t, bits>::lane_count> &rhs)
{
	using register_t = SimdLib::Register<element_t, bits>;
	constexpr std::size_t groupLanes = 128 / (sizeof(element_t) * 8);
	std::array<element_t, register_t::lane_count> expected{};
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		element_t total{};
		for (std::size_t lane = 0; lane < groupLanes; ++lane)
		{
			if ((imm8 & (1 << (lane + 4))) != 0)
				total += lhs[group * groupLanes + lane] * rhs[group * groupLanes + lane];
		}
		for (std::size_t lane = 0; lane < groupLanes; ++lane)
		{
			if ((imm8 & (1 << lane)) != 0)
				expected[group * groupLanes + lane] = total;
		}
	}
	return expected;
}

/** @brief Verifies one dot-product immediate against an independent selection-and-reduction oracle. */
template <int imm8, class element_t, std::size_t bits>
void require_dot_product_immediate(const std::array<element_t, SimdLib::Register<element_t, bits>::lane_count> &lhs,
								   const std::array<element_t, SimdLib::Register<element_t, bits>::lane_count> &rhs)
{
	using register_t = SimdLib::Register<element_t, bits>;
	const auto actual = register_t::from_array(lhs).template dot_product<imm8>(register_t::from_array(rhs)).to_array();
	REQUIRE(actual == dot_product_oracle<imm8, element_t, bits>(lhs, rhs));
}
/** @brief Verifies extrema and absolute-value lane semantics for one supported source shape. */
template <class element_t, std::size_t bits> void require_extrema_and_absolute_contract()
{
	using register_t = SimdLib::Register<element_t, bits>;
	std::array<element_t, register_t::lane_count> lhs{};
	std::array<element_t, register_t::lane_count> rhs{};
	std::array<element_t, register_t::lane_count> minima{};
	std::array<element_t, register_t::lane_count> maxima{};
	std::array<element_t, register_t::lane_count> absolutes{};
	for (std::size_t index = 0; index < lhs.size(); ++index)
	{
		if constexpr (std::is_unsigned_v<element_t>)
			lhs[index] = static_cast<element_t>(index * 3 + 1);
		else
			lhs[index] = static_cast<element_t>((index % 2 == 0 ? -1 : 1) * static_cast<int>(index + 1));
		rhs[index] = static_cast<element_t>(index + 2);
		minima[index] = std::min(lhs[index], rhs[index]);
		maxima[index] = std::max(lhs[index], rhs[index]);
		if constexpr (std::is_unsigned_v<element_t>)
			absolutes[index] = lhs[index];
		else
			absolutes[index] = static_cast<element_t>(std::abs(lhs[index]));
	}
	const register_t left = register_t::from_array(lhs);
	const register_t right = register_t::from_array(rhs);
	REQUIRE(left.min(right).to_array() == minima);
	REQUIRE(left.max(right).to_array() == maxima);
	REQUIRE(left.absolute().to_array() == absolutes);
	if constexpr (std::is_integral_v<element_t> && std::is_signed_v<element_t>)
	{
		for (const auto value : register_t::broadcast(std::numeric_limits<element_t>::lowest()).absolute().to_array())
			REQUIRE(value == std::numeric_limits<element_t>::lowest());
	}
}

/** @brief Verifies rounded unsigned average semantics for one supported lane type. */
template <class element_t, std::size_t bits> void require_average_contract()
{
	using register_t = SimdLib::Register<element_t, bits>;
	std::array<element_t, register_t::lane_count> lhs{};
	std::array<element_t, register_t::lane_count> rhs{};
	std::array<element_t, register_t::lane_count> expected{};
	for (std::size_t index = 0; index < lhs.size(); ++index)
	{
		lhs[index] = static_cast<element_t>(index + 1);
		rhs[index] = static_cast<element_t>(index + 4);
		expected[index] = static_cast<element_t>((static_cast<unsigned>(lhs[index]) + static_cast<unsigned>(rhs[index]) + 1U) / 2U);
	}
	REQUIRE(register_t::from_array(lhs).average(register_t::from_array(rhs)).to_array() == expected);
}

/** @brief Verifies lane order and 128-bit grouping for one supported horizontal arithmetic type. */
template <class element_t, std::size_t bits> void require_horizontal_contract()
{
	using register_t = SimdLib::Register<element_t, bits>;
	constexpr std::size_t groupLanes = 128 / (sizeof(element_t) * 8);
	std::array<element_t, register_t::lane_count> lhs{};
	std::array<element_t, register_t::lane_count> rhs{};
	std::array<element_t, register_t::lane_count> expectedAdd{};
	std::array<element_t, register_t::lane_count> expectedSubtract{};
	for (std::size_t index = 0; index < lhs.size(); ++index)
	{
		lhs[index] = static_cast<element_t>(index + 2);
		rhs[index] = static_cast<element_t>(index + 20);
	}
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		const std::size_t base = group * groupLanes;
		const std::size_t half = groupLanes / 2;
		for (std::size_t pair = 0; pair < half; ++pair)
		{
			expectedAdd[base + pair] = static_cast<element_t>(lhs[base + pair * 2] + lhs[base + pair * 2 + 1]);
			expectedSubtract[base + pair] = static_cast<element_t>(lhs[base + pair * 2] - lhs[base + pair * 2 + 1]);
			expectedAdd[base + half + pair] = static_cast<element_t>(rhs[base + pair * 2] + rhs[base + pair * 2 + 1]);
			expectedSubtract[base + half + pair] = static_cast<element_t>(rhs[base + pair * 2] - rhs[base + pair * 2 + 1]);
		}
	}
	const register_t left = register_t::from_array(lhs);
	const register_t right = register_t::from_array(rhs);
	REQUIRE(left.horizontal_add(right).to_array() == expectedAdd);
	REQUIRE(left.horizontal_subtract(right).to_array() == expectedSubtract);
}
/** @brief Verifies extrema, absolute value, square root, average, and multiply-add behavior. */
template <std::size_t bits> void require_lane_specialized_arithmetic()
{
	using integers = SimdLib::Register<std::int32_t, bits>;
	std::array<std::int32_t, integers::lane_count> lhsValues{};
	std::array<std::int32_t, integers::lane_count> rhsValues{};
	for (std::size_t index = 0; index < lhsValues.size(); ++index)
	{
		lhsValues[index] = static_cast<std::int32_t>((index % 2 == 0 ? -1 : 1) * static_cast<int>(index + 2));
		rhsValues[index] = static_cast<std::int32_t>(5 - static_cast<int>(index));
	}
	const integers lhs = integers::from_array(lhsValues);
	const integers rhs = integers::from_array(rhsValues);
	std::array<std::int32_t, integers::lane_count> minima{};
	std::array<std::int32_t, integers::lane_count> maxima{};
	std::array<std::int32_t, integers::lane_count> absolutes{};
	for (std::size_t index = 0; index < lhsValues.size(); ++index)
	{
		minima[index] = std::min(lhsValues[index], rhsValues[index]);
		maxima[index] = std::max(lhsValues[index], rhsValues[index]);
		absolutes[index] =
			lhsValues[index] == std::numeric_limits<std::int32_t>::lowest() ? lhsValues[index] : static_cast<std::int32_t>(std::abs(lhsValues[index]));
	}
	REQUIRE(lhs.min(rhs).to_array() == minima);
	REQUIRE(lhs.max(rhs).to_array() == maxima);
	REQUIRE(lhs.absolute().to_array() == absolutes);

	using bytes = SimdLib::Register<std::uint8_t, bits>;
	const auto averaged = bytes::broadcast(2).average(bytes::broadcast(7)).to_array();
	for (const auto value : averaged)
		REQUIRE(value == 5);

	using floats = SimdLib::Register<float, bits>;
	std::array<float, floats::lane_count> squareValues{};
	std::array<float, floats::lane_count> rootValues{};
	for (std::size_t index = 0; index < squareValues.size(); ++index)
	{
		rootValues[index] = static_cast<float>(index + 1);
		squareValues[index] = rootValues[index] * rootValues[index];
	}
	REQUIRE(floats::from_array(squareValues).sqrt().to_array() == rootValues);
	const auto multiplyAdded = floats::broadcast(2.0F).multiply_add(floats::broadcast(3.0F), floats::broadcast(4.0F)).to_array();
	for (const auto value : multiplyAdded)
		REQUIRE(value == 10.0F);
}

/** @brief Verifies 128-bit grouping for magnitude, normalization, and horizontal operations. */
template <std::size_t bits> void require_grouped_operations()
{
	using floats = SimdLib::Register<float, bits>;
	std::array<float, floats::lane_count> values{};
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		const std::size_t base = group * 4;
		values[base] = group == 0 ? 3.0F : 5.0F;
		values[base + 1] = group == 0 ? 4.0F : 12.0F;
	}
	const auto magnitude = floats::from_array(values).magnitude().to_array();
	const auto normalized = floats::from_array(values).normalize().to_array();
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		const std::size_t base = group * 4;
		const float expectedMagnitude = group == 0 ? 5.0F : 13.0F;
		for (std::size_t offset = 0; offset < 4; ++offset)
			REQUIRE(magnitude[base + offset] == expectedMagnitude);
		REQUIRE(std::abs(normalized[base] - values[base] / expectedMagnitude) < 0.0001F);
		REQUIRE(std::abs(normalized[base + 1] - values[base + 1] / expectedMagnitude) < 0.0001F);
	}

	using integers = SimdLib::Register<std::int32_t, bits>;
	std::array<std::int32_t, integers::lane_count> lhs{};
	std::array<std::int32_t, integers::lane_count> rhs{};
	std::array<std::int32_t, integers::lane_count> expectedAdd{};
	std::array<std::int32_t, integers::lane_count> expectedSubtract{};
	for (std::size_t index = 0; index < lhs.size(); ++index)
	{
		lhs[index] = static_cast<std::int32_t>(index + 1);
		rhs[index] = static_cast<std::int32_t>(20 + index);
	}
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		const std::size_t base = group * 4;
		expectedAdd[base] = lhs[base] + lhs[base + 1];
		expectedAdd[base + 1] = lhs[base + 2] + lhs[base + 3];
		expectedAdd[base + 2] = rhs[base] + rhs[base + 1];
		expectedAdd[base + 3] = rhs[base + 2] + rhs[base + 3];
		expectedSubtract[base] = lhs[base] - lhs[base + 1];
		expectedSubtract[base + 1] = lhs[base + 2] - lhs[base + 3];
		expectedSubtract[base + 2] = rhs[base] - rhs[base + 1];
		expectedSubtract[base + 3] = rhs[base + 2] - rhs[base + 3];
	}
	const integers left = integers::from_array(lhs);
	const integers right = integers::from_array(rhs);
	REQUIRE(left.horizontal_add(right).to_array() == expectedAdd);
	REQUIRE(left.horizontal_subtract(right).to_array() == expectedSubtract);
}

/** @brief Verifies first-tie positions and unique highest-lane extrema for one integral shape. */
template <class element_t, std::size_t bits> void require_position_contract()
{
	using register_t = SimdLib::Register<element_t, bits>;
	CAPTURE(bits, sizeof(element_t), std::is_signed_v<element_t>);
	constexpr std::size_t minimumTiePosition = register_t::lane_count > 2 ? 1 : 0;
	constexpr std::size_t maximumTiePosition = register_t::lane_count > 2 ? 2 : 0;
	std::array<element_t, register_t::lane_count> values{};
	values.fill(element_t{5});
	values[minimumTiePosition] = element_t{1};
	values.back() = element_t{1};
	REQUIRE(register_t::from_array(values).min_position() == minimumTiePosition);
	values.fill(element_t{5});
	values[maximumTiePosition] = element_t{9};
	values.back() = element_t{9};
	REQUIRE(register_t::from_array(values).max_position() == maximumTiePosition);
	values.fill(element_t{5});
	values.back() = element_t{1};
	REQUIRE(register_t::from_array(values).min_position() == register_t::lane_count - 1);
	values.fill(element_t{5});
	values.back() = element_t{9};
	REQUIRE(register_t::from_array(values).max_position() == register_t::lane_count - 1);
}

/** @brief Verifies lane saturation and signed horizontal saturation. */
template <std::size_t bits> void require_saturation_contract()
{
	using signed_bytes = SimdLib::Register<std::int8_t, bits>;
	using unsigned_bytes = SimdLib::Register<std::uint8_t, bits>;
	using signed_words = SimdLib::Register<std::int16_t, bits>;
	using unsigned_words = SimdLib::Register<std::uint16_t, bits>;
	for (const auto value : signed_bytes::broadcast(120).add_saturated(signed_bytes::broadcast(20)).to_array())
		REQUIRE(value == std::numeric_limits<std::int8_t>::max());
	for (const auto value : unsigned_bytes::broadcast(3).subtract_saturated(unsigned_bytes::broadcast(9)).to_array())
		REQUIRE(value == 0);
	for (const auto value : signed_words::broadcast(-30'000).subtract_saturated(signed_words::broadcast(10'000)).to_array())
		REQUIRE(value == std::numeric_limits<std::int16_t>::lowest());
	for (const auto value : unsigned_words::broadcast(65'000).add_saturated(unsigned_words::broadcast(1'000)).to_array())
		REQUIRE(value == std::numeric_limits<std::uint16_t>::max());

	std::array<std::int16_t, signed_words::lane_count> left{};
	std::array<std::int16_t, signed_words::lane_count> right{};
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		const std::size_t base = group * 8;
		left[base] = 30'000;
		left[base + 1] = 10'000;
		left[base + 2] = -30'000;
		left[base + 3] = -10'000;
		right[base] = 30'000;
		right[base + 1] = -10'000;
	}
	const auto added = signed_words::from_array(left).horizontal_add_saturated(signed_words::from_array(right)).to_array();
	const auto subtracted = signed_words::from_array(left).horizontal_subtract_saturated(signed_words::from_array(right)).to_array();
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		const std::size_t base = group * 8;
		REQUIRE(added[base] == std::numeric_limits<std::int16_t>::max());
		REQUIRE(added[base + 1] == std::numeric_limits<std::int16_t>::lowest());
		REQUIRE(subtracted[base] == 20'000);
		REQUIRE(subtracted[base + 1] == -20'000);
	}
}

/** @brief Returns the independently computed unsigned 16-bit saturated sum. */
[[nodiscard]] constexpr std::uint16_t saturated_add_u16(std::uint16_t lhs, std::uint16_t rhs) noexcept
{
	const auto sum = static_cast<std::uint32_t>(lhs) + static_cast<std::uint32_t>(rhs);
	return static_cast<std::uint16_t>(std::min(sum, static_cast<std::uint32_t>(std::numeric_limits<std::uint16_t>::max())));
}

/** @brief Returns the independently computed unsigned 16-bit saturated difference. */
[[nodiscard]] constexpr std::uint16_t saturated_subtract_u16(std::uint16_t lhs, std::uint16_t rhs) noexcept
{
	return lhs < rhs ? std::uint16_t{0} : static_cast<std::uint16_t>(lhs - rhs);
}

/** @brief Verifies every unsigned horizontal saturation lane against independent scalar edge-case oracles. */
template <std::size_t bits> void require_unsigned_horizontal_saturation_contract()
{
	using register_t = SimdLib::Register<std::uint16_t, bits>;
	using pair_t = std::array<std::uint16_t, 2>;
	constexpr std::array pairCases{
		pair_t{0, 0},			pair_t{0, 1},			pair_t{1, 0},			pair_t{1, 1},			pair_t{1, 2},			pair_t{2, 1},
		pair_t{32'767, 32'768}, pair_t{32'768, 32'767}, pair_t{32'768, 32'768}, pair_t{65'535, 0},		pair_t{0, 65'535},		pair_t{65'535, 1},
		pair_t{1, 65'535},		pair_t{65'535, 65'535}, pair_t{40'000, 25'535}, pair_t{40'000, 25'536}, pair_t{12'345, 54'321}, pair_t{54'321, 12'345},
	};

	for (std::size_t rotation = 0; rotation < pairCases.size(); ++rotation)
	{
		std::array<std::uint16_t, register_t::lane_count> lhs{};
		std::array<std::uint16_t, register_t::lane_count> rhs{};
		std::array<std::uint16_t, register_t::lane_count> expectedAdd{};
		std::array<std::uint16_t, register_t::lane_count> expectedSubtract{};
		std::size_t caseIndex = rotation;
		for (std::size_t group = 0; group < bits / 128; ++group)
		{
			const std::size_t base = group * 8;
			for (std::size_t pair = 0; pair < 4; ++pair)
			{
				const auto &values = pairCases[caseIndex++ % pairCases.size()];
				lhs[base + pair * 2] = values[0];
				lhs[base + pair * 2 + 1] = values[1];
				expectedAdd[base + pair] = saturated_add_u16(values[0], values[1]);
				expectedSubtract[base + pair] = saturated_subtract_u16(values[0], values[1]);
			}
			for (std::size_t pair = 0; pair < 4; ++pair)
			{
				const auto &values = pairCases[caseIndex++ % pairCases.size()];
				rhs[base + pair * 2] = values[0];
				rhs[base + pair * 2 + 1] = values[1];
				expectedAdd[base + 4 + pair] = saturated_add_u16(values[0], values[1]);
				expectedSubtract[base + 4 + pair] = saturated_subtract_u16(values[0], values[1]);
			}
		}

		CAPTURE(bits, rotation);
		const auto lhsRegister = register_t::from_array(lhs);
		const auto rhsRegister = register_t::from_array(rhs);
		REQUIRE(lhsRegister.horizontal_add_saturated(rhsRegister).to_array() == expectedAdd);
		REQUIRE(lhsRegister.horizontal_subtract_saturated(rhsRegister).to_array() == expectedSubtract);
	}
}

/** @brief Verifies promoted multiply-add and byte-difference result grouping. */
template <std::size_t bits> void require_promoted_results()
{
	using words = SimdLib::Register<std::int16_t, bits>;
	using dwords = SimdLib::multiply_add_adjacent_result_t<std::int16_t, bits>;
	std::array<std::int16_t, words::lane_count> lhs{};
	std::array<std::int16_t, words::lane_count> rhs{};
	std::array<std::int32_t, dwords::lane_count> expected{};
	for (std::size_t index = 0; index < lhs.size(); ++index)
	{
		lhs[index] = static_cast<std::int16_t>(index + 1);
		rhs[index] = static_cast<std::int16_t>((index % 3) + 2);
	}
	for (std::size_t index = 0; index < expected.size(); ++index)
		expected[index] = static_cast<std::int32_t>(lhs[index * 2]) * rhs[index * 2] + static_cast<std::int32_t>(lhs[index * 2 + 1]) * rhs[index * 2 + 1];
	REQUIRE(words::from_array(lhs).multiply_add_adjacent(words::from_array(rhs)).to_array() == expected);

	using bytes = SimdLib::Register<std::uint8_t, bits>;
	std::array<std::uint8_t, bytes::lane_count> unsignedBytes{};
	std::array<std::uint8_t, bytes::lane_count> signedBytes{};
	std::array<std::int16_t, SimdLib::byte_multiply_add_result_t<std::uint8_t, bits>::lane_count> maddExpected{};
	for (std::size_t index = 0; index < unsignedBytes.size(); ++index)
	{
		unsignedBytes[index] = static_cast<std::uint8_t>((index % 5) + 1);
		signedBytes[index] = static_cast<std::uint8_t>(static_cast<std::int8_t>((index % 2 == 0) ? -3 : 4));
	}
	for (std::size_t index = 0; index < maddExpected.size(); ++index)
		maddExpected[index] = static_cast<std::int16_t>(static_cast<int>(unsignedBytes[index * 2]) * static_cast<std::int8_t>(signedBytes[index * 2]) +
														static_cast<int>(unsignedBytes[index * 2 + 1]) * static_cast<std::int8_t>(signedBytes[index * 2 + 1]));
	const bytes byteLhs = bytes::from_array(unsignedBytes);
	const bytes byteRhs = bytes::from_array(signedBytes);
	REQUIRE(byteLhs.multiply_add_unsigned_signed_bytes(byteRhs).to_array() == maddExpected);

	std::array<std::uint8_t, bytes::lane_count> saturatedLhs{};
	std::array<std::uint8_t, bytes::lane_count> saturatedRhs{};
	std::array<std::int16_t, SimdLib::byte_multiply_add_result_t<std::uint8_t, bits>::lane_count> saturatedExpected{};
	for (std::size_t index = 0; index < saturatedExpected.size(); ++index)
	{
		saturatedLhs[index * 2] = std::numeric_limits<std::uint8_t>::max();
		saturatedLhs[index * 2 + 1] = std::numeric_limits<std::uint8_t>::max();
		const std::int8_t signedFactor = index % 2 == 0 ? std::numeric_limits<std::int8_t>::max() : std::numeric_limits<std::int8_t>::lowest();
		saturatedRhs[index * 2] = static_cast<std::uint8_t>(signedFactor);
		saturatedRhs[index * 2 + 1] = static_cast<std::uint8_t>(signedFactor);
		saturatedExpected[index] = index % 2 == 0 ? std::numeric_limits<std::int16_t>::max() : std::numeric_limits<std::int16_t>::lowest();
	}
	REQUIRE(bytes::from_array(saturatedLhs).multiply_add_unsigned_signed_bytes(bytes::from_array(saturatedRhs)).to_array() == saturatedExpected);

	std::array<std::uint64_t, SimdLib::sad_result_t<std::uint8_t, bits>::lane_count> sadExpected{};
	for (std::size_t block = 0; block < sadExpected.size(); ++block)
		for (std::size_t offset = 0; offset < 8; ++offset)
			sadExpected[block] +=
				static_cast<std::uint64_t>(std::abs(static_cast<int>(unsignedBytes[block * 8 + offset]) - static_cast<int>(signedBytes[block * 8 + offset])));
	REQUIRE(byteLhs.sum_absolute_byte_differences(byteRhs).to_array() == sadExpected);

	require_multi_sad_immediate<0, bits>();
	require_multi_sad_immediate<0x1B, bits>();
	require_multi_sad_immediate<0x3F, bits>();
	require_multi_sad_immediate<255, bits>();
}

/** @brief Verifies alternating floating arithmetic and immediate-controlled dot-product output lanes. */
template <class element_t, std::size_t bits> void require_floating_specialized_operations()
{
	using register_t = SimdLib::Register<element_t, bits>;
	constexpr std::size_t groupLanes = 128 / (sizeof(element_t) * 8);
	std::array<element_t, register_t::lane_count> lhs{};
	std::array<element_t, register_t::lane_count> rhs{};
	for (std::size_t index = 0; index < rhs.size(); ++index)
	{
		lhs[index] = static_cast<element_t>(index % 4 + 1);
		rhs[index] = static_cast<element_t>(index % 5 + 2);
	}
	const auto alternating = register_t::broadcast(element_t{10}).add_subtract(register_t::from_array(rhs)).to_array();
	for (std::size_t index = 0; index < alternating.size(); ++index)
		REQUIRE(alternating[index] == (index % 2 == 0 ? element_t{10} - rhs[index] : element_t{10} + rhs[index]));

	std::array<element_t, register_t::lane_count> squares{};
	std::array<element_t, register_t::lane_count> roots{};
	std::array<element_t, register_t::lane_count> magnitudeInput{};
	for (std::size_t index = 0; index < squares.size(); ++index)
	{
		roots[index] = static_cast<element_t>(index % groupLanes + 1);
		squares[index] = roots[index] * roots[index];
	}
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		magnitudeInput[group * groupLanes] = element_t{3};
		magnitudeInput[group * groupLanes + 1] = element_t{4};
	}
	REQUIRE(register_t::from_array(squares).sqrt().to_array() == roots);
	const auto magnitudes = register_t::from_array(magnitudeInput).magnitude().to_array();
	const auto normalized = register_t::from_array(magnitudeInput).normalize().to_array();
	for (std::size_t group = 0; group < bits / 128; ++group)
	{
		const std::size_t base = group * groupLanes;
		for (std::size_t lane = 0; lane < groupLanes; ++lane)
			REQUIRE(magnitudes[base + lane] == element_t{5});
		REQUIRE(std::abs(normalized[base] - static_cast<element_t>(0.6)) < static_cast<element_t>(0.0001));
		REQUIRE(std::abs(normalized[base + 1] - static_cast<element_t>(0.8)) < static_cast<element_t>(0.0001));
	}
	for (const auto value :
		 register_t::broadcast(element_t{2}).multiply_add(register_t::broadcast(element_t{3}), register_t::broadcast(element_t{4})).to_array())
		REQUIRE(value == element_t{10});
	require_dot_product_immediate<0, element_t, bits>(lhs, rhs);
	require_dot_product_immediate<0x11, element_t, bits>(lhs, rhs);
	require_dot_product_immediate<0xD3, element_t, bits>(lhs, rhs);
	require_dot_product_immediate<255, element_t, bits>(lhs, rhs);
}

TEST_CASE("Register specialized lane arithmetic follows scalar semantics", "[simdlib][register][specialized][arithmetic]")
{
	require_lane_specialized_arithmetic<128>();
	require_grouped_operations<128>();
	SIMDLIB_REGISTER_IF_256(require_lane_specialized_arithmetic<256>();)
	SIMDLIB_REGISTER_IF_256(require_grouped_operations<256>();)
#define SIMDLIB_REQUIRE_EXTREMA_AND_ABSOLUTE(type)                                                                                                             \
	require_extrema_and_absolute_contract<type, 128>();                                                                                                        \
	SIMDLIB_REGISTER_IF_256(require_extrema_and_absolute_contract<type, 256>();)
	SIMDLIB_REQUIRE_EXTREMA_AND_ABSOLUTE(std::int8_t);
	SIMDLIB_REQUIRE_EXTREMA_AND_ABSOLUTE(std::uint8_t);
	SIMDLIB_REQUIRE_EXTREMA_AND_ABSOLUTE(std::int16_t);
	SIMDLIB_REQUIRE_EXTREMA_AND_ABSOLUTE(std::uint16_t);
	SIMDLIB_REQUIRE_EXTREMA_AND_ABSOLUTE(std::int32_t);
	SIMDLIB_REQUIRE_EXTREMA_AND_ABSOLUTE(std::uint32_t);
	SIMDLIB_REQUIRE_EXTREMA_AND_ABSOLUTE(std::int64_t);
	SIMDLIB_REQUIRE_EXTREMA_AND_ABSOLUTE(std::uint64_t);
	SIMDLIB_REQUIRE_EXTREMA_AND_ABSOLUTE(float);
	SIMDLIB_REQUIRE_EXTREMA_AND_ABSOLUTE(double);
#undef SIMDLIB_REQUIRE_EXTREMA_AND_ABSOLUTE
	require_average_contract<std::uint8_t, 128>();
	require_average_contract<std::uint16_t, 128>();
	SIMDLIB_REGISTER_IF_256(require_average_contract<std::uint8_t, 256>();)
	SIMDLIB_REGISTER_IF_256(require_average_contract<std::uint16_t, 256>();)
#define SIMDLIB_REQUIRE_HORIZONTAL(type)                                                                                                                       \
	require_horizontal_contract<type, 128>();                                                                                                                  \
	SIMDLIB_REGISTER_IF_256(require_horizontal_contract<type, 256>();)
	SIMDLIB_REQUIRE_HORIZONTAL(std::int16_t);
	SIMDLIB_REQUIRE_HORIZONTAL(std::uint16_t);
	SIMDLIB_REQUIRE_HORIZONTAL(std::int32_t);
	SIMDLIB_REQUIRE_HORIZONTAL(std::uint32_t);
	SIMDLIB_REQUIRE_HORIZONTAL(float);
	SIMDLIB_REQUIRE_HORIZONTAL(double);
#undef SIMDLIB_REQUIRE_HORIZONTAL
#define SIMDLIB_REQUIRE_INTEGER_ROOTS_AND_MAGNITUDE(type)                                                                                                      \
	require_integer_roots_and_magnitude<type, 128>();                                                                                                          \
	SIMDLIB_REGISTER_IF_256(require_integer_roots_and_magnitude<type, 256>();)
	SIMDLIB_REQUIRE_INTEGER_ROOTS_AND_MAGNITUDE(std::int8_t);
	SIMDLIB_REQUIRE_INTEGER_ROOTS_AND_MAGNITUDE(std::uint8_t);
	SIMDLIB_REQUIRE_INTEGER_ROOTS_AND_MAGNITUDE(std::int16_t);
	SIMDLIB_REQUIRE_INTEGER_ROOTS_AND_MAGNITUDE(std::uint16_t);
	SIMDLIB_REQUIRE_INTEGER_ROOTS_AND_MAGNITUDE(std::int32_t);
	SIMDLIB_REQUIRE_INTEGER_ROOTS_AND_MAGNITUDE(std::uint32_t);
	SIMDLIB_REQUIRE_INTEGER_ROOTS_AND_MAGNITUDE(std::int64_t);
	SIMDLIB_REQUIRE_INTEGER_ROOTS_AND_MAGNITUDE(std::uint64_t);
#undef SIMDLIB_REQUIRE_INTEGER_ROOTS_AND_MAGNITUDE
}

TEST_CASE("Register positions cover first ties and the highest lane", "[simdlib][register][specialized][position]")
{
#define SIMDLIB_REQUIRE_POSITIONS(type)                                                                                                                        \
	require_position_contract<type, 128>();                                                                                                                    \
	SIMDLIB_REGISTER_IF_256(require_position_contract<type, 256>();)
	SIMDLIB_REQUIRE_POSITIONS(std::int8_t);
	SIMDLIB_REQUIRE_POSITIONS(std::uint8_t);
	SIMDLIB_REQUIRE_POSITIONS(std::int16_t);
	SIMDLIB_REQUIRE_POSITIONS(std::uint16_t);
	SIMDLIB_REQUIRE_POSITIONS(std::int32_t);
	SIMDLIB_REQUIRE_POSITIONS(std::uint32_t);
	SIMDLIB_REQUIRE_POSITIONS(std::int64_t);
	SIMDLIB_REQUIRE_POSITIONS(std::uint64_t);
#undef SIMDLIB_REQUIRE_POSITIONS
}

TEST_CASE("Register saturation preserves lane and 128-bit grouping semantics", "[simdlib][register][specialized][saturation]")
{
	require_saturation_contract<128>();
	require_unsigned_horizontal_saturation_contract<128>();
	SIMDLIB_REGISTER_IF_256(require_saturation_contract<256>();)
	SIMDLIB_REGISTER_IF_256(require_unsigned_horizontal_saturation_contract<256>();)
}

TEST_CASE("Register promoted results preserve lane order and signedness", "[simdlib][register][specialized][promoted]")
{
	require_promoted_results<128>();
	SIMDLIB_REGISTER_IF_256(require_promoted_results<256>();)
#define SIMDLIB_REQUIRE_ADJACENT_CONTRACT(type)                                                                                                                \
	require_adjacent_multiply_add_contract<type, 128>();                                                                                                       \
	SIMDLIB_REGISTER_IF_256(require_adjacent_multiply_add_contract<type, 256>();)
	SIMDLIB_REQUIRE_ADJACENT_CONTRACT(std::int8_t);
	SIMDLIB_REQUIRE_ADJACENT_CONTRACT(std::uint8_t);
	SIMDLIB_REQUIRE_ADJACENT_CONTRACT(std::int16_t);
	SIMDLIB_REQUIRE_ADJACENT_CONTRACT(std::uint16_t);
	SIMDLIB_REQUIRE_ADJACENT_CONTRACT(std::int32_t);
	SIMDLIB_REQUIRE_ADJACENT_CONTRACT(std::uint32_t);
	SIMDLIB_REQUIRE_ADJACENT_CONTRACT(std::int64_t);
	SIMDLIB_REQUIRE_ADJACENT_CONTRACT(std::uint64_t);
#undef SIMDLIB_REQUIRE_ADJACENT_CONTRACT
}

TEST_CASE("Register floating specialized operations preserve immediate output behavior", "[simdlib][register][specialized][floating]")
{
	require_floating_specialized_operations<float, 128>();
	require_floating_specialized_operations<double, 128>();
	SIMDLIB_REGISTER_IF_256(require_floating_specialized_operations<float, 256>();)
	SIMDLIB_REGISTER_IF_256(require_floating_specialized_operations<double, 256>();)
}

} // namespace

#undef SIMDLIB_REGISTER_IF_256
