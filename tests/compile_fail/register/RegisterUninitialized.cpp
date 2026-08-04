#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/Register.h>

#include <cstdint>
#include <type_traits>

/** @brief Marker used to probe for an uninitialized construction escape hatch. */
struct uninitialized_t final
{
};

using register_type = SimdLib::Register<std::int32_t, 128>;

static_assert(std::is_constructible_v<register_type, uninitialized_t>, "SIMDLIB_REGISTER_REJECTS_UNINITIALIZED_CONSTRUCTION");
