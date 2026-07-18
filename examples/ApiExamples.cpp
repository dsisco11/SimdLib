#include <SimdLib/Format.h>
#include <SimdLib/SimdLib.h>

#include <array>
#include <cstdint>
#include <format>
#include <span>
#include <string>

int main()
{
	using api = SimdLib::Api<128, std::uint32_t>;
	const auto api_sum = api::add(api::setr(1, 2, 3, 4), api::set1(10));
	if (api::to_array(api_sum) != std::array<std::uint32_t, 4>{11, 12, 13, 14})
	{
		return 1;
	}

	const SimdLib::SimdVector<std::uint32_t, 4> vector{3, 5, 7, 9};
	if (vector[2] != 7 || std::format("{}", vector) != "{3, 5, 7, 9}")
	{
		return 2;
	}

	const std::array<std::uint8_t, 8> values{1, 2, 3, 4, 5, 6, 7, 8};
	if (!SimdLib::SimdAlgo<8, 8>::AnyEqual(std::span<const std::uint8_t, 8>{values}, std::uint8_t{6}))
	{
		return 3;
	}

	if (SimdLib::Bmi::pext_u32(std::uint32_t{0b1101'0010}, std::uint32_t{0b1111'0000}) != 0b1101)
	{
		return 4;
	}

	const SimdLib::uint128_t wide = SimdLib::uint128_t{0xFFFF'FFFF'FFFF'FFFFULL} + SimdLib::uint128_t{1};
	if (wide.low() != 0 || wide.high() != 1 || std::format("{}", wide) != "18446744073709551616")
	{
		return 5;
	}

	const std::array<std::uint8_t, 8> bytes{0, 1, 0, 2, 0, 3, 0, 4};
	std::array<std::uint8_t, 1> bits{};
	SimdLib::SimdResample::ReduceBytesToBitsBy8_Any(bytes, bits);
	return bits[0] == 0b1010'1010 ? 0 : 6;
}
