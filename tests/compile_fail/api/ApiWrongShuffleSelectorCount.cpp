#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/Api.h>

#include <cstdint>

using byte_api = SimdLib::Api<128, std::uint8_t>;
using word_api = SimdLib::Api<128, std::int16_t>;

/** @brief Reports whether an Api accepts fewer logical selectors than output lanes. */
template <class api_t>
concept accepts_too_few_shuffle_selectors =
	requires(typename api_t::vector_t value) { api_t::template shuffle<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14>(value); };

/** @brief Reports whether an Api accepts more logical selectors than output lanes. */
template <class api_t>
concept accepts_too_many_shuffle_selectors = requires(typename api_t::vector_t value) { api_t::template shuffle<0, 1, 2, 3, 4, 5, 6, 7, 0>(value); };

static_assert(accepts_too_few_shuffle_selectors<byte_api> || accepts_too_many_shuffle_selectors<word_api>, "SIMDLIB_API_REJECTS_WRONG_SHUFFLE_SELECTOR_COUNT");
