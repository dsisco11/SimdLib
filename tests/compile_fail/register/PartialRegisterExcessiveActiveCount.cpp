#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/PartialRegister.h>

#include <cstdint>

using invalid_partial_register = SimdLib::PartialRegister<std::int32_t, 128, 5>;

invalid_partial_register value;
