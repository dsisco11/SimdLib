#include <SimdLib/Api.h>

static_assert(!SimdLib::is_api_available_v<64, int>);
#if SIMDLIB_HAS_SSE42
static_assert(SimdLib::Api<128, int>::element_count == 4);
static_assert(SimdLib::NativeApi<int>::register_width == (SIMDLIB_HAS_AVX2 ? 256 : 128));
#else
static_assert(!SimdLib::is_api_available_v<128, int>);
#endif
