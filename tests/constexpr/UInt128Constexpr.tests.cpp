#include <SimdLib/UInt128.h>

namespace SimdLib
{
// Migrated verbatim from UInt128.h; additions follow the preserved assertion block.
static_assert(uint128_t{std::numeric_limits<std::uint64_t>::max(), 0} + uint128_t{1} == uint128_t{0, 1});
static_assert(uint128_t{0, 1} - uint128_t{1} == uint128_t{std::numeric_limits<std::uint64_t>::max(), 0});
static_assert((uint128_t{1} << 127) == uint128_t{0, std::uint64_t{1} << 63});
static_assert((uint128_t{1} << 128) == uint128_t{});
static_assert((uint128_t{0, std::uint64_t{1} << 63} >> 127) == uint128_t{1});
static_assert(popcount(std::numeric_limits<uint128_t>::max()) == 128);
/**
 * @brief Expands constant-evaluation proof across uint128 arithmetic, shifts, masks, and bit helpers.
 * @return True when representative values and every shift boundary match the public contract.
 */
[[nodiscard]] consteval bool uint128_contract() noexcept
{
	constexpr uint128_t lhs{0xFEDC'BA98'7654'3210ULL, 0x0123'4567'89AB'CDEFULL};
	constexpr uint128_t rhs{0x1111'2222'3333'4444ULL, 0x5555'6666'7777'8888ULL};
	if (lhs + rhs != uint128_t{0x0FED'DCBA'A987'7654ULL, 0x5678'ABCE'0123'5678ULL})
		return false;
	if (lhs - rhs != uint128_t{0xEDCB'9876'4320'EDCCULL, 0xABCD'DF01'1234'4567ULL})
		return false;
	if ((lhs & rhs) != uint128_t{0x1010'2200'3210'0000ULL, 0x0101'4466'0123'8888ULL} ||
		(lhs | rhs) != uint128_t{0xFFDD'BABA'7777'7654ULL, 0x5577'6767'FFFF'CDEFULL} ||
		(lhs ^ rhs) != uint128_t{0xEFCD'98BA'4567'7654ULL, 0x5476'2301'FEDC'4567ULL} || ~lhs != uint128_t{0x0123'4567'89AB'CDEFULL, 0xFEDC'BA98'7654'3210ULL})
		return false;
	if ((uint128_t{1} << 0) != uint128_t{1} || (uint128_t{1} << 63) != uint128_t{std::uint64_t{1} << 63} || (uint128_t{1} << 64) != uint128_t{0, 1} ||
		(uint128_t{1} << 127) != uint128_t{0, std::uint64_t{1} << 63} || (uint128_t{1} << 128) != uint128_t{} || (uint128_t{1} << 129) != uint128_t{})
		return false;
	constexpr uint128_t highBit{0, std::uint64_t{1} << 63};
	if ((highBit >> 0) != highBit || (highBit >> 63) != uint128_t{0, 1} || (highBit >> 64) != uint128_t{std::uint64_t{1} << 63} ||
		(highBit >> 127) != uint128_t{1} || (highBit >> 128) != uint128_t{} || (highBit >> 129) != uint128_t{})
		return false;
	if (uint128_t::create_mask(0) != uint128_t{} || uint128_t::create_mask(64) != uint128_t{~std::uint64_t{0}} ||
		uint128_t::create_mask(65) != uint128_t{~std::uint64_t{0}, 1} || uint128_t::create_mask(128) != std::numeric_limits<uint128_t>::max())
		return false;
	return popcount(lhs) == std::popcount(lhs.low()) + std::popcount(lhs.high()) && countr_zero(uint128_t{}) == 128 && countl_zero(uint128_t{}) == 128 &&
		   bit_width(highBit) == 128 && bit_floor(highBit) == highBit && bit_ceil(highBit) == highBit && has_single_bit(highBit) &&
		   Bmi::bextr(lhs, 17, 61) == ((lhs >> 61) & uint128_t::create_mask(17)) &&
		   Bmi::bextr(lhs, 61u | (17u << 8u) | 0xFFFF'0000u) == ((lhs >> 61) & uint128_t::create_mask(17));
}

static_assert(uint128_contract());
} // namespace SimdLib
