#include "../LogicalShuffleTestSupport.h"

#include <SimdLib/Register.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

namespace
{

/** @brief Compile-time list of supported Register element types. */
template <class... element_types> struct register_element_types
{
};

using supported_register_element_types =
	register_element_types<std::int8_t, std::uint8_t, std::int16_t, std::uint16_t, std::int32_t, std::uint32_t, std::int64_t, std::uint64_t, float, double>;

/**
 * @brief Expands one logical selector array into a Register shuffle during constant evaluation.
 * @tparam register_t Register specialization under test.
 * @tparam selectors Logical source-lane selectors.
 * @tparam positions Output lane positions.
 * @param value Source Register.
 * @return Constant-evaluated shuffled Register.
 */
template <class register_t, auto selectors, std::size_t... positions>
[[nodiscard]] consteval register_t register_logical_shuffle_value(register_t value, std::index_sequence<positions...>) noexcept
{
	return value.template shuffle<selectors[positions]...>();
}

/**
 * @brief Verifies one constant-evaluated Register shuffle against the scalar oracle.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @tparam selectors Logical source-lane selectors.
 * @return True when every result lane preserves the oracle's object representation.
 */
template <class element_t, std::size_t bits, auto selectors> [[nodiscard]] consteval bool register_logical_shuffle_case() noexcept
{
	using register_t = SimdLib::Register<element_t, bits>;
	constexpr auto source = SimdLib::Tests::LogicalShuffle::distinct_lanes<element_t, bits>();
	constexpr register_t source_register = register_t::from_array(source);
	constexpr register_t shuffled = register_logical_shuffle_value<register_t, selectors>(source_register, std::make_index_sequence<register_t::lane_count>{});
	constexpr auto actual = shuffled.to_array();
	constexpr auto expected = SimdLib::Tests::LogicalShuffle::logical_shuffle_oracle<element_t, bits, selectors>(source);
	return SimdLib::Tests::LogicalShuffle::same_object_representations(actual, expected);
}

/**
 * @brief Verifies nonidentity and repeated-selector constexpr Register shuffles.
 * @tparam element_t Logical lane type.
 * @tparam bits Register width in bits.
 * @return True when both independent scalar-oracle comparisons succeed.
 */
template <class element_t, std::size_t bits> [[nodiscard]] consteval bool register_logical_shuffle_contract() noexcept
{
	return register_logical_shuffle_case<element_t, bits, SimdLib::Tests::LogicalShuffle::reverse_selectors<element_t, bits>()>() &&
		   register_logical_shuffle_case<element_t, bits, SimdLib::Tests::LogicalShuffle::repeated_selectors<element_t, bits>()>();
}

/**
 * @brief Expands one byte-selector array into a Register byte shuffle during constant evaluation.
 * @tparam register_t Register specialization under test.
 * @tparam selectors Source-byte selectors.
 * @tparam positions Output byte positions.
 * @param value Source Register.
 * @return Constant-evaluated byte-shuffled Register.
 */
template <class register_t, auto selectors, std::size_t... positions>
[[nodiscard]] consteval register_t register_byte_shuffle_value(register_t value, std::index_sequence<positions...>) noexcept
{
	return value.template shuffle_bytes<selectors[positions]...>();
}

/**
 * @brief Verifies one constant-evaluated Register byte shuffle against the scalar byte oracle.
 * @tparam element_t Logical lane type retained by the result.
 * @tparam bits Register width in bits.
 * @tparam selectors Source-byte selectors.
 * @return True when every result byte matches the independently selected source byte.
 */
template <class element_t, std::size_t bits, auto selectors> [[nodiscard]] consteval bool register_byte_shuffle_case() noexcept
{
	using register_t = SimdLib::Register<element_t, bits>;
	constexpr auto source = SimdLib::Tests::LogicalShuffle::distinct_lanes<element_t, bits>();
	constexpr register_t source_register = register_t::from_array(source);
	constexpr register_t shuffled = register_byte_shuffle_value<register_t, selectors>(source_register, std::make_index_sequence<register_t::byte_count>{});
	constexpr auto actual = std::bit_cast<std::array<std::uint8_t, register_t::byte_count>>(shuffled.to_array());
	constexpr auto source_bytes = std::bit_cast<std::array<std::uint8_t, register_t::byte_count>>(source);
	constexpr auto expected = SimdLib::Tests::LogicalShuffle::logical_shuffle_oracle<std::uint8_t, bits, selectors>(source_bytes);
	return actual == expected;
}

/**
 * @brief Verifies local and cross-half constant-evaluated Register byte shuffles.
 * @tparam element_t Logical lane type retained by the result.
 * @tparam bits Register width in bits.
 * @return True when every independent scalar-oracle comparison succeeds.
 */
template <class element_t, std::size_t bits> [[nodiscard]] consteval bool register_byte_shuffle_contract() noexcept
{
	if constexpr (bits == 128)
		return register_byte_shuffle_case<element_t, bits, SimdLib::Tests::LogicalShuffle::reverse_selectors<std::uint8_t, bits>()>();
	else
		return register_byte_shuffle_case<element_t, bits, SimdLib::Tests::LogicalShuffle::reverse_selectors<std::uint8_t, bits>()>() &&
			   register_byte_shuffle_case<element_t, bits, SimdLib::Tests::LogicalShuffle::mixed_half_selectors<std::uint8_t>()>();
}

/** @brief Constructs a register from an expanded compile-time lane array. */
template <class register_t, std::size_t... indices>
[[nodiscard]] consteval register_t from_lanes(const std::array<typename register_t::element_type, register_t::lane_count> &values,
											  std::index_sequence<indices...>) noexcept
{
	return register_t::from_lanes(values[indices]...);
}

/** @brief Verifies all constant-evaluable Register construction and lane operations. */
template <class element_t, std::size_t bits> [[nodiscard]] consteval bool register_constexpr_contract() noexcept
{
	using register_type = SimdLib::Register<element_t, bits>;
	std::array<element_t, register_type::lane_count> values{};
	for (std::size_t index = 0; index < values.size(); ++index)
		values[index] = static_cast<element_t>(index + 1);
	std::array<element_t, register_type::lane_count> broadcast_values{};
	broadcast_values.fill(static_cast<element_t>(7));
	const std::array<element_t, register_type::lane_count> zeros{};
#if SIMDLIB_COMPILER_MSVC
	const register_type value{};
	const register_type zero = register_type::zero();
	const register_type broadcast = register_type::broadcast(static_cast<element_t>(7));
	const register_type array_value = register_type::from_array(values);
	const register_type lane_value = from_lanes<register_type>(values, std::make_index_sequence<register_type::lane_count>{});
	const register_type native_value{array_value.native};
	const element_t first_lane = array_value.template lane<0>();
	const register_type changed_value = array_value.template with_lane<register_type::lane_count - 1>(static_cast<element_t>(43));
	(void)value;
	(void)zero;
	(void)broadcast;
	(void)lane_value;
	(void)native_value;
	return first_lane == values.front() && changed_value.template lane<register_type::lane_count - 1>() == static_cast<element_t>(43);
#else
	if (register_type{}.to_array() != zeros || register_type::zero().to_array() != zeros)
		return false;
	if (register_type::broadcast(static_cast<element_t>(7)).to_array() != broadcast_values)
		return false;
	const auto array_value = register_type::from_array(values);
	if (array_value.to_array() != values)
		return false;
	if (from_lanes<register_type>(values, std::make_index_sequence<register_type::lane_count>{}).to_array() != values)
		return false;
	const register_type native_value{array_value.native};
	if (native_value.to_array() != values || array_value.template lane<0>() != values.front() ||
		array_value.template lane<register_type::lane_count - 1>() != values.back())
		return false;
	const auto changed_lanes = array_value.template with_lane<register_type::lane_count - 1>(static_cast<element_t>(43)).to_array();
	return changed_lanes.front() == values.front() && changed_lanes.back() == static_cast<element_t>(43);
#endif
}

/** @brief Verifies constant-evaluated mask comparisons, combination, reductions, and selection. */
template <class element_t, std::size_t bits> [[nodiscard]] consteval bool register_mask_constexpr_contract() noexcept
{
	using register_type = SimdLib::Register<element_t, bits>;
	using mask_type = typename register_type::mask_type;
#if SIMDLIB_COMPILER_MSVC
	const mask_type mask{};
	(void)mask;
	return true;
#else
	std::array<element_t, register_type::lane_count> left{};
	std::array<element_t, register_type::lane_count> right{};
	for (std::size_t index = 0; index < left.size(); ++index)
	{
		left[index] = static_cast<element_t>((index % 2) == 0 ? 2 : 0);
		right[index] = static_cast<element_t>(1);
	}
	typename mask_type::bits_type expected = 0;
	for (std::size_t index = 0; index < mask_type::lane_count; index += 2)
		expected |= typename mask_type::bits_type{1} << index;
	const auto lhs = register_type::from_array(left);
	const auto rhs = register_type::from_array(right);
	const auto greater = lhs.compare_greater(rhs);
	const auto less = lhs.compare_less(rhs);
	const mask_type rewrapped{greater.native};
	if (greater.bits() != expected || greater.none() || !greater.any() || greater.all())
		return false;
	if (rewrapped.bits() != expected)
		return false;
	if (!(greater | less).all() || !(greater & less).none() || (greater ^ less).bits() != (greater | less).bits() || !(~(greater | less)).none())
		return false;
	const auto selected = greater.select(register_type::broadcast(static_cast<element_t>(11)), register_type::broadcast(static_cast<element_t>(22))).to_array();
	for (std::size_t index = 0; index < selected.size(); ++index)
	{
		if (selected[index] != static_cast<element_t>((index % 2) == 0 ? 11 : 22))
			return false;
	}
	return lhs == lhs && lhs != rhs && lhs.compare_equal(lhs).all() && lhs.compare_greater_equal(rhs).bits() == expected &&
		   lhs.compare_less_equal(rhs).bits() == less.bits();
#endif
}

/** @brief Verifies constant-evaluated first-minimum and first-maximum position reductions. */
template <class element_t, std::size_t bits>
	requires std::is_integral_v<element_t>
[[nodiscard]] consteval bool register_position_constexpr_contract() noexcept
{
	using register_type = SimdLib::Register<element_t, bits>;
	std::array<element_t, register_type::lane_count> values{};
	values.fill(static_cast<element_t>(7));
	values[0] = static_cast<element_t>(1);
	values[register_type::lane_count - 1] = static_cast<element_t>(12);
	const auto value = register_type::from_array(values);
	return value.min_position() == 0 && value.max_position() == register_type::lane_count - 1;
}

/** @brief Verifies constant-evaluated bitwise expressions, assignments, and sign reductions. */
template <class element_t, std::size_t bits> [[nodiscard]] consteval bool register_bitwise_constexpr_contract() noexcept
{
	using register_type = SimdLib::Register<element_t, bits>;
	const auto value = register_type::broadcast(static_cast<element_t>(-1));
	const auto zero = register_type::zero();
#if SIMDLIB_COMPILER_MSVC
	const auto intersection = value & value;
	const auto combined = value | zero;
	const auto toggled = value ^ value;
	const auto inverted = ~~value;
	const auto excluded = value.andnot(value);
	auto reassigned = value;
	reassigned = reassigned & value;
	reassigned = reassigned | zero;
	reassigned = reassigned ^ value;
	(void)intersection;
	(void)combined;
	(void)toggled;
	(void)inverted;
	(void)excluded;
	(void)reassigned;
	return true;
#else
	if ((value & value).to_array() != value.to_array() || (value | zero).to_array() != value.to_array() || (value ^ value).to_array() != zero.to_array() ||
		(~~value).to_array() != value.to_array() || value.andnot(value).to_array() != zero.to_array())
		return false;
	auto reassigned = value;
	reassigned = reassigned & value;
	reassigned = reassigned | zero;
	reassigned = reassigned ^ value;
	return reassigned.to_array() == zero.to_array() && value.lane_sign_bits() != 0 && value.movemask() != 0;
#endif
}

/** @brief Verifies constant-evaluated per-lane shift boundary semantics. */
template <class element_t, std::size_t bits>
	requires std::is_integral_v<element_t>
[[nodiscard]] consteval bool register_lane_shift_constexpr_contract() noexcept
{
	using register_type = SimdLib::Register<element_t, bits>;
	using unsigned_type = std::make_unsigned_t<element_t>;
	constexpr int lane_width = std::numeric_limits<unsigned_type>::digits;
	constexpr unsigned_type high_bit = unsigned_type{1} << (lane_width - 1);
	const auto value = register_type::broadcast(std::bit_cast<element_t>(high_bit));
#if SIMDLIB_COMPILER_MSVC
	const auto left = value << lane_width;
	const auto logical = value.logical_shift_right(lane_width - 1);
	const auto right = value >> (lane_width + 1);
	(void)left;
	(void)logical;
	(void)right;
	return true;
#else
	const auto zeros = register_type::zero().to_array();
	if ((value << 0).to_array() != value.to_array() || (value << lane_width).to_array() != zeros || (value << (lane_width + 1)).to_array() != zeros ||
		value.logical_shift_right(lane_width).to_array() != zeros || value.logical_shift_right(lane_width + 1).to_array() != zeros)
		return false;
	for (const auto lane : value.logical_shift_right(lane_width - 1).to_array())
		if (lane != element_t{1})
			return false;
	if constexpr (std::is_signed_v<element_t>)
	{
		for (const auto lane : (value >> lane_width).to_array())
			if (lane != element_t{-1})
				return false;
	}
	else if ((value >> lane_width).to_array() != zeros)
		return false;
	auto reassigned = value;
	reassigned = reassigned << lane_width;
	reassigned = value;
	reassigned = reassigned >> (lane_width + 1);
	return true;
#endif
}

/** @brief Verifies constant-evaluated 128-bit byte and static whole-register shifts. */
[[nodiscard]] consteval bool register_complete_shift_constexpr_contract() noexcept
{
	using register_type = SimdLib::Register<std::uint8_t, 128>;
	std::array<std::uint8_t, register_type::lane_count> lanes{};
	for (std::size_t index = 0; index < lanes.size(); ++index)
		lanes[index] = static_cast<std::uint8_t>(index + 1);
	const auto value = register_type::from_array(lanes);
#if SIMDLIB_COMPILER_MSVC
	const auto bytes = value.byte_shift_left(1);
	(void)bytes;
	return true;
#else
	const auto zeros = register_type::zero().to_array();
	return value.byte_shift_left(0).to_array() == lanes && value.byte_shift_left(16).to_array() == zeros && value.byte_shift_left(17).to_array() == zeros &&
		   value.byte_shift_right(16).to_array() == zeros && value.bit_shift_left(128).to_array() == zeros && value.bit_shift_right(128).to_array() == zeros &&
		   value.template bit_shift_left<128>().to_array() == zeros && value.template bit_shift_left<129>().to_array() == zeros &&
		   value.template bit_shift_right<128>().to_array() == zeros && value.template bit_shift_right<129>().to_array() == zeros;
#endif
}

/** @brief Verifies every supported bit-cast and numeric-conversion constexpr cell for one source and target type. */
template <class source_t, class target_t, std::size_t bits> [[nodiscard]] consteval bool register_conversion_constexpr_cell() noexcept
{
	using source_register = SimdLib::Register<source_t, bits>;
	const auto source = source_register::broadcast(static_cast<source_t>(1));
	if constexpr (SimdLib::IRegister::BitCast<source_register, target_t>)
	{
#if SIMDLIB_COMPILER_MSVC
		(void)source;
#else
		const auto round_trip = source.template bit_cast<target_t>().template bit_cast<source_t>();
		if (round_trip.template lane<0>() != static_cast<source_t>(1))
			return false;
#endif
	}
	if constexpr (SimdLib::IRegister::Convert<source_register, target_t>)
	{
		const auto converted = source.template convert<target_t>();
		if (converted.template lane<0>() != static_cast<target_t>(1))
			return false;
	}
	return true;
}

/** @brief Verifies every target type for one source type in the constexpr conversion matrix. */
template <class source_t, std::size_t bits, class... target_types>
[[nodiscard]] consteval bool register_conversion_constexpr_targets(register_element_types<target_types...>) noexcept
{
	return (register_conversion_constexpr_cell<source_t, target_types, bits>() && ...);
}

/** @brief Verifies one supported or rejected low-lane widening constexpr cell. */
template <class source_t, class target_t, std::size_t source_bits, std::size_t target_bits>
[[nodiscard]] consteval bool register_widen_constexpr_cell() noexcept
{
	using source_register = SimdLib::Register<source_t, source_bits>;
	const auto source = source_register::broadcast(static_cast<source_t>(1));
	if constexpr (SimdLib::IRegister::WidenLow<source_register, target_t, target_bits>)
	{
		const auto widened = source.template widen_low<target_t, target_bits>();
		return widened.template lane<0>() == static_cast<target_t>(1);
	}
	return true;
}

/** @brief Verifies both supported destination widths for one widening source and target type. */
template <class source_t, class target_t, std::size_t source_bits> [[nodiscard]] consteval bool register_widen_constexpr_widths() noexcept
{
	return register_widen_constexpr_cell<source_t, target_t, source_bits, 128>() && register_widen_constexpr_cell<source_t, target_t, source_bits, 256>();
}

/** @brief Verifies every widening target type for one source type. */
template <class source_t, std::size_t source_bits, class... target_types>
[[nodiscard]] consteval bool register_widen_constexpr_targets(register_element_types<target_types...>) noexcept
{
	return (register_widen_constexpr_widths<source_t, target_types, source_bits>() && ...);
}

/** @brief Verifies constant-evaluated rearrangement, reinterpretation, numeric conversion, and widening. */
template <std::size_t bits> [[nodiscard]] consteval bool register_rearrangement_conversion_constexpr_contract() noexcept
{
#if SIMDLIB_COMPILER_MSVC
	return SimdLib::IRegister::UnpackLow<SimdLib::Register<std::int32_t, bits>> &&
		   SimdLib::IRegister::ShuffleLow<SimdLib::Register<std::int16_t, bits>, 0x1B> && SimdLib::IRegister::Blend<SimdLib::Register<float, bits>, 0xA5> &&
		   SimdLib::IRegister::BitCast<SimdLib::Register<std::int32_t, bits>, float> &&
		   SimdLib::IRegister::Convert<SimdLib::Register<float, bits>, std::int32_t>;
#else
	using bytes_t = SimdLib::Register<std::uint8_t, bits>;
	using words_t = SimdLib::Register<std::int16_t, bits>;
	using ints_t = SimdLib::Register<std::int32_t, bits>;
	using floats_t = SimdLib::Register<float, bits>;
	std::array<std::uint8_t, bytes_t::lane_count> bytes{};
	std::array<std::int16_t, words_t::lane_count> words{};
	std::array<std::int32_t, ints_t::lane_count> ints{};
	for (std::size_t lane = 0; lane < bytes.size(); ++lane)
		bytes[lane] = static_cast<std::uint8_t>(lane + 1);
	for (std::size_t lane = 0; lane < words.size(); ++lane)
		words[lane] = static_cast<std::int16_t>(lane + 1);
	for (std::size_t lane = 0; lane < ints.size(); ++lane)
		ints[lane] = static_cast<std::int32_t>(lane + 1);

	const auto byte_value = bytes_t::from_array(bytes);
	const auto word_value = words_t::from_array(words);
	const auto int_value = ints_t::from_array(ints);
	const auto unpacked = int_value.unpack_low(ints_t::broadcast(40));
	const auto unpacked_high = int_value.unpack_high(ints_t::broadcast(40));
	const auto low_shuffle = word_value.template shuffle_low<0x1B>();
	const auto high_shuffle = word_value.template shuffle_high<0x1B>();
	const auto blended = word_value.template blend<0xA5>(words_t::broadcast(70));
	const auto reinterpreted = int_value.template bit_cast<float>().template bit_cast<std::int32_t>();
	const auto converted = int_value.template convert<float>();
	const auto rounded = floats_t::broadcast(2.5F).template convert<std::int32_t>();
	const auto widened =
		SimdLib::Register<std::int8_t, 128>::from_lanes(-8, -7, -6, -5, -4, -3, -2, -1, 1, 2, 3, 4, 5, 6, 7, 8).template widen_low<std::int16_t, bits>();
	if constexpr (bits == 128)
	{
		const auto shuffled = byte_value.template shuffle<15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0>();
		(void)shuffled;
	}
	else
	{
		const auto shuffled =
			byte_value.template shuffle<15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16>();
		const auto lower = int_value.lower_half();
		(void)shuffled;
		(void)lower;
	}

	const auto unpacked_lanes = unpacked.to_array();
	const auto unpacked_high_lanes = unpacked_high.to_array();
	const auto low_lanes = low_shuffle.to_array();
	const auto high_lanes = high_shuffle.to_array();
	const auto blend_lanes = blended.to_array();
	if (unpacked_lanes[0] != 1 || unpacked_lanes[1] != 40 || unpacked_high_lanes[0] != 3 || unpacked_high_lanes[1] != 40 || reinterpreted.to_array() != ints)
		return false;
	for (std::size_t group = 0; group < words.size(); group += 8)
	{
		for (std::size_t lane = 0; lane < 4; ++lane)
		{
			if (low_lanes[group + lane] != words[group + 3 - lane] || low_lanes[group + 4 + lane] != words[group + 4 + lane] ||
				high_lanes[group + lane] != words[group + lane] || high_lanes[group + 4 + lane] != words[group + 7 - lane])
				return false;
		}
	}
	for (std::size_t lane = 0; lane < blend_lanes.size(); ++lane)
	{
		const std::int16_t expected = (0xA5u & (1u << (lane % 8))) != 0 ? 70 : words[lane];
		if (blend_lanes[lane] != expected)
			return false;
	}
	for (std::size_t lane = 0; lane < converted.lane_count; ++lane)
	{
		if (converted.to_array()[lane] != static_cast<float>(ints[lane]) || rounded.to_array()[lane] != 2)
			return false;
	}
	constexpr std::array<std::int8_t, 16> widen_source{-8, -7, -6, -5, -4, -3, -2, -1, 1, 2, 3, 4, 5, 6, 7, 8};
	const auto widened_lanes = widened.to_array();
	for (std::size_t lane = 0; lane < widened_lanes.size(); ++lane)
	{
		if (widened_lanes[lane] != widen_source[lane])
			return false;
	}
	return true;
#endif
}

#define SIMDLIB_ASSERT_REGISTER_CONSTEXPR(element_type)                                                                                                        \
	static_assert(register_constexpr_contract<element_type, SIMDLIB_REGISTER_TEST_WIDTH>());                                                                   \
	static_assert(register_mask_constexpr_contract<element_type, SIMDLIB_REGISTER_TEST_WIDTH>());                                                              \
	static_assert(register_bitwise_constexpr_contract<element_type, SIMDLIB_REGISTER_TEST_WIDTH>())

SIMDLIB_ASSERT_REGISTER_CONSTEXPR(std::int8_t);
SIMDLIB_ASSERT_REGISTER_CONSTEXPR(std::uint8_t);
SIMDLIB_ASSERT_REGISTER_CONSTEXPR(std::int16_t);
SIMDLIB_ASSERT_REGISTER_CONSTEXPR(std::uint16_t);
SIMDLIB_ASSERT_REGISTER_CONSTEXPR(std::int32_t);
SIMDLIB_ASSERT_REGISTER_CONSTEXPR(std::uint32_t);
SIMDLIB_ASSERT_REGISTER_CONSTEXPR(std::int64_t);
SIMDLIB_ASSERT_REGISTER_CONSTEXPR(std::uint64_t);
SIMDLIB_ASSERT_REGISTER_CONSTEXPR(float);
SIMDLIB_ASSERT_REGISTER_CONSTEXPR(double);

#undef SIMDLIB_ASSERT_REGISTER_CONSTEXPR

#define SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(element_type) static_assert(register_lane_shift_constexpr_contract<element_type, SIMDLIB_REGISTER_TEST_WIDTH>())

SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::int8_t);
SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::uint8_t);
SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::int16_t);
SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::uint16_t);
SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::int32_t);
SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::uint32_t);
SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::int64_t);
SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::uint64_t);

