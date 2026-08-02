#include <SimdLib/Api.h>

#include <array>
#include <cstddef>
#include <cstdint>

#ifndef SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH
#error "SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH must select the fixture register width"
#endif

namespace
{

/** @brief Clears the inactive suffix required by the partial-predicate invariant. */
[[nodiscard]] constexpr typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t normalize_partial_mask_native(
	typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t native) noexcept
{
	using api_t = SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>;
	constexpr auto active_lane_filter = []() constexpr {
		std::array<std::uint32_t, api_t::element_count> lanes{};
		for (std::size_t lane = 0; lane < 3; ++lane)
			lanes[lane] = 0xffffffffU;
		return lanes;
	}();
	return api_t::bitwise_and(native, api_t::construct(active_lane_filter));
}

} // namespace

/** @brief Raw Api mirror for immutable partial-predicate composition with required suffix projection. */
extern "C" [[nodiscard]] typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t
simdlib_partial_mask_codegen_compose(typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t lhs,
	typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t rhs) noexcept
{
	using api_t = SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>;
	const auto normalized_rhs = normalize_partial_mask_native(rhs);
	const auto normalized_lhs = normalize_partial_mask_native(lhs);
	return normalize_partial_mask_native(api_t::bitwise_not(api_t::bitwise_xor(normalized_rhs, normalized_lhs)));
}

/** @brief Raw Api mirror for the PartialRegister inactive-lane projection boundary. */
extern "C" [[nodiscard]] typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t
simdlib_partial_register_codegen_import(typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t native) noexcept
{
	return normalize_partial_mask_native(native);
}
