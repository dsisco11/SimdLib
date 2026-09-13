#include <SimdLib/PartialRegister.h>

#include <cstdint>

#ifndef SIMDLIB_PARTIAL_REGISTER_REJECTS_INACTIVE_UPPER_HALF
#error "SIMDLIB_PARTIAL_REGISTER_REJECTS_INACTIVE_UPPER_HALF"
#endif

using invalid_partial_register = SimdLib::PartialRegister<std::uint32_t, 256, 4>;

int main()
{
	return static_cast<int>(sizeof(invalid_partial_register));
}