#undef SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR

#define SIMDLIB_ASSERT_REGISTER_LOGICAL_SHUFFLE_CONSTEXPR(element_type)                                                                                        \
	static_assert(register_logical_shuffle_contract<element_type, SIMDLIB_REGISTER_TEST_WIDTH>())

SIMDLIB_ASSERT_REGISTER_LOGICAL_SHUFFLE_CONSTEXPR(std::int8_t);
SIMDLIB_ASSERT_REGISTER_LOGICAL_SHUFFLE_CONSTEXPR(std::uint8_t);
SIMDLIB_ASSERT_REGISTER_LOGICAL_SHUFFLE_CONSTEXPR(std::int16_t);
SIMDLIB_ASSERT_REGISTER_LOGICAL_SHUFFLE_CONSTEXPR(std::uint16_t);
SIMDLIB_ASSERT_REGISTER_LOGICAL_SHUFFLE_CONSTEXPR(std::int32_t);
SIMDLIB_ASSERT_REGISTER_LOGICAL_SHUFFLE_CONSTEXPR(std::uint32_t);
SIMDLIB_ASSERT_REGISTER_LOGICAL_SHUFFLE_CONSTEXPR(std::int64_t);
SIMDLIB_ASSERT_REGISTER_LOGICAL_SHUFFLE_CONSTEXPR(std::uint64_t);
SIMDLIB_ASSERT_REGISTER_LOGICAL_SHUFFLE_CONSTEXPR(float);
SIMDLIB_ASSERT_REGISTER_LOGICAL_SHUFFLE_CONSTEXPR(double);

