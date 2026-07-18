#define SIMDLIB_HAS_SSE 0
#define SIMDLIB_HAS_SSE2 0
#define SIMDLIB_HAS_SSE3 0
#define SIMDLIB_HAS_SSSE3 0
#define SIMDLIB_HAS_SSE41 0
#define SIMDLIB_HAS_SSE42 0
#define SIMDLIB_HAS_AVX 0
#define SIMDLIB_HAS_AVX2 0
#define SIMDLIB_HAS_FMA 0
#define SIMDLIB_HAS_BMI1 0
#define SIMDLIB_HAS_BMI2 0

#include <SimdLib/SimdLib.h>

#include <cstdint>

static_assert(!SimdLib::is_api_available_v<128, std::int32_t>);
static_assert(!SimdLib::is_api_available_v<256, float>);
static_assert(SimdLib::Bmi::bzhi(std::uint32_t{0xFFFF}, 8) == 0xFF);
static_assert((SimdLib::uint128_t{1} << 64).high() == 1);

int ConfigDisabledPublicHeadersProbe() noexcept
{
	return 0;
}
