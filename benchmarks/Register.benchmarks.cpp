#include <SimdLib/PartialRegister.h>
#include <SimdLib/Register.h>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>

namespace
{

/** @brief Returns a process-local runtime seed that prevents compile-time operand folding. */
[[nodiscard]] std::uint64_t runtime_seed() noexcept
{
	return static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()) | std::uint64_t{1};
}

/**
 * @brief Generates runtime-derived floating operands for one complete register.
 * @tparam count Number of generated lanes.
 * @param state Mutable pseudo-random state.
 * @return Complete floating lane array whose values are finite and nonzero.
 */
template <std::size_t count> [[nodiscard]] std::array<float, count> make_float_lanes(std::uint64_t &state) noexcept
{
	std::array<float, count> result{};
	for (std::size_t lane = 0; lane < count; ++lane)
	{
		state ^= state << 13;
		state ^= state >> 7;
		state ^= state << 17;
		result[lane] = static_cast<float>((state & 0x3ffU) + 1U) / 37.0F;
	}
	return result;
}

/**
 * @brief Generates runtime-derived nonzero unsigned divisors for one complete register.
 * @tparam count Number of generated lanes.
 * @param state Mutable pseudo-random state.
 * @return Complete unsigned lane array containing values in the range one through 31.
 */
template <std::size_t count> [[nodiscard]] std::array<std::uint32_t, count> make_unsigned_lanes(std::uint64_t &state) noexcept
{
	std::array<std::uint32_t, count> result{};
	for (std::size_t lane = 0; lane < count; ++lane)
	{
		state ^= state << 13;
		state ^= state >> 7;
		state ^= state << 17;
		result[lane] = static_cast<std::uint32_t>((state & 0x1fU) + 1U);
	}
	return result;
}

} // namespace

TEST_CASE("PartialRegister invariant-maintenance benchmarks", "[simdlib][benchmark][partial-register]")
{
	using partial128 = SimdLib::PartialRegister<float, 128, 3>;
	using partial256 = SimdLib::PartialRegister<float, 256, 5>;
	using integer_partial256 = SimdLib::PartialRegister<std::uint32_t, 256, 5>;
	auto seed = runtime_seed();
	const auto lhs128_lanes = make_float_lanes<partial128::lane_count>(seed);
	const auto rhs128_lanes = make_float_lanes<partial128::lane_count>(seed);
	const auto lhs256_lanes = make_float_lanes<partial256::lane_count>(seed);
	const auto rhs256_lanes = make_float_lanes<partial256::lane_count>(seed);
	const auto integer_lhs_lanes = make_unsigned_lanes<integer_partial256::lane_count>(seed);
	const auto integer_rhs_lanes = make_unsigned_lanes<integer_partial256::lane_count>(seed);
	const auto lhs128 = partial128::load(std::span<const float, partial128::lane_count>{lhs128_lanes});
	const auto rhs128 = partial128::load(std::span<const float, partial128::lane_count>{rhs128_lanes});
	const auto lhs256 = partial256::load(std::span<const float, partial256::lane_count>{lhs256_lanes});
	const auto rhs256 = partial256::load(std::span<const float, partial256::lane_count>{rhs256_lanes});
	const auto integer_lhs = integer_partial256::load(std::span<const std::uint32_t, integer_partial256::lane_count>{integer_lhs_lanes});
	const auto integer_rhs = integer_partial256::load(std::span<const std::uint32_t, integer_partial256::lane_count>{integer_rhs_lanes});

	BENCHMARK("PartialRegister 128-bit three-lane add")
	{
		return (lhs128 + rhs128).native;
	};
	BENCHMARK("Raw Api 128-bit canonical three-lane add")
	{
		return partial128::api_type::add(lhs128.native, rhs128.native);
	};
	BENCHMARK("PartialRegister 256-bit five-lane add")
	{
		return (lhs256 + rhs256).native;
	};
	BENCHMARK("Raw Api 256-bit canonical five-lane add")
	{
		return partial256::api_type::add(lhs256.native, rhs256.native);
	};
	BENCHMARK("PartialRegister 256-bit five-lane division")
	{
		return (integer_lhs / integer_rhs).native;
	};
	BENCHMARK("PartialRegister 256-bit five-lane compare and select")
	{
		return lhs256.compare_greater(rhs256).select(lhs256, rhs256).native;
	};
	BENCHMARK("PartialRegister 256-bit five-lane shuffle")
	{
		return lhs256.template shuffle<4, 3, 2, 1, 0>().native;
	};
}

