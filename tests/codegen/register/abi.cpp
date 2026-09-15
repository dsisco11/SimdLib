#include <SimdLib/Register.h>

using value_type = SimdLib::Register<float, SIMDLIB_CONTRACT_WIDTH>;
#if SIMDLIB_COMPILER_MSVC
#define CONTRACT_NOINLINE __declspec(noinline)
#else
#define CONTRACT_NOINLINE __attribute__((noinline))
#endif

#pragma region Aggregate ABI group
/** @brief Returns the production addition through the actual aggregate boundary. */
extern "C" CONTRACT_NOINLINE value_type SIMD_FLAGS(InOut, RegisterOnly)
simdlib_contract_abi_binary_callee(value_type lhs, value_type rhs) noexcept
{
    return lhs + rhs;
}

/** @brief Preserves aggregate parameters and the observable out-of-line target. */
extern "C" CONTRACT_NOINLINE value_type SIMD_FLAGS(InOut, RegisterOnly)
simdlib_contract_abi_binary_caller(value_type lhs, value_type rhs) noexcept
{
    return simdlib_contract_abi_binary_callee(lhs, rhs);
}
#pragma endregion
