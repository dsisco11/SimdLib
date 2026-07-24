#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/Register.h>

#include <cstdint>

using register_type = SimdLib::Register<std::int32_t, 128>;

/** @brief Reports whether numeric conversion accepts a target outside the complete-register backend contract. */
template <class value_t>
concept accepts_unsupported_conversion_target = requires(value_t value) { value.template convert<double>(); };

static_assert(accepts_unsupported_conversion_target<register_type>, "SIMDLIB_REGISTER_REJECTS_UNSUPPORTED_CONVERSION_TARGET");
