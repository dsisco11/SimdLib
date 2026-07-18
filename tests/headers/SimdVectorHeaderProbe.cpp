#include <SimdLib/SimdVector.h>

#if SIMDLIB_HAS_SSE42
static_assert(SimdLib::SimdVector<int, 3>::simd_width == 128);
#else
static_assert(!SimdLib::is_api_available_v<128, int>);
#endif
