#include <SimdLib/Bmi.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>

#ifndef SIMDLIB_BMI_EXPECT_BMI1
#define SIMDLIB_BMI_EXPECT_BMI1 0
#endif

#ifndef SIMDLIB_BMI_EXPECT_BMI2
#define SIMDLIB_BMI_EXPECT_BMI2 0
#endif

namespace SimdLib
{
// This verifies the cycle-free wide-integer customization boundary. Bmi.h
// deliberately does not name or include this type.
class uint128_t final
{
  public:
	constexpr uint128_t() noexcept = default;
	constexpr uint128_t(const std::uint64_t value) noexcept : low_(value) {}
	constexpr uint128_t(const std::uint64_t low, const std::uint64_t high) noexcept : low_(low), high_(high) {}

	[[nodiscard]] constexpr std::uint64_t low() const noexcept
	{
		return low_;
	}
	[[nodiscard]] constexpr std::uint64_t high() const noexcept
	{
		return high_;
	}
	[[nodiscard]] constexpr explicit operator bool() const noexcept
	{
		return low_ != 0 || high_ != 0;
	}

	friend constexpr bool operator==(const uint128_t &, const uint128_t &) noexcept = default;
	friend constexpr bool operator<(const uint128_t lhs, const uint128_t rhs) noexcept
	{
		return lhs.high_ < rhs.high_ || (lhs.high_ == rhs.high_ && lhs.low_ < rhs.low_);
	}
	friend constexpr uint128_t operator~(const uint128_t value) noexcept
	{
		return {~value.low_, ~value.high_};
	}
	friend constexpr uint128_t operator&(const uint128_t lhs, const uint128_t rhs) noexcept
	{
		return {lhs.low_ & rhs.low_, lhs.high_ & rhs.high_};
	}
	friend constexpr uint128_t operator|(const uint128_t lhs, const uint128_t rhs) noexcept
	{
		return {lhs.low_ | rhs.low_, lhs.high_ | rhs.high_};
	}
	friend constexpr uint128_t operator^(const uint128_t lhs, const uint128_t rhs) noexcept
	{
		return {lhs.low_ ^ rhs.low_, lhs.high_ ^ rhs.high_};
	}
	friend constexpr uint128_t operator+(const uint128_t lhs, const uint128_t rhs) noexcept
	{
		const std::uint64_t low = lhs.low_ + rhs.low_;
		return {low, lhs.high_ + rhs.high_ + static_cast<std::uint64_t>(low < lhs.low_)};
	}
	friend constexpr uint128_t operator-(const uint128_t lhs, const uint128_t rhs) noexcept
	{
		return {lhs.low_ - rhs.low_, lhs.high_ - rhs.high_ - static_cast<std::uint64_t>(lhs.low_ < rhs.low_)};
	}
	friend constexpr uint128_t operator-(const uint128_t value) noexcept
	{
		return uint128_t{} - value;
	}
	friend constexpr uint128_t operator<<(const uint128_t value, const unsigned count) noexcept
	{
		if (count == 0)
			return value;
		if (count >= 128)
			return {};
		if (count >= 64)
			return {0, value.low_ << (count - 64)};
		return {value.low_ << count, (value.high_ << count) | (value.low_ >> (64 - count))};
	}
	friend constexpr uint128_t operator>>(const uint128_t value, const unsigned count) noexcept
	{
		if (count == 0)
			return value;
		if (count >= 128)
			return {};
		if (count >= 64)
			return {value.high_ >> (count - 64), 0};
		return {(value.low_ >> count) | (value.high_ << (64 - count)), value.high_ >> count};
	}

  private:
	std::uint64_t low_ = 0;
	std::uint64_t high_ = 0;
};

[[nodiscard]] constexpr int bit_width(const uint128_t value) noexcept
{
	return value.high() == 0 ? std::bit_width(value.low()) : 64 + std::bit_width(value.high());
}

[[nodiscard]] constexpr uint128_t bit_floor(const uint128_t value) noexcept
{
	if (value.high() != 0)
		return {0, std::bit_floor(value.high())};
	return {std::bit_floor(value.low()), 0};
}
} // namespace SimdLib

template <> class std::numeric_limits<SimdLib::uint128_t>
{
  public:
	static constexpr bool is_specialized = true;
	static constexpr bool is_signed = false;
	static constexpr bool is_integer = true;
	static constexpr int digits = 128;
	[[nodiscard]] static constexpr SimdLib::uint128_t min() noexcept
	{
		return {};
	}
	[[nodiscard]] static constexpr SimdLib::uint128_t lowest() noexcept
	{
		return {};
	}
	[[nodiscard]] static constexpr SimdLib::uint128_t max() noexcept
	{
		return {~std::uint64_t{0}, ~std::uint64_t{0}};
	}
};

