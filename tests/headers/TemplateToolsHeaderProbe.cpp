#include <SimdLib/TemplateTools.h>

#include <array>
#include <tuple>
#include <type_traits>

static_assert(std::same_as<SimdLib::select_unsigned_integer_t<0>, std::uint8_t>);
static_assert(std::same_as<SimdLib::select_unsigned_integer_t<8>, std::uint8_t>);
static_assert(std::same_as<SimdLib::select_unsigned_integer_t<9>, std::uint16_t>);
static_assert(std::same_as<SimdLib::select_unsigned_integer_t<16>, std::uint16_t>);
static_assert(std::same_as<SimdLib::select_unsigned_integer_t<17>, std::uint32_t>);
static_assert(std::same_as<SimdLib::select_unsigned_integer_t<32>, std::uint32_t>);
static_assert(std::same_as<SimdLib::select_unsigned_integer_t<33>, std::uint64_t>);
static_assert(std::same_as<SimdLib::select_signed_integer_t<64>, std::int64_t>);
static_assert(SimdLib::integer_like<std::uint64_t>);
static_assert(!SimdLib::integer_like<bool>);
static_assert(!SimdLib::integer_like<float>);

consteval bool template_utility_contract()
{
	int sum = 0;
	SimdLib::constexpr_for_each([&](const int value) { sum += value; }, 1, 2, 3);
	SimdLib::constexpr_for<0, 4, 1>([&]<auto Index>() { sum += static_cast<int>(Index); });
	const auto tuple = std::tuple{4, 5};
	SimdLib::constexpr_for_tuple(tuple, [&](const std::size_t index, const int value) { sum += static_cast<int>(index) + value; });
	return sum == 22 && SimdLib::force_consteval(7) == 7 && std::same_as<decltype(SimdLib::sorted_unique), const SimdLib::sorted_unique_t>;
}

static_assert(template_utility_contract());
