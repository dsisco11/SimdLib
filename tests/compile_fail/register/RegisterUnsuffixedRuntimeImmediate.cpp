#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/Register.h>

#include <cstdint>

using register_type = SimdLib::Register<std::uint64_t, 128>;

/** @brief Reports whether Register exposes an unsuffixed runtime complete-register byte shift. */
template <class value_t>
concept accepts_runtime_byte_shift = requires(value_t value, int count) {
	value.byte_shift_left(count);
	value.byte_shift_right(count);
};

/** @brief Reports whether Register exposes an unsuffixed runtime complete-register bit shift. */
template <class value_t>
concept accepts_runtime_bit_shift = requires(value_t value, int count) {
	value.bit_shift_left(count);
	value.bit_shift_right(count);
};

static_assert(accepts_runtime_byte_shift<register_type> || accepts_runtime_bit_shift<register_type>,
			  "SIMDLIB_REGISTER_REJECTS_UNSUFFIXED_RUNTIME_IMMEDIATE_CONTROLS");