#define SIMDLIB_HAS_SSE42 1
#define SIMDLIB_HAS_AVX2 1
#include <SimdLib/Register.h>

#include <cstdint>

using register_type = SimdLib::Register<std::uint8_t, 128>;
using wide_register_type = SimdLib::Register<std::uint8_t, 256>;

/** @brief Reports whether a logical shuffle accepts a selector outside the source register. */
template <class value_t>
concept accepts_invalid_shuffle_selector = requires(value_t value) { value.template shuffle<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16>(); };

/** @brief Reports whether a logical shuffle accepts a selector from another 128-bit source group. */
template <class value_t>
concept accepts_cross_group_shuffle_selector = requires(value_t value) {
	value.template shuffle<16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31>();
};

static_assert(accepts_invalid_shuffle_selector<register_type> || accepts_cross_group_shuffle_selector<wide_register_type>,
			  "SIMDLIB_REGISTER_REJECTS_INVALID_SHUFFLE_SELECTOR");