#undef SIMDLIB_ASSERT_REGISTER_LOGICAL_SHUFFLE_CONSTEXPR

#define SIMDLIB_ASSERT_REGISTER_BYTE_SHUFFLE_CONSTEXPR(element_type) static_assert(register_byte_shuffle_contract<element_type, SIMDLIB_REGISTER_TEST_WIDTH>())

SIMDLIB_ASSERT_REGISTER_BYTE_SHUFFLE_CONSTEXPR(std::int32_t);
SIMDLIB_ASSERT_REGISTER_BYTE_SHUFFLE_CONSTEXPR(double);

#undef SIMDLIB_ASSERT_REGISTER_BYTE_SHUFFLE_CONSTEXPR

static_assert(register_complete_shift_constexpr_contract());
static_assert(register_rearrangement_conversion_constexpr_contract<SIMDLIB_REGISTER_TEST_WIDTH>());
static_assert(register_position_constexpr_contract<std::int8_t, SIMDLIB_REGISTER_TEST_WIDTH>());
static_assert(register_position_constexpr_contract<std::uint8_t, SIMDLIB_REGISTER_TEST_WIDTH>());
static_assert(register_position_constexpr_contract<std::int16_t, SIMDLIB_REGISTER_TEST_WIDTH>());
static_assert(register_position_constexpr_contract<std::uint16_t, SIMDLIB_REGISTER_TEST_WIDTH>());
static_assert(register_position_constexpr_contract<std::int32_t, SIMDLIB_REGISTER_TEST_WIDTH>());
static_assert(register_position_constexpr_contract<std::uint32_t, SIMDLIB_REGISTER_TEST_WIDTH>());
static_assert(register_position_constexpr_contract<std::int64_t, SIMDLIB_REGISTER_TEST_WIDTH>());
static_assert(register_position_constexpr_contract<std::uint64_t, SIMDLIB_REGISTER_TEST_WIDTH>());
#define SIMDLIB_ASSERT_REGISTER_CONVERSION_CONSTEXPR(source_type)                                                                                              \
	static_assert(register_conversion_constexpr_targets<source_type, SIMDLIB_REGISTER_TEST_WIDTH>(supported_register_element_types{}));                        \
	static_assert(register_widen_constexpr_targets<source_type, SIMDLIB_REGISTER_TEST_WIDTH>(supported_register_element_types{}))

SIMDLIB_ASSERT_REGISTER_CONVERSION_CONSTEXPR(std::int8_t);
SIMDLIB_ASSERT_REGISTER_CONVERSION_CONSTEXPR(std::uint8_t);
SIMDLIB_ASSERT_REGISTER_CONVERSION_CONSTEXPR(std::int16_t);
SIMDLIB_ASSERT_REGISTER_CONVERSION_CONSTEXPR(std::uint16_t);
SIMDLIB_ASSERT_REGISTER_CONVERSION_CONSTEXPR(std::int32_t);
SIMDLIB_ASSERT_REGISTER_CONVERSION_CONSTEXPR(std::uint32_t);
SIMDLIB_ASSERT_REGISTER_CONVERSION_CONSTEXPR(std::int64_t);
SIMDLIB_ASSERT_REGISTER_CONVERSION_CONSTEXPR(std::uint64_t);
SIMDLIB_ASSERT_REGISTER_CONVERSION_CONSTEXPR(float);
SIMDLIB_ASSERT_REGISTER_CONVERSION_CONSTEXPR(double);

#undef SIMDLIB_ASSERT_REGISTER_CONVERSION_CONSTEXPR

} // namespace
