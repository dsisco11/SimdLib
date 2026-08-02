#include <SimdLib/PartialRegister.h>
#include <SimdLib/IPartialRegisterMask.h>

#include <cstdint>
#include <type_traits>

namespace
{

/** @brief Reports whether a mask type exposes mutable intersection assignment. */
template <class mask_t>
concept HasBitwiseAndAssign = requires(mask_t lhs, mask_t rhs) { lhs &= rhs; };

/** @brief Reports whether a mask type exposes mutable union assignment. */
template <class mask_t>
concept HasBitwiseOrAssign = requires(mask_t lhs, mask_t rhs) { lhs |= rhs; };

/** @brief Reports whether a mask type exposes mutable exclusive-union assignment. */
template <class mask_t>
concept HasBitwiseXorAssign = requires(mask_t lhs, mask_t rhs) { lhs ^= rhs; };

/** @brief Checks object layout and compile-time geometry for one partial SIMD value. */
template <class element_t, std::size_t bits, std::size_t active_lane_count> consteval bool has_partial_register_shape()
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_lane_count>;
	using native_t = typename value_t::native_type;
	return std::is_final_v<value_t> && std::is_aggregate_v<value_t> && !std::is_polymorphic_v<value_t> && sizeof(value_t) == sizeof(native_t) && alignof(value_t) == alignof(native_t) &&
		   std::is_standard_layout_v<value_t> &&
		   std::is_trivially_copy_constructible_v<value_t> && !std::is_trivially_default_constructible_v<value_t> &&
		   std::is_trivially_move_constructible_v<value_t> && std::is_trivially_copy_assignable_v<value_t> &&
		   std::is_trivially_move_assignable_v<value_t> && std::is_trivially_destructible_v<value_t> && std::is_trivially_copyable_v<value_t> &&
		   value_t::register_width == bits && value_t::byte_count == bits / 8 && value_t::native_lane_count == bits / (sizeof(element_t) * 8) &&
		   value_t::lane_count == active_lane_count && value_t::active_byte_count == active_lane_count * sizeof(element_t) &&
		   value_t::inactive_lane_count == value_t::native_lane_count - active_lane_count;
}

/** @brief Checks object layout, concepts, and metadata for one partial predicate geometry. */
template <class element_t, std::size_t bits, std::size_t active_lane_count> consteval bool has_partial_register_mask_shape()
{
	using mask_t = SimdLib::PartialRegisterMask<element_t, bits, active_lane_count>;
	using native_t = typename mask_t::native_type;
	return std::is_final_v<mask_t> && std::is_aggregate_v<mask_t> && !std::is_polymorphic_v<mask_t> && sizeof(mask_t) == sizeof(native_t) && alignof(mask_t) == alignof(native_t) &&
		   std::is_standard_layout_v<mask_t> && std::is_trivially_copy_constructible_v<mask_t> && !std::is_trivially_default_constructible_v<mask_t> &&
		   std::is_trivially_move_constructible_v<mask_t> && std::is_trivially_copy_assignable_v<mask_t> && std::is_trivially_move_assignable_v<mask_t> &&
		   std::is_trivially_destructible_v<mask_t> && std::is_trivially_copyable_v<mask_t> && SimdLib::IPartialRegisterMask::Type<mask_t> &&
		   SimdLib::IPartialRegisterMask::Reductions<mask_t> && SimdLib::IPartialRegisterMask::Composition<mask_t> && SimdLib::IPartialRegisterMask::Select<mask_t> &&
		   !HasBitwiseAndAssign<mask_t> && !HasBitwiseOrAssign<mask_t> && !HasBitwiseXorAssign<mask_t> &&
		   mask_t::register_width == bits && mask_t::native_lane_count == bits / (sizeof(element_t) * 8) && mask_t::lane_count == active_lane_count;
}

/** @brief Checks every supported partial extent for one element and register-width pair. */
template <class element_t, std::size_t bits, std::size_t... active_lane_counts>
consteval bool has_partial_register_shapes(std::index_sequence<active_lane_counts...>)
{
	return (has_partial_register_shape<element_t, bits, active_lane_counts + 1>() && ...);
}

/** @brief Checks every supported partial predicate extent for one element and register-width pair. */
template <class element_t, std::size_t bits, std::size_t... active_lane_counts>
consteval bool has_partial_register_mask_shapes(std::index_sequence<active_lane_counts...>)
{
	return (has_partial_register_mask_shape<element_t, bits, active_lane_counts + 1>() && ...);
}

/** @brief Checks all non-complete logical extents for one element and register-width pair. */
template <class element_t, std::size_t bits> consteval bool has_all_partial_register_shapes()
{
	return has_partial_register_shapes<element_t, bits>(
		std::make_index_sequence<SimdLib::Api<bits, element_t>::element_count - 1>{});
}

/** @brief Checks all non-complete logical predicate extents for one element and register-width pair. */
template <class element_t, std::size_t bits> consteval bool has_all_partial_register_mask_shapes()
{
	return has_partial_register_mask_shapes<element_t, bits>(
		std::make_index_sequence<SimdLib::Api<bits, element_t>::element_count - 1>{});
}

#define SIMDLIB_ASSERT_PARTIAL_REGISTER_SHAPES(element_type, width) \
	static_assert(has_all_partial_register_shapes<element_type, width>()); \
	static_assert(has_all_partial_register_mask_shapes<element_type, width>())

SIMDLIB_ASSERT_PARTIAL_REGISTER_SHAPES(std::int8_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_PARTIAL_REGISTER_SHAPES(std::uint8_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_PARTIAL_REGISTER_SHAPES(std::int16_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_PARTIAL_REGISTER_SHAPES(std::uint16_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_PARTIAL_REGISTER_SHAPES(std::int32_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_PARTIAL_REGISTER_SHAPES(std::uint32_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_PARTIAL_REGISTER_SHAPES(std::int64_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_PARTIAL_REGISTER_SHAPES(std::uint64_t, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_PARTIAL_REGISTER_SHAPES(float, SIMDLIB_REGISTER_TEST_WIDTH);
SIMDLIB_ASSERT_PARTIAL_REGISTER_SHAPES(double, SIMDLIB_REGISTER_TEST_WIDTH);

#undef SIMDLIB_ASSERT_PARTIAL_REGISTER_SHAPES

using native_partial_register_type = SimdLib::NativePartialRegister<float, 1>;
static_assert(native_partial_register_type::register_width == (SimdLib::is_register_available_v<float, 256> ? 256 : 128));

} // namespace
