#pragma once

#include "../LogicalShuffleTestSupport.h"

#include <SimdLib/Api.h>
#include <SimdLib/SimdVector.h>

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>
#include <utility>

namespace SimdLib::Tests::Constexpr
{

/**
 * @brief Expands one logical selector array into an Api shuffle during constant evaluation.
 * @tparam api_t Api specialization under test.
 * @tparam selectors Logical source-lane selectors.
 * @tparam positions Output lane positions.
 * @param value Source native register.
 * @return Constant-evaluated shuffled native register.
 */
template <class api_t, auto selectors, std::size_t... positions>
[[nodiscard]] constexpr auto logical_shuffle_value(typename api_t::vector_t value, std::index_sequence<positions...>) noexcept
{
	return api_t::template shuffle<selectors[positions]...>(value);
}

/**
 * @brief Verifies one constant-evaluated Api shuffle against the scalar oracle.
 * @tparam Width SIMD register width in bits.
 * @tparam Element Logical lane type.
 * @tparam selectors Logical source-lane selectors.
 * @return True when every result lane preserves the oracle's object representation.
 */
template <std::size_t Width, class Element, auto selectors> [[nodiscard]] consteval bool logical_shuffle_case() noexcept
{
	using api_t = Api<Width, Element>;
	constexpr auto source = LogicalShuffle::distinct_lanes<Element, Width>();
	constexpr auto actual =
		api_t::to_array(logical_shuffle_value<api_t, selectors>(api_t::construct(source), std::make_index_sequence<api_t::element_count>{}));
	constexpr auto expected = LogicalShuffle::logical_shuffle_oracle<Element, Width, selectors>(source);
	return LogicalShuffle::same_object_representations(actual, expected);
}

/**
 * @brief Verifies nonidentity and repeated-selector constexpr logical shuffles.
 * @tparam Width SIMD register width in bits.
 * @tparam Element Logical lane type.
 * @return True when every independent scalar-oracle comparison succeeds.
 */
template <std::size_t Width, class Element> [[nodiscard]] consteval bool logical_shuffle_contract() noexcept
{
	if constexpr (Width == 256)
	{
		return logical_shuffle_case<Width, Element, LogicalShuffle::reverse_selectors<Element, Width>()>() &&
			   logical_shuffle_case<Width, Element, LogicalShuffle::repeated_selectors<Element, Width>()>() &&
			   logical_shuffle_case<Width, Element, LogicalShuffle::swap_half_selectors<Element>()>() &&
			   logical_shuffle_case<Width, Element, LogicalShuffle::mixed_half_selectors<Element>()>();
	}
	else
	{
		return logical_shuffle_case<Width, Element, LogicalShuffle::reverse_selectors<Element, Width>()>() &&
			   logical_shuffle_case<Width, Element, LogicalShuffle::repeated_selectors<Element, Width>()>();
	}
}

/**
 * @brief Creates a deterministic lane sequence for constexpr API contracts.
 * @tparam Width SIMD register width in bits.
 * @tparam Element SIMD lane type.
 * @return Lane values in increasing logical order.
 */
template <std::size_t Width, class Element> [[nodiscard]] constexpr auto lane_values() noexcept
{
	using simd = Api<Width, Element>;
	std::array<Element, simd::element_count> values{};
	for (std::size_t index = 0; index < values.size(); ++index)
		values[index] = static_cast<Element>(index + 1);
	return values;
}

/**
 * @brief Verifies the explicitly named portable lane helpers during constant evaluation.
 * @tparam Width SIMD register width in bits.
 * @tparam Element SIMD lane type.
 * @return True when get, set, and value-returning insertion preserve the expected lanes.
 */
template <std::size_t Width, class Element> [[nodiscard]] consteval bool detail_lane_helper_contract() noexcept
{
	using simd = Api<Width, Element>;
	constexpr auto values = lane_values<Width, Element>();
	auto value = Detail::register_from_array<typename simd::vector_t>(values);

	for (std::size_t index = 0; index < simd::element_count; ++index)
	{
		if (Detail::register_get_constexpr<Element>(value, index) != values[index])
			return false;
	}

	constexpr std::size_t last = simd::element_count - 1;
	value = Detail::register_insert_constexpr<Element>(value, values[0], last);
	if (Detail::register_get_constexpr<Element>(value, last) != values[0])
		return false;

	Detail::register_set_constexpr<Element>(value, 0, values[last]);
	return Detail::register_get_constexpr<Element>(value, 0) == values[last];
}

/**
 * @brief Verifies constexpr construction, transfer, broadcast, and element access.
 * @tparam Width SIMD register width in bits.
 * @tparam Element SIMD lane type.
 * @return True when the public construction contract holds.
 */
template <std::size_t Width, class Element> [[nodiscard]] consteval bool construction_contract() noexcept
{
	using simd = Api<Width, Element>;
	constexpr auto values = lane_values<Width, Element>();
	constexpr auto constructed = simd::construct(values);
	if (simd::to_array(constructed) != values)
		return false;
	constexpr auto partial = simd::template load_partial<simd::element_count - 1>(std::span<const Element>{values});
	auto partialExpected = values;
	partialExpected.back() = Element{};
	if (simd::to_array(partial) != partialExpected)
		return false;
	if (simd::to_array(simd::setzero()) != std::array<Element, simd::element_count>{})
		return false;

	std::array<Element, simd::element_count> broadcastExpected{};
	broadcastExpected.fill(static_cast<Element>(7));
	if (simd::to_array(simd::set1(static_cast<Element>(7))) != broadcastExpected)
		return false;

	constexpr auto setrValue = []<std::size_t... Indices>(std::index_sequence<Indices...>) constexpr noexcept
	{ return simd::setr(static_cast<Element>(Indices + 1)...); }(std::make_index_sequence<simd::element_count>{});
	if (simd::to_array(setrValue) != values)
		return false;
	if (simd::extract_slow(constructed, 0) != values.front() || simd::extract_slow(constructed, static_cast<int>(simd::element_count - 1)) != values.back())
		return false;

	constexpr Element replacement = static_cast<Element>(42);
	const auto replaced = simd::insert_slow(constructed, replacement, static_cast<int>(simd::element_count - 1));
	return simd::extract_slow(replaced, static_cast<int>(simd::element_count - 1)) == replacement;
}

/**
 * @brief Verifies generator-based native register construction during constant evaluation.
 * @tparam Implementation Native byte backend selected by the owning width-specific probe.
 * @return True when every generated lane preserves its compile-time index-derived value.
 */
template <class Implementation> [[nodiscard]] consteval bool static_register_construction_contract() noexcept
{
	constexpr auto value = Detail::make_static_register<Implementation>([]<std::size_t index>() constexpr noexcept
	{
		return static_cast<std::uint8_t>((index * 7U) + 3U);
	});
	std::array<std::uint8_t, sizeof(value)> expected{};
	for (std::size_t index = 0; index < expected.size(); ++index)
		expected[index] = static_cast<std::uint8_t>((index * 7U) + 3U);
	return Detail::register_to_array<std::uint8_t>(value) == expected;
}

/**
 * @brief Verifies signed and unsigned 64-bit forward-order construction during constant evaluation.
 * @return True when lane order and complete unsigned bit patterns are preserved.
 */
[[nodiscard]] consteval bool setr_64bit_construction_contract() noexcept
{
	using signed_words = Api<128, std::int64_t>;
	constexpr std::array<std::int64_t, 2> signed_values{
		std::numeric_limits<std::int64_t>::lowest(),
		std::numeric_limits<std::int64_t>::max(),
	};
	if (signed_words::to_array(signed_words::setr(signed_values[0], signed_values[1])) != signed_values)
		return false;

	using unsigned_words = Api<128, std::uint64_t>;
	constexpr std::array<std::uint64_t, 2> unsigned_values{
		0x8000'0000'0000'0001ULL,
		0xFEDC'BA98'7654'3210ULL,
	};
	return unsigned_words::to_array(unsigned_words::setr(unsigned_values[0], unsigned_values[1])) == unsigned_values;
}

/**
 * @brief Produces deterministic comparison operands.
 * @tparam Width SIMD register width in bits.
 * @tparam Element SIMD lane type.
 * @param offset Selects the left or right comparison pattern.
 * @return Comparison lanes containing equality and both order directions.
 */
template <std::size_t Width, class Element> [[nodiscard]] constexpr auto comparison_values(const unsigned offset) noexcept
{
	using simd = Api<Width, Element>;
	std::array<Element, simd::element_count> values{};
	for (std::size_t index = 0; index < values.size(); ++index)
	{
		const unsigned group = static_cast<unsigned>(index % 3);
		values[index] = static_cast<Element>(offset == 0 ? group + 1 : (group == 0 ? 1 : 4 - group));
	}
	return values;
}

/**
 * @brief Builds the byte-granular mask expected from a scalar comparison.
 * @tparam Width SIMD register width in bits.
 * @tparam Element SIMD lane type.
 * @tparam Predicate Scalar comparison predicate type.
 * @param lhs Left lane values.
 * @param rhs Right lane values.
 * @param predicate Scalar predicate applied to each lane pair.
 * @return Byte-granular comparison mask.
 */
template <std::size_t Width, class Element, class Predicate>
[[nodiscard]] constexpr auto comparison_mask(const std::array<Element, Api<Width, Element>::element_count> &lhs,
											 const std::array<Element, Api<Width, Element>::element_count> &rhs, Predicate predicate) noexcept
{
	using simd = Api<Width, Element>;
	typename simd::mask_t result = 0;
	constexpr typename simd::mask_t laneMask = static_cast<typename simd::mask_t>((typename simd::mask_t{1} << sizeof(Element)) - 1);
	for (std::size_t index = 0; index < lhs.size(); ++index)
		if (predicate(lhs[index], rhs[index]))
			result |= static_cast<typename simd::mask_t>(laneMask << (index * sizeof(Element)));
	return result;
}

/**
 * @brief Builds the lane-granular mask expected from a scalar comparison.
 * @tparam Width SIMD register width in bits.
 * @tparam Element SIMD lane type.
 * @tparam Predicate Scalar comparison predicate type.
 * @param lhs Left lane values.
 * @param rhs Right lane values.
 * @param predicate Scalar predicate applied to each lane pair.
 * @return Mask containing one bit per matching lane.
 */
template <std::size_t Width, class Element, class Predicate>
[[nodiscard]] constexpr auto comparison_slim_mask(const std::array<Element, Api<Width, Element>::element_count> &lhs,
												  const std::array<Element, Api<Width, Element>::element_count> &rhs, Predicate predicate) noexcept
{
	using simd = Api<Width, Element>;
	typename simd::mask_t result = 0;
	for (std::size_t index = 0; index < lhs.size(); ++index)
		if (predicate(lhs[index], rhs[index]))
			result |= typename simd::mask_t{1} << index;
	return result;
}

/**
 * @brief Verifies every public constexpr comparison helper for one SIMD shape.
 * @tparam Width SIMD register width in bits.
 * @tparam Element SIMD lane type.
 * @return True when equality and ordering masks match scalar predicates.
 */
template <std::size_t Width, class Element> [[nodiscard]] consteval bool comparison_contract() noexcept
{
	using simd = Api<Width, Element>;
	constexpr auto lhsValues = comparison_values<Width, Element>(0);
	constexpr auto rhsValues = comparison_values<Width, Element>(1);
	constexpr auto lhs = simd::construct(lhsValues);
	constexpr auto rhs = simd::construct(rhsValues);
	constexpr auto equal = comparison_mask<Width>(lhsValues, rhsValues, [](const Element lhsValue, const Element rhsValue) { return lhsValue == rhsValue; });
	constexpr auto greater = comparison_mask<Width>(lhsValues, rhsValues, [](const Element lhsValue, const Element rhsValue) { return lhsValue > rhsValue; });
	constexpr auto less = comparison_mask<Width>(lhsValues, rhsValues, [](const Element lhsValue, const Element rhsValue) { return lhsValue < rhsValue; });
	constexpr auto equalSlim =
		comparison_slim_mask<Width>(lhsValues, rhsValues, [](const Element lhsValue, const Element rhsValue) { return lhsValue == rhsValue; });
	constexpr auto greaterSlim =
		comparison_slim_mask<Width>(lhsValues, rhsValues, [](const Element lhsValue, const Element rhsValue) { return lhsValue > rhsValue; });
	constexpr auto lessSlim =
		comparison_slim_mask<Width>(lhsValues, rhsValues, [](const Element lhsValue, const Element rhsValue) { return lhsValue < rhsValue; });
	using unsigned_element_t = select_unsigned_integer_t<sizeof(Element) * 8>;
	constexpr Element trueLane = std::bit_cast<Element>(std::numeric_limits<unsigned_element_t>::max());
	std::array<Element, simd::element_count> equalLanes{};
	std::array<Element, simd::element_count> greaterLanes{};
	std::array<Element, simd::element_count> greaterEqualLanes{};
	std::array<Element, simd::element_count> lessLanes{};
	std::array<Element, simd::element_count> lessEqualLanes{};
	std::array<Element, simd::element_count> selectedLanes{};
	for (std::size_t index = 0; index < lhsValues.size(); ++index)
	{
		equalLanes[index] = lhsValues[index] == rhsValues[index] ? trueLane : Element{};
		greaterLanes[index] = lhsValues[index] > rhsValues[index] ? trueLane : Element{};
		greaterEqualLanes[index] = lhsValues[index] >= rhsValues[index] ? trueLane : Element{};
		lessLanes[index] = lhsValues[index] < rhsValues[index] ? trueLane : Element{};
		lessEqualLanes[index] = lhsValues[index] <= rhsValues[index] ? trueLane : Element{};
		selectedLanes[index] = lhsValues[index] == rhsValues[index] ? lhsValues[index] : rhsValues[index];
	}
	const auto matchesObjectRepresentation = [](const auto native, const auto &expected) constexpr noexcept
	{ return std::bit_cast<std::array<std::uint8_t, Width / 8>>(simd::to_array(native)) == std::bit_cast<std::array<std::uint8_t, Width / 8>>(expected); };
	return matchesObjectRepresentation(simd::compare_equal(lhs, rhs), equalLanes) &&
		   matchesObjectRepresentation(simd::compare_greater(lhs, rhs), greaterLanes) &&
		   matchesObjectRepresentation(simd::compare_greater_equal(lhs, rhs), greaterEqualLanes) &&
		   matchesObjectRepresentation(simd::compare_less(lhs, rhs), lessLanes) &&
		   matchesObjectRepresentation(simd::compare_less_equal(lhs, rhs), lessEqualLanes) &&
		   matchesObjectRepresentation(simd::select(simd::compare_equal(lhs, rhs), lhs, rhs), selectedLanes) && simd::cmp_eq_mask(lhs, rhs) == equal &&
		   simd::cmp_gt_mask(lhs, rhs) == greater && simd::cmp_ge_mask(lhs, rhs) == (equal | greater) && simd::cmp_lt_mask(lhs, rhs) == less &&
		   simd::cmp_le_mask(lhs, rhs) == (equal | less) && simd::cmp_eq_slim(lhs, rhs) == equalSlim && simd::cmp_gt_slim(lhs, rhs) == greaterSlim &&
		   simd::cmp_ge_slim(lhs, rhs) == (equalSlim | greaterSlim) && simd::cmp_lt_slim(lhs, rhs) == lessSlim &&
		   simd::cmp_le_slim(lhs, rhs) == (equalSlim | lessSlim);
}

/**
 * @brief Verifies every public constexpr bitwise operation for one SIMD shape.
 * @tparam Width SIMD register width in bits.
 * @tparam Element SIMD lane type.
 * @return True when all operations preserve the expected object-representation bits.
 */
template <std::size_t Width, class Element> [[nodiscard]] consteval bool bitwise_contract() noexcept
{
	using simd = Api<Width, Element>;
	std::array<std::uint8_t, Width / 8> left_bytes{};
	std::array<std::uint8_t, Width / 8> right_bytes{};
	std::array<std::uint8_t, Width / 8> expected_and{};
	std::array<std::uint8_t, Width / 8> expected_or{};
	std::array<std::uint8_t, Width / 8> expected_xor{};
	std::array<std::uint8_t, Width / 8> expected_andnot{};
	std::array<std::uint8_t, Width / 8> expected_not{};
	for (std::size_t byte = 0; byte < left_bytes.size(); ++byte)
	{
		left_bytes[byte] = static_cast<std::uint8_t>(byte * 37u + 0x35u);
		right_bytes[byte] = static_cast<std::uint8_t>(byte * 19u + 0xA6u);
		expected_and[byte] = left_bytes[byte] & right_bytes[byte];
		expected_or[byte] = left_bytes[byte] | right_bytes[byte];
		expected_xor[byte] = left_bytes[byte] ^ right_bytes[byte];
		expected_andnot[byte] = static_cast<std::uint8_t>(~left_bytes[byte]) & right_bytes[byte];
		expected_not[byte] = static_cast<std::uint8_t>(~left_bytes[byte]);
	}
	const auto lhs = simd::construct(std::bit_cast<std::array<Element, simd::element_count>>(left_bytes));
	const auto rhs = simd::construct(std::bit_cast<std::array<Element, simd::element_count>>(right_bytes));
	return std::bit_cast<std::array<std::uint8_t, Width / 8>>(simd::to_array(simd::bitwise_and(lhs, rhs))) == expected_and &&
		   std::bit_cast<std::array<std::uint8_t, Width / 8>>(simd::to_array(simd::bitwise_or(lhs, rhs))) == expected_or &&
		   std::bit_cast<std::array<std::uint8_t, Width / 8>>(simd::to_array(simd::bitwise_xor(lhs, rhs))) == expected_xor &&
		   std::bit_cast<std::array<std::uint8_t, Width / 8>>(simd::to_array(simd::bitwise_andnot(lhs, rhs))) == expected_andnot &&
		   std::bit_cast<std::array<std::uint8_t, Width / 8>>(simd::to_array(simd::bitwise_not(lhs))) == expected_not;
}

/**
 * @brief Creates deterministic object-representation bytes for movemask contracts.
 * @tparam Width SIMD register width in bits.
 * @return Byte sequence with varying sign bits.
 */
template <std::size_t Width> [[nodiscard]] constexpr auto movemask_bytes() noexcept
{
	std::array<std::uint8_t, Width / 8> bytes{};
	for (std::size_t index = 0; index < bytes.size(); ++index)
		bytes[index] = static_cast<std::uint8_t>((index * 19u) | (index % 3u == 1u ? 0u : 0x80u));
	return bytes;
}

/**
 * @brief Reinterprets deterministic bytes as SIMD lane values.
 * @tparam Width SIMD register width in bits.
 * @tparam Element SIMD lane type.
 * @return Full register of lane values.
 */
template <std::size_t Width, class Element> [[nodiscard]] constexpr auto movemask_values() noexcept
{
	using simd = Api<Width, Element>;
	constexpr auto bytes = movemask_bytes<Width>();
	static_assert(sizeof(bytes) == sizeof(std::array<Element, simd::element_count>));
	return std::bit_cast<std::array<Element, simd::element_count>>(bytes);
}

/**
 * @brief Computes the scalar byte-granular movemask oracle.
 * @tparam Width SIMD register width in bits.
 * @tparam Element SIMD lane type.
 * @return Expected byte-granular mask.
 */
template <std::size_t Width, class Element> [[nodiscard]] constexpr auto expected_movemask() noexcept
{
	using simd = Api<Width, Element>;
	constexpr auto bytes = movemask_bytes<Width>();
	typename simd::mask_t result = 0;
	for (std::size_t index = 0; index < bytes.size(); ++index)
		result |= static_cast<typename simd::mask_t>((bytes[index] >> 7) & 1u) << index;
	return result;
}

/**
 * @brief Computes the scalar element-granular movemask oracle.
 * @tparam Width SIMD register width in bits.
 * @tparam Element SIMD lane type.
 * @return Expected element-granular mask.
 */
template <std::size_t Width, class Element> [[nodiscard]] constexpr auto expected_movemask_slim() noexcept
{
	using simd = Api<Width, Element>;
	constexpr auto bytes = movemask_bytes<Width>();
	typename simd::mask_t result = 0;
	for (std::size_t index = 0; index < simd::element_count; ++index)
	{
		const std::size_t signByte = (index + 1) * sizeof(Element) - 1;
		result |= static_cast<typename simd::mask_t>((bytes[signByte] >> 7) & 1u) << index;
	}
	return result;
}

/**
 * @brief Verifies byte- and element-granular constexpr movemasks.
 * @tparam Width SIMD register width in bits.
 * @tparam Element SIMD lane type.
 * @return True when both masks match scalar object-representation oracles.
 */
template <std::size_t Width, class Element> [[nodiscard]] consteval bool movemask_contract() noexcept
{
	using simd = Api<Width, Element>;
	constexpr auto value = simd::construct(movemask_values<Width, Element>());
	return simd::movemask(value) == expected_movemask<Width, Element>() && simd::movemask_slim(value) == expected_movemask_slim<Width, Element>();
}

/**
 * @brief Verifies constexpr extrema positions and first-position tie semantics.
 * @tparam Width SIMD register width in bits.
 * @tparam Element Integral SIMD lane type.
 * @return True when extrema positions match the prepared lane layout.
 */
template <std::size_t Width, std::integral Element> [[nodiscard]] consteval bool extrema_position_contract() noexcept
{
	using simd = Api<Width, Element>;
	std::array<Element, simd::element_count> values{};
	values.fill(static_cast<Element>(3));
	values.front() = std::numeric_limits<Element>::lowest();
	values.back() = std::numeric_limits<Element>::max();
	if constexpr (simd::element_count > 2)
		values[1] = std::numeric_limits<Element>::lowest();
	const auto value = simd::construct(values);
	return simd::min_position(value) == 0 && simd::max_position(value) == simd::element_count - 1 &&
		   simd::min_position(simd::set1(std::numeric_limits<Element>::lowest())) == 0 &&
		   simd::max_position(simd::set1(std::numeric_limits<Element>::max())) == 0;
}

/**
 * @brief Verifies constexpr logical and arithmetic lane-shift boundaries.
 * @tparam Width SIMD register width in bits.
 * @tparam Element Integral SIMD lane type.
 * @return True when zero, one, and final-valid-bit shifts match scalar values.
 */
template <std::size_t Width, std::integral Element> [[nodiscard]] consteval bool lane_shift_contract() noexcept
{
	using simd = Api<Width, Element>;
	constexpr auto positive = simd::set1(static_cast<Element>(4));
	if (simd::to_array(simd::shift_left(positive, 0)) != simd::to_array(positive) ||
		simd::extract_slow(simd::shift_left(positive, 1), 0) != static_cast<Element>(8) ||
		simd::extract_slow(simd::shift_right(positive, 1), 0) != static_cast<Element>(2))
		return false;
	constexpr int finalShift = static_cast<int>(sizeof(Element) * 8 - 1);
	constexpr int widthShift = static_cast<int>(sizeof(Element) * 8);
	if (simd::extract_slow(simd::shift_left(simd::set1(static_cast<Element>(1)), finalShift), 0) !=
			static_cast<Element>(std::make_unsigned_t<Element>{1} << finalShift) ||
		simd::to_array(simd::shift_left(positive, widthShift)) != std::array<Element, simd::element_count>{} ||
		simd::to_array(simd::shift_left(positive, widthShift + 1)) != std::array<Element, simd::element_count>{} ||
		simd::to_array(simd::shift_right(positive, widthShift)) != std::array<Element, simd::element_count>{} ||
		simd::to_array(simd::shift_right(positive, widthShift + 1)) != std::array<Element, simd::element_count>{})
		return false;
	if constexpr (std::is_signed_v<Element>)
		return simd::extract_slow(simd::shift_right_arithmetic(simd::set1(static_cast<Element>(-8)), 1), 0) == static_cast<Element>(-4) &&
			   simd::extract_slow(simd::shift_right_arithmetic(simd::set1(static_cast<Element>(-8)), widthShift), 0) == static_cast<Element>(-1) &&
			   simd::extract_slow(simd::shift_right_arithmetic(simd::set1(static_cast<Element>(-8)), widthShift + 1), 0) == static_cast<Element>(-1);
	return true;
}

/**
 * @brief Verifies whole-register bit and byte shift boundaries for the 128-bit API.
 * @return True when negative, zero, final, width, and beyond-width counts match the contract.
 */
[[nodiscard]] consteval bool whole_register_shift_contract() noexcept
{
	using words = Api<128, std::uint64_t>;
	constexpr auto value = words::setr(std::uint64_t{1}, std::uint64_t{1} << 63);
	constexpr auto original = std::array<std::uint64_t, 2>{1, std::uint64_t{1} << 63};
	if (words::to_array(words::shift_bits_left_slow(value, -1)) != original || words::to_array(words::shift_bits_left_slow(value, 0)) != original ||
		words::to_array(words::shift_bits_left_slow(value, 1)) != std::array<std::uint64_t, 2>{2, 0} ||
		words::to_array(words::shift_bits_left_slow(value, 63)) != std::array<std::uint64_t, 2>{std::uint64_t{1} << 63, 0} ||
		words::to_array(words::shift_bits_left_slow(value, 64)) != std::array<std::uint64_t, 2>{0, 1} ||
		words::to_array(words::shift_bits_left_slow(value, 65)) != std::array<std::uint64_t, 2>{0, 2} ||
		words::to_array(words::shift_bits_left_slow(value, 127)) != std::array<std::uint64_t, 2>{0, std::uint64_t{1} << 63} ||
		words::to_array(words::shift_bits_left_slow(value, 128)) != std::array<std::uint64_t, 2>{} ||
		words::to_array(words::shift_bits_left_slow(value, 129)) != std::array<std::uint64_t, 2>{})
		return false;
	if (words::to_array(words::shift_bits_right_slow(value, -1)) != original || words::to_array(words::shift_bits_right_slow(value, 0)) != original ||
		words::to_array(words::shift_bits_right_slow(value, 1)) != std::array<std::uint64_t, 2>{0, std::uint64_t{1} << 62} ||
		words::to_array(words::shift_bits_right_slow(value, 63)) != std::array<std::uint64_t, 2>{0, 1} ||
		words::to_array(words::shift_bits_right_slow(value, 64)) != std::array<std::uint64_t, 2>{std::uint64_t{1} << 63, 0} ||
		words::to_array(words::shift_bits_right_slow(value, 65)) != std::array<std::uint64_t, 2>{std::uint64_t{1} << 62, 0} ||
		words::to_array(words::shift_bits_right_slow(value, 127)) != std::array<std::uint64_t, 2>{1, 0} ||
		words::to_array(words::shift_bits_right_slow(value, 128)) != std::array<std::uint64_t, 2>{} ||
		words::to_array(words::shift_bits_right_slow(value, 129)) != std::array<std::uint64_t, 2>{})
		return false;
	if (words::to_array(words::template shift_bits_left<0>(value)) != original ||
		words::to_array(words::template shift_bits_left<1>(value)) != std::array<std::uint64_t, 2>{2, 0} ||
		words::to_array(words::template shift_bits_left<63>(value)) != std::array<std::uint64_t, 2>{std::uint64_t{1} << 63, 0} ||
		words::to_array(words::template shift_bits_left<64>(value)) != std::array<std::uint64_t, 2>{0, 1} ||
		words::to_array(words::template shift_bits_left<65>(value)) != std::array<std::uint64_t, 2>{0, 2} ||
		words::to_array(words::template shift_bits_left<127>(value)) != std::array<std::uint64_t, 2>{0, std::uint64_t{1} << 63} ||
		words::to_array(words::template shift_bits_left<128>(value)) != std::array<std::uint64_t, 2>{} ||
		words::to_array(words::template shift_bits_left<129>(value)) != std::array<std::uint64_t, 2>{})
		return false;
	if (words::to_array(words::template shift_bits_right<0>(value)) != original ||
		words::to_array(words::template shift_bits_right<1>(value)) != std::array<std::uint64_t, 2>{0, std::uint64_t{1} << 62} ||
		words::to_array(words::template shift_bits_right<63>(value)) != std::array<std::uint64_t, 2>{0, 1} ||
		words::to_array(words::template shift_bits_right<64>(value)) != std::array<std::uint64_t, 2>{std::uint64_t{1} << 63, 0} ||
		words::to_array(words::template shift_bits_right<65>(value)) != std::array<std::uint64_t, 2>{std::uint64_t{1} << 62, 0} ||
		words::to_array(words::template shift_bits_right<127>(value)) != std::array<std::uint64_t, 2>{1, 0} ||
		words::to_array(words::template shift_bits_right<128>(value)) != std::array<std::uint64_t, 2>{} ||
		words::to_array(words::template shift_bits_right<129>(value)) != std::array<std::uint64_t, 2>{})
		return false;

	using bytes = Api<128, std::uint8_t>;
	constexpr auto byteValues = lane_values<128, std::uint8_t>();
	constexpr auto byteValue = bytes::construct(byteValues);
	std::array<std::uint8_t, bytes::element_count> left15{};
	std::array<std::uint8_t, bytes::element_count> right15{};
	left15.back() = byteValues.front();
	right15.front() = byteValues.back();
	return bytes::to_array(bytes::shift_bytes_left_slow(byteValue, -1)) == byteValues &&
		   bytes::to_array(bytes::shift_bytes_left_slow(byteValue, 0)) == byteValues &&
		   bytes::to_array(bytes::shift_bytes_left_slow(byteValue, 15)) == left15 &&
		   bytes::to_array(bytes::shift_bytes_left_slow(byteValue, 16)) == std::array<std::uint8_t, bytes::element_count>{} &&
		   bytes::to_array(bytes::shift_bytes_left_slow(byteValue, 17)) == std::array<std::uint8_t, bytes::element_count>{} &&
		   bytes::to_array(bytes::shift_bytes_right_slow(byteValue, -1)) == byteValues &&
		   bytes::to_array(bytes::shift_bytes_right_slow(byteValue, 0)) == byteValues &&
		   bytes::to_array(bytes::shift_bytes_right_slow(byteValue, 15)) == right15 &&
		   bytes::to_array(bytes::shift_bytes_right_slow(byteValue, 16)) == std::array<std::uint8_t, bytes::element_count>{} &&
		   bytes::to_array(bytes::shift_bytes_right_slow(byteValue, 17)) == std::array<std::uint8_t, bytes::element_count>{};
}

/**
 * @brief Verifies one immediate complete-register byte shift during constant evaluation.
 * @tparam Width SIMD register width in bits.
 * @tparam Count Compile-time byte count.
 * @return `true` when both directions match an independent scalar byte oracle.
 */
template <std::size_t Width, std::size_t Count> [[nodiscard]] consteval bool immediate_byte_shift_count_contract() noexcept
{
	using api = Api<Width, std::uint8_t>;
	std::array<std::uint8_t, api::byte_count> source{};
	std::array<std::uint8_t, api::byte_count> expected_left{};
	std::array<std::uint8_t, api::byte_count> expected_right{};
	for (std::size_t index = 0; index < source.size(); ++index)
		source[index] = static_cast<std::uint8_t>(index * 7 + 1);
	if constexpr (Count < api::byte_count)
	{
		for (std::size_t index = Count; index < source.size(); ++index)
			expected_left[index] = source[index - Count];
		for (std::size_t index = 0; index + Count < source.size(); ++index)
			expected_right[index] = source[index + Count];
	}
	const auto value = api::construct(source);
	const auto left = api::to_array(api::template shift_bytes_left<static_cast<int>(Count)>(value));
	const auto right = api::to_array(api::template shift_bytes_right<static_cast<int>(Count)>(value));
	if (left != expected_left || right != expected_right)
		return false;
	if constexpr (Width == 128)
		return left == api::to_array(api::template shift_bits_left<static_cast<int>(Count * 8)>(value)) &&
			   right == api::to_array(api::template shift_bits_right<static_cast<int>(Count * 8)>(value));
	return true;
}

/**
 * @brief Verifies all required immediate byte-shift boundary counts during constant evaluation.
 * @tparam Width SIMD register width in bits.
 * @return `true` when every required count passes in both directions.
 */
template <std::size_t Width> [[nodiscard]] consteval bool immediate_byte_shift_contract() noexcept
{
	return immediate_byte_shift_count_contract<Width, 0>() && immediate_byte_shift_count_contract<Width, 1>() &&
		   immediate_byte_shift_count_contract<Width, 7>() && immediate_byte_shift_count_contract<Width, 8>() &&
		   immediate_byte_shift_count_contract<Width, 15>() && immediate_byte_shift_count_contract<Width, 16>() &&
		   immediate_byte_shift_count_contract<Width, 17>() && immediate_byte_shift_count_contract<Width, 31>() &&
		   immediate_byte_shift_count_contract<Width, 32>() && immediate_byte_shift_count_contract<Width, 33>();
}
/**
 * @brief Verifies immediate blend through the implementation-layer constant-evaluation entry point.
 * @tparam Width SIMD register width in bits.
 * @tparam Element Lane type supported by immediate blend.
 * @return `true` when the compile-time mask selects the expected lanes.
 */
template <std::size_t Width, class Element> [[nodiscard]] consteval bool immediate_blend_contract() noexcept
{
	using api = Api<Width, Element>;
	std::array<Element, api::element_count> left{};
	std::array<Element, api::element_count> right{};
	std::array<Element, api::element_count> expected{};
	for (std::size_t index = 0; index < api::element_count; ++index)
	{
		left[index] = static_cast<Element>(index + 1);
		right[index] = static_cast<Element>(index + 33);
		expected[index] = (0xA5u & (1u << (index % 8))) != 0 ? right[index] : left[index];
	}
	return api::to_array(api::template blend<0xA5>(api::construct(left), api::construct(right))) == expected;
}
/** @brief Result bundle shared by constexpr and forced-runtime parity checks. */
template <std::size_t Width> struct ApiContractSnapshot final
{
	using simd = Api<Width, std::int32_t>;
	std::array<std::int32_t, simd::element_count> lanes{};
	typename simd::mask_t equalMask{};
	typename simd::mask_t greaterMask{};
	std::size_t minimumPosition{};
	std::size_t maximumPosition{};

	/** @brief Compares all observable snapshot fields. */
	friend constexpr bool operator==(const ApiContractSnapshot &, const ApiContractSnapshot &) noexcept = default;
};

/**
 * @brief Evaluates public API operations for constexpr-versus-runtime parity.
 * @tparam Width SIMD register width in bits.
 * @param lhsValues Left operand lane values.
 * @param rhsValues Right operand lane values.
 * @return Observable API results.
 */
template <std::size_t Width>
[[nodiscard]] constexpr ApiContractSnapshot<Width> evaluate_api_contract(
	const std::array<std::int32_t, Api<Width, std::int32_t>::element_count> &lhsValues,
	const std::array<std::int32_t, Api<Width, std::int32_t>::element_count> &rhsValues) noexcept
{
	using simd = Api<Width, std::int32_t>;
	const auto lhs = simd::construct(lhsValues);
	const auto rhs = simd::construct(rhsValues);
	return {
		simd::to_array(simd::shift_left(lhs, 1)), simd::cmp_eq_mask(lhs, rhs), simd::cmp_gt_mask(lhs, rhs), simd::min_position(lhs), simd::max_position(lhs),
	};
}

/**
 * @brief Verifies basic constexpr SimdVector construction.
 * @tparam ElementCount Logical vector lane count.
 * @return True when the default, array, and broadcast constructors are constant evaluable.
 */
template <std::size_t ElementCount> [[nodiscard]] consteval bool simd_vector_contract() noexcept
{
	using vector = SimdVector<std::int32_t, static_cast<int>(ElementCount)>;
	std::array<std::int32_t, ElementCount> values{};
	for (std::size_t index = 0; index < ElementCount; ++index)
		values[index] = static_cast<std::int32_t>(index + 1);
	const vector empty{};
	const vector lhs{values};
	const vector broadcast{2};
	(void)empty;
	(void)lhs;
	(void)broadcast;
	return true;
}
} // namespace SimdLib::Tests::Constexpr
