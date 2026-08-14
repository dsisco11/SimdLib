#include <SimdLib/PartialRegisterMask.h>

#include <cstddef>
#include <cstdint>

#ifndef SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH
#error "SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH must select the fixture register width"
#endif

constexpr std::size_t partial_codegen_active_lane_count = SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH == 256 ? 5 : 3;

/** @brief Composes two partial predicates without introducing mutable predicate storage. */
extern "C" [[nodiscard]] typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t simdlib_partial_mask_codegen_compose(
	typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t lhs,
	typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t rhs) noexcept
{
	using mask_t = SimdLib::PartialRegisterMask<std::uint32_t, SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, partial_codegen_active_lane_count>;
	return (~(mask_t::from_native(lhs) ^ mask_t::from_native(rhs))).to_native();
}

/** @brief Imports one native value through the PartialRegister inactive-lane projection boundary. */
extern "C" [[nodiscard]] typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS
simdlib_partial_register_codegen_import(typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t native) noexcept
{
	using register_t = SimdLib::PartialRegister<std::uint32_t, SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, partial_codegen_active_lane_count>;
	return register_t::from_native(native).to_native();
}

/** @brief Adds two already-canonical three-lane partial registers without suffix repair. */
extern "C" [[nodiscard]] typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS
simdlib_partial_register_codegen_add(
	SimdLib::PartialRegister<std::uint32_t, SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, partial_codegen_active_lane_count> lhs,
	SimdLib::PartialRegister<std::uint32_t, SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, partial_codegen_active_lane_count> rhs) noexcept
{
	return (lhs + rhs).native;
}

/** @brief Divides three active lanes while neutralizing and clearing the inactive divisor suffix. */
extern "C" [[nodiscard]] typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::int32_t>::vector_t SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS
simdlib_partial_register_codegen_divide(
	SimdLib::PartialRegister<std::int32_t, SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, partial_codegen_active_lane_count> lhs,
	SimdLib::PartialRegister<std::int32_t, SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, partial_codegen_active_lane_count> rhs) noexcept
{
	return (lhs / rhs).native;
}
