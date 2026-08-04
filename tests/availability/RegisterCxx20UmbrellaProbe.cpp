#include <SimdLib/SimdLib.h>

static_assert(SIMDLIB_REGISTER_INTERFACE_AVAILABLE == 0);
static_assert(SIMDLIB_REQUIRE_REGISTER_INTERFACE == 0);
static_assert(SimdLib::version_major == SimdLib::Config::version_major);
