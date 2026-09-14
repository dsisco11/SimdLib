#include <SimdLib/SimdLib.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

#ifndef SIMDLIB_INSTALLED_CONSUMER_BITS
#define SIMDLIB_INSTALLED_CONSUMER_BITS 128
#endif

/**
 * @brief Exercises PartialRegister exclusively through an installed SimdLib package.
 * @return Zero when logical transfers, operations, predicates, and inactive lanes satisfy their contracts.
 */
int main()
{
#if SIMDLIB_INSTALLED_CONSUMER_BITS == 256
	using Register = SimdLib::partial_uint32x8<5>;
#else
	using Register = SimdLib::partial_uint32x4<3>;
#endif
	static_assert(SimdLib::IRegister::CoreSurface<Register>);

	std::array<std::uint32_t, Register::lane_count> source{};
	for (std::size_t lane = 0; lane < source.size(); ++lane)
		source[lane] = static_cast<std::uint32_t>(lane + 1);
	const Register loaded = Register::load(std::span{source});
	const Register result = (loaded + Register::broadcast(2)).max(Register::broadcast(4));
	const auto selected = result.compare_greater(Register::broadcast(5)).select(result, Register::zero());

	std::array<std::uint32_t, Register::lane_count> stored{};
	selected.store(std::span{stored});
	for (std::size_t lane = 0; lane < stored.size(); ++lane)
	{
		const auto incremented = source[lane] + 2;
		const auto clamped = incremented < 4 ? 4 : incremented;
		if (stored[lane] != (clamped > 5 ? clamped : 0))
			return 1;
	}

	const auto native_lanes = Register::api_type::to_array(selected.to_native());
	for (std::size_t lane = Register::lane_count; lane < Register::native_lane_count; ++lane)
	{
		if (std::bit_cast<std::array<std::byte, sizeof(std::uint32_t)>>(native_lanes[lane]) != std::array<std::byte, sizeof(std::uint32_t)>{})
			return 2;
	}
	return 0;
}
