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
} // namespace

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
		mix_digest(actual_digest, Bmi::pext_u64(lhs, rhs));

		mix_digest(reference_digest, rhs & ~lhs);
		mix_digest(reference_digest, reference_bzhi(lhs, index));
		mix_digest(reference_digest, lhs & (std::uint64_t{0} - lhs));
		mix_digest(reference_digest, lhs & (lhs - 1));
		mix_digest(reference_digest, lhs ^ (lhs - 1));
		mix_digest(reference_digest, reference_pdep(lhs, rhs));
		mix_digest(reference_digest, reference_pext(lhs, rhs));
	}
	CHECK(actual_digest == reference_digest);
	std::cout << "SIMDLIB_BMI_RESULT_DIGEST=" << std::hex << actual_digest << '\n';
}
