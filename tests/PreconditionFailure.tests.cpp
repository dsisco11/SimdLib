#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdio>
#include <cstdlib>

namespace
{
/** @brief Unique marker emitted only by the dedicated precondition-failure harness. */
inline constexpr char expected_precondition_failure_marker[] = "SIMDLIB_PRECONDITION_FAILURE_EXPECTED_18A7E3";

inline constexpr int precondition_failure_diagnostic_exit_code = 73;

/** @brief Terminates the isolated test process after proving a precondition fired.
 *  @param message Diagnostic supplied by the failed public precondition.
 *
 *  The marker is written and flushed before termination so CTest can distinguish
 *  the expected contract failure from an unrelated process failure.
 */
[[noreturn]] void FailPrecondition(const char *message) noexcept
{
	(void)message;
	std::fputs(expected_precondition_failure_marker, stderr);
	std::fputc('\n', stderr);
	std::fflush(stderr);
	std::exit(precondition_failure_diagnostic_exit_code);
}
} // namespace

#define SIMDLIB_PRECONDITION(condition, message)                                                                                                               \
	do                                                                                                                                                         \
	{                                                                                                                                                          \
		if (!(condition))                                                                                                                                      \
			FailPrecondition(message);                                                                                                                         \
	} while (false)

#include <SimdLib/Api.h>
#include <SimdLib/SimdAlgo.h>
#include <SimdLib/SimdResample.h>

#undef SIMDLIB_PRECONDITION

#include <array>
#include <cstdint>
#include <span>

namespace
{
using Api128 = SimdLib::Api<128, std::uint32_t>;
using Algo = SimdLib::SimdAlgo<8, 1>;
} // namespace

TEST_CASE("Api load_aligned terminates for a misaligned source", "[simdlib][preconditions][checks]")
{
	alignas(Api128::byte_count) std::array<std::uint32_t, Api128::element_count + 1> data{};
	(void)Api128::load_aligned(std::span<const std::uint32_t, Api128::element_count>(data.data() + 1, Api128::element_count));
	FAIL("Api::load_aligned accepted a misaligned source");
}
TEST_CASE("Api load_partial terminates for an undersized source", "[simdlib][preconditions][checks]")
{
	const std::array<std::uint32_t, 1> data{};
	(void)Api128::load_partial<2>(std::span<const std::uint32_t>(data));
	FAIL("Api::load_partial accepted an undersized source");
}

TEST_CASE("Api store_aligned terminates for a misaligned destination", "[simdlib][preconditions][checks]")
{
	alignas(Api128::byte_count) std::array<std::uint32_t, Api128::element_count + 1> data{};
	Api128::store_aligned(Api128::setzero(), std::span<std::uint32_t, Api128::element_count>(data.data() + 1, Api128::element_count));
	FAIL("Api::store_aligned accepted a misaligned destination");
}

TEST_CASE("Api byte store terminates for an undersized destination", "[simdlib][preconditions][checks]")
{
	std::array<std::byte, Api128::byte_count - 1> data{};
	Api128::store(Api128::setzero(), std::span<std::byte>(data));
	FAIL("Api::store accepted an undersized byte destination");
}

TEST_CASE("SimdAlgo BitwiseAnd terminates for mismatched extents", "[simdlib][preconditions][checks]")
{
	const std::array<std::uint8_t, 2> lhs{};
	const std::array<std::uint8_t, 1> rhs{};
	std::array<std::uint8_t, 2> write{};
	Algo::BitwiseAnd(lhs, rhs, write);
	FAIL("SimdAlgo::BitwiseAnd accepted mismatched extents");
}

TEST_CASE("SimdAlgo BitwiseOr terminates for mismatched extents", "[simdlib][preconditions][checks]")
{
	const std::array<std::uint8_t, 2> lhs{};
	const std::array<std::uint8_t, 1> rhs{};
	std::array<std::uint8_t, 2> write{};
	Algo::BitwiseOr(lhs, rhs, write);
	FAIL("SimdAlgo::BitwiseOr accepted mismatched extents");
}

TEST_CASE("SimdAlgo BitwiseXor terminates for mismatched extents", "[simdlib][preconditions][checks]")
{
	const std::array<std::uint8_t, 2> lhs{};
	const std::array<std::uint8_t, 1> rhs{};
	std::array<std::uint8_t, 2> write{};
	Algo::BitwiseXor(lhs, rhs, write);
	FAIL("SimdAlgo::BitwiseXor accepted mismatched extents");
}

TEST_CASE("SimdAlgo BitwiseNot terminates for mismatched extents", "[simdlib][preconditions][checks]")
{
	const std::array<std::uint8_t, 2> lhs{};
	std::array<std::uint8_t, 1> write{};
	Algo::BitwiseNot(lhs, write);
	FAIL("SimdAlgo::BitwiseNot accepted mismatched extents");
}

TEST_CASE("SimdAlgo BitwiseAndNot terminates for mismatched extents", "[simdlib][preconditions][checks]")
{
	const std::array<std::uint8_t, 2> lhs{};
	const std::array<std::uint8_t, 1> rhs{};
	std::array<std::uint8_t, 2> write{};
	Algo::BitwiseAndNot(lhs, rhs, write);
	FAIL("SimdAlgo::BitwiseAndNot accepted mismatched extents");
}

TEST_CASE("SimdResample reduce any terminates for an invalid shape", "[simdlib][preconditions][checks]")
{
	const std::array<std::uint8_t, 7> src{};
	std::array<std::uint8_t, 1> dst{};
	SimdLib::SimdResample::ReduceBytesToBitsBy8_Any(src, dst);
	FAIL("SimdResample::ReduceBytesToBitsBy8_Any accepted an invalid shape");
}

TEST_CASE("SimdResample reduce all terminates for an invalid shape", "[simdlib][preconditions][checks]")
{
	const std::array<std::uint8_t, 7> src{};
	std::array<std::uint8_t, 1> dst{};
	SimdLib::SimdResample::ReduceBytesToBitsBy8_All(src, dst);
	FAIL("SimdResample::ReduceBytesToBitsBy8_All accepted an invalid shape");
}

TEST_CASE("SimdResample reduce parity terminates for an invalid shape", "[simdlib][preconditions][checks]")
{
	const std::array<std::uint8_t, 7> src{};
	std::array<std::uint8_t, 1> dst{};
	SimdLib::SimdResample::ReduceBytesToBitsBy8_Parity(src, dst);
	FAIL("SimdResample::ReduceBytesToBitsBy8_Parity accepted an invalid shape");
}

TEST_CASE("SimdResample expand terminates for an invalid shape", "[simdlib][preconditions][checks]")
{
	const std::array<std::uint8_t, 1> src{};
	std::array<std::uint8_t, 7> dst{};
	SimdLib::SimdResample::ExpandBitsToBytesBy8(src, dst);
	FAIL("SimdResample::ExpandBitsToBytesBy8 accepted an invalid shape");
}