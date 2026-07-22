#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/Register.h>

#include <cstdint>
#include <type_traits>

using register_type = SimdLib::Register<std::int32_t, 128>;

static_assert(std::is_convertible_v<std::int32_t, register_type>, "SIMDLIB_REGISTER_REJECTS_IMPLICIT_SCALAR");
