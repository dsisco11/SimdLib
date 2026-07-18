#include <SimdLib/SimdAlgo.h>

#if SIMDLIB_HAS_SSE42
static_assert(SimdLib::SimdAlgo<8, 8>::read_width == 8);
#else
static_assert(!SimdLib::is_api_available_v<128, std::uint8_t>);
#endif
