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
	return SimdLib::RegisterAvailable<element_t, bits> &&
		SimdLib::is_register_available_v<element_t, bits> &&
		has_complete_register_value_traits<register_type>() &&
		has_complete_register_value_traits<mask_type>() &&
		!has_out_of_range_lane<register_type> && !has_out_of_range_with_lane<register_type> &&
		register_type::register_width == bits && register_type::byte_count == bits / 8 &&
		register_type::lane_count == bits / (sizeof(element_t) * 8);
}

#define SIMDLIB_ASSERT_REGISTER_SHAPES(element_type, width) \
	static_assert(has_complete_register_shapes<element_type, width>())

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
