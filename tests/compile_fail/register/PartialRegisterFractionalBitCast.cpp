#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/IRegister.h>
#include <SimdLib/PartialRegister.h>

#include <cstdint>

using register_type = SimdLib::PartialRegister<std::uint8_t, 128, 13>;

static_assert(SimdLib::IRegister::BitCast<register_type, std::uint32_t>, "SIMDLIB_PARTIAL_REGISTER_REJECTS_FRACTIONAL_BIT_CAST_RESULT");
