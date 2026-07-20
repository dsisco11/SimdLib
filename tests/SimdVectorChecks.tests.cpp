#include <cstddef>

namespace SimdVectorCheckProbe
{
inline std::size_t invocation_count = 0;
inline bool every_condition_passed = true;

/** @brief Records a checks-enabled SimdVector precondition evaluation.
 *  @param condition Condition evaluated by the public operation.
 *  @param message Operation description supplied to the precondition hook.
 */
inline void RecordPrecondition(const bool condition, const char* message) noexcept
{
	++invocation_count;
	every_condition_passed = every_condition_passed && condition;
	(void)message;
}

/** @brief Resets the checks-enabled precondition observations. */
inline void Reset() noexcept
{
	invocation_count = 0;
	every_condition_passed = true;
}
}

#define SIMDLIB_PRECONDITION(condition, message) ::SimdVectorCheckProbe::RecordPrecondition((condition), (message))

#include <SimdLib/SimdVector.h>

#undef SIMDLIB_PRECONDITION

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>

TEST_CASE("SimdVector checks validate partial results and bypass full vectors", "[simdlib][vector][checks]")
{
	using Partial = SimdLib::SimdVector<std::int32_t, 3>;
	using Full = SimdLib::SimdVector<std::int32_t, 4>;

	const Partial partial_lhs(20, -21, 22);
	const auto partial_rhs = Partial::simd::setr(4, 5, 6, 0);
	const auto partial_lower = Partial::simd::setr(-5, -5, -5, 100);
	const auto partial_upper = Partial::simd::setr(5, 5, 5, -100);
	SimdVectorCheckProbe::Reset();
	const Partial partial_quotient(partial_lhs / partial_rhs);
	const Partial partial_remainder(partial_lhs % partial_rhs);
	const Partial partial_clamped(partial_lhs.clamp(partial_lower, partial_upper));
	REQUIRE(partial_quotient.toArray() == std::array<std::int32_t, 4>{5, -4, 3, 0});
	REQUIRE(partial_remainder.toArray() == std::array<std::int32_t, 4>{0, -1, 4, 0});
	REQUIRE(partial_clamped.toArray() == std::array<std::int32_t, 4>{5, -5, 5, 0});
	REQUIRE(SimdVectorCheckProbe::invocation_count == 3);
	REQUIRE(SimdVectorCheckProbe::every_condition_passed);

	const Full full_lhs(20, -21, 22, 24);
	const Full full_rhs(4, 5, 6, 8);
	SimdVectorCheckProbe::Reset();
	const Full full_quotient(full_lhs / full_rhs.getRegister());
	const Full full_remainder(full_lhs % full_rhs.getRegister());
	const Full full_clamped(full_lhs.clamp(-5, 5));
	REQUIRE(full_quotient.toArray() == std::array<std::int32_t, 4>{5, -4, 3, 3});
	REQUIRE(full_remainder.toArray() == std::array<std::int32_t, 4>{0, -1, 4, 0});
	REQUIRE(full_clamped.toArray() == std::array<std::int32_t, 4>{5, -5, 5, 5});
	REQUIRE(SimdVectorCheckProbe::invocation_count == 0);
	REQUIRE(SimdVectorCheckProbe::every_condition_passed);
}
