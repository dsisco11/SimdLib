#define SIMDLIB_HAS_SSE42 1
#define SIMDLIB_HAS_AVX2 1
#include <SimdLib/Register.h>

#include <cstdint>

using register_type = SimdLib::Register<std::int8_t, 256>;

/** @brief Reports whether widening accepts a 256-bit source that lacks a one-result backend mapping. */
template <class value_t>
concept accepts_unavailable_width_change = requires(value_t value) { value.template widen_low<std::int16_t, 256>(); };

static_assert(accepts_unavailable_width_change<register_type>, "SIMDLIB_REGISTER_REJECTS_UNAVAILABLE_WIDTH_CHANGE");
