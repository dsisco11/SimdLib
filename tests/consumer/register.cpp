#include <SimdLib/SimdLib.h>

#include <cstdint>

#if !SIMDLIB_REQUIRE_REGISTER_INTERFACE
#error "The Register target must publish its requirement signal to consumers"
#endif

#if defined(_MSC_VER) && !defined(__clang__)
static_assert(_MSVC_LANG > 202002L);
#else
static_assert(__cplusplus > 202002L);
#endif

namespace
{
using Register = SimdLib::Register<std::uint32_t, 128>;
using RegisterMask = Register::mask_type;

/**
 * @brief Exercises a downstream non-inline Register boundary with the supported convention.
 * @param value Input register.
 * @return Input register increased by one in every lane.
 */
Register VECTORCALL increment(Register value) noexcept
{
	return value + Register::broadcast(1);
}
} // namespace

/**
 * @brief Verifies umbrella exposure and complete-register behavior for an external consumer.
 * @return Zero when the consumer contract is satisfied.
 */
int main()
{
	const Register expected = Register::broadcast(4);
	const Register actual = increment(Register::broadcast(3));
	const RegisterMask equal = actual.compare_equal(expected);
	return equal.all() && equal.select(actual, Register::zero()) == expected ? 0 : 1;
}