namespace
{
using SimdLib::uint128_t;
namespace Bmi = SimdLib::Bmi;

static_assert((SIMDLIB_BMI_EXPECT_BMI1 != 0) == SimdLib::Config::has_bmi1);
static_assert((SIMDLIB_BMI_EXPECT_BMI2 != 0) == SimdLib::Config::has_bmi2);
static_assert(Bmi::integer_like<std::int32_t>);
static_assert(Bmi::integer_like<uint128_t>);
static_assert(!Bmi::integer_like<bool>);
static_assert(!Bmi::integer_like<float>);

template <std::unsigned_integral int_t> [[nodiscard]] constexpr int_t reference_bzhi(const int_t value, const unsigned index) noexcept
{
	constexpr unsigned width = sizeof(int_t) * 8u;
	if (index == 0)
		return 0;
	if (index >= width)
		return value;
	return value & static_cast<int_t>((int_t{1} << index) - int_t{1});
}

template <std::unsigned_integral int_t> [[nodiscard]] constexpr int_t reference_bextr(const int_t value, const unsigned start, const unsigned len) noexcept
{
	constexpr unsigned width = sizeof(int_t) * 8u;
	if (len == 0 || start >= width)
		return 0;
	const unsigned count = len < (width - start) ? len : (width - start);
	return reference_bzhi<int_t>(value >> start, count);
}

template <std::unsigned_integral int_t> [[nodiscard]] constexpr int_t reference_pdep(const int_t source, const int_t mask) noexcept
{
	int_t result = 0;
	int_t source_bit = 1;
	for (int_t remaining = mask; remaining != 0; remaining &= remaining - 1)
	{
		const int_t destination_bit = remaining & (int_t{0} - remaining);
		if ((source & source_bit) != 0)
			result |= destination_bit;
		source_bit <<= 1;
	}
	return result;
}

template <std::unsigned_integral int_t> [[nodiscard]] constexpr int_t reference_pext(const int_t source, const int_t mask) noexcept
{
	int_t result = 0;
	int_t destination_bit = 1;
	for (int_t remaining = mask; remaining != 0; remaining &= remaining - 1)
	{
		const int_t source_bit = remaining & (int_t{0} - remaining);
		if ((source & source_bit) != 0)
			result |= destination_bit;
		destination_bit <<= 1;
	}
	return result;
}

struct product128
{
	std::uint64_t low;
	std::uint64_t high;
};

[[nodiscard]] constexpr product128 reference_mulx64(const std::uint64_t lhs, const std::uint64_t rhs) noexcept
{
	std::array<std::uint32_t, 4> left{};
	std::array<std::uint32_t, 4> right{};
	std::array<std::uint32_t, 8> product{};
	for (unsigned index = 0; index < 4; ++index)
	{
		left[index] = static_cast<std::uint16_t>(lhs >> (index * 16));
		right[index] = static_cast<std::uint16_t>(rhs >> (index * 16));
	}
	for (unsigned left_index = 0; left_index < 4; ++left_index)
	{
		std::uint64_t carry = 0;
		for (unsigned right_index = 0; right_index < 4; ++right_index)
		{
			const unsigned result_index = left_index + right_index;
			const std::uint64_t total = product[result_index] + static_cast<std::uint64_t>(left[left_index]) * right[right_index] + carry;
			product[result_index] = static_cast<std::uint16_t>(total);
			carry = total >> 16;
		}
		unsigned result_index = left_index + 4;
		while (carry != 0 && result_index < product.size())
		{
			const std::uint64_t total = product[result_index] + carry;
			product[result_index] = static_cast<std::uint16_t>(total);
			carry = total >> 16;
			++result_index;
		}
	}
	std::uint64_t low = 0;
	std::uint64_t high = 0;
	for (unsigned index = 0; index < 4; ++index)
	{
		low |= static_cast<std::uint64_t>(product[index]) << (index * 16);
		high |= static_cast<std::uint64_t>(product[index + 4]) << (index * 16);
	}
	return {low, high};
}

[[nodiscard]] constexpr std::uint64_t low_mask(const unsigned count) noexcept
{
	if (count == 0)
		return 0;
	if (count >= 64)
		return ~std::uint64_t{0};
	return (std::uint64_t{1} << count) - 1;
}

[[nodiscard]] constexpr uint128_t reference_bzhi128(const uint128_t value, const unsigned index) noexcept
{
	if (index == 0)
		return {};
	if (index >= 128)
		return value;
	if (index <= 64)
		return {value.low() & low_mask(index), 0};
	return {value.low(), value.high() & low_mask(index - 64)};
}

[[nodiscard]] constexpr uint128_t reference_blsi128(const uint128_t value) noexcept
{
	if (value.low() != 0)
		return {value.low() & (std::uint64_t{0} - value.low()), 0};
	return {0, value.high() & (std::uint64_t{0} - value.high())};
}

[[nodiscard]] constexpr uint128_t reference_blsr128(const uint128_t value) noexcept
{
	if (value.low() != 0)
		return {value.low() & (value.low() - 1), value.high()};
	return {0, value.high() & (value.high() - 1)};
}

[[nodiscard]] constexpr uint128_t reference_blsmsk128(const uint128_t value) noexcept
{
	if (value.low() != 0)
		return {value.low() ^ (value.low() - 1), 0};
	return {~std::uint64_t{0}, value.high() ^ (value.high() - 1)};
}

class random64
{
  public:
	explicit constexpr random64(const std::uint64_t seed) noexcept : state_(seed) {}
	[[nodiscard]] constexpr std::uint64_t next() noexcept
	{
		state_ ^= state_ << 13;
		state_ ^= state_ >> 7;
		state_ ^= state_ << 17;
		return state_;
	}

