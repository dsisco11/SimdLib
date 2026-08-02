#include <SimdLib/PartialRegister.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace
{

/** @brief Reports whether one scalar value has an all-bits-zero representation. */
template <class element_t> [[nodiscard]] consteval bool has_zero_bits(element_t value) noexcept
{
	const auto bytes = std::bit_cast<std::array<std::byte, sizeof(element_t)>>(value);
	for (const auto byte : bytes)
		if (byte != std::byte{})
			return false;
	return true;
}

/** @brief Verifies constant-evaluated partial construction and active-only observation. */
consteval bool has_constexpr_partial_construction() noexcept
{
	using value_t = SimdLib::PartialRegister<std::uint32_t, 128, 3>;
	using api_t = typename value_t::api_type;
	constexpr std::array<std::uint32_t, 3> source{1U, 2U, 3U};
	const auto zero = value_t::zero();
	const auto broadcast = value_t::broadcast(7U);
	const auto listed = value_t::from_lanes(1U, 2U, 3U);
	const auto array_value = value_t::from_array(source);
	const auto listed_native = api_t::to_array(listed.to_native());
	return zero.to_array() == std::array<std::uint32_t, 3>{} &&
		broadcast.to_array() == std::array<std::uint32_t, 3>{7U, 7U, 7U} && listed.to_array() == source &&
		array_value.to_array() == source && listed_native[0] == 1U && listed_native[1] == 2U && listed_native[2] == 3U &&
		has_zero_bits(listed_native[3]);
}

/** @brief Verifies constant-evaluated active lane extraction and replacement. */
consteval bool has_constexpr_partial_lane_access() noexcept
{
	using value_t = SimdLib::PartialRegister<std::uint64_t, 128, 1>;
	using api_t = typename value_t::api_type;
	const auto original = value_t::from_lanes(11U);
	const auto replaced = original.template with_lane<0>(29U);
	const auto native = api_t::to_array(replaced.to_native());
	return original.template lane<0>() == 11U && replaced.template lane<0>() == 29U && native[0] == 29U && has_zero_bits(native[1]);
}

static_assert(has_constexpr_partial_construction());
static_assert(has_constexpr_partial_lane_access());

} // namespace
