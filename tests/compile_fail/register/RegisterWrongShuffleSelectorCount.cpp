#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/Register.h>

#include <cstdint>

using register_type = SimdLib::Register<std::uint8_t, 128>;

/** @brief Reports whether a logical shuffle accepts fewer selectors than result lanes. */
template <class value_t>
concept accepts_wrong_shuffle_selector_count = requires(value_t value) { value.template shuffle<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14>(); };

static_assert(accepts_wrong_shuffle_selector_count<register_type>, "SIMDLIB_REGISTER_REJECTS_WRONG_SHUFFLE_SELECTOR_COUNT");
