#include <SimdLib/Register.h>

#include <array>
#include <cstddef>
#include <cstdint>
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
	const register_type native_value(array_value.native());
	(void)value;
	(void)zero;
	(void)broadcast;
	(void)lane_value;
	(void)native_value;
	return true;
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
	const register_type native_value(array_value.native());
	if (native_value.to_array() != values || array_value.template lane<0>() != values.front() ||
		array_value.template lane<register_type::lane_count - 1>() != values.back())
		return false;
	const auto changed_lanes =
		array_value.template with_lane<register_type::lane_count - 1>(static_cast<element_t>(43)).to_array();
	return changed_lanes.front() == values.front() && changed_lanes.back() == static_cast<element_t>(43);
#endif
}

#define SIMDLIB_ASSERT_REGISTER_CONSTEXPR(element_type) \
	static_assert(register_constexpr_contract<element_type, SIMDLIB_REGISTER_TEST_WIDTH>())

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

} // namespace
