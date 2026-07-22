#include <SimdLib/Register.h>

#if !SIMDLIB_REQUIRE_REGISTER_INTERFACE
#error "The Register target must publish its requirement signal to consumers"
#endif

#if defined(_MSC_VER) && !defined(__clang__)
static_assert(_MSVC_LANG > 202002L);
#else
static_assert(__cplusplus > 202002L);
#endif

/**
 * @brief Verifies that an external consumer receives the opt-in Register target requirements.
 * @return Zero when the compile-time contract was satisfied.
 */
int main()
{
	return 0;
}
