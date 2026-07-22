#include <SimdLib/Register.h>

#if SIMDLIB_COMPILER_MSVC
#define SIMDLIB_CODEGEN_NOINLINE __declspec(noinline)
#else
#define SIMDLIB_CODEGEN_NOINLINE __attribute__((noinline))
#endif

using api_type = SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, float>;
using native_type = typename api_type::vector_t;

/** @brief Records raw-vector behavior under the platform-default calling convention. */
SIMDLIB_CODEGEN_NOINLINE native_type simdlib_codegen_default(native_type lhs, native_type rhs) noexcept
{
	return api_type::add(lhs, rhs);
}

#undef SIMDLIB_CODEGEN_NOINLINE
