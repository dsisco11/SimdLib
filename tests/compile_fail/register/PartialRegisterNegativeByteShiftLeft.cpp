#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/PartialRegister.h>

#include <cstdint>

using register_type = SimdLib::PartialRegister<std::uint8_t, 128, 13>;

/** @brief Instantiates an invalid negative PartialRegister immediate byte-left count. */
void invalid_negative_partial_byte_left_shift(register_type value)
{
	(void)value.template shift_bytes_left<-1>();
}
