#include <SimdLib/Register.h>

#if SIMDLIB_COMPILER_MSVC
#define SIMDLIB_CODEGEN_NOINLINE __declspec(noinline)
#else
#define SIMDLIB_CODEGEN_NOINLINE __attribute__((noinline))
#endif

using api_type = SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, float>;
using register_type = SimdLib::Register<float, SIMDLIB_REGISTER_TEST_WIDTH>;

/** @brief Records wrapper behavior under the platform-default calling convention. */
SIMDLIB_CODEGEN_NOINLINE register_type simdlib_codegen_default(register_type lhs, register_type rhs) noexcept
{
	return register_type{api_type::add(lhs.native, rhs.native)};
}

#undef SIMDLIB_CODEGEN_NOINLINE
