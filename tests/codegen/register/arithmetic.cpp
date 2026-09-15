#include <SimdLib/Register.h>

using value_type = SimdLib::Register<float, SIMDLIB_CONTRACT_WIDTH>;
using native_type = value_type::native_type;
#if SIMDLIB_COMPILER_MSVC
#define CONTRACT_NOINLINE __declspec(noinline)
#else
#define CONTRACT_NOINLINE __attribute__((noinline))
#endif

#pragma region Arithmetic observations
/** @brief Exposes one production packed addition with runtime vector inputs. */
extern "C" CONTRACT_NOINLINE native_type SIMD_FLAGS(InOut, RegisterOnly)
simdlib_contract_operation_add_f32(native_type lhs, native_type rhs) noexcept
{
    return (value_type{lhs} + value_type{rhs}).native;
}

/** @brief Exposes intentional constant lowering without artificial dependencies. */
extern "C" CONTRACT_NOINLINE native_type SIMD_FLAGS(InOut, RegisterOnly)
simdlib_contract_operation_zero_f32() noexcept
{
    return value_type::zero().native;
}

/** @brief Exposes a valid identity method whose complete body may only return. */
extern "C" CONTRACT_NOINLINE native_type SIMD_FLAGS(InOut, RegisterOnly)
simdlib_contract_expression_native(native_type value) noexcept
{
    return value_type{value}.native;
}
#pragma endregion
