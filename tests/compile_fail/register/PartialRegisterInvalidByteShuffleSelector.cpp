#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/IRegister.h>
#include <SimdLib/PartialRegister.h>

#include <cstdint>

using register_type = SimdLib::PartialRegister<std::uint8_t, 128, 3>;

static_assert(SimdLib::IRegister::ShuffleBytes<register_type, 0, 1, 3>, "SIMDLIB_PARTIAL_REGISTER_REJECTS_INVALID_BYTE_SHUFFLE_SELECTOR");
