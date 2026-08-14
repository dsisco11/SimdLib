#include "register_api.h"

#include <array>
#include <bit>
#include <cstddef>
#include <span>

#if !SIMDLIB_REQUIRE_REGISTER_INTERFACE
#error "The Register target must publish its requirement signal to consumers"
#endif

#if defined(_MSC_VER) && !defined(__clang__)
static_assert(_MSVC_LANG > 202002L);
#else
static_assert(__cplusplus > 202002L);
#endif

/**
 * @brief Verifies cross-translation-unit use of flagged Register and native SIMD boundaries.
 * @return Zero when both downstream declaration contracts are satisfied.
 */
int main()
{
	using namespace SimdLibConsumer;
	const Register expected = Register::broadcast(4);
	const Register actual = increment(Register::broadcast(3));
	const RegisterMask equal = actual.compare_equal(expected);
	if (!equal.all() || equal.select(actual, Register::zero()) != expected)
	{
		return 1;
	}

	const auto native = increment_native(_mm_set1_epi32(3));
	if (_mm_cvtsi128_si32(native) != 4)
		return 2;

	const std::array<std::uint32_t, PartialRegister::lane_count> source{2, 4, 8};
	const PartialRegister partial = increment_partial(PartialRegister::load(std::span{source}));
	std::array<std::uint32_t, PartialRegister::lane_count> stored{};
	partial.store(std::span{stored});
	if (stored != std::array<std::uint32_t, PartialRegister::lane_count>{3, 5, 9})
		return 3;

	const auto native_lanes = PartialRegister::api_type::to_array(partial.to_native());
	if (std::bit_cast<std::array<std::byte, sizeof(std::uint32_t)>>(native_lanes[3]) != std::array<std::byte, sizeof(std::uint32_t)>{})
		return 4;
	return 0;
}
