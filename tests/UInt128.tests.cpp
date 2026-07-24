#include <SimdLib/UInt128.h>
#ifndef SIMDLIB_EXPECT_CARRY_PATH
#define SIMDLIB_EXPECT_CARRY_PATH -1
#endif

#if SIMDLIB_EXPECT_CARRY_PATH == 1
#if !SIMDLIB_USE_COMPILER_CARRY_INTRINSICS || !SIMDLIB_COMPILER_MSVC || !defined(_M_X64)
#error "The MSVC carry-path profile must select _addcarry_u64 and _subborrow_u64."
#endif
#elif SIMDLIB_EXPECT_CARRY_PATH == 2
#if !SIMDLIB_USE_COMPILER_CARRY_INTRINSICS || (!SIMDLIB_COMPILER_CLANG && !SIMDLIB_COMPILER_GCC)
#error "The compiler-builtin carry-path profile must select overflow builtins."
#endif
#elif SIMDLIB_EXPECT_CARRY_PATH == 0
#if SIMDLIB_USE_COMPILER_CARRY_INTRINSICS
#error "The portable carry-path profile must disable compiler carry intrinsics."
#endif
#endif

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <bit>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <type_traits>

namespace
{
using SimdLib::uint128_t;

struct words128 final
{
	std::uint64_t low = 0;
	std::uint64_t high = 0;

