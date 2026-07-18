#include <SimdLib/Format.h>

#include <format>
#include <string>

std::string FormatFromSecondTranslationUnit();

int main()
{
	const SimdLib::SimdVector<int, 3> value{1, 2, 3};
	return std::format("{}", value) == "{1, 2, 3}" &&
			   FormatFromSecondTranslationUnit() == "18446744073709551616"
		   ? 0
		   : 1;
}
