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
#include <SimdLib/Config.h>

static_assert(!SimdLib::Config::has_sse);
static_assert(!SimdLib::Config::has_sse2);
static_assert(!SimdLib::Config::has_sse3);
static_assert(!SimdLib::Config::has_ssse3);
static_assert(!SimdLib::Config::has_sse41);
static_assert(!SimdLib::Config::has_sse42);
static_assert(!SimdLib::Config::has_avx);
static_assert(!SimdLib::Config::has_avx2);
static_assert(!SimdLib::Config::has_fma);
static_assert(!SimdLib::Config::has_bmi1);
static_assert(!SimdLib::Config::has_bmi2);

int ConfigDisabledInstructionsProbe() noexcept
{
	return 0;
}
