#define SIMDLIB_HAS_SSE42 1
#define SIMDLIB_HAS_AVX2 1
#include <SimdLib/Api.h>

#include <cstdint>

using api = SimdLib::Api<256, std::uint8_t>;

/** @brief Instantiates invalid negative immediate byte counts in both directions. */
void invalid_negative_complete_byte_shifts(api::int_vector_t value)
{
	(void)api::template shift_bytes_left<-1>(value);
	(void)api::template shift_bytes_right<-1>(value);
}