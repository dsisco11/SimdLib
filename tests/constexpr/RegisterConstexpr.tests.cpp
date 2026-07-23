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

/** @brief Constructs a register from an expanded compile-time lane array. */
template <class register_t, std::size_t... indices>
[[nodiscard]] consteval register_t from_lanes(
	const std::array<typename register_t::element_type, register_t::lane_count> &values,
	std::index_sequence<indices...>) noexcept
{
	return register_t::from_lanes(values[indices]...);
}

/** @brief Verifies all constant-evaluable Register construction and lane operations. */
template <class element_t, std::size_t bits>
[[nodiscard]] consteval bool register_constexpr_contract() noexcept
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
	const register_type lane_value = from_lanes<register_type>(values,
		std::make_index_sequence<register_type::lane_count>{});
	const register_type native_value{array_value.native};
	const element_t first_lane = array_value.template lane<0>();
	const register_type changed_value =
		array_value.template with_lane<register_type::lane_count - 1>(static_cast<element_t>(43));
	(void)value;
	(void)zero;
	(void)broadcast;
	(void)lane_value;
	(void)native_value;
	return first_lane == values.front() &&
		changed_value.template lane<register_type::lane_count - 1>() == static_cast<element_t>(43);
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
	const auto changed_lanes =
		array_value.template with_lane<register_type::lane_count - 1>(static_cast<element_t>(43)).to_array();
	return changed_lanes.front() == values.front() && changed_lanes.back() == static_cast<element_t>(43);
#endif
}

/** @brief Verifies constant-evaluated mask comparisons, combination, reductions, and selection. */
template <class element_t, std::size_t bits>
[[nodiscard]] consteval bool register_mask_constexpr_contract() noexcept
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
	if (!(greater | less).all() || !(greater & less).none() || (greater ^ less).bits() != (greater | less).bits())
		return false;
	const auto selected = greater.select(register_type::broadcast(static_cast<element_t>(11)),
		register_type::broadcast(static_cast<element_t>(22))).to_array();
	for (std::size_t index = 0; index < selected.size(); ++index)
	{
		if (selected[index] != static_cast<element_t>((index % 2) == 0 ? 11 : 22))
			return false;
	}
	return lhs == lhs && lhs != rhs && lhs.compare_greater_equal(rhs).bits() == expected &&
		lhs.compare_less_equal(rhs).bits() == less.bits();
#endif
}

/** @brief Verifies constant-evaluated bitwise expressions, assignments, and sign reductions. */
template <class element_t, std::size_t bits>
[[nodiscard]] consteval bool register_bitwise_constexpr_contract() noexcept
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
	if ((value & value).to_array() != value.to_array() || (value | zero).to_array() != value.to_array() ||
		(value ^ value).to_array() != zero.to_array() || (~~value).to_array() != value.to_array() ||
		value.andnot(value).to_array() != zero.to_array())
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
	if ((value << 0).to_array() != value.to_array() || (value << lane_width).to_array() != zeros ||
		(value << (lane_width + 1)).to_array() != zeros ||
		value.logical_shift_right(lane_width).to_array() != zeros ||
		value.logical_shift_right(lane_width + 1).to_array() != zeros)
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
	return value.byte_shift_left(0).to_array() == lanes && value.byte_shift_left(16).to_array() == zeros &&
		value.byte_shift_left(17).to_array() == zeros && value.byte_shift_right(16).to_array() == zeros &&
		value.template bit_shift_left<128>().to_array() == zeros &&
		value.template bit_shift_left<129>().to_array() == zeros &&
		value.template bit_shift_right<128>().to_array() == zeros &&
		value.template bit_shift_right<129>().to_array() == zeros;
#endif
}

#define SIMDLIB_ASSERT_REGISTER_CONSTEXPR(element_type) \
	static_assert(register_constexpr_contract<element_type, SIMDLIB_REGISTER_TEST_WIDTH>()); \
	static_assert(register_mask_constexpr_contract<element_type, SIMDLIB_REGISTER_TEST_WIDTH>()); \
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

#define SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(element_type) \
	static_assert(register_lane_shift_constexpr_contract<element_type, SIMDLIB_REGISTER_TEST_WIDTH>())

SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::int8_t);
SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::uint8_t);
SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::int16_t);
SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::uint16_t);
SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::int32_t);
SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::uint32_t);
SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::int64_t);
SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR(std::uint64_t);

#undef SIMDLIB_ASSERT_REGISTER_SHIFT_CONSTEXPR

static_assert(register_complete_shift_constexpr_contract());

} // namespace
