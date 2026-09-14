#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/PartialRegister.h>

#include <cstdint>

using register_type = SimdLib::PartialRegister<std::uint8_t, 128, 13>;

/** @brief Instantiates an invalid negative PartialRegister immediate bit-right count. */
void invalid_negative_partial_bit_right_shift(register_type value)
{
	(void)value.template shift_bits_right<-1>();
}
