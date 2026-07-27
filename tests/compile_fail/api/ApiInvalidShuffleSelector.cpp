#define SIMDLIB_HAS_SSE42 1
#define SIMDLIB_HAS_AVX2 1
#include <SimdLib/Api.h>

#include <cstdint>

using dword_api = SimdLib::Api<128, std::uint32_t>;
using qword_api = SimdLib::Api<128, std::int64_t>;
using wide_float_api = SimdLib::Api<256, float>;

/** @brief Reports whether a 32-bit Api accepts a selector outside the source register. */
template <class api_t>
concept accepts_out_of_range_dword_selector = requires(typename api_t::vector_t value) { api_t::template shuffle<0, 1, 2, 4>(value); };

/** @brief Reports whether a 64-bit Api accepts a selector outside the source register. */
template <class api_t>
concept accepts_out_of_range_qword_selector = requires(typename api_t::vector_t value) { api_t::template shuffle<0, 2>(value); };

/** @brief Reports whether a floating Api accepts a selector from another 128-bit source group. */
template <class api_t>
concept accepts_cross_group_float_selector = requires(typename api_t::vector_t value) { api_t::template shuffle<4, 1, 2, 3, 4, 5, 6, 7>(value); };

static_assert(accepts_out_of_range_dword_selector<dword_api> || accepts_out_of_range_qword_selector<qword_api> ||
				  accepts_cross_group_float_selector<wide_float_api>,
			  "SIMDLIB_API_REJECTS_INVALID_SHUFFLE_SELECTOR");
