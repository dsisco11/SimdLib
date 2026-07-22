#pragma once

#include <SimdLib/Config.h>

#if !SIMDLIB_REGISTER_INTERFACE_AVAILABLE && !SIMDLIB_REQUIRE_REGISTER_INTERFACE
#error "SIMDLIB_REGISTER_HEADER_REQUIRES_CXX23: <SimdLib/Register.h> requires C++23 explicit object parameter support"
#endif
