#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <SimdLib/SimdApi.h>

#include <concepts>

static_assert(SimdLib::is_simd_api_available_v<128, int> == SimdLib::is_api_available_v<128, int>);
static_assert(SimdLib::SimdApiAvailable<128, int> == SimdLib::ApiAvailable<128, int>);
#if SIMDLIB_HAS_SSE42
static_assert(std::same_as<SimdLib::SimdApi<128, int>, SimdLib::Api<128, int>>);
#else
static_assert(!SimdLib::is_simd_api_available_v<128, int>);
#endif

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
