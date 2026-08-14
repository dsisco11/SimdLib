#include <SimdLib/PartialRegister.h>

#include <cstdint>

/**
 * @brief Exercises a PartialRegister boundary using only the isolated public headers.
 * @param value PartialRegister value returned unchanged.
 * @return The supplied partial value.
 */
SimdLib::PartialRegister<unsigned, 128, 3> SIMD_FLAGS(InOut, RegisterOnly, ForceInline)
	installed_partial_register_identity(const SimdLib::PartialRegister<unsigned, 128, 3> value) noexcept
{
	return value;
}

static_assert(sizeof(SimdLib::PartialRegister<unsigned, 128, 3>) == 16);
static_assert(SimdLib::IRegister::CoreSurface<SimdLib::PartialRegister<unsigned, 128, 3>>);
