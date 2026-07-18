#include <SimdLib/Format.h>

#include <format>
#include <string>

std::string FormatFromSecondTranslationUnit()
{
	return std::format("{}", SimdLib::uint128_t{0, 1});
}
