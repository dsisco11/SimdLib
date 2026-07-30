#include <SimdLib/Register.h>

/**
 * @brief Exercises a Register boundary using only the isolated public headers.
 * @param value Register value returned unchanged.
 * @return The supplied register value.
 */
SimdLib::Register<unsigned, 128> SIMD_FLAGS(InOut, RegisterOnly, ForceInline) installed_register_identity(const SimdLib::Register<unsigned, 128> value) noexcept
{
	return value;
}

static_assert(sizeof(SimdLib::Register<unsigned, 128>) == 16);
