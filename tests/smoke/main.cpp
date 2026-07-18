#include <SimdLib/SimdLib.h>

#include <array>
#include <cstdint>

int second_translation_unit_version() noexcept;
std::uint32_t second_translation_unit_resample() noexcept;

namespace
{
std::uint32_t first_translation_unit_resample() noexcept
{
	const std::array<std::uint8_t, 8> bytes{0, 1, 0xFF, 2, 0, 0x80, 3, 0};
	std::array<std::uint8_t, 1> any{};
	std::array<std::uint8_t, 1> all{};
	std::array<std::uint8_t, 1> parity{};
	std::array<std::uint8_t, 8> expanded{};
	SimdLib::SimdResample::ReduceBytesToBitsBy8_Any(bytes, any);
	SimdLib::SimdResample::ReduceBytesToBitsBy8_All(bytes, all);
	SimdLib::SimdResample::ReduceBytesToBitsBy8_Parity(bytes, parity);
	SimdLib::SimdResample::ExpandBitsToBytesBy8(any, expanded);
	return any[0] + all[0] + parity[0] + expanded[1];
}
}

int main()
{
	return SimdLib::version_major + SimdLib::version_minor + SimdLib::version_patch == second_translation_unit_version()
	        && first_translation_unit_resample() == second_translation_unit_resample()
	    ? 0
	    : 1;
}
