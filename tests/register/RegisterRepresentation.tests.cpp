#include <SimdLib/Register.h>

#include <cstdint>
#include <type_traits>

namespace
{

/** @brief Reports whether a compile-time lane outside the logical register is observable. */
template <class value_t>
concept has_out_of_range_lane = requires(value_t value) { value.template lane<value_t::lane_count>(); };

/** @brief Reports whether a compile-time lane outside the logical register is replaceable. */
template <class value_t>
concept has_out_of_range_with_lane = requires(value_t value) {
	value.template with_lane<value_t::lane_count>(typename value_t::element_type{});
};

/** @brief Reports whether any intentionally unsupported scalar arithmetic expression is available. */
template <class value_t>
concept has_scalar_arithmetic = requires(value_t value, typename value_t::element_type scalar) {
	value + scalar;
	value - scalar;
	value * scalar;
	value / scalar;
};

/** @brief Reports whether remainder operators are available for a register type. */
template <class value_t>
concept has_remainder = requires(value_t lhs, value_t rhs) { lhs % rhs; };

/** @brief Verifies that the intentionally disabled compound-assignment surface remains unavailable. */
template <class value_t>
consteval bool has_no_compound_assignments()
{
	return !requires(value_t lhs, value_t rhs) { lhs += rhs; } &&
		!requires(value_t lhs, value_t rhs) { lhs -= rhs; } &&
		!requires(value_t lhs, value_t rhs) { lhs *= rhs; } &&
		!requires(value_t lhs, value_t rhs) { lhs /= rhs; } &&
		!requires(value_t lhs, value_t rhs) { lhs %= rhs; } &&
		!requires(value_t lhs, value_t rhs) { lhs &= rhs; } &&
		!requires(value_t lhs, value_t rhs) { lhs |= rhs; } &&
		!requires(value_t lhs, value_t rhs) { lhs ^= rhs; } &&
		!requires(value_t lhs) { lhs <<= 1; } &&
		!requires(value_t lhs) { lhs >>= 1; };
}

/** @brief Reports whether per-lane shift operators are available for a register type. */
template <class value_t>
concept has_lane_shifts = requires(value_t value) {
	value << 1;
	value >> 1;
	value.logical_shift_right(1);
};

/** @brief Reports whether 128-bit-only complete-register shifts are available. */
template <class value_t>
concept has_complete_register_shifts = requires(value_t value) {
	value.byte_shift_left(1);
	value.byte_shift_right(1);
	value.bit_shift_left(1);
	value.bit_shift_right(1);
	value.template bit_shift_left<1>();
	value.template bit_shift_right<1>();
};

/** @brief Reports whether an invalid negative static complete-register shift is accepted. */
template <class value_t>
concept has_negative_static_shift = requires(value_t value) {
	value.template bit_shift_left<-1>();
	value.template bit_shift_right<-1>();
};

/** @brief Checks the aggregate predicate construction and conversion contract. */
template <class mask_t, class register_t>
consteval bool has_mask_construction_contract()
{
	return std::is_constructible_v<mask_t, typename mask_t::native_type> &&
		!std::is_constructible_v<mask_t, typename mask_t::bits_type> &&
		!std::is_constructible_v<mask_t, register_t> && !std::is_convertible_v<mask_t, bool>;
}

/** @brief Checks the required object-model traits for one register-shaped value type. */
template <class value_t>
consteval bool has_complete_register_value_traits()
{
	using native_type = typename value_t::native_type;
	return sizeof(value_t) == sizeof(native_type) && alignof(value_t) == alignof(native_type) &&
		std::is_standard_layout_v<value_t> && std::is_trivially_copy_constructible_v<value_t> &&
		!std::is_trivially_default_constructible_v<value_t> &&
		std::is_trivially_move_constructible_v<value_t> && std::is_trivially_copy_assignable_v<value_t> &&
		std::is_trivially_move_assignable_v<value_t> && std::is_trivially_destructible_v<value_t> &&
		std::is_trivially_copyable_v<value_t>;
}

/** @brief Checks Register and RegisterMask shape invariants for one element type and width. */
template <class element_t, std::size_t bits>
consteval bool has_complete_register_shapes()
{
	using register_type = SimdLib::Register<element_t, bits>;
	using mask_type = SimdLib::RegisterMask<element_t, bits>;
	static_assert(std::is_aggregate_v<register_type>);
	static_assert(std::is_aggregate_v<mask_type>);
	return SimdLib::RegisterAvailable<element_t, bits> &&
		SimdLib::is_register_available_v<element_t, bits> &&
		has_complete_register_value_traits<register_type>() &&
		has_complete_register_value_traits<mask_type>() &&
		has_mask_construction_contract<mask_type, register_type>() &&
		!has_out_of_range_lane<register_type> && !has_out_of_range_with_lane<register_type> &&
		has_no_compound_assignments<register_type>() && has_no_compound_assignments<mask_type>() &&
		register_type::register_width == bits && register_type::byte_count == bits / 8 &&
		register_type::lane_count == bits / (sizeof(element_t) * 8) &&
		mask_type::register_width == bits && mask_type::lane_count == register_type::lane_count &&
		std::same_as<typename mask_type::bits_type, std::uint32_t>;
}

/** @brief Checks the exact operator surface for one element type and width. */
template <class element_t, std::size_t bits>
consteval bool has_exact_operation_constraints()
{
	using register_type = SimdLib::Register<element_t, bits>;
	constexpr bool integral = std::is_integral_v<element_t>;
	return !has_scalar_arithmetic<register_type> && has_remainder<register_type> == integral &&
		has_lane_shifts<register_type> == integral &&
		has_complete_register_shifts<register_type> == (integral && bits == 128) &&
		!has_negative_static_shift<register_type>;
}

#define SIMDLIB_ASSERT_REGISTER_SHAPES(element_type, width) \
	static_assert(has_complete_register_shapes<element_type, width>()); \
	static_assert(has_exact_operation_constraints<element_type, width>())

SIMDLIB_ASSERT_REGISTER_SHAPES(std::int8_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_REGISTER_SHAPES(std::uint8_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_REGISTER_SHAPES(std::int16_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_REGISTER_SHAPES(std::uint16_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_REGISTER_SHAPES(std::int32_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_REGISTER_SHAPES(std::uint32_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_REGISTER_SHAPES(std::int64_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_REGISTER_SHAPES(std::uint64_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_REGISTER_SHAPES(float, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_REGISTER_SHAPES(double, SIMDLIB_REGISTER_TEST_WIDTH);

#undef SIMDLIB_ASSERT_REGISTER_SHAPES

using native_register_type = SimdLib::NativeRegister<float>;
static_assert(native_register_type::register_width == (SimdLib::is_register_available_v<float, 256> ? 256 : 128));

} // namespace
