#include "fixture.h"

#include <array>
#include <bit>

/**
 * @brief Verifies PartialRegister and its predicate across translation-unit boundaries.
 * @return Zero when active results match and every inactive lane remains bitwise zero.
 */
int main()
{
	using namespace SimdLibPartialRegisterOdr;
	const Register actual = add(Register::broadcast(2), Register::broadcast(3));
	const Register expected = Register::broadcast(5);
	if (!equal(actual, expected).all() || actual.to_array() != expected.to_array())
		return 1;

	const auto native_lanes = Register::api_type::to_array(actual.to_native());
	for (std::size_t lane = Register::lane_count; lane < Register::native_lane_count; ++lane)
	{
		if (std::bit_cast<std::array<std::byte, sizeof(std::uint32_t)>>(native_lanes[lane]) != std::array<std::byte, sizeof(std::uint32_t)>{})
			return 2;
	}
	return 0;
}
