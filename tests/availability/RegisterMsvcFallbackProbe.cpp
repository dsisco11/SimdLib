#if !defined(_MSC_VER) || defined(__clang__)
#error "The Microsoft fallback probe requires Microsoft C++"
#endif

#ifdef __cpp_explicit_this_parameter
#undef __cpp_explicit_this_parameter
#endif

#include <SimdLib/Register.h>

static_assert(_MSC_VER >= 1944);
static_assert(_MSVC_LANG > 202002L);
static_assert(SIMDLIB_REGISTER_INTERFACE_AVAILABLE == 1);
static_assert(SIMDLIB_REQUIRE_REGISTER_INTERFACE == 1);
