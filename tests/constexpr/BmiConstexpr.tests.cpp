#include <SimdLib/Bmi.h>

namespace SimdLib::Bmi
{
// Migrated verbatim from Bmi.h; additions follow the preserved assertion block.
static_assert(boolmask<std::uint8_t>(true) == 0xFF);
static_assert(boolmask<std::uint8_t>(false) == 0x00);
static_assert(boolmask<std::uint32_t>(true) == 0xFFFF'FFFF);
static_assert(boolmask<std::uint32_t>(false) == 0x0000'0000);
static_assert(boolmask<std::uint64_t>(true) == 0xFFFF'FFFF'FFFF'FFFF);
static_assert(boolmask<std::uint64_t>(false) == 0x0000'0000'0000'0000);
static_assert(select<std::int32_t>(1, 2, false) == 1);
static_assert(select<std::int32_t>(1, 2, true) == 2);
static_assert(select<std::int32_t>(-7, 4, false) == -7);
static_assert(select<std::int32_t>(-7, 4, true) == 4);
static_assert(select<std::uint32_t>(0xAAAA'AAAAu, 0x5555'5555u, false) == 0xAAAA'AAAAu);
static_assert(select<std::uint32_t>(0xAAAA'AAAAu, 0x5555'5555u, true) == 0x5555'5555u);
static_assert(max<std::int32_t>(1, 2) == 2);
static_assert(max<std::int32_t>(2, 1) == 2);
static_assert(max<std::int32_t>(-2, -5) == -2);
static_assert(max<std::int32_t>(7, 7) == 7);
static_assert(max<std::uint32_t>(3u, 9u) == 9u);
static_assert(max<std::uint32_t>(0u, 0u) == 0u);
static_assert(min<std::int32_t>(1, 2) == 1);
static_assert(min<std::int32_t>(2, 1) == 1);
static_assert(min<std::int32_t>(-2, -5) == -5);
static_assert(min<std::int32_t>(7, 7) == 7);
static_assert(min<std::uint32_t>(3u, 9u) == 3u);
static_assert(min<std::uint32_t>(0u, 0u) == 0u);
static_assert(abs<std::int32_t>(0) == 0);
static_assert(abs<std::int32_t>(7) == 7);
static_assert(abs<std::int32_t>(-7) == 7);
static_assert(abs<std::uint32_t>(0u) == 0u);
static_assert(abs<std::uint32_t>(7u) == 7u);
static_assert(blsi<std::uint32_t>(0b10100) == 0b00100);
static_assert(blsr<std::uint32_t>(0b1011) == 0b1010);
static_assert(blse<std::uint32_t>(0b10111) == std::make_tuple(std::uint32_t{0b10110}, std::uint32_t{0b1}));
static_assert(blsioff<std::uint32_t>(0b10100, 0b00001) == 0b00100);
static_assert(blsioff<std::uint32_t>(0b10100, 0b00010) == 0b00100);
static_assert(blsioff<std::uint32_t>(0b10100, 0b00100) == 0b00100);
static_assert(blsioff<std::uint32_t>(0b10100, 0b01000) == 0b10000);
static_assert(pp_xor<std::uint32_t>(0b01110) == 0b01001);
static_assert(pp_xor<std::uint32_t>(0b11110) == 0b10001);
static_assert(ps_xor<std::uint32_t>(0b01110) == 0b10010);
static_assert(ps_xor<std::uint32_t>(0b11110) == 0b100010);
static_assert(pp_or<std::uint8_t>(0b0100) == 0b0111);
static_assert(pp_or<std::uint16_t>(0b0100) == 0b0111);
static_assert(pp_or<std::uint32_t>(0b0) == 0b0);
static_assert(pp_or<std::uint32_t>(0b0100) == 0b0111);
static_assert(pp_or<std::uint32_t>(0b10100) == 0b11111);
static_assert(ps_or<std::int32_t>(0x0) == 0x0);
static_assert(ps_or<std::int32_t>(0b0100) == std::int32_t{-4});
static_assert(ps_or<std::uint32_t>(0b0100) == 0xFFFF'FFFC);
static_assert(ps_or<std::uint32_t>(0b10100) == 0xFFFF'FFFC);
static_assert(pp_lsor<std::uint32_t>(0b0) == 0b0);
static_assert(pp_lsor<std::uint32_t>(0b0100) == 0b0111);
static_assert(pp_lsor<std::uint32_t>(0b10100) == 0b00111);
static_assert(pp_and<std::uint32_t>(0b01101110) == 0b00100110);
static_assert(ps_and<std::uint32_t>(0b01101110) == 0b01001100);
static_assert(pp_andn<std::uint32_t>(0b01110) == 0b01000);
static_assert(ps_andn<std::uint32_t>(0b01110) == 0b00010);
static_assert(ps_andn<std::uint32_t>(0b001100) == 0b0000100);
static_assert(pp_andni<std::uint32_t>(0b01111) == 0b00000);
static_assert(pp_andni<std::uint32_t>(0b01110) == 0b00001);
static_assert(pp_andni<std::uint32_t>(0b01100) == 0b00010);
static_assert(pp_andni<std::uint32_t>(0b1011000) == 0b100100);
static_assert(ps_andni<std::uint32_t>(0b01110) == 0b10000);
static_assert(ps_andni<std::uint32_t>(0b001100) == 0b010000);
static_assert(ps_andni<std::uint32_t>(0b101100) == 0b1010000);
static_assert(bmsi<std::uint32_t>(0b10111) == 0b10000);
static_assert(bmsi<std::uint32_t>(0b0) == 0b0);
static_assert(bmsr<std::uint32_t>(0b1011) == 0b0011);
static_assert(bmsr<std::uint32_t>(0b1000) == 0b0000);
static_assert(bmse<std::uint32_t>(0b10111) == std::make_tuple(std::uint32_t{0b00111}, std::uint32_t{0b10000}));
static_assert(bzlo<std::uint32_t>(0b10111, 0) == 0b10111);
static_assert(bzlo<std::uint32_t>(0b10111, 1) == 0b10110);
static_assert(bzlo<std::uint32_t>(0b10111, 2) == 0b10100);
static_assert(bzlo<std::uint32_t>(0b10111, 3) == 0b10000);
static_assert(PartialSumBLSI<std::uint32_t>(0b1011) == 24);
static_assert(PartialSumBLSI<std::uint32_t>(0b1100) == 28);
static_assert(PartialSumBLSI<std::uint32_t>(0b1110) == 31);
static_assert(PartialSumBLSI<std::uint32_t>(0b1111) == 32);
static_assert(flipr_unset<std::uint32_t>(0b01011) == 0b01111);
static_assert(maskr_unset<std::uint32_t>(0b01011) == 0b00100);
static_assert(maskl_trailing_one<std::uint32_t>(0b010111) == 0b00100);
static_assert(maskl_trailing_one<std::uint32_t>(0b010110) == 0b0);
static_assert(clear_trailing_ones<std::uint32_t>(0b1011) == 0b1000);
static_assert(flip_trailing_zeros<std::uint32_t>(0b10100) == 0b10111);
static_assert(mask_trailing_zeros<std::uint32_t>(0b101000) == 0b0111);
static_assert(mask_trailing_zeros_or_zero<std::uint32_t>(0b101000) == 0b0111);
static_assert(mask_trailing_zeros_or_zero<std::uint32_t>(0) == 0u);
static_assert(mask_bits_lower_than_lsb<std::uint32_t>(0b101000) == 0b0111);
static_assert(mask_bits_lower_than_lsb<std::uint32_t>(0) == 0u);
static_assert(mask_bits_lower_than_lsb_or_all_ones<std::uint32_t>(0b101000) == 0b0111);
static_assert(mask_bits_lower_than_lsb_or_all_ones<std::uint32_t>(0) == 0xFFFF'FFFFu);
static_assert(mask_trailing_ones<std::uint32_t>(0b10111) == 0b00111);
static_assert(mask_trailing_ones<std::uint32_t>(0b10110) == 0b0);
static_assert(mask_leading_zeros<std::uint8_t>(0b000101) == 0xF8);
static_assert(mask_leading_zeros<std::uint16_t>(0b000101) == 0xFFF8);
static_assert(mask_leading_zeros<std::uint32_t>(0b000101) == 0xFFFFFFF8);
static_assert(mask_leading_ones<std::uint8_t>(0b11100000) == 0b11100000);
static_assert(mask_leading_ones<std::uint8_t>(0b11110101) == 0b11110000);
static_assert(mask_leading_ones<std::uint16_t>(0xFFF5) == 0xFFF0);
static_assert(mask_leading_ones<std::uint32_t>(0xFFFFFFFFU) == 0xFFFFFFFFU);
static_assert(mask_leading_ones<std::uint32_t>(0xFFFFFFF5U) == 0xFFFFFFF0U);
static_assert(clear_leading_ones<std::uint8_t>(0b11110000) == 0b00000000);
static_assert(clear_leading_ones<std::uint8_t>(0b11110101) == 0b00000101);
static_assert(clear_leading_ones<std::uint8_t>(0b11101011) == 0b00001011);
static_assert(clear_leading_ones<std::uint8_t>(0b10101111) == 0b00101111);
static_assert(clear_leading_ones<std::uint16_t>(0xFFF5) == 0b101);
static_assert(clear_leading_ones<std::uint32_t>(0xFFFFFFF5) == 0b101);
static_assert(clear_lowest_set_bits<std::uint32_t>(0b1011) == 0b1000);
static_assert(clear_lowest_set_bits<std::uint32_t>(0b10110) == 0b10000);
static_assert(consume_bit_sequence_right<std::uint32_t>(0b1011) == std::make_tuple(std::uint32_t{0b1000}, std::uint32_t{0b0011}));
static_assert(consume_bit_sequence_right<std::uint32_t>(0b10110) == std::make_tuple(std::uint32_t{0b10000}, std::uint32_t{0b00110}));
static_assert(consume_bit_sequence_left<std::uint32_t>(0b0110111) == std::make_tuple(std::uint32_t{0b0000111}, std::uint32_t{0b0110000}));
static_assert(consume_bit_sequence_left<std::uint32_t>(0b01101110) == std::make_tuple(std::uint32_t{0b00001110}, std::uint32_t{0b01100000}));
static_assert(left_collapse_trailing_bits<std::uint32_t>(0b10111) == 0b10100);
static_assert(left_collapse_trailing_bits<std::uint32_t>(0b10110) == 0b10110);
static_assert(clear_bits_lower_than<std::uint32_t>(0b10111, 0b00100) == 0b10100);
static_assert(clear_bits_lower_than<std::uint32_t>(0b10111, 0b00010) == 0b10110);
static_assert(extract_bits_lower_than<std::uint32_t>(0b10111, 0b00100) == 0b00011);
static_assert(extract_bits_lower_than<std::uint32_t>(0b10111, 0b00010) == 0b00001);
static_assert(extract_bits_higher_than<std::uint32_t>(0b10111, 0b00100) == 0b10000);
static_assert(extract_bits_higher_than<std::uint32_t>(0b10111, 0b00010) == 0b10100);
static_assert(extract_bits_higher_than<std::uint32_t>(0b10111, 0b00001) == 0b10110);
/**
 * @brief Expands the BMI constexpr contract across one integral width and signedness.
 * @tparam Integer Integral type under test.
 * @return True when
 * representative generic helpers preserve their bit contracts.
 */
template <std::integral Integer> [[nodiscard]] consteval bool bmi_width_contract() noexcept
{
	using unsigned_type = std::make_unsigned_t<Integer>;
	constexpr unsigned_type value = static_cast<unsigned_type>(0b10110100);
	constexpr unsigned_type mask = static_cast<unsigned_type>(0b01101010);
	const auto typedValue = static_cast<Integer>(value);
	const auto typedMask = static_cast<Integer>(mask);
	Integer high{};
	const Integer low = mulx<Integer>(static_cast<Integer>(3), static_cast<Integer>(7), high);
	return static_cast<unsigned_type>(andn<Integer>(typedMask, typedValue)) == static_cast<unsigned_type>((~mask) & value) &&
		   static_cast<unsigned_type>(bzhi<Integer>(typedValue, 4)) == static_cast<unsigned_type>(value & 0x0F) &&
		   static_cast<unsigned_type>(blsi<Integer>(typedValue)) == static_cast<unsigned_type>(value & (unsigned_type{0} - value)) &&
		   static_cast<unsigned_type>(blsr<Integer>(typedValue)) == static_cast<unsigned_type>(value & (value - 1)) &&
		   static_cast<unsigned_type>(pdep_u32(static_cast<std::uint32_t>(value), static_cast<std::uint32_t>(mask))) == 0x20u &&
		   static_cast<unsigned_type>(pext_u32(static_cast<std::uint32_t>(value), static_cast<std::uint32_t>(mask))) == 0x04u &&
		   low == static_cast<Integer>(21) && high == Integer{};
}

static_assert(blsmsk<std::uint32_t>(0b10100) == 0b00111);
static_assert(bextr<std::uint32_t>(0xFEDC'BA98u, 8, 12) == 0xCBu);
static_assert(pdep_u64(0b101u, 0b01010100u) == 0b01000100u);
static_assert(pext_u64(0b01000100u, 0b01010100u) == 0b101u);
static_assert(bmi_width_contract<std::int8_t>());
static_assert(bmi_width_contract<std::uint8_t>());
static_assert(bmi_width_contract<std::int16_t>());
static_assert(bmi_width_contract<std::uint16_t>());
static_assert(bmi_width_contract<std::int32_t>());
static_assert(bmi_width_contract<std::uint32_t>());
static_assert(bmi_width_contract<std::int64_t>());
static_assert(bmi_width_contract<std::uint64_t>());
} // namespace SimdLib::Bmi