  private:
	std::uint64_t state_;
};

constexpr bool constexpr_contract()
{
	if (Bmi::andn(std::uint32_t{0x0F0F0F0F}, std::uint32_t{0xFFFF0000}) != 0xF0F00000)
		return false;
	if (Bmi::bzhi(std::uint64_t{0xFEDCBA9876543210}, 36) != 0x0000000876543210)
		return false;
	if (Bmi::blsi(std::uint64_t{0xA800}) != 0x800)
		return false;
	if (Bmi::blsr(std::uint64_t{0xA800}) != 0xA000)
		return false;
	if (Bmi::blsmsk(std::uint64_t{0xA800}) != 0xFFF)
		return false;
	std::uint64_t high = 0;
	if (Bmi::mulx(std::uint64_t{0xFFFFFFFFFFFFFFFF}, std::uint64_t{2}, high) != 0xFFFFFFFFFFFFFFFE || high != 1)
		return false;
	if (Bmi::pdep_u32(0b1011, 0b01010110) != reference_pdep(std::uint32_t{0b1011}, std::uint32_t{0b01010110}))
		return false;
	if (Bmi::pext_u32(0b11010110, 0b01010110) != reference_pext(std::uint32_t{0b11010110}, std::uint32_t{0b01010110}))
		return false;
	constexpr uint128_t wide{0x0123456789ABCDEF, 0xFEDCBA9876543210};
	return Bmi::andn(wide, ~uint128_t{}) == ~wide && Bmi::bzhi(wide, 73) == reference_bzhi128(wide, 73);
}
static_assert(constexpr_contract());

void mix_digest(std::uint64_t &digest, const std::uint64_t value)
{
	digest ^= value;
	digest *= 1099511628211ULL;
}

template <std::signed_integral signed_t> void require_signed_bit_pattern_contract(const signed_t source, const signed_t rhs)
{
	using unsigned_t = std::make_unsigned_t<signed_t>;
	const unsigned_t source_bits = std::bit_cast<unsigned_t>(source);
	const unsigned_t rhs_bits = std::bit_cast<unsigned_t>(rhs);
	const auto bits = [](const signed_t value) { return std::bit_cast<unsigned_t>(value); };

	CHECK(bits(Bmi::andn(source, rhs)) == static_cast<unsigned_t>((~source_bits) & rhs_bits));
	CHECK(bits(Bmi::bzhi(source, 13)) == reference_bzhi(source_bits, 13));
	CHECK(bits(Bmi::blsi(source)) == static_cast<unsigned_t>(source_bits & (unsigned_t{0} - source_bits)));
	CHECK(bits(Bmi::blsr(source)) == static_cast<unsigned_t>(source_bits & (source_bits - 1)));
	CHECK(bits(Bmi::blsmsk(source)) == static_cast<unsigned_t>(source_bits ^ (source_bits - 1)));
	CHECK(bits(Bmi::bextr(source, 17, 5)) == reference_bextr(source_bits, 5, 17));

	signed_t high{};
	const signed_t low = Bmi::mulx(source, rhs, high);
	if constexpr (sizeof(signed_t) <= 4)
	{
		const std::uint64_t product = static_cast<std::uint64_t>(source_bits) * rhs_bits;
		CHECK(bits(low) == static_cast<unsigned_t>(product));
		CHECK(bits(high) == static_cast<unsigned_t>(product >> std::numeric_limits<unsigned_t>::digits));
	}
	else
	{
		const auto product = reference_mulx64(source_bits, rhs_bits);
		CHECK(bits(low) == product.low);
		CHECK(bits(high) == product.high);
	}
}
} // namespace

