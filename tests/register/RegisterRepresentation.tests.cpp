#include <SimdLib/IRegister.h>
#include <SimdLib/Register.h>

#include <cstdint>
#include <type_traits>
#include <utility>

namespace
{

/** @brief Reports whether any intentionally unsupported scalar arithmetic expression is available. */
template <class value_t>
concept has_scalar_arithmetic = requires(value_t value, typename value_t::element_type scalar) {
	value + scalar;
	value - scalar;
	value * scalar;
	value / scalar;
};

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

/** @brief Reports whether a Register accepts one complete homogeneous logical lane list. */
template <class value_t, std::size_t... indices> consteval bool has_complete_lane_construction(std::index_sequence<indices...>)
{
	using element_t = typename value_t::element_type;
	return SimdLib::IRegister::FromLanes<value_t, decltype((static_cast<void>(indices), element_t{}))...>;
}
/** @brief Checks Register and RegisterMask shape invariants for one element type and width. */
template <class element_t, std::size_t bits> consteval bool has_complete_register_shapes()
{
	using register_type = SimdLib::Register<element_t, bits>;
	using mask_type = SimdLib::RegisterMask<element_t, bits>;
	static_assert(std::is_aggregate_v<register_type>);
	static_assert(std::is_aggregate_v<mask_type>);
	return SimdLib::RegisterAvailable<element_t, bits> && SimdLib::is_register_available_v<element_t, bits> && SimdLib::IRegister::Type<register_type> &&
		   SimdLib::IRegister::Zero<register_type> && SimdLib::IRegister::Broadcast<register_type> &&
		   has_complete_lane_construction<register_type>(std::make_index_sequence<register_type::lane_count>{}) &&
		   SimdLib::IRegister::FromArray<register_type> && SimdLib::IRegister::Load<register_type> && SimdLib::IRegister::LoadAligned<register_type> &&
		   SimdLib::IRegister::LoadBytes<register_type> && SimdLib::IRegister::Store<register_type> && SimdLib::IRegister::StoreAligned<register_type> &&
		   SimdLib::IRegister::StoreBytes<register_type> && SimdLib::IRegister::ToArray<register_type> && SimdLib::IRegister::Lane<register_type, 0> &&
		   SimdLib::IRegister::WithLane<register_type, 0> && SimdLib::IRegister::BitwiseAnd<register_type> && SimdLib::IRegister::BitwiseOr<register_type> &&
		   SimdLib::IRegister::BitwiseXor<register_type> && SimdLib::IRegister::BitwiseNot<register_type> && SimdLib::IRegister::BitwiseAndNot<register_type> &&
		   SimdLib::IRegister::Movemask<register_type> && SimdLib::IRegister::LaneSignBits<register_type> && SimdLib::IRegister::CompareEqual<register_type> &&
		   SimdLib::IRegister::CompareGreater<register_type> && SimdLib::IRegister::CompareGreaterEqual<register_type> &&
		   SimdLib::IRegister::CompareLess<register_type> && SimdLib::IRegister::CompareLessEqual<register_type> && SimdLib::IRegister::Equal<register_type> &&
		   SimdLib::IRegister::NotEqual<register_type> && has_complete_register_value_traits<register_type>() &&
		   has_complete_register_value_traits<mask_type>() && has_mask_construction_contract<mask_type, register_type>() &&
		   !SimdLib::IRegister::Lane<register_type, register_type::lane_count> && !SimdLib::IRegister::WithLane<register_type, register_type::lane_count> &&
		   has_no_compound_assignments<register_type>() && has_no_compound_assignments<mask_type>() && register_type::register_width == bits &&
		   register_type::byte_count == bits / 8 && register_type::lane_count == bits / (sizeof(element_t) * 8) && mask_type::register_width == bits &&
		   mask_type::lane_count == register_type::lane_count && std::same_as<typename mask_type::bits_type, std::uint32_t>;
}

/** @brief Checks the exact operator surface for one element type and width. */
template <class element_t, std::size_t bits> consteval bool has_exact_operation_constraints()
{
	using register_type = SimdLib::Register<element_t, bits>;
	constexpr bool integral = std::is_integral_v<element_t>;
	return !has_scalar_arithmetic<register_type> && SimdLib::IRegister::Modulus<register_type> == integral &&
		   SimdLib::IRegister::ShiftLeft<register_type> == integral && SimdLib::IRegister::LogicalShiftRight<register_type> == integral &&
		   SimdLib::IRegister::ShiftRight<register_type> == integral && SimdLib::IRegister::ByteShiftLeft<register_type> == (integral && bits == 128) &&
		   SimdLib::IRegister::ByteShiftRight<register_type> == (integral && bits == 128) &&
		   SimdLib::IRegister::BitShiftLeft<register_type> == (integral && bits == 128) &&
		   SimdLib::IRegister::BitShiftRight<register_type> == (integral && bits == 128) &&
		   SimdLib::IRegister::IndexedBitShiftLeft<register_type, 1> == (integral && bits == 128) &&
		   SimdLib::IRegister::IndexedBitShiftRight<register_type, 1> == (integral && bits == 128) &&
		   !SimdLib::IRegister::IndexedBitShiftLeft<register_type, -1> && !SimdLib::IRegister::IndexedBitShiftRight<register_type, -1>;
}

#define SIMDLIB_ASSERT_REGISTER_SHAPES(element_type, width)                                                                                                    \
	static_assert(has_complete_register_shapes<element_type, width>());                                                                                        \
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
