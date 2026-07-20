#include <cstddef>
#include <cstdlib>

namespace SimdLibPreconditionProbe
{
inline constexpr int contract_failure_exit_code = 73;

/** @brief Terminates the isolated probe with the contract-failure exit code.
 *  @param message Diagnostic supplied by the failed public precondition.
 */
[[noreturn]] inline void Fail(const char *message) noexcept
{
	(void)message;
	std::exit(contract_failure_exit_code);
}
} // namespace SimdLibPreconditionProbe

#define SIMDLIB_PRECONDITION(condition, message)                                                                                                               \
	do                                                                                                                                                         \
	{                                                                                                                                                          \
		if (!(condition))                                                                                                                                      \
			::SimdLibPreconditionProbe::Fail(message);                                                                                                         \
	} while (false)

#include <SimdLib/Api.h>
#include <SimdLib/SimdAlgo.h>
#include <SimdLib/SimdResample.h>

#undef SIMDLIB_PRECONDITION

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace
{
using Api128 = SimdLib::Api<128, std::uint32_t>;
using Algo = SimdLib::SimdAlgo<8, 1>;

/** @brief Invokes the selected caller-facing operation with an invalid runtime contract.
 *  @param scenario Stable scenario name supplied by CTest.
 *  @return Zero only if the selected precondition unexpectedly permits execution to continue.
 */
int RunScenario(const std::string_view scenario) noexcept
{
	if (scenario == "api_load_aligned")
	{
		alignas(Api128::byte_count) std::array<std::uint32_t, Api128::element_count + 1> data{};
		(void)Api128::load_aligned(std::span<const std::uint32_t, Api128::element_count>(data.data() + 1, Api128::element_count));
	}
	else if (scenario == "api_load_partial")
	{
		const std::array<std::uint32_t, 1> data{};
		(void)Api128::load_partial<2>(std::span<const std::uint32_t>(data));
	}
	else if (scenario == "api_store_aligned")
	{
		alignas(Api128::byte_count) std::array<std::uint32_t, Api128::element_count + 1> data{};
		Api128::store_aligned(Api128::setzero(), std::span<std::uint32_t, Api128::element_count>(data.data() + 1, Api128::element_count));
	}
	else if (scenario == "api_store_bytes")
	{
		std::array<std::byte, Api128::byte_count - 1> data{};
		Api128::store(Api128::setzero(), std::span<std::byte>(data));
	}
	else if (scenario == "algo_bitwise_and")
	{
		const std::array<std::uint8_t, 2> lhs{};
		const std::array<std::uint8_t, 1> rhs{};
		std::array<std::uint8_t, 2> write{};
		Algo::BitwiseAnd(lhs, rhs, write);
	}
	else if (scenario == "algo_bitwise_or")
	{
		const std::array<std::uint8_t, 2> lhs{};
		const std::array<std::uint8_t, 1> rhs{};
		std::array<std::uint8_t, 2> write{};
		Algo::BitwiseOr(lhs, rhs, write);
	}
	else if (scenario == "algo_bitwise_xor")
	{
		const std::array<std::uint8_t, 2> lhs{};
		const std::array<std::uint8_t, 1> rhs{};
		std::array<std::uint8_t, 2> write{};
		Algo::BitwiseXor(lhs, rhs, write);
	}
	else if (scenario == "algo_bitwise_not")
	{
		const std::array<std::uint8_t, 2> lhs{};
		std::array<std::uint8_t, 1> write{};
		Algo::BitwiseNot(lhs, write);
	}
	else if (scenario == "algo_bitwise_andnot")
	{
		const std::array<std::uint8_t, 2> lhs{};
		const std::array<std::uint8_t, 1> rhs{};
		std::array<std::uint8_t, 2> write{};
		Algo::BitwiseAndNot(lhs, rhs, write);
	}
	else if (scenario == "resample_reduce_any")
	{
		const std::array<std::uint8_t, 7> src{};
		std::array<std::uint8_t, 1> dst{};
		SimdLib::SimdResample::ReduceBytesToBitsBy8_Any(src, dst);
	}
	else if (scenario == "resample_reduce_all")
	{
		const std::array<std::uint8_t, 7> src{};
		std::array<std::uint8_t, 1> dst{};
		SimdLib::SimdResample::ReduceBytesToBitsBy8_All(src, dst);
	}
	else if (scenario == "resample_reduce_parity")
	{
		const std::array<std::uint8_t, 7> src{};
		std::array<std::uint8_t, 1> dst{};
		SimdLib::SimdResample::ReduceBytesToBitsBy8_Parity(src, dst);
	}
	else if (scenario == "resample_expand")
	{
		const std::array<std::uint8_t, 1> src{};
		std::array<std::uint8_t, 7> dst{};
		SimdLib::SimdResample::ExpandBitsToBytesBy8(src, dst);
	}
	else
	{
		return 64;
	}

	return 0;
}
} // namespace

/** @brief Runs one isolated invalid-contract scenario for the CTest death-test driver.
 *  @param argc Argument count; exactly one scenario argument is required.
 *  @param argv Argument vector containing the scenario name.
 *  @return Probe status, where contract failures terminate earlier with code 73.
 */
int main(const int argc, char **argv) noexcept
{
	return argc == 2 ? RunScenario(argv[1]) : 64;
}
