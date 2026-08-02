#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/PartialRegister.h>

#include <cstdint>
#include <type_traits>

using partial_register_type = SimdLib::PartialRegister<std::int32_t, 128, 3>;

static_assert(std::is_convertible_v<partial_register_type::native_type, partial_register_type>,
	"SIMDLIB_PARTIAL_REGISTER_REJECTS_IMPLICIT_NATIVE");