TEST_CASE("BMI signed helpers preserve two's-complement bit patterns", "[simdlib][bmi][signed][regression]")
{
	require_signed_bit_pattern_contract<std::int32_t>(std::bit_cast<std::int32_t>(0xF234'5678u), std::bit_cast<std::int32_t>(0x8ACE'1357u));
	require_signed_bit_pattern_contract<std::int64_t>(std::bit_cast<std::int64_t>(0xF234'5678'9ABC'DEF0ull),
													  std::bit_cast<std::int64_t>(0x8ACE'1357'2468'BDF1ull));
}

TEST_CASE("BMI absolute value handles signed boundaries without arithmetic overflow", "[simdlib][bmi][signed][abs]")
{
	CHECK(Bmi::abs(std::uint32_t{0}) == 0);
	CHECK(Bmi::abs(std::uint32_t{0xFFFF'FFFF}) == 0xFFFF'FFFF);
	CHECK(Bmi::abs(std::int8_t{0}) == 0);
	CHECK(Bmi::abs(std::int8_t{-127}) == 127);
	CHECK(Bmi::abs(std::numeric_limits<std::int8_t>::min()) == std::numeric_limits<std::int8_t>::min());
	CHECK(Bmi::abs(std::int32_t{1} << 30) == (std::int32_t{1} << 30));
	CHECK(Bmi::abs(-(std::int32_t{1} << 30)) == (std::int32_t{1} << 30));
	CHECK(Bmi::abs(std::numeric_limits<std::int32_t>::max()) == std::numeric_limits<std::int32_t>::max());
	CHECK(Bmi::abs(std::numeric_limits<std::int32_t>::min()) == std::numeric_limits<std::int32_t>::min());
	CHECK(Bmi::abs(std::numeric_limits<std::int64_t>::min()) == std::numeric_limits<std::int64_t>::min());
}

TEST_CASE("BMI selection and ordering helpers have table-driven public contracts", "[simdlib][bmi][derived][table]")
{
	constexpr std::array<std::tuple<std::int32_t, std::int32_t>, 7> pairs{{
		{-7, 4},
		{4, -7},
		{0, 0},
		{1, 1},
		{std::numeric_limits<std::int32_t>::min(), std::numeric_limits<std::int32_t>::max()},
		{std::numeric_limits<std::int32_t>::max(), std::numeric_limits<std::int32_t>::min()},
		{-1, 0},
	}};
	CHECK(Bmi::boolmask<std::uint8_t>(false) == std::uint8_t{0});
	CHECK(Bmi::boolmask<std::uint8_t>(true) == std::uint8_t{0xFF});
	CHECK(Bmi::boolmask<std::uint16_t>(true) == std::uint16_t{0xFFFF});
	CHECK(Bmi::boolmask<std::uint32_t>(true) == std::uint32_t{0xFFFF'FFFF});
	CHECK(Bmi::boolmask<std::uint64_t>(true) == std::uint64_t{0xFFFF'FFFF'FFFF'FFFF});
	CHECK(Bmi::boolmask<uint128_t>(false) == uint128_t{});
	CHECK(Bmi::boolmask<uint128_t>(true) == std::numeric_limits<uint128_t>::max());
	CHECK(Bmi::select(uint128_t{1}, uint128_t{2}, false) == uint128_t{1});
	CHECK(Bmi::select(uint128_t{1}, uint128_t{2}, true) == uint128_t{2});

	for (const auto &[lhs, rhs] : pairs)
	{
		CHECK(Bmi::select(lhs, rhs, false) == lhs);
		CHECK(Bmi::select(lhs, rhs, true) == rhs);
		CHECK(Bmi::min(lhs, rhs) == (lhs < rhs ? lhs : rhs));
		CHECK(Bmi::max(lhs, rhs) == (lhs < rhs ? rhs : lhs));
	}
	CHECK(Bmi::min(std::uint32_t{0x8000'0000}, std::uint32_t{7}) == 7);
	CHECK(Bmi::max(std::uint32_t{0x8000'0000}, std::uint32_t{7}) == 0x8000'0000);
}

TEST_CASE("BMI derived unary helpers match exhaustive 8-bit scalar oracles", "[simdlib][bmi][derived][exhaustive]")
{
	for (unsigned source = 0; source <= 0xFF; ++source)
	{
		const auto value = static_cast<std::uint8_t>(source);
		const auto complement = static_cast<std::uint8_t>(~value);
		const auto subtract_one = static_cast<std::uint8_t>(value - 1u);
		const auto add_one = static_cast<std::uint8_t>(value + 1u);
		const auto lsb = static_cast<std::uint8_t>(value & static_cast<std::uint8_t>(0u - value));
		const auto msb = value == 0 ? std::uint8_t{0} : static_cast<std::uint8_t>(std::uint8_t{1} << (std::bit_width(value) - 1));
		const auto prefix = value == 0 ? std::uint8_t{0} : static_cast<std::uint8_t>((std::uint16_t{1} << std::bit_width(value)) - 1);
		const auto suffix = value == 0 ? std::uint8_t{0} : static_cast<std::uint8_t>(0xFFu << std::countr_zero(value));
		const auto lowest_prefix = value == 0 ? std::uint8_t{0} : static_cast<std::uint8_t>((std::uint16_t{1} << (std::countr_zero(value) + 1)) - 1);

		CHECK(Bmi::pp_xor(value) == static_cast<std::uint8_t>(value ^ (value >> 1)));
		CHECK(Bmi::ps_xor(value) == static_cast<std::uint8_t>(value ^ static_cast<std::uint8_t>(value << 1)));
		CHECK(Bmi::pp_or(value) == prefix);
		CHECK(Bmi::ps_or(value) == suffix);
		CHECK(Bmi::pp_lsor(value) == lowest_prefix);
		CHECK(Bmi::pp_and(value) == static_cast<std::uint8_t>(value & (value >> 1)));
		CHECK(Bmi::ps_and(value) == static_cast<std::uint8_t>(value & static_cast<std::uint8_t>(value << 1)));
		CHECK(Bmi::pp_andn(value) == static_cast<std::uint8_t>(value & static_cast<std::uint8_t>(~(value >> 1))));
		CHECK(Bmi::ps_andn(value) == static_cast<std::uint8_t>(value & static_cast<std::uint8_t>(~static_cast<std::uint8_t>(value << 1))));
		CHECK(Bmi::pp_andni(value) == static_cast<std::uint8_t>((value >> 1) & complement));
		CHECK(Bmi::ps_andni(value) == static_cast<std::uint8_t>(static_cast<std::uint8_t>(value << 1) & complement));

		std::uint8_t extracted_lsb = 0xA5;
		CHECK(Bmi::blse(value, extracted_lsb) == static_cast<std::uint8_t>(value ^ lsb));
		CHECK(extracted_lsb == lsb);
		CHECK(Bmi::blse(value) == std::tuple{static_cast<std::uint8_t>(value ^ lsb), lsb});
		CHECK(Bmi::bmsi(value) == msb);
		CHECK(Bmi::bmsr(value) == static_cast<std::uint8_t>(value ^ msb));
		int msb_index = 99;
		CHECK(Bmi::bmsr(value, msb_index) == static_cast<std::uint8_t>(value ^ msb));
		CHECK(msb_index == static_cast<int>(std::bit_width(value)) - 1);
		std::uint8_t extracted_msb = 0xA5;
		CHECK(Bmi::bmse(value, extracted_msb) == static_cast<std::uint8_t>(value ^ msb));
		CHECK(extracted_msb == msb);
		CHECK(Bmi::bmse(value) == std::tuple{static_cast<std::uint8_t>(value ^ msb), msb});
		CHECK(Bmi::bmsmsk(value) == prefix);

		CHECK(Bmi::flipr_unset(value) == static_cast<std::uint8_t>(value | add_one));
		CHECK(Bmi::maskr_unset(value) == static_cast<std::uint8_t>(complement & static_cast<std::uint8_t>(0u - complement)));
		CHECK(Bmi::maskl_trailing_one(value) == static_cast<std::uint8_t>((complement & static_cast<std::uint8_t>(0u - complement)) >> 1));
		CHECK(Bmi::clear_trailing_ones(value) == static_cast<std::uint8_t>(value & add_one));
		CHECK(Bmi::flip_trailing_zeros(value) == static_cast<std::uint8_t>(value | subtract_one));
		CHECK(Bmi::mask_trailing_zeros(value) == static_cast<std::uint8_t>(lsb - 1u));
		CHECK(Bmi::mask_trailing_zeros_or_zero(value) == (value == 0 ? 0 : static_cast<std::uint8_t>(lsb - 1u)));
		CHECK(Bmi::mask_bits_lower_than_lsb(value) == (value == 0 ? 0 : static_cast<std::uint8_t>(lsb - 1u)));
		CHECK(Bmi::mask_bits_lower_than_lsb_or_all_ones(value) == (value == 0 ? std::uint8_t{0xFF} : static_cast<std::uint8_t>(lsb - 1u)));
		CHECK(Bmi::mask_trailing_ones(value) == static_cast<std::uint8_t>((complement & static_cast<std::uint8_t>(0u - complement)) - 1u));
		CHECK(Bmi::mask_leading_zeros(value) == static_cast<std::uint8_t>(~prefix));
		const auto complement_prefix = complement == 0 ? std::uint8_t{0} : static_cast<std::uint8_t>((std::uint16_t{1} << std::bit_width(complement)) - 1);
		const auto leading_ones = static_cast<std::uint8_t>(~complement_prefix);
		CHECK(Bmi::mask_leading_ones(value) == leading_ones);
		CHECK(Bmi::clear_leading_ones(value) == static_cast<std::uint8_t>(value & static_cast<std::uint8_t>(~leading_ones)));
	}

	constexpr std::array<std::uint16_t, 10> boundaries{0, 1, 2, 3, 0x7F, 0x80, 0xFF, 0x8000, 0xFFFE, 0xFFFF};
	for (const std::uint16_t value : boundaries)
	{
		CHECK(Bmi::pp_or(value) == (value == 0 ? 0 : static_cast<std::uint16_t>((std::uint32_t{1} << std::bit_width(value)) - 1)));
		CHECK(Bmi::bmsi(value) == (value == 0 ? 0 : static_cast<std::uint16_t>(std::uint16_t{1} << (std::bit_width(value) - 1))));
	}
}

TEST_CASE("BMI sequence, partition, partial-sum, and left-deposit helpers retain their contracts", "[simdlib][bmi][derived][sequence]")
{
	constexpr std::array<std::tuple<std::uint32_t, std::uint32_t>, 8> values{{
		{0, 0},
		{1, 3},
		{0b1011, 0b0111},
		{0b10110, 0b01110},
		{0b0110111, 0b0001111},
		{0x8000'0000, 0x8000'0000},
		{0xFFFF'FFFE, 0xFFFF'FFFE},
		{0xFFFF'FFFF, 0xFFFF'FFFF},
	}};
	for (const auto &[value, expected_out_mask] : values)
	{
		std::uint32_t low_sequence = 0;
		if (value != 0)
		{
			std::uint32_t bit = value & (0u - value);
			while (bit != 0 && (value & bit) != 0)
			{
				low_sequence |= bit;
				bit <<= 1;
			}
		}
		std::uint32_t consumed_mask = 0xDEAD'BEEF;
		CHECK(Bmi::clear_lowest_set_bits(value) == (value & ~low_sequence));
		CHECK(Bmi::clear_lowest_set_bits(value, consumed_mask) == (value & ~low_sequence));
		CHECK(consumed_mask == expected_out_mask);
		CHECK(Bmi::consume_bit_sequence_right(value) == std::tuple{value & ~low_sequence, low_sequence});

		std::uint32_t high_sequence = 0;
		if (value != 0)
		{
			std::uint32_t bit = std::uint32_t{1} << (std::bit_width(value) - 1);
			while (bit != 0 && (value & bit) != 0)
			{
				high_sequence |= bit;
				bit >>= 1;
			}
		}
		CHECK(Bmi::consume_bit_sequence_left(value) == std::tuple{value & ~high_sequence, high_sequence});
		const std::uint32_t trailing_sequence = (value & 1u) == 0 ? 0 : low_sequence;
		CHECK(Bmi::left_collapse_trailing_bits(value) == (value & ~(trailing_sequence >> 1)));
	}

	CHECK(Bmi::blsioff(std::uint32_t{0b10100}, std::uint32_t{0b00001}) == 0b00100);
	CHECK(Bmi::blsioff(std::uint32_t{0b10100}, std::uint32_t{0b00100}) == 0b00100);
	CHECK(Bmi::blsioff(std::uint32_t{0b10100}, std::uint32_t{0b01000}) == 0b10000);
	CHECK(Bmi::bzlo(std::uint32_t{0b10111}, 0) == 0b10111);
	CHECK(Bmi::bzlo(std::uint32_t{0b10111}, 3) == 0b10000);
	CHECK(Bmi::bzlo(std::uint32_t{0b10111}, 32) == 0);
	CHECK(Bmi::PartialSumBLSI(std::uint32_t{0b1011}) == 24);
	CHECK(Bmi::PartialSumBLSI(std::uint32_t{0b1111}) == 32);
	CHECK(Bmi::PartialSumBLSMSK(std::uint32_t{0b1011}) == 37);
	CHECK(Bmi::PartialSumBLSMSK(std::uint32_t{0b1111}) == 49);

	constexpr std::uint32_t partition_value = 0b10111;
	for (const std::uint32_t target : {1u, 2u, 4u, 8u, 0x8000'0000u})
	{
		CHECK(Bmi::clear_bits_lower_than(partition_value, target) == (partition_value & ~(target - 1u)));
		CHECK(Bmi::clear_bits_higher_than(partition_value, target) == (partition_value & ((target << 1) - 1u)));
		CHECK(Bmi::extract_bits_lower_than(partition_value, target) == (partition_value & (target - 1u)));
		CHECK(Bmi::extract_bits_higher_than(partition_value, target) == (partition_value & ~((target << 1) - 1u)));
	}

	random64 random{0x4F1B'BCDC'6762'FA9DULL};
	for (unsigned iteration = 0; iteration < 4096; ++iteration)
	{
		const auto source32 = static_cast<std::uint32_t>(random.next());
		const auto mask32 = static_cast<std::uint32_t>(random.next());
		const std::uint64_t source64 = random.next();
		const std::uint64_t mask64 = random.next();
		CHECK(Bmi::pdepl_u32(source32, mask32) == reference_pdep(source32 >> (std::popcount(~mask32) & 31), mask32));
		CHECK(Bmi::pdepl_u64(source64, mask64) == reference_pdep(source64 >> (std::popcount(~mask64) & 63), mask64));
	}
}

TEST_CASE("BMI exhaustive 8-bit domains match scalar references", "[simdlib][bmi][exhaustive]")
{
	for (unsigned source = 0; source <= 0xFF; ++source)
	{
		const auto byte = static_cast<std::uint8_t>(source);
		CHECK(Bmi::blsi(byte) == static_cast<std::uint8_t>(byte & static_cast<std::uint8_t>(0u - byte)));
		CHECK(Bmi::blsr(byte) == static_cast<std::uint8_t>(byte & static_cast<std::uint8_t>(byte - 1u)));
		CHECK(Bmi::blsmsk(byte) == static_cast<std::uint8_t>(byte ^ static_cast<std::uint8_t>(byte - 1u)));
		for (unsigned index = 0; index <= 10; ++index)
			CHECK(Bmi::bzhi(byte, index) == reference_bzhi(byte, index));
		for (unsigned start = 0; start <= 10; ++start)
			for (unsigned len = 0; len <= 10; ++len)
				CHECK(Bmi::bextr(byte, static_cast<std::uint8_t>(len), static_cast<std::uint8_t>(start)) == reference_bextr(byte, start, len));

		for (unsigned mask = 0; mask <= 0xFF; ++mask)
		{
			const auto mask_byte = static_cast<std::uint8_t>(mask);
			CHECK(Bmi::andn(byte, mask_byte) == static_cast<std::uint8_t>(mask_byte & static_cast<std::uint8_t>(~byte)));
			CHECK(Bmi::pdep_u32(source, mask) == reference_pdep<std::uint32_t>(source, mask));
			CHECK(Bmi::pext_u32(source, mask) == reference_pext<std::uint32_t>(source, mask));
		}
	}
}

TEST_CASE("BMI exhaustive 16-bit unary domains and boundary indices match scalar references", "[simdlib][bmi][exhaustive]")
{
	constexpr std::array<unsigned, 7> indices{0, 1, 7, 8, 15, 16, 17};
	for (unsigned source = 0; source <= 0xFFFF; ++source)
	{
		const auto word = static_cast<std::uint16_t>(source);
		CHECK(Bmi::blsi(word) == static_cast<std::uint16_t>(word & static_cast<std::uint16_t>(0u - word)));
		CHECK(Bmi::blsr(word) == static_cast<std::uint16_t>(word & static_cast<std::uint16_t>(word - 1u)));
		CHECK(Bmi::blsmsk(word) == static_cast<std::uint16_t>(word ^ static_cast<std::uint16_t>(word - 1u)));
		for (const unsigned index : indices)
			CHECK(Bmi::bzhi(word, index) == reference_bzhi(word, index));
	}
}

TEST_CASE("BMI randomized 32-bit operations match scalar references", "[simdlib][bmi][random][u32]")
{
	random64 random{0xC001D00D12345678ULL};
	for (unsigned iteration = 0; iteration < 4096; ++iteration)
	{
		const auto lhs = static_cast<std::uint32_t>(random.next());
		const auto rhs = static_cast<std::uint32_t>(random.next());
		const unsigned index = static_cast<unsigned>(random.next() % 41);
		const unsigned start = static_cast<unsigned>(random.next() % 41);
		const unsigned len = static_cast<unsigned>(random.next() % 41);
		CHECK(Bmi::andn(lhs, rhs) == (rhs & ~lhs));
		CHECK(Bmi::bzhi(lhs, index) == reference_bzhi(lhs, index));
		CHECK(Bmi::blsi(lhs) == (lhs & (0u - lhs)));
		CHECK(Bmi::blsr(lhs) == (lhs & (lhs - 1u)));
		CHECK(Bmi::blsmsk(lhs) == (lhs ^ (lhs - 1u)));
		CHECK(Bmi::bextr(lhs, static_cast<std::uint8_t>(len), static_cast<std::uint8_t>(start)) == reference_bextr(lhs, start, len));
		CHECK(Bmi::pdep_u32(lhs, rhs) == reference_pdep(lhs, rhs));
		CHECK(Bmi::pext_u32(lhs, rhs) == reference_pext(lhs, rhs));
		std::uint32_t high = 0;
		const std::uint32_t low = Bmi::mulx(lhs, rhs, high);
		const std::uint64_t product = static_cast<std::uint64_t>(lhs) * rhs;
		CHECK(low == static_cast<std::uint32_t>(product));
		CHECK(high == static_cast<std::uint32_t>(product >> 32));
	}
}

TEST_CASE("BMI randomized 64-bit operations match scalar references", "[simdlib][bmi][random][u64]")
{
	random64 random{0x9E3779B97F4A7C15ULL};
	for (unsigned iteration = 0; iteration < 4096; ++iteration)
	{
		const std::uint64_t lhs = random.next();
		const std::uint64_t rhs = random.next();
		const unsigned index = static_cast<unsigned>(random.next() % 73);
		const unsigned start = static_cast<unsigned>(random.next() % 73);
		const unsigned len = static_cast<unsigned>(random.next() % 73);
		CHECK(Bmi::andn(lhs, rhs) == (rhs & ~lhs));
		CHECK(Bmi::bzhi(lhs, index) == reference_bzhi(lhs, index));
		CHECK(Bmi::blsi(lhs) == (lhs & (std::uint64_t{0} - lhs)));
		CHECK(Bmi::blsr(lhs) == (lhs & (lhs - 1)));
		CHECK(Bmi::blsmsk(lhs) == (lhs ^ (lhs - 1)));
		CHECK(Bmi::bextr(lhs, static_cast<std::uint8_t>(len), static_cast<std::uint8_t>(start)) == reference_bextr(lhs, start, len));
		CHECK(Bmi::pdep_u64(lhs, rhs) == reference_pdep(lhs, rhs));
		CHECK(Bmi::pext_u64(lhs, rhs) == reference_pext(lhs, rhs));
		std::uint64_t high = 0;
		const std::uint64_t low = Bmi::mulx(lhs, rhs, high);
		const product128 expected = reference_mulx64(lhs, rhs);
		CHECK(low == expected.low);
		CHECK(high == expected.high);
	}
}

TEST_CASE("BMI generic boundary supports root uint128_t without an include cycle", "[simdlib][bmi][random][u128]")
{
	random64 random{0xD1B54A32D192ED03ULL};
	for (unsigned iteration = 0; iteration < 4096; ++iteration)
	{
		const uint128_t lhs{random.next(), random.next()};
		const uint128_t rhs{random.next(), random.next()};
		const unsigned index = static_cast<unsigned>(random.next() % 145);
		CHECK(Bmi::andn(lhs, rhs) == uint128_t{rhs.low() & ~lhs.low(), rhs.high() & ~lhs.high()});
		CHECK(Bmi::bzhi(lhs, index) == reference_bzhi128(lhs, index));
		CHECK(Bmi::blsi(lhs) == reference_blsi128(lhs));
		CHECK(Bmi::blsr(lhs) == reference_blsr128(lhs));
		CHECK(Bmi::blsmsk(lhs) == reference_blsmsk128(lhs));
	}
}

TEST_CASE("BMI feature paths produce the scalar-reference result digest", "[simdlib][bmi][digest]")
{
	random64 random{0xA0761D6478BD642FULL};
	std::uint64_t actual_digest = 1469598103934665603ULL;
	std::uint64_t reference_digest = 1469598103934665603ULL;
	for (unsigned iteration = 0; iteration < 2048; ++iteration)
	{
		const std::uint64_t lhs = random.next();
		const std::uint64_t rhs = random.next();
		const unsigned index = static_cast<unsigned>(random.next() % 137);
		mix_digest(actual_digest, Bmi::andn(lhs, rhs));
		mix_digest(actual_digest, Bmi::bzhi(lhs, index));
		mix_digest(actual_digest, Bmi::blsi(lhs));
		mix_digest(actual_digest, Bmi::blsr(lhs));
		mix_digest(actual_digest, Bmi::blsmsk(lhs));
		mix_digest(actual_digest, Bmi::pdep_u64(lhs, rhs));
		mix_digest(actual_digest, Bmi::pdepl_u64(lhs, rhs));
		mix_digest(actual_digest, Bmi::pext_u64(lhs, rhs));
		mix_digest(actual_digest, Bmi::pp_or(lhs));
		mix_digest(actual_digest, Bmi::ps_or(lhs));
		mix_digest(actual_digest, Bmi::clear_lowest_set_bits(lhs));
		mix_digest(actual_digest, std::get<0>(Bmi::consume_bit_sequence_right(lhs)));
		mix_digest(actual_digest, std::get<1>(Bmi::consume_bit_sequence_left(lhs)));

		mix_digest(reference_digest, rhs & ~lhs);
		mix_digest(reference_digest, reference_bzhi(lhs, index));
		mix_digest(reference_digest, lhs & (std::uint64_t{0} - lhs));
		mix_digest(reference_digest, lhs & (lhs - 1));
		mix_digest(reference_digest, lhs ^ (lhs - 1));
		mix_digest(reference_digest, reference_pdep(lhs, rhs));
		mix_digest(reference_digest, reference_pdep(lhs >> (std::popcount(~rhs) & 63), rhs));
		mix_digest(reference_digest, reference_pext(lhs, rhs));
		mix_digest(reference_digest, Bmi::pp_or(lhs));
		mix_digest(reference_digest, Bmi::ps_or(lhs));
		mix_digest(reference_digest, Bmi::clear_lowest_set_bits(lhs));
		mix_digest(reference_digest, std::get<0>(Bmi::consume_bit_sequence_right(lhs)));
		mix_digest(reference_digest, std::get<1>(Bmi::consume_bit_sequence_left(lhs)));
	}
	CHECK(actual_digest == reference_digest);
	std::cout << "SIMDLIB_BMI_RESULT_DIGEST=" << std::hex << actual_digest << '\n';
}

TEST_CASE("BMI documentation examples produce their documented results", "[simdlib][bmi][documentation]")
{
	CHECK(Bmi::abs(-7) == 7);
	CHECK(Bmi::andn(0b1100U, 0b1010U) == 0b0010U);
	CHECK(Bmi::bextr(0b1101'0110U, 3, 2) == 0b101U);
	CHECK(Bmi::blse(0b10100U) == std::tuple{0b10000U, 0b00100U});
	CHECK(Bmi::blsi(0b10100U) == 0b00100U);
	CHECK(Bmi::blsioff(0b10100, 0b01000) == 0b10000);
	CHECK(Bmi::blsmsk(0b10100U) == 0b00111U);
	CHECK(Bmi::blsr(0b10100U) == 0b10000U);
	CHECK(Bmi::bmse(0b10100U) == std::tuple{0b00100U, 0b10000U});
	CHECK(Bmi::bmsi(0b10100U) == 0b10000U);
	CHECK(Bmi::bmsmsk(0b10100U) == 0b11111U);
	CHECK(Bmi::bmsr(0b10100U) == 0b00100U);
	CHECK(Bmi::boolmask<std::uint32_t>(true) == 0xFFFFFFFFU);
	CHECK(Bmi::bzhi(0b1111'0110U, 4U) == 0b0000'0110U);
	CHECK(Bmi::bzlo(0b11111, 3) == 0b11000);
	CHECK(Bmi::clear_bits_higher_than(0b10111, 0b100) == 0b00111);
	CHECK(Bmi::clear_bits_lower_than(0b10111, 0b100) == 0b10100);
	CHECK(Bmi::clear_leading_ones<std::uint8_t>(0b1101'0101U) == 0b0001'0101U);
	CHECK(Bmi::clear_lowest_set_bits(0b1011) == 0b1000);
	CHECK(Bmi::clear_trailing_ones(0b1011) == 0b1000);
	CHECK(Bmi::consume_bit_sequence_left(0b0110'111U) == std::tuple{0b0000'111U, 0b0110'000U});
	CHECK(Bmi::consume_bit_sequence_right(0b1011U) == std::tuple{0b1000U, 0b0011U});
	CHECK(Bmi::extract_bits_higher_than(0b10111, 0b001) == 0b10110);
	CHECK(Bmi::extract_bits_lower_than(0b10111, 0b100) == 0b00011);
	CHECK(Bmi::flip_trailing_zeros(0b10100) == 0b10111);
	CHECK(Bmi::flipr_unset(0b01011) == 0b01111);
	CHECK(Bmi::left_collapse_trailing_bits(0b10111) == 0b10100);
	CHECK(Bmi::mask_bits_lower_than_lsb(0b101000) == 0b000111);
	CHECK(Bmi::mask_bits_lower_than_lsb_or_all_ones(0b101000) == 0b000111);
	CHECK(Bmi::mask_leading_ones<std::uint8_t>(0b1110'1011U) == 0b1110'0000U);
	CHECK(Bmi::mask_leading_zeros<std::uint8_t>(0b0001'0101U) == 0b1110'0000U);
	CHECK(Bmi::mask_trailing_ones(0b10111) == 0b00111);
	CHECK(Bmi::mask_trailing_zeros(0b10100) == 0b011);
	CHECK(Bmi::mask_trailing_zeros_or_zero(0b10100) == 0b00011);
	CHECK(Bmi::maskl_trailing_one(0b010111) == 0b00100);
	CHECK(Bmi::maskr_unset(0b01011) == 0b00100);
	CHECK(Bmi::max(4, 9) == 9);
	CHECK(Bmi::min(4, 9) == 4);
	std::uint8_t high = 0;
	CHECK(Bmi::mulx<std::uint8_t>(0xFF, 0x02, high) == 0xFE);
	CHECK(high == 0x01);
	CHECK(Bmi::PartialSumBLSI<std::uint32_t>(0b1011U) == 24U);
	CHECK(Bmi::PartialSumBLSMSK<std::uint32_t>(0b1011U) == 37U);
	CHECK(Bmi::pdep_u32(0b101U, 0b0101'0100U) == 0b0100'0100U);
	CHECK(Bmi::pdep_u64(0b101ULL, 0b0101'0100ULL) == 0b0100'0100ULL);
	CHECK(Bmi::pdepl_u32(0xA000'0000U, 0b0101'0100U) == 0b0100'0100U);
	CHECK(Bmi::pdepl_u64(0xA000'0000'0000'0000ULL, 0b0101'0100ULL) == 0b0100'0100ULL);
	CHECK(Bmi::pext_u32(0b0100'0100U, 0b0101'0100U) == 0b101U);
	CHECK(Bmi::pext_u64(0b0100'0100ULL, 0b0101'0100ULL) == 0b101ULL);
	CHECK(Bmi::pp_and(0b01101110) == 0b00100110);
	CHECK(Bmi::pp_andn(0b01110) == 0b01000);
	CHECK(Bmi::pp_andni(0b01110) == 0b00001);
	CHECK(Bmi::pp_lsor(SimdLib::uint128_t{0b10100}) == SimdLib::uint128_t{0b00111});
	CHECK(Bmi::pp_or(SimdLib::uint128_t{0b10100}) == SimdLib::uint128_t{0b11111});
	CHECK(Bmi::pp_xor(0b01110) == 0b01001);
	CHECK(Bmi::ps_and(0b01101110) == 0b01001100);
	CHECK(Bmi::ps_andn(0b01110) == 0b00010);
	CHECK(Bmi::ps_andni(0b01110) == 0b10000);
	CHECK(Bmi::ps_or<std::uint8_t>(0b0001'0100U) == 0b1111'1100U);
	CHECK(Bmi::ps_xor(0b01110) == 0b10010);
	CHECK(Bmi::select(4U, 9U, true) == 9U);
}
