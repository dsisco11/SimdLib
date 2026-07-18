#include <SimdLib/SimdLib.h>

int main()
{
	const SimdLib::uint128_t value{41};
	if (value + SimdLib::uint128_t{1} != 42 || SimdLib::Bmi::blsi(SimdLib::uint128_t{12}) != 4)
	{
		return 1;
	}

	const SimdLib::SimdVector<int, 3> vector{2, 4, 8};
	if (vector[1] != 4)
	{
		return 2;
	}
	using api = SimdLib::Api<128, int>;
	if (api::to_array(api::add(api::set1(2), api::set1(3)))[0] != 5)
	{
		return 3;
	}
	return 0;
}
