#include <SimdLib/PartialRegister.h>
#include <cstdint>

using value_type = SimdLib::PartialRegister<std::uint32_t, SIMDLIB_CONTRACT_WIDTH, SIMDLIB_CONTRACT_ACTIVE>;

/** @brief Imports arbitrary native lanes and observes the sanitized public value. */
extern "C" value_type::native_type SIMD_FLAGS(InOut, RegisterOnly)
simdlib_contract_value_import(value_type::native_type input) noexcept
{
    return value_type::from_native(input).to_native();
}
