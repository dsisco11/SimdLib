#include <SimdLib/PartialRegister.h>

#include <array>
#include <cstddef>
#include <cstdint>

#ifndef SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH
#error "SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH must select the fixture register width"
#endif

constexpr std::size_t partial_codegen_active_lane_count = SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH == 256 ? 5 : 3;

namespace
{

/** @brief Clears the inactive suffix required by the partial-predicate invariant. */
[[nodiscard]] constexpr typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t normalize_partial_mask_native(
	typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t native) noexcept
{
	using api_t = SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>;
	constexpr auto active_lane_filter = []() constexpr {
		std::array<std::uint32_t, api_t::element_count> lanes{};
		for (std::size_t lane = 0; lane < partial_codegen_active_lane_count; ++lane)
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
	SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_register_codegen_import(
		typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t native) noexcept
{
	return normalize_partial_mask_native(native);
}

/** @brief Raw Api mirror for closed three-lane partial-register addition. */
extern "C" [[nodiscard]] typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t
	SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_register_codegen_add(
	SimdLib::PartialRegister<std::uint32_t, SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, partial_codegen_active_lane_count> lhs,
	SimdLib::PartialRegister<std::uint32_t, SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, partial_codegen_active_lane_count> rhs) noexcept
{
	using api_t = SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>;
	return api_t::add(lhs.native, rhs.native);
}

/** @brief Raw Api mirror for neutralized three-lane partial-register division. */
extern "C" [[nodiscard]] typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::int32_t>::vector_t
	SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS simdlib_partial_register_codegen_divide(
	SimdLib::PartialRegister<std::int32_t, SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, partial_codegen_active_lane_count> lhs,
	SimdLib::PartialRegister<std::int32_t, SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, partial_codegen_active_lane_count> rhs) noexcept
{
	using api_t = SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::int32_t>;
	constexpr auto inactive_identity = []() constexpr {
		std::array<std::int32_t, api_t::element_count> lanes{};
		for (std::size_t lane = partial_codegen_active_lane_count; lane < api_t::element_count; ++lane)
			lanes[lane] = 1;
		return lanes;
	}();
	constexpr auto active_filter = []() constexpr {
		std::array<std::int32_t, api_t::element_count> lanes{};
		for (std::size_t lane = 0; lane < partial_codegen_active_lane_count; ++lane)
			lanes[lane] = -1;
		return lanes;
	}();
	const auto divisors = api_t::bitwise_or(rhs.native, api_t::construct(inactive_identity));
	return api_t::bitwise_and(api_t::divide(lhs.native, divisors), api_t::construct(active_filter));
}