	friend constexpr bool operator==(const words128 &, const words128 &) noexcept = default;
};

/** @brief Describes one heterogeneous signed-integral comparison contract. */
struct heterogeneous_comparison_case final
{
	uint128_t lhs;
	std::int64_t rhs;
	bool equal;
	std::strong_ordering ordering;
};

/** @brief Describes one dynamic extraction boundary and its exact result. */
struct extraction_case final
{
	std::uint8_t length;
	std::uint8_t start;
	uint128_t expected;
};

/** @brief Describes one bit-ceiling boundary and its exact result. */
struct bit_ceil_case final
{
	uint128_t value;
	uint128_t expected;
};

[[nodiscard]] constexpr words128 words(const uint128_t value) noexcept
{
	return {value.low(), value.high()};
}

/**
 * @brief Calls the deprecated dynamic extraction compatibility API.
 * @param value Source value.
 * @param length Requested bit count.
 * @param start First source bit.
 * @return The extracted and low-aligned value.
 */
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#elif defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
[[nodiscard]] uint128_t deprecated_extract(const uint128_t value, const std::uint8_t length, const std::uint8_t start) noexcept
{
	return value.extract(length, start);
}
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/**
 * @brief Calls a fixed-width mask specialization with a runtime offset.
 * @tparam Width Compile-time mask width.
 * @param offset Runtime bit offset.
 * @return The shifted and truncated mask.
 */
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
template <int Width> [[nodiscard]] uint128_t runtime_create_mask(const int offset) noexcept
{
	return uint128_t::create_mask<Width>(offset);
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

[[nodiscard]] constexpr words128 add_words(const words128 lhs, const words128 rhs) noexcept
{
	const std::uint64_t low = lhs.low + rhs.low;
	return {low, lhs.high + rhs.high + static_cast<std::uint64_t>(low < lhs.low)};
}

[[nodiscard]] constexpr words128 subtract_words(const words128 lhs, const words128 rhs) noexcept
{
	return {lhs.low - rhs.low, lhs.high - rhs.high - static_cast<std::uint64_t>(lhs.low < rhs.low)};
}

[[nodiscard]] constexpr words128 shift_left_words(const words128 value, const unsigned count) noexcept
{
	if (count == 0)
		return value;
	if (count >= 128)
		return {};
	if (count >= 64)
		return {0, value.low << (count - 64)};
	return {value.low << count, (value.high << count) | (value.low >> (64 - count))};
}

[[nodiscard]] constexpr words128 shift_right_words(const words128 value, const unsigned count) noexcept
{
	if (count == 0)
		return value;
	if (count >= 128)
		return {};
	if (count >= 64)
		return {value.high >> (count - 64), 0};
	return {(value.low >> count) | (value.high << (64 - count)), value.high >> count};
}

[[nodiscard]] constexpr int compare_words(const words128 lhs, const words128 rhs) noexcept
{
	if (lhs.high != rhs.high)
		return lhs.high < rhs.high ? -1 : 1;
	if (lhs.low != rhs.low)
		return lhs.low < rhs.low ? -1 : 1;
	return 0;
}

struct random64 final
{
	std::uint64_t state;

	[[nodiscard]] std::uint64_t next() noexcept
	{
		state ^= state >> 12;
		state ^= state << 25;
		state ^= state >> 27;
		return state * 2685821657736338717ULL;
	}
};

constexpr bool constexpr_contract() noexcept
{
	using namespace SimdLib;
	using namespace SimdLib::Bmi;
	const uint128_t lhs{0xFEDC'BA98'7654'3210ULL, 0x0123'4567'89AB'CDEFULL};
	const uint128_t rhs{0x1111'2222'3333'4444ULL, 0x5555'6666'7777'8888ULL};

	if (words(lhs + rhs) != add_words(words(lhs), words(rhs)))
		return false;
	if (words(lhs - rhs) != subtract_words(words(lhs), words(rhs)))
		return false;
	uint128_t compound = lhs;
	compound += rhs;
	if (compound != lhs + rhs)
		return false;
	compound -= rhs;
	if (compound != lhs)
		return false;
	if ((lhs & rhs) != uint128_t{lhs.low() & rhs.low(), lhs.high() & rhs.high()})
		return false;
	if ((lhs | rhs) != uint128_t{lhs.low() | rhs.low(), lhs.high() | rhs.high()})
		return false;
	if ((lhs ^ rhs) != uint128_t{lhs.low() ^ rhs.low(), lhs.high() ^ rhs.high()})
		return false;
	if ((~lhs) != uint128_t{~lhs.low(), ~lhs.high()})
		return false;
	compound = lhs;
	compound &= rhs;
	compound |= uint128_t{0x10, 0x20};
	compound ^= uint128_t{0x01, 0x02};
	if (compound != (((lhs & rhs) | uint128_t{0x10, 0x20}) ^ uint128_t{0x01, 0x02}))
		return false;

	constexpr std::array<unsigned, 10> shifts{0, 1, 63, 64, 65, 127, 128, 129, 255, 256};
	for (const unsigned shift : shifts)
	{
		if (words(lhs << shift) != shift_left_words(words(lhs), shift))
			return false;
		if (words(lhs >> shift) != shift_right_words(words(lhs), shift))
			return false;
		compound = lhs;
		compound <<= shift;
		if (compound != lhs << shift)
			return false;
		compound = lhs;
		compound >>= shift;
		if (compound != lhs >> shift)
			return false;
	}
	if ((lhs << -4) != lhs || (lhs >> -4) != lhs)
		return false;

	compound = uint128_t{std::numeric_limits<std::uint64_t>::max(), 7};
	const uint128_t beforeIncrement = compound++;
	if (beforeIncrement != uint128_t{std::numeric_limits<std::uint64_t>::max(), 7} || compound != uint128_t{0, 8})
		return false;
	const uint128_t beforeDecrement = compound--;
	if (beforeDecrement != uint128_t{0, 8} || compound != uint128_t{std::numeric_limits<std::uint64_t>::max(), 7})
		return false;
	if (++compound != uint128_t{0, 8})
		return false;
	if (--compound != uint128_t{std::numeric_limits<std::uint64_t>::max(), 7})
		return false;
	if (-uint128_t{1} != std::numeric_limits<uint128_t>::max())
		return false;
	if (lhs.abs_diff(rhs) != (lhs > rhs ? lhs - rhs : rhs - lhs))
		return false;

	if (!(lhs < rhs) || !(uint128_t{7} == 7u) || !(uint128_t{7} > -1))
		return false;
	if (static_cast<std::uint32_t>(lhs) != 0x7654'3210U)
		return false;
	if (!static_cast<bool>(lhs) || static_cast<bool>(uint128_t{}))
		return false;
	if (lhs.getBlock(0) != lhs.low() || lhs.getBlock(1) != lhs.high())
		return false;

	if (popcount(lhs) != std::popcount(lhs.low()) + std::popcount(lhs.high()))
		return false;
	if (countr_zero(uint128_t{}) != 128 || countl_zero(uint128_t{}) != 128)
		return false;
	if (countr_one(std::numeric_limits<uint128_t>::max()) != 128)
		return false;
	if (countl_one(std::numeric_limits<uint128_t>::max()) != 128)
		return false;
	if (bit_width(uint128_t{0, 1}) != 65)
		return false;
	if (bit_floor(uint128_t{0, 3}) != uint128_t{0, 2})
		return false;
	if (bit_ceil(uint128_t{0, 3}) != uint128_t{0, 4})
		return false;
	if (!has_single_bit(uint128_t{0, 8}) || has_single_bit(uint128_t{3}))
		return false;

	if (uint128_t::create_mask(0) != uint128_t{})
		return false;
	if (uint128_t::create_mask(64) != uint128_t{~std::uint64_t{0}, 0})
		return false;
	if (uint128_t::create_mask(128) != std::numeric_limits<uint128_t>::max())
		return false;
	if (uint128_t::create_mask<5>(62) != (uint128_t::create_mask(5) << 62))
		return false;
	if (Bmi::blsi(lhs) != (lhs & -lhs))
		return false;
	if (Bmi::blsr(lhs) != (lhs & (lhs - uint128_t{1})))
		return false;
	if (Bmi::blsmsk(lhs) != (lhs ^ (lhs - uint128_t{1})))
		return false;
	if (Bmi::bzhi(lhs, 65) != (lhs & uint128_t::create_mask(65)))
		return false;
	if (Bmi::andn(lhs, rhs) != (~lhs & rhs))
		return false;
	if (Bmi::bextr(lhs, 17, 61) != ((lhs >> 61) & uint128_t::create_mask(17)))
		return false;

	if (42_u128 != uint128_t{42})
		return false;
	if (std::hash<uint128_t>{}(lhs) != static_cast<std::size_t>(lhs.low() ^ lhs.high()))
		return false;
	static_assert(std::numeric_limits<uint128_t>::digits == 128 && std::numeric_limits<uint128_t>::is_modulo);
	return true;
}

static_assert(constexpr_contract());
static_assert(sizeof(uint128_t) == 16);
static_assert(alignof(uint128_t) == 16);
static_assert(std::is_standard_layout_v<uint128_t>);
static_assert(std::is_trivially_copyable_v<uint128_t>);

struct operation_snapshot final
{
	std::array<uint128_t, 30> values{};
	std::array<int, 7> counts{};
	std::array<bool, 6> predicates{};
	std::uint64_t narrowed = 0;
	std::size_t hash = 0;

	friend constexpr bool operator==(const operation_snapshot &, const operation_snapshot &) noexcept = default;
};

[[nodiscard]] constexpr operation_snapshot snapshot(uint128_t lhs, const uint128_t rhs) noexcept
{
	using namespace SimdLib;
	uint128_t addAssign = lhs;
	addAssign += rhs;
	uint128_t subtractAssign = lhs;
	subtractAssign -= rhs;
	uint128_t andAssign = lhs;
	andAssign &= rhs;
	uint128_t orAssign = lhs;
	orAssign |= rhs;
	uint128_t xorAssign = lhs;
	xorAssign ^= rhs;
	uint128_t preIncrement = lhs;
	++preIncrement;
	uint128_t preDecrement = lhs;
	--preDecrement;
	uint128_t postIncrement = lhs;
	const uint128_t postIncrementResult = postIncrement++;
	uint128_t postDecrement = lhs;
	const uint128_t postDecrementResult = postDecrement--;

	return {
		{
			lhs + rhs,
			lhs - rhs,
			addAssign,
			subtractAssign,
			lhs & rhs,
			lhs | rhs,
			lhs ^ rhs,
			~lhs,
			andAssign,
			orAssign,
			xorAssign,
			lhs << 0,
			lhs << 1,
			lhs << 63,
			lhs << 64,
			lhs << 127,
			lhs << 128,
			lhs >> 1,
			lhs >> 64,
			lhs >> 127,
			-lhs,
			lhs.abs_diff(rhs),
			preIncrement,
			preDecrement,
			postIncrementResult,
			postIncrement,
			postDecrementResult,
			postDecrement,
			Bmi::bextr(lhs, 29, 57),
			uint128_t::create_mask<19>(61),
		},
		{
			popcount(lhs),
			countr_zero(lhs),
			countr_one(lhs),
			countl_zero(lhs),
			countl_one(lhs),
			bit_width(lhs),
			std::numeric_limits<uint128_t>::digits,
		},
		{
			lhs == rhs,
			lhs<rhs, lhs>
				rhs,
			static_cast<bool>(lhs),
			has_single_bit(lhs),
			std::numeric_limits<uint128_t>::is_modulo,
		},
		static_cast<std::uint64_t>(lhs),
		std::hash<uint128_t>{}(lhs),
	};
}

constexpr uint128_t snapshot_lhs{0xFEDC'BA98'7654'3210ULL, 0x0123'4567'89AB'CDEFULL};
constexpr uint128_t snapshot_rhs{0x1111'2222'3333'4444ULL, 0x5555'6666'7777'8888ULL};
constexpr operation_snapshot constant_snapshot = snapshot(snapshot_lhs, snapshot_rhs);
static_assert(constant_snapshot == snapshot(snapshot_lhs, snapshot_rhs));

void mix_digest(std::uint64_t &digest, const uint128_t value) noexcept
{
	digest ^= value.low();
	digest *= 1099511628211ULL;
	digest ^= value.high();
	digest *= 1099511628211ULL;
}

#if defined(__SIZEOF_INT128__)
__extension__ typedef unsigned __int128 native_uint128;

[[nodiscard]] constexpr native_uint128 native(const uint128_t value) noexcept
{
	return (static_cast<native_uint128>(value.high()) << 64) | value.low();
}

[[nodiscard]] constexpr uint128_t library(const native_uint128 value) noexcept
{
	return {static_cast<std::uint64_t>(value), static_cast<std::uint64_t>(value >> 64)};
}
#endif
} // namespace

TEST_CASE("uint128 selected carry and borrow implementation executes with volatile inputs", "[simdlib][uint128][carry][compiler-path]")
{
	volatile std::uint64_t lhsLow = 0xFFFF'FFFF'FFFF'FFFFULL;
	volatile std::uint64_t lhsHigh = 7;
	volatile std::uint64_t rhsLow = 1;
	volatile std::uint64_t rhsHigh = 3;
	const uint128_t lhs{lhsLow, lhsHigh};
	const uint128_t rhs{rhsLow, rhsHigh};
	CHECK(lhs + rhs == uint128_t{0, 11});
	CHECK(lhs - rhs == uint128_t{0xFFFF'FFFF'FFFF'FFFEULL, 4});
}
TEST_CASE("uint128 carry and borrow propagation matches the two-word oracle", "[simdlib][uint128][carry]")
{
	constexpr std::array<std::uint64_t, 8> values{
		0, 1, 2, 0x7FFF'FFFF'FFFF'FFFFULL, 0x8000'0000'0000'0000ULL, 0xFFFF'FFFF'FFFF'FFFEULL, 0xFFFF'FFFF'FFFF'FFFFULL, 0xA5A5'5A5A'1234'FEDCULL};
	for (const auto lhs : values)
	{
		for (const auto rhs : values)
		{
			const uint128_t left{lhs, 0x1234'5678'9ABC'DEF0ULL};
			const uint128_t right{rhs, 0x0FED'CBA9'8765'4321ULL};
			const words128 expectedSum = add_words(words(left), words(right));
			const words128 expectedDifference = subtract_words(words(left), words(right));
			CHECK(words(left + right) == expectedSum);
			CHECK(words(left - right) == expectedDifference);
		}
	}
}

TEST_CASE("uint128 register facade preserves lane order", "[simdlib][uint128][simd]")
{
#if SIMDLIB_HAS_SSE42
	const uint128_t value{0x0123'4567'89AB'CDEFULL, 0xFEDC'BA98'7654'3210ULL};
	const auto registerValue = value.to_register();
	CHECK(uint128_t::from_register(registerValue) == value);
	CHECK((SimdLib::Api<128, std::uint64_t>::extract<0>(registerValue) == value.low()));
	CHECK((SimdLib::Api<128, std::uint64_t>::extract<1>(registerValue) == value.high()));
#else
	SUCCEED("The scalar profile intentionally has no SIMD register facade");
#endif
}

TEST_CASE("uint128 integral construction and heterogeneous comparisons are explicit", "[simdlib][uint128][comparison]")
{
	const uint128_t fromNegative{std::int64_t{-1}};
	CHECK(fromNegative.low() == std::numeric_limits<std::uint64_t>::max());
	CHECK(fromNegative.high() == 0);
	CHECK_FALSE(fromNegative == std::int64_t{-1});
	CHECK((fromNegative <=> std::int64_t{-1}) == std::strong_ordering::greater);

	const std::array cases{
		heterogeneous_comparison_case{uint128_t{}, -1, false, std::strong_ordering::greater},
		heterogeneous_comparison_case{uint128_t{}, 0, true, std::strong_ordering::equal},
		heterogeneous_comparison_case{uint128_t{41}, 42, false, std::strong_ordering::less},
		heterogeneous_comparison_case{uint128_t{42}, 42, true, std::strong_ordering::equal},
		heterogeneous_comparison_case{uint128_t{43}, 42, false, std::strong_ordering::greater},
		heterogeneous_comparison_case{uint128_t{0, 1}, std::numeric_limits<std::int64_t>::max(), false, std::strong_ordering::greater},
	};
	for (const auto &test : cases)
	{
		volatile std::uint64_t low = test.lhs.low();
		volatile std::uint64_t high = test.lhs.high();
		volatile std::int64_t rhs = test.rhs;
		const uint128_t lhs{low, high};
		CAPTURE(lhs.low(), lhs.high(), rhs);
		CHECK((lhs == rhs) == test.equal);
		CHECK((lhs <=> rhs) == test.ordering);
	}

	CHECK(uint128_t{42} == std::int32_t{42});
	CHECK(uint128_t{42} == std::uint64_t{42});
	CHECK_FALSE(uint128_t{43} == std::uint64_t{42});
	CHECK((uint128_t{41} <=> std::uint64_t{42}) == std::strong_ordering::less);
	CHECK((uint128_t{42} <=> std::uint64_t{42}) == std::strong_ordering::equal);
	CHECK((uint128_t{0, 1} <=> std::numeric_limits<std::uint64_t>::max()) == std::strong_ordering::greater);
	CHECK(uint128_t{true} == uint128_t{1});
	CHECK(uint128_t{false} == uint128_t{});
}

TEST_CASE("uint128 deprecated extraction remains compatible with Bmi bextr at boundaries", "[simdlib][uint128][extract][compatibility]")
{
	volatile std::uint64_t sourceLow = 0x0123'4567'89AB'CDEFULL;
	volatile std::uint64_t sourceHigh = 0xFEDC'BA98'7654'3210ULL;
	const uint128_t source{sourceLow, sourceHigh};
	const std::array cases{
		extraction_case{0, 0, {}},	 extraction_case{1, 127, uint128_t{1}},		extraction_case{1, 128, {}},
		extraction_case{8, 200, {}}, extraction_case{12, 60, uint128_t{0x100}}, extraction_case{16, 120, uint128_t{0xFE}},
	};
	for (const auto &test : cases)
	{
		volatile std::uint8_t length = test.length;
		volatile std::uint8_t start = test.start;
		CAPTURE(length, start);
		CHECK(deprecated_extract(source, length, start) == test.expected);
		CHECK(SimdLib::Bmi::bextr(source, length, start) == test.expected);
	}
}

TEST_CASE("uint128 masks and bit helpers cover word boundaries", "[simdlib][uint128][bits]")
{
	const std::array<int, 11> widths{-1, 0, 1, 63, 64, 65, 96, 127, 128, 129, 255};
	for (const int width : widths)
	{
		const uint128_t mask = uint128_t::create_mask(width);
		const unsigned clamped = width <= 0 ? 0u : width >= 128 ? 128u : static_cast<unsigned>(width);
		CHECK(SimdLib::popcount(mask) == static_cast<int>(clamped));
		if (clamped == 0)
			CHECK(mask == uint128_t{});
		else
		{
			CHECK(SimdLib::countr_one(mask) == static_cast<int>(clamped));
			CHECK(SimdLib::bit_width(mask) == static_cast<int>(clamped));
		}
	}

	CHECK((uint128_t::create_mask<1>(63) == uint128_t{std::uint64_t{1} << 63, 0}));
	CHECK((uint128_t::create_mask<1>(64) == uint128_t{0, 1}));
	CHECK((uint128_t::create_mask<64>(64) == uint128_t{0, std::numeric_limits<std::uint64_t>::max()}));
	CHECK((uint128_t::create_mask<65>(63) == uint128_t{std::uint64_t{1} << 63, std::numeric_limits<std::uint64_t>::max()}));
	CHECK(uint128_t::create_mask<128>(1) == uint128_t{~std::uint64_t{0} << 1, ~std::uint64_t{0}});
	volatile int negativeOffset = -7;
	volatile int zeroOffset = 0;
	volatile int lowBoundaryOffset = 63;
	volatile int wordBoundaryOffset = 64;
	volatile int finalBitOffset = 127;
	volatile int widthOffset = 128;
	volatile int oversizedOffset = 129;
	CHECK(runtime_create_mask<5>(negativeOffset) == uint128_t::create_mask(5));
	CHECK(runtime_create_mask<5>(zeroOffset) == uint128_t::create_mask(5));
	CHECK(runtime_create_mask<5>(lowBoundaryOffset) == (uint128_t::create_mask(5) << 63));
	CHECK(runtime_create_mask<5>(wordBoundaryOffset) == uint128_t{0, 0x1F});
	CHECK(runtime_create_mask<5>(finalBitOffset) == uint128_t{0, std::uint64_t{1} << 63});
	CHECK(runtime_create_mask<5>(widthOffset) == uint128_t{});
	CHECK(runtime_create_mask<5>(oversizedOffset) == uint128_t{});
}

TEST_CASE("uint128 shifts define every boundary count", "[simdlib][uint128][shift]")
{
	volatile std::uint64_t valueLow = 0x0123'4567'89AB'CDEFULL;
	volatile std::uint64_t valueHigh = 0xFEDC'BA98'7654'3210ULL;
	const uint128_t value{valueLow, valueHigh};
	constexpr std::array<unsigned, 11> shifts{0, 1, 63, 64, 65, 127, 128, 129, 191, 255, 256};
	for (const unsigned shift : shifts)
	{
		CAPTURE(shift);
		CHECK(words(value << shift) == shift_left_words(words(value), shift));
		CHECK(words(value >> shift) == shift_right_words(words(value), shift));
	}
	CHECK((value << -1) == value);
	CHECK((value >> -1) == value);
	volatile bool falseCount = false;
	volatile bool trueCount = true;
	CHECK((value << falseCount) == value);
	CHECK((value >> falseCount) == value);
	CHECK((value << trueCount) == (value << 1));
	CHECK((value >> trueCount) == (value >> 1));
	volatile std::uint64_t oversized = std::numeric_limits<std::uint64_t>::max();
	CHECK((value << oversized) == uint128_t{});
	CHECK((value >> oversized) == uint128_t{});
}

TEST_CASE("uint128 public integer surface remains constexpr-equivalent at runtime", "[simdlib][uint128][surface]")
{
	CHECK(constexpr_contract());
	volatile std::uint64_t lhsLow = snapshot_lhs.low();
	volatile std::uint64_t lhsHigh = snapshot_lhs.high();
	volatile std::uint64_t rhsLow = snapshot_rhs.low();
	volatile std::uint64_t rhsHigh = snapshot_rhs.high();
	const operation_snapshot runtimeSnapshot = snapshot(uint128_t{lhsLow, lhsHigh}, uint128_t{rhsLow, rhsHigh});
	CHECK(runtimeSnapshot == constant_snapshot);
	CHECK(std::numeric_limits<uint128_t>::min() == uint128_t{});
	CHECK(std::numeric_limits<uint128_t>::lowest() == uint128_t{});
	CHECK((std::numeric_limits<uint128_t>::max() == uint128_t{~std::uint64_t{0}, ~std::uint64_t{0}}));
	CHECK(std::numeric_limits<uint128_t>::epsilon() == uint128_t{});
	CHECK(std::numeric_limits<uint128_t>::round_error() == uint128_t{});
	CHECK(std::numeric_limits<uint128_t>::infinity() == uint128_t{});
	CHECK(std::numeric_limits<uint128_t>::quiet_NaN() == uint128_t{});
	CHECK(std::numeric_limits<uint128_t>::signaling_NaN() == uint128_t{});
	CHECK(std::numeric_limits<uint128_t>::denorm_min() == uint128_t{});
	CHECK(SimdLib::countr_zero(uint128_t{}) == 128);
	CHECK(SimdLib::countl_zero(uint128_t{}) == 128);
	CHECK(SimdLib::bit_ceil(uint128_t{1, std::uint64_t{1} << 63}) == uint128_t{});
}

TEST_CASE("uint128 bit ceil covers identity rounding and overflow boundaries", "[simdlib][uint128][bits][ceil]")
{
	const std::array cases{
		bit_ceil_case{uint128_t{}, uint128_t{1}},
		bit_ceil_case{uint128_t{1}, uint128_t{1}},
		bit_ceil_case{uint128_t{5}, uint128_t{8}},
		bit_ceil_case{uint128_t{1, 1}, uint128_t{0, 2}},
		bit_ceil_case{uint128_t{std::numeric_limits<std::uint64_t>::max(), (std::uint64_t{1} << 63) - 1}, uint128_t{0, std::uint64_t{1} << 63}},
		bit_ceil_case{uint128_t{0, std::uint64_t{1} << 63}, uint128_t{0, std::uint64_t{1} << 63}},
		bit_ceil_case{uint128_t{1, std::uint64_t{1} << 63}, uint128_t{}},
		bit_ceil_case{std::numeric_limits<uint128_t>::max(), uint128_t{}},
	};
	for (const auto &test : cases)
	{
		volatile std::uint64_t low = test.value.low();
		volatile std::uint64_t high = test.value.high();
		const uint128_t value{low, high};
		CAPTURE(value.low(), value.high());
		CHECK(SimdLib::bit_ceil(value) == test.expected);
	}
}

TEST_CASE("uint128 optimized operations match the portable two-word oracle", "[simdlib][uint128][oracle]")
{
	random64 random{0xD1B5'4A32'D192'ED03ULL};
	for (unsigned iteration = 0; iteration < 8192; ++iteration)
	{
		const uint128_t lhs{random.next(), random.next()};
		const uint128_t rhs{random.next(), random.next()};
		const unsigned shift = static_cast<unsigned>(random.next() % 260);
		CAPTURE(iteration, shift, lhs.low(), lhs.high(), rhs.low(), rhs.high());

		CHECK(words(lhs + rhs) == add_words(words(lhs), words(rhs)));
		CHECK(words(lhs - rhs) == subtract_words(words(lhs), words(rhs)));
		CHECK((words(lhs & rhs) == words128{lhs.low() & rhs.low(), lhs.high() & rhs.high()}));
		CHECK((words(lhs | rhs) == words128{lhs.low() | rhs.low(), lhs.high() | rhs.high()}));
		CHECK((words(lhs ^ rhs) == words128{lhs.low() ^ rhs.low(), lhs.high() ^ rhs.high()}));
		CHECK((words(~lhs) == words128{~lhs.low(), ~lhs.high()}));
		CHECK(words(lhs << shift) == shift_left_words(words(lhs), shift));
		CHECK(words(lhs >> shift) == shift_right_words(words(lhs), shift));
		CHECK((lhs < rhs) == (compare_words(words(lhs), words(rhs)) < 0));
		CHECK((lhs == rhs) == (compare_words(words(lhs), words(rhs)) == 0));

		uint128_t incremented = lhs;
		++incremented;
		CHECK(words(incremented) == add_words(words(lhs), {1, 0}));
		uint128_t decremented = lhs;
		--decremented;
		CHECK(words(decremented) == subtract_words(words(lhs), {1, 0}));
	}
}

#if defined(__SIZEOF_INT128__)
TEST_CASE("uint128 optimized operations match compiler-native unsigned 128-bit arithmetic", "[simdlib][uint128][native]")
{
	random64 random{0x94D0'49BB'1331'11EBULL};
	for (unsigned iteration = 0; iteration < 4096; ++iteration)
	{
		const uint128_t lhs{random.next(), random.next()};
		const uint128_t rhs{random.next(), random.next()};
		const unsigned shift = static_cast<unsigned>(random.next() % 260);
		const native_uint128 nativeLhs = native(lhs);
		const native_uint128 nativeRhs = native(rhs);
		CHECK(lhs + rhs == library(nativeLhs + nativeRhs));
		CHECK(lhs - rhs == library(nativeLhs - nativeRhs));
		CHECK((lhs & rhs) == library(nativeLhs & nativeRhs));
		CHECK((lhs | rhs) == library(nativeLhs | nativeRhs));
		CHECK((lhs ^ rhs) == library(nativeLhs ^ nativeRhs));
		CHECK((~lhs) == library(~nativeLhs));
		CHECK((lhs << shift) == (shift >= 128 ? uint128_t{} : library(nativeLhs << shift)));
		CHECK((lhs >> shift) == (shift >= 128 ? uint128_t{} : library(nativeLhs >> shift)));
		CHECK((lhs < rhs) == (nativeLhs < nativeRhs));
	}
}
#endif

TEST_CASE("uint128 compiler paths produce the portable-oracle result digest", "[simdlib][uint128][digest]")
{
	random64 random{0xA076'1D64'78BD'642FULL};
	std::uint64_t actualDigest = 1469598103934665603ULL;
	std::uint64_t oracleDigest = 1469598103934665603ULL;
	for (unsigned iteration = 0; iteration < 4096; ++iteration)
	{
		const uint128_t lhs{random.next(), random.next()};
		const uint128_t rhs{random.next(), random.next()};
		const unsigned shift = static_cast<unsigned>(random.next() % 260);
		mix_digest(actualDigest, lhs + rhs);
		mix_digest(actualDigest, lhs - rhs);
		mix_digest(actualDigest, lhs & rhs);
		mix_digest(actualDigest, lhs | rhs);
		mix_digest(actualDigest, lhs ^ rhs);
		mix_digest(actualDigest, ~lhs);
		mix_digest(actualDigest, lhs << shift);
		mix_digest(actualDigest, lhs >> shift);

		const auto add = add_words(words(lhs), words(rhs));
		const auto subtract = subtract_words(words(lhs), words(rhs));
		mix_digest(oracleDigest, uint128_t{add.low, add.high});
		mix_digest(oracleDigest, uint128_t{subtract.low, subtract.high});
		mix_digest(oracleDigest, uint128_t{lhs.low() & rhs.low(), lhs.high() & rhs.high()});
		mix_digest(oracleDigest, uint128_t{lhs.low() | rhs.low(), lhs.high() | rhs.high()});
		mix_digest(oracleDigest, uint128_t{lhs.low() ^ rhs.low(), lhs.high() ^ rhs.high()});
		mix_digest(oracleDigest, uint128_t{~lhs.low(), ~lhs.high()});
		const auto left = shift_left_words(words(lhs), shift);
		const auto right = shift_right_words(words(lhs), shift);
		mix_digest(oracleDigest, uint128_t{left.low, left.high});
		mix_digest(oracleDigest, uint128_t{right.low, right.high});
	}
	CHECK(actualDigest == oracleDigest);
	std::cout << "SIMDLIB_UINT128_RESULT_DIGEST=" << std::hex << actualDigest << '\n';
}

TEST_CASE("uint128_t documentation examples produce their documented results", "[simdlib][uint128][documentation]")
{
	REQUIRE(uint128_t{42} == uint128_t{42});
	{
		const uint128_t temporary{42};
		REQUIRE(temporary == uint128_t{42});
	}
	REQUIRE(uint128_t{10}.abs_diff(uint128_t{3}) == uint128_t{7});
	REQUIRE(SimdLib::bit_ceil(uint128_t{9}) == uint128_t{16});
	REQUIRE(SimdLib::bit_floor(uint128_t{9}) == uint128_t{8});
	REQUIRE(SimdLib::bit_width(uint128_t{9}) == 4);
	REQUIRE(SimdLib::countl_one(uint128_t{0, 0xE000'0000'0000'0000ULL}) == 3);
	REQUIRE(SimdLib::countl_zero(uint128_t{1}) == 127);
	REQUIRE(SimdLib::countr_one(uint128_t{7}) == 3);
	REQUIRE(SimdLib::countr_zero(uint128_t{8}) == 3);
	REQUIRE(uint128_t::create_mask(4) == uint128_t{0b1111});
	REQUIRE(deprecated_extract(uint128_t{0xABCD}, 8, 4) == uint128_t{0xBC});
	REQUIRE(uint128_t{5, 7}.getBlock(1) == 7);
	REQUIRE(SimdLib::has_single_bit(uint128_t{8}));
	REQUIRE(uint128_t{5, 7}.high() == 7);
	REQUIRE(uint128_t{5, 7}.low() == 5);
	REQUIRE(SimdLib::popcount(uint128_t{0b1011}) == 3);
	using namespace SimdLib;
	REQUIRE(42_u128 == uint128_t{42});
	REQUIRE(static_cast<bool>(uint128_t{1}));
	REQUIRE(static_cast<std::uint64_t>(uint128_t{42}) == 42);
	REQUIRE(uint128_t{40} + uint128_t{2} == uint128_t{42});
	REQUIRE(uint128_t{45} - uint128_t{3} == uint128_t{42});
	REQUIRE(uint128_t{42} == uint128_t{42});
	REQUIRE((uint128_t{1} <=> uint128_t{2}) == std::strong_ordering::less);
	REQUIRE((uint128_t{0b1100} & uint128_t{0b1010}) == uint128_t{0b1000});
	REQUIRE((uint128_t{0b1100} | uint128_t{0b1010}) == uint128_t{0b1110});
	REQUIRE((uint128_t{0b1100} ^ uint128_t{0b1010}) == uint128_t{0b0110});
	REQUIRE(~uint128_t{} == uint128_t{std::numeric_limits<std::uint64_t>::max(), std::numeric_limits<std::uint64_t>::max()});
	REQUIRE((uint128_t{3} << 2) == uint128_t{12});
	REQUIRE((uint128_t{12} >> 2) == uint128_t{3});
#if SIMDLIB_HAS_SSE2
	REQUIRE(uint128_t::from_register(SimdLib::Api<128, std::uint64_t>::construct({5, 7})) == uint128_t{5, 7});
	REQUIRE(SimdLib::Api<128, std::uint64_t>::to_array(uint128_t{5, 7}.to_register()) == std::array<std::uint64_t, 2>{5, 7});
#endif
	uint128_t result{};
	result = uint128_t{42};
	REQUIRE(result == uint128_t{42});
	result = uint128_t{40};
	result += uint128_t{2};
	REQUIRE(result == uint128_t{42});
	result = uint128_t{45};
	result -= uint128_t{3};
	REQUIRE(result == uint128_t{42});
	result = uint128_t{0b1100};
	result &= uint128_t{0b1010};
	REQUIRE(result == uint128_t{0b1000});
	result = uint128_t{0b1100};
	result |= uint128_t{0b1010};
	REQUIRE(result == uint128_t{0b1110});
	result = uint128_t{0b1100};
	result ^= uint128_t{0b1010};
	REQUIRE(result == uint128_t{0b0110});
	result = uint128_t{3};
	result <<= 2;
	REQUIRE(result == uint128_t{12});
	result = uint128_t{12};
	result >>= 2;
	REQUIRE(result == uint128_t{3});
	result = uint128_t{41};
	++result;
	REQUIRE(result == uint128_t{42});
	result = uint128_t{43};
	--result;
	REQUIRE(result == uint128_t{42});
}
