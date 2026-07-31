#include <SimdLib/Config.h>

#ifndef SIMDLIB_EXPECT_DEFAULT_CHECKS
#error "SIMDLIB_EXPECT_DEFAULT_CHECKS must be defined by the owning configuration profile"
#endif

#if SIMDLIB_EXPECT_DEFAULT_CHECKS && defined(NDEBUG)
#error "The checks-enabled Debug configuration unexpectedly defines NDEBUG"
#endif

static_assert(SIMDLIB_ENABLE_CHECKS == SIMDLIB_EXPECT_DEFAULT_CHECKS, "The default checks state does not match the owning configuration profile");
