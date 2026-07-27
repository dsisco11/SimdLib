#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/Register.h>

#include <cstdint>

using byte_register = SimdLib::Register<std::uint8_t, 128>;

/** @brief Reports whether a byte shuffle accepts fewer selectors than register bytes. */
template <class value_t>
concept accepts_too_few_byte_shuffle_selectors = requires(value_t value) { value.template shuffle_bytes<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14>(); };

/** @brief Reports whether a byte shuffle accepts more selectors than register bytes. */
template <class value_t>
concept accepts_too_many_byte_shuffle_selectors =
	requires(value_t value) { value.template shuffle_bytes<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0>(); };

static_assert(accepts_too_few_byte_shuffle_selectors<byte_register> || accepts_too_many_byte_shuffle_selectors<byte_register>,
			  "SIMDLIB_REGISTER_REJECTS_WRONG_BYTE_SHUFFLE_SELECTOR_COUNT");
