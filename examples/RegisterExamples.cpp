#include <SimdLib/SimdLib.h>

#include <cmath>
#include <cstdint>
#include <limits>

namespace
{
using StableRegister = SimdLib::Register<float, 128>;

/**
 * @brief Demonstrates a stable-width non-inline consumer boundary.
 * @param value Input register.
 * @return Input lanes increased by one.
 */
StableRegister SIMD_FLAGS(InOut) add_one(StableRegister value) noexcept
{
	return value + StableRegister::broadcast(1.0F);
}
} // namespace

/**
 * @brief Exercises complete-register and RegisterMask workflows from the umbrella header.
 * @return Zero when every example result satisfies its contract.
 */
int main()
{
	using Register = SimdLib::NativeRegister<float>;
	using Mask = Register::mask_type;

	const Register values = Register::broadcast(2.0F);
	const Register threshold = Register::broadcast(1.0F);
	const Mask greater = values.compare_greater(threshold);
	const Mask equal = values.compare_equal(values);
	const Mask selected_lanes = greater & equal;
	const typename Mask::native_type observed_predicate = selected_lanes.native;
	const Mask restored_predicate{observed_predicate};
	const Register selected = restored_predicate.select(values, Register::zero());
	if (!restored_predicate.any() || !restored_predicate.all() || restored_predicate.none() || restored_predicate.bits() == 0 || selected != values)
		return 1;

	const Register nan = Register::broadcast(std::numeric_limits<float>::quiet_NaN());
	if (nan.compare_equal(nan).any())
		return 2;

	const Register positive_zero = Register::broadcast(0.0F);
	const Register negative_zero = Register::broadcast(-0.0F);
	if (!positive_zero.compare_equal(negative_zero).all())
		return 3;

	return add_one(StableRegister::broadcast(4.0F)) == StableRegister::broadcast(5.0F) ? 0 : 4;
}