TEST_CASE("Register runtime-derived wrapper and raw benchmarks", "[simdlib][benchmark][register]")
{
	using register128 = SimdLib::Register<float, 128>;
	using register256 = SimdLib::Register<float, 256>;
	using api128 = typename register128::api_type;
	using api256 = typename register256::api_type;
	using integer_register128 = SimdLib::Register<std::uint32_t, 128>;
	using integer_register256 = SimdLib::Register<std::uint32_t, 256>;
	using integer_api128 = typename integer_register128::api_type;
	using integer_api256 = typename integer_register256::api_type;

	auto seed = runtime_seed();
	const auto lhs128_lanes = make_float_lanes<register128::lane_count>(seed);
	const auto rhs128_lanes = make_float_lanes<register128::lane_count>(seed);
	const auto lhs256_lanes = make_float_lanes<register256::lane_count>(seed);
	const auto rhs256_lanes = make_float_lanes<register256::lane_count>(seed);
	const auto integer_lhs128_lanes = make_unsigned_lanes<integer_register128::lane_count>(seed);
	const auto integer_rhs128_lanes = make_unsigned_lanes<integer_register128::lane_count>(seed);
	const auto integer_lhs256_lanes = make_unsigned_lanes<integer_register256::lane_count>(seed);
	const auto integer_rhs256_lanes = make_unsigned_lanes<integer_register256::lane_count>(seed);

	const auto lhs128 = register128::load(std::span<const float, register128::lane_count>{lhs128_lanes});
	const auto rhs128 = register128::load(std::span<const float, register128::lane_count>{rhs128_lanes});
	const auto lhs256 = register256::load(std::span<const float, register256::lane_count>{lhs256_lanes});
	const auto rhs256 = register256::load(std::span<const float, register256::lane_count>{rhs256_lanes});
	const auto integer_lhs128 = integer_register128::load(std::span<const std::uint32_t, integer_register128::lane_count>{integer_lhs128_lanes});
	const auto integer_rhs128 = integer_register128::load(std::span<const std::uint32_t, integer_register128::lane_count>{integer_rhs128_lanes});
	const auto integer_lhs256 = integer_register256::load(std::span<const std::uint32_t, integer_register256::lane_count>{integer_lhs256_lanes});
	const auto integer_rhs256 = integer_register256::load(std::span<const std::uint32_t, integer_register256::lane_count>{integer_rhs256_lanes});
	const auto mask128 = lhs128.compare_greater(rhs128);
	const auto mask256 = lhs256.compare_greater(rhs256);
	const auto raw_mask128 = api128::compare_greater(lhs128.native, rhs128.native);
	const auto raw_mask256 = api256::compare_greater(lhs256.native, rhs256.native);

	BENCHMARK("Register 128-bit float add")
	{
		return (lhs128 + rhs128).native;
	};
	BENCHMARK("Raw Api 128-bit float add")
	{
		return api128::add(lhs128.native, rhs128.native);
	};
	BENCHMARK("Register 256-bit float add")
	{
		return (lhs256 + rhs256).native;
	};
	BENCHMARK("Raw Api 256-bit float add")
	{
		return api256::add(lhs256.native, rhs256.native);
	};
	BENCHMARK("Register 128-bit mask select")
	{
		return mask128.select(lhs128, rhs128).native;
	};
	BENCHMARK("Raw Api 128-bit mask select")
	{
		return api128::select(raw_mask128, lhs128.native, rhs128.native);
	};
	BENCHMARK("Register 256-bit mask select")
	{
		return mask256.select(lhs256, rhs256).native;
	};
	BENCHMARK("Raw Api 256-bit mask select")
	{
		return api256::select(raw_mask256, lhs256.native, rhs256.native);
	};
	BENCHMARK("Register 128-bit unsigned division")
	{
		return (integer_lhs128 / integer_rhs128).native;
	};
	BENCHMARK("Raw Api 128-bit unsigned division")
	{
		return integer_api128::divide(integer_lhs128.native, integer_rhs128.native);
	};
	BENCHMARK("Register 256-bit unsigned division")
	{
		return (integer_lhs256 / integer_rhs256).native;
	};
	BENCHMARK("Raw Api 256-bit unsigned division")
	{
		return integer_api256::divide(integer_lhs256.native, integer_rhs256.native);
	};
}
