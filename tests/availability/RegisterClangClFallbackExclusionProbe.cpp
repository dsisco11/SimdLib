#if !defined(__clang__) || !defined(_MSC_VER)
#error "The clang-cl fallback-exclusion probe requires clang-cl"
#endif

#ifdef __cpp_explicit_this_parameter
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wbuiltin-macro-redefined"
#undef __cpp_explicit_this_parameter
#pragma clang diagnostic pop
#endif

#include <SimdLib/Config.h>

static_assert(_MSC_VER >= 1944);
static_assert(_MSVC_LANG > 202002L);
static_assert(SIMDLIB_COMPILER_CLANG == 1);
static_assert(SIMDLIB_COMPILER_MSVC == 0);
static_assert(SIMDLIB_REGISTER_INTERFACE_AVAILABLE == 0);
