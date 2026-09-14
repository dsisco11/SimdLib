#define SIMDLIB_HAS_SSE42 1
#define SIMDLIB_HAS_AVX2 0
#include <SimdLib/PartialRegister.h>

#include <cstdint>

using unavailable_partial_register = SimdLib::PartialRegister<std::int32_t, 256, 3>;

unavailable_partial_register value;
