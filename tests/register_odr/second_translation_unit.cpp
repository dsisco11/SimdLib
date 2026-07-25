#include <SimdLib/SimdLib.h>

#include <cstdint>

namespace
{
using Register = SimdLib::Register<std::uint32_t, 128>;
using RegisterMask = Register::mask_type;
} // namespace

/**
 * @brief Adds two complete registers through the public umbrella header.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return Lane-wise sum.
 */
Register VECTORCALL second_translation_unit_add(Register lhs, Register rhs) noexcept
{
	return lhs + rhs;
}

/**
 * @brief Compares two complete registers through the public umbrella header.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return Per-lane equality predicate.
 */
RegisterMask VECTORCALL second_translation_unit_equal(Register lhs, Register rhs) noexcept
{
	return lhs.compare_equal(rhs);
}
