#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/Register.h>

#include <cstdint>

using byte_register = SimdLib::Register<std::uint8_t, 128>;

/** @brief Reports whether a byte shuffle accepts a selector outside the source register. */
template <class value_t>
concept accepts_out_of_range_byte_selector = requires(value_t value) { value.template shuffle_bytes<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16>(); };

static_assert(accepts_out_of_range_byte_selector<byte_register>, "SIMDLIB_REGISTER_REJECTS_INVALID_BYTE_SHUFFLE_SELECTOR");
