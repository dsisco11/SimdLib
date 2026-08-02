#include <SimdLib/PartialRegisterMask.h>

#include <cstdint>

#ifndef SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH
#error "SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH must select the fixture register width"
#endif

/** @brief Composes two partial predicates without introducing mutable predicate storage. */
extern "C" [[nodiscard]] typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t
simdlib_partial_mask_codegen_compose(typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t lhs,
	typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t rhs) noexcept
{
	using mask_t = SimdLib::PartialRegisterMask<std::uint32_t, SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, 3>;
	return (~(mask_t::from_native(lhs) ^ mask_t::from_native(rhs))).to_native();
}

/** @brief Imports one native value through the PartialRegister inactive-lane projection boundary. */
extern "C" [[nodiscard]] typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t
simdlib_partial_register_codegen_import(typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t native) noexcept
{
	using register_t = SimdLib::PartialRegister<std::uint32_t, SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, 3>;
	return register_t::from_native(native).to_native();
}

/** @brief Broadcasts one scalar directly into a three-lane partial register. */
extern "C" [[nodiscard]] typename SimdLib::Api<SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, std::uint32_t>::vector_t
simdlib_partial_register_codegen_broadcast(std::uint32_t value) noexcept
{
	using register_t = SimdLib::PartialRegister<std::uint32_t, SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH, 3>;
	return register_t::broadcast(value).native;
}
