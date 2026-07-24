#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <cstdlib>

namespace
{
/** @brief Unique marker emitted only by the Register precondition-failure harness. */
inline constexpr char expected_register_precondition_failure_marker[] = "SIMDLIB_REGISTER_PRECONDITION_FAILURE_EXPECTED_61B4C2";

/** @brief Diagnostic process exit code used after an expected precondition failure. */
inline constexpr int register_precondition_failure_exit_code = 74;

/**
 * @brief Terminates the isolated test process after proving a Register precondition fired.
 * @param message Diagnostic supplied by the failed public precondition.
 */
[[noreturn]] void fail_register_precondition(const char *message) noexcept
{
	(void)message;
	std::fputs(expected_register_precondition_failure_marker, stderr);
	std::fputc('\n', stderr);
	std::fflush(stderr);
	std::exit(register_precondition_failure_exit_code);
}
} // namespace

#define SIMDLIB_PRECONDITION(condition, message)                                                                                                               \
	do                                                                                                                                                         \
	{                                                                                                                                                          \
		if (!(condition))                                                                                                                                      \
			fail_register_precondition(message);                                                                                                               \
	} while (false)

#include <SimdLib/Register.h>

#undef SIMDLIB_PRECONDITION

#include <array>
#include <cstdint>
#include <span>

TEST_CASE("Register left shift rejects a negative per-lane count", "[simdlib][register][preconditions]")
{
	using register_type = SimdLib::Register<std::uint32_t, 128>;
	(void)(register_type::broadcast(1U) << -1);
	FAIL("Register left shift accepted a negative count");
}

TEST_CASE("Register logical right shift rejects a negative per-lane count", "[simdlib][register][preconditions]")
{
	using register_type = SimdLib::Register<std::int32_t, 128>;
	(void)register_type::broadcast(-1).logical_shift_right(-1);
	FAIL("Register logical right shift accepted a negative count");
}

TEST_CASE("Register arithmetic right shift rejects a negative per-lane count", "[simdlib][register][preconditions]")
{
	using register_type = SimdLib::Register<std::int32_t, 128>;
	(void)(register_type::broadcast(-1) >> -1);
	FAIL("Register arithmetic right shift accepted a negative count");
}

TEST_CASE("Register aligned load rejects a misaligned source", "[simdlib][register][preconditions]")
{
	using register_type = SimdLib::Register<std::uint32_t, 128>;
	alignas(register_type::byte_count) std::array<std::uint32_t, register_type::lane_count + 1> source{};
	(void)register_type::load_aligned(std::span<const std::uint32_t, register_type::lane_count>{source.data() + 1, register_type::lane_count});
	FAIL("Register aligned load accepted a misaligned source");
}

TEST_CASE("Register aligned store rejects a misaligned destination", "[simdlib][register][preconditions]")
{
	using register_type = SimdLib::Register<std::uint32_t, 128>;
	alignas(register_type::byte_count) std::array<std::uint32_t, register_type::lane_count + 1> destination{};
	register_type::zero().store_aligned(std::span<std::uint32_t, register_type::lane_count>{destination.data() + 1, register_type::lane_count});
	FAIL("Register aligned store accepted a misaligned destination");
}
