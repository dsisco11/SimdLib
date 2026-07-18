#include <SimdLib/Bmi.h>
#include <SimdLib/UInt128.h>

#include <cstdint>

namespace
{
constexpr SimdLib::uint128_t low{0xFFFF'FFFF'FFFF'FFFFULL, 0};
constexpr SimdLib::uint128_t carried = low + SimdLib::uint128_t{1};

static_assert(carried.low() == 0);
static_assert(carried.high() == 1);
static_assert((carried << 63).high() == (std::uint64_t{1} << 63));
static_assert(SimdLib::Bmi::pdep_u32(std::uint32_t{0b1011}, std::uint32_t{0b0101'0101}) == 0b0100'0101);
static_assert(SimdLib::Bmi::pext_u32(std::uint32_t{0b1101'0010}, std::uint32_t{0b1111'0000}) == 0b1101);
static_assert(SimdLib::Bmi::bzhi(std::uint32_t{0xFFFF}, 8) == 0xFF);
} // namespace

int ConstexprProbe() noexcept
{
	return static_cast<int>(carried.high());
}
