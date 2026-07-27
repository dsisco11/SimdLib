#define SIMDLIB_HAS_SSE42 1
#define SIMDLIB_HAS_AVX2 1
#include <SimdLib/Register.h>

#include <cstdint>

using dword_register = SimdLib::Register<std::uint32_t, 128>;
using qword_register = SimdLib::Register<std::int64_t, 128>;
using wide_float_register = SimdLib::Register<float, 256>;

/** @brief Reports whether a 32-bit Register accepts a selector outside the source register. */
template <class value_t>
concept accepts_out_of_range_dword_selector = requires(value_t value) { value.template shuffle<0, 1, 2, 4>(); };

/** @brief Reports whether a 64-bit Register accepts a selector outside the source register. */
template <class value_t>
concept accepts_out_of_range_qword_selector = requires(value_t value) { value.template shuffle<0, 2>(); };

/** @brief Reports whether a floating Register accepts a selector from another 128-bit source group. */
template <class value_t>
concept accepts_cross_group_float_selector = requires(value_t value) { value.template shuffle<4, 1, 2, 3, 4, 5, 6, 7>(); };

static_assert(accepts_out_of_range_dword_selector<dword_register> || accepts_out_of_range_qword_selector<qword_register> ||
				  accepts_cross_group_float_selector<wide_float_register>,
			  "SIMDLIB_REGISTER_REJECTS_INVALID_SHUFFLE_SELECTOR");