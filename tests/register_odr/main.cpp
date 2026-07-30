#include <SimdLib/SimdLib.h>

#include <cstdint>

namespace
{
using Register = SimdLib::Register<std::uint32_t, 128>;
using RegisterMask = Register::mask_type;
} // namespace

/**
 * @brief Adds two complete registers in a second translation unit.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return Lane-wise sum.
 */
Register SIMD_FLAGS(InOut) second_translation_unit_add(Register lhs, Register rhs) noexcept;

/**
 * @brief Compares two complete registers in a second translation unit.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return Per-lane equality predicate.
 */
RegisterMask SIMD_FLAGS(InOut) second_translation_unit_equal(Register lhs, Register rhs) noexcept;

/**
 * @brief Verifies umbrella exposure and inline Register definitions across translation units.
 * @return Zero when the cross-translation-unit results are correct.
 */
int main()
{
	const Register expected = Register::broadcast(5);
	const Register actual = second_translation_unit_add(Register::broadcast(2), Register::broadcast(3));
	const RegisterMask equal = second_translation_unit_equal(actual, expected);
	return equal.all() && equal.select(actual, Register::zero()) == expected ? 0 : 1;
}
