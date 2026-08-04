#define SIMDLIB_HAS_SSE42 1
#define SIMDLIB_HAS_AVX2 1
#include <SimdLib/Register.h>

#include <cstdint>

using register_type = SimdLib::Register<std::uint8_t, 256>;

/** @brief Instantiates invalid negative Register immediate byte counts in both directions. */
void invalid_negative_complete_byte_shifts(register_type value)
{
	(void)value.template shift_bytes_left<-1>();
	(void)value.template shift_bytes_right<-1>();
}