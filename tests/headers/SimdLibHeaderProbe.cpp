#include <SimdLib/SimdLib.h>

#if defined(_FORMAT_) || defined(_LIBCPP_FORMAT) || defined(_GLIBCXX_FORMAT)
#error "SimdLib/SimdLib.h must not include <format>; include SimdLib/Format.h explicitly"
#endif

#if SIMDLIB_HAS_SSE42
static_assert(SimdLib::Api<128, unsigned>::byte_count == 16);
#else
static_assert(!SimdLib::is_api_available_v<128, unsigned>);
#endif
