#define SIMDLIB_HAS_SSE42 1
#define SIMDLIB_HAS_AVX2 1
#include <SimdLib/IRegister.h>
#include <SimdLib/PartialRegister.h>

#include <cstdint>

using register_type = SimdLib::PartialRegister<std::int32_t, 128, 1>;

static_assert(SimdLib::IRegister::WidenLow<register_type, std::int64_t, 256>, "SIMDLIB_PARTIAL_REGISTER_REJECTS_INACTIVE_UPPER_HALF_WIDEN_RESULT");
