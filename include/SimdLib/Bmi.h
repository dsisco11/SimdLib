#pragma once

#include <SimdLib/Config.h>

#include <bit>
#include <concepts>
#include <cstdint>
#if SIMDLIB_TARGET_X86
#include <immintrin.h>
#endif
#if SIMDLIB_COMPILER_MSVC && SIMDLIB_TARGET_X86
#include <intrin.h>
#endif
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

namespace SimdLib::Bmi
{
template <class T>
concept integer_like = std::numeric_limits<T>::is_specialized && std::numeric_limits<T>::is_integer &&
	!std::same_as<std::remove_cv_t<T>, bool>;

#pragma region Pre-Optimized Generic Integer Operations
// These methods are versions of common std methods that would usually optimize down into roughtly the same code as is written here, but we optimize these ahead
// of time to guarantee the optimizations will be applied even in debug builds.

/// @brief Turns a boolean value into an integer-width bitmask of all ones or zeros (0 for false, all 1s for true).
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t boolmask(const bool state) noexcept
{
	if constexpr (std::is_integral_v<int_t>)
	{
		return static_cast<int_t>(-static_cast<std::make_signed_t<int_t>>(state));
	}
	else
	{
		return -int_t{state};
	}
}

/// @brief Branchless selection between two values based on a switch bit.
/// @param selectionBit The bit that will determine which value to select. (0 = lhs, 1 = rhs)
template <integer_like int_t>
[[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t select(const int_t lhs, const int_t rhs, const bool selectionBit) noexcept
{
	const int_t mask = boolmask<int_t>(selectionBit);
	return (~mask & lhs) | (rhs & mask); // Select between lhs and rhs
}

/// @brief Branchless find maximum of two values.
template <std::integral int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t max(const int_t lhs, const int_t rhs) noexcept
{
	return select(lhs, rhs, lhs < rhs);
}

/// @brief Branchless find minimum of two values.
template <std::integral int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t min(const int_t lhs, const int_t rhs) noexcept
{
	return select(lhs, rhs, lhs > rhs);
}

/// @brief Branchless find absolute value of the input.
/// @note For the minimum signed value, returns the unchanged two's-complement magnitude bit pattern because its positive magnitude is not representable.
template <std::integral int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t abs(const int_t lhs) noexcept
{
	if constexpr (std::is_signed_v<int_t>)
	{
		using unsigned_t = std::make_unsigned_t<int_t>;
		const unsigned_t value = std::bit_cast<unsigned_t>(lhs);
		const unsigned_t sign = value >> (std::numeric_limits<unsigned_t>::digits - 1);
		const unsigned_t mask = unsigned_t{0} - sign;
		return std::bit_cast<int_t>(static_cast<unsigned_t>((value ^ mask) + sign));
	}
	else
	{
		return lhs;
	}
}

#pragma region Tests
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
#pragma endregion // Tests

#pragma endregion // Common Building Blocks

#pragma region BMI Cannon Intrinsics

namespace Detail
{
template <std::integral int_t> using unsigned_t = std::make_unsigned_t<int_t>;

template <std::integral int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr int_t from_unsigned(const unsigned_t<int_t> value) noexcept
{
	return std::bit_cast<int_t>(value);
}

template <std::integral int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr unsigned_t<int_t> to_unsigned(const int_t value) noexcept
{
	return std::bit_cast<unsigned_t<int_t>>(value);
}

template <std::integral int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr int_t portable_andn(const int_t lhs, const int_t rhs) noexcept
{
	return from_unsigned<int_t>(to_unsigned(rhs) & ~to_unsigned(lhs));
}

template <std::integral int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr int_t portable_bzhi(const int_t source, const unsigned index) noexcept
{
	using unsigned_type = unsigned_t<int_t>;
	constexpr unsigned bit_count = static_cast<unsigned>(sizeof(int_t) * 8u);
	if (index == 0)
	{
		return int_t{0};
	}
	if (index >= bit_count)
	{
		return source;
	}
	const unsigned_type mask = static_cast<unsigned_type>((unsigned_type{1} << index) - unsigned_type{1});
	return from_unsigned<int_t>(to_unsigned(source) & mask);
}

template <std::integral int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr int_t portable_blsi(const int_t source) noexcept
{
	using unsigned_type = unsigned_t<int_t>;
	const unsigned_type value = to_unsigned(source);
	return from_unsigned<int_t>(value & (unsigned_type{0} - value));
}

template <std::integral int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr int_t portable_blsr(const int_t source) noexcept
{
	using unsigned_type = unsigned_t<int_t>;
	const unsigned_type value = to_unsigned(source);
	return from_unsigned<int_t>(value & (value - unsigned_type{1}));
}

template <std::integral int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr int_t portable_blsmsk(const int_t source) noexcept
{
	using unsigned_type = unsigned_t<int_t>;
	const unsigned_type value = to_unsigned(source);
	return from_unsigned<int_t>(value ^ (value - unsigned_type{1}));
}

template <std::integral int_t>
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr int_t portable_mulx(const int_t lhs, const int_t rhs, int_t &hi) noexcept
	requires(!std::same_as<std::remove_cv_t<int_t>, bool>)
{
	using unsigned_type = unsigned_t<int_t>;
	const unsigned_type unsigned_lhs = to_unsigned(lhs);
	const unsigned_type unsigned_rhs = to_unsigned(rhs);

	if constexpr (sizeof(unsigned_type) <= sizeof(std::uint32_t))
	{
		constexpr unsigned bit_count = static_cast<unsigned>(sizeof(unsigned_type) * 8u);
		const std::uint64_t product = static_cast<std::uint64_t>(unsigned_lhs) * static_cast<std::uint64_t>(unsigned_rhs);
		hi = from_unsigned<int_t>(static_cast<unsigned_type>(product >> bit_count));
		return from_unsigned<int_t>(static_cast<unsigned_type>(product));
	}
	else
	{
		static_assert(sizeof(unsigned_type) == sizeof(std::uint64_t));
		const std::uint64_t a = static_cast<std::uint64_t>(unsigned_lhs);
		const std::uint64_t b = static_cast<std::uint64_t>(unsigned_rhs);
		const std::uint64_t a_lo = static_cast<std::uint32_t>(a);
		const std::uint64_t a_hi = a >> 32;
		const std::uint64_t b_lo = static_cast<std::uint32_t>(b);
		const std::uint64_t b_hi = b >> 32;
		const std::uint64_t low_product = a_lo * b_lo;
		const std::uint64_t middle = a_hi * b_lo + (low_product >> 32);
		std::uint64_t middle_low = static_cast<std::uint32_t>(middle);
		const std::uint64_t middle_high = middle >> 32;
		middle_low += a_lo * b_hi;
		const std::uint64_t high_product = a_hi * b_hi + middle_high + (middle_low >> 32);
		const std::uint64_t low_result = (middle_low << 32) | static_cast<std::uint32_t>(low_product);
		hi = from_unsigned<int_t>(static_cast<unsigned_type>(high_product));
		return from_unsigned<int_t>(static_cast<unsigned_type>(low_result));
	}
}
} // namespace Detail

/// @brief Compute the bitwise NOT of LHS and then AND with RHS.
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t andn(const int_t lhs, const int_t rhs) noexcept
{
	if constexpr (std::integral<int_t>)
	{
#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_BMI1
		if (!std::is_constant_evaluated())
		{
			#if SIMDLIB_TARGET_X64
			if constexpr (sizeof(int_t) == sizeof(std::uint64_t))
			{
				return static_cast<int_t>(_andn_u64(static_cast<std::uint64_t>(lhs), static_cast<std::uint64_t>(rhs)));
			}
			else
			#endif
			if constexpr (sizeof(int_t) == sizeof(std::uint32_t))
			{
				return static_cast<int_t>(_andn_u32(static_cast<std::uint32_t>(lhs), static_cast<std::uint32_t>(rhs)));
			}
		}
#endif
		return Detail::portable_andn(lhs, rhs);
	}
	else
	{
		return rhs & ~lhs;
	}
}

/// @brief Copy all bits from source integer, and reset (set to 0) the high bits in output starting at index.
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t bzhi(const int_t source, unsigned index) noexcept
{
	if constexpr (std::integral<int_t>)
	{
#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_BMI2
		if (!std::is_constant_evaluated())
		{
			#if SIMDLIB_TARGET_X64
			if constexpr (sizeof(int_t) == sizeof(std::uint64_t))
			{
				return static_cast<int_t>(_bzhi_u64(static_cast<std::uint64_t>(source), index));
			}
			else
			#endif
			if constexpr (sizeof(int_t) == sizeof(std::uint32_t))
			{
				return static_cast<int_t>(_bzhi_u32(static_cast<std::uint32_t>(source), index));
			}
		}
#endif
		return Detail::portable_bzhi(source, index);
	}
	else
	{
		if (index == 0)
		{
			return int_t{0};
		}
		if (index >= static_cast<unsigned>(std::numeric_limits<int_t>::digits))
		{
			return source;
		}
		return source & ((int_t{1} << index) - int_t{1});
	}
}

/// @brief Extract the lowest set bit from source integer and set the corresponding bit in dst. All other bits in dst are zeroed, and all bits are zeroed if no
/// bits are set in source.
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t blsi(const int_t source) noexcept
{
	if constexpr (std::integral<int_t>)
	{
#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_BMI1
		if (!std::is_constant_evaluated())
		{
			#if SIMDLIB_TARGET_X64
			if constexpr (sizeof(int_t) == sizeof(std::uint64_t))
				return static_cast<int_t>(_blsi_u64(static_cast<std::uint64_t>(source)));
			else
			#endif
			if constexpr (sizeof(int_t) == sizeof(std::uint32_t))
				return static_cast<int_t>(_blsi_u32(static_cast<std::uint32_t>(source)));
		}
#endif
		return Detail::portable_blsi(source);
	}
	else
		return source & (int_t{0} - source);
}
static_assert(blsi<std::uint32_t>(0b10100) == 0b00100);

/// @brief Copy all bits from source to dst, and reset (set to 0) the bit in dst that corresponds to the lowest set bit in source.
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t blsr(const int_t source) noexcept
{
	if constexpr (std::integral<int_t>)
	{
#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_BMI1
		if (!std::is_constant_evaluated())
		{
			#if SIMDLIB_TARGET_X64
			if constexpr (sizeof(int_t) == sizeof(std::uint64_t))
				return static_cast<int_t>(_blsr_u64(static_cast<std::uint64_t>(source)));
			else
			#endif
			if constexpr (sizeof(int_t) == sizeof(std::uint32_t))
				return static_cast<int_t>(_blsr_u32(static_cast<std::uint32_t>(source)));
		}
#endif
		return Detail::portable_blsr(source);
	}
	else
		return source & (source - int_t{1});
}
static_assert(blsr<std::uint32_t>(0b1011) == 0b1010);

/// @brief Extract and reset the lowest set bit in source.
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t blse(const int_t source, int_t &out_lsb) noexcept
{
	out_lsb = blsi(source);
	return source ^ out_lsb;
}

/// @brief Extract and reset the lowest set bit in source.
/// @return A tuple containing the source integer with the bits reset and the extracted bits.
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static std::tuple<int_t, int_t> blse(const int_t source) noexcept
{
	const int_t out_lsb = blsi(source);
	return {source ^ out_lsb, out_lsb};
}
static_assert(blse<std::uint32_t>(0b10111) == std::make_tuple(std::uint32_t{0b10110}, std::uint32_t{0b1}));

/**
 * @brief Extracts the first source bit at or above an inclusive one-hot boundary. [eg: blsioff(0b10100, 0b01000) => 0b10000]
 *
 * Searches from starting_bit toward the right (high-bits) and returns the first set source bit encountered. Returns zero when starting_bit is zero or no set
 * source bit exists within the searched range.
 *
 * @param source The bit pattern to search.
 * @param starting_bit A one-hot bit defining the inclusive lower boundary.
 * @return The matching bit as a one-hot value, or zero if none exists.
 */
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t blsioff(const int_t source, const int_t starting_bit) noexcept
{
	return source & static_cast<int_t>(~source + starting_bit);
}
static_assert(blsioff<std::uint32_t>(0b10100, 0b00001) == 0b00100);
static_assert(blsioff<std::uint32_t>(0b10100, 0b00010) == 0b00100);
static_assert(blsioff<std::uint32_t>(0b10100, 0b00100) == 0b00100);
static_assert(blsioff<std::uint32_t>(0b10100, 0b01000) == 0b10000);

/// @brief Set all the lower bits of dst up to and including the lowest set bit in source.
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t blsmsk(const int_t source) noexcept
{
	if constexpr (std::integral<int_t>)
	{
#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_BMI1
		if (!std::is_constant_evaluated())
		{
			#if SIMDLIB_TARGET_X64
			if constexpr (sizeof(int_t) == sizeof(std::uint64_t))
				return static_cast<int_t>(_blsmsk_u64(static_cast<std::uint64_t>(source)));
			else
			#endif
			if constexpr (sizeof(int_t) == sizeof(std::uint32_t))
				return static_cast<int_t>(_blsmsk_u32(static_cast<std::uint32_t>(source)));
		}
#endif
		return Detail::portable_blsmsk(source);
	}
	else
		return source ^ (source - int_t{1});
}

/**
 * @brief Multiplies the unsigned object-representation values of two integers. [eg: mulx<uint8_t>(0xFF, 0x02, hi) => 0xFE, hi = 0x01]
 *
 * Computes the full double-width product, returns its low word, and stores its high word in hi. Signed inputs are interpreted by their bit patterns, not as
 * signed mathematical values. This does not read or write arithmetic flags.
 *
 * @param lhs The first word-sized operand.
 * @param rhs The second word-sized operand.
 * @param hi Receives the high word of the full product.
 * @return The low word of the full product.
 */
template <std::integral int_t>
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mulx(const int_t lhs, const int_t rhs, int_t &hi) noexcept
	requires(!std::same_as<std::remove_cv_t<int_t>, bool>)
{
#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_BMI2
	if (!std::is_constant_evaluated())
	{
		#if SIMDLIB_TARGET_X64
		if constexpr (sizeof(int_t) == sizeof(std::uint64_t))
		{
			unsigned long long intrinsic_hi = 0;
			const unsigned long long low = _mulx_u64(static_cast<unsigned long long>(lhs), static_cast<unsigned long long>(rhs), &intrinsic_hi);
			hi = static_cast<int_t>(intrinsic_hi);
			return static_cast<int_t>(low);
		}
		else
		#endif
		if constexpr (sizeof(int_t) == sizeof(std::uint32_t))
		{
#if !defined(__GNUC__) || defined(__clang__) || defined(__i386__)
			unsigned int intrinsic_hi = 0;
			const unsigned int low = _mulx_u32(static_cast<unsigned int>(lhs), static_cast<unsigned int>(rhs), &intrinsic_hi);
			hi = static_cast<int_t>(intrinsic_hi);
			return static_cast<int_t>(low);
#endif
		}
	}
#endif
	return Detail::portable_mulx(lhs, rhs, hi);
}

#pragma endregion

#pragma region Parallel Prefix/Suffix Operations

/// @brief Computes a distance-1 parallel-prefix XOR stage by XORing each bit with its adjacent bit to the right (high-bits). [eg: pp_xor(0b01110) => 0b01001]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t pp_xor(const int_t value) noexcept
{
	return (value >> 1) ^ value;
}
static_assert(pp_xor<std::uint32_t>(0b01110) == 0b01001);
static_assert(pp_xor<std::uint32_t>(0b11110) == 0b10001);

/// @brief Computes a distance-1 parallel-suffix XOR stage by XORing each bit with its adjacent bit to the left (low-bits). [eg: ps_xor(0b01110) => 0b10010]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t ps_xor(const int_t value) noexcept
{
	return (value << 1) ^ value;
}
static_assert(ps_xor<std::uint32_t>(0b01110) == 0b10010);
static_assert(ps_xor<std::uint32_t>(0b11110) == 0b100010);

/// @brief Computes the parallel-prefix OR of the given value, which is the result of or'ing each bit with all bits to the left (low-bits). [eg: 10100 => 11111 ]
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t pp_or(const int_t value) noexcept
{
	using Bmi::bzhi;
	using std::bit_width;
	return bzhi(std::numeric_limits<int_t>::max(), bit_width(value));
}
static_assert(pp_or<std::uint8_t>(0b0100) == 0b0111);
static_assert(pp_or<std::uint16_t>(0b0100) == 0b0111);
static_assert(pp_or<std::uint32_t>(0b0) == 0b0);
static_assert(pp_or<std::uint32_t>(0b0100) == 0b0111);
static_assert(pp_or<std::uint32_t>(0b10100) == 0b11111);

/// @brief Computes the parallel-suffix OR of the given value, which is the result of or'ing each bit with all bits to the right (high-bits). [eg: 010100 => 1...100 ]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t ps_or(const int_t value) noexcept
{
	return value | (int_t{0} - value);
	// return value | ((~value) + 1);
}
static_assert(ps_or<std::int32_t>(0x0) == 0x0);
static_assert(ps_or<std::int32_t>(0b0100) == std::int32_t{-4});
static_assert(ps_or<std::uint32_t>(0b0100) == 0xFFFF'FFFC);
static_assert(ps_or<std::uint32_t>(0b10100) == 0xFFFF'FFFC);

/// @brief Computes the parallel-prefix-least-significant-OR of the given value, which is the result of clearing all bits to the right (high-bits) of the lsb and
/// then or'ing each bit with all bits to the left (low-bits). [eg: 10100 => 00111 ]
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t pp_lsor(const int_t value) noexcept
{
	using Bmi::blsi;
	using Bmi::bzhi;
	using std::bit_width;
	return bzhi(std::numeric_limits<int_t>::max(), bit_width(blsi(value)));
}
static_assert(pp_lsor<std::uint32_t>(0b0) == 0b0);
static_assert(pp_lsor<std::uint32_t>(0b0100) == 0b0111);
static_assert(pp_lsor<std::uint32_t>(0b10100) == 0b00111);

/// @brief Computes a distance-1 parallel-prefix AND stage by ANDing each bit with its adjacent bit to the right (high-bits). [eg: pp_and(0b01101110) => 0b00100110]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t pp_and(const int_t value) noexcept
{
	return value & (value >> 1);
}
static_assert(pp_and<std::uint32_t>(0b01101110) == 0b00100110);

/// @brief Computes a distance-1 parallel-suffix AND stage by ANDing each bit with its adjacent bit to the left (low-bits). [eg: ps_and(0b01101110) => 0b01001100]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t ps_and(const int_t value) noexcept
{
	return value & (value << 1);
}
static_assert(ps_and<std::uint32_t>(0b01101110) == 0b01001100);

/// @brief Computes a distance-1 parallel-prefix AND-NOT stage, retaining set bits whose adjacent bit to the right (high-bits) is clear. [eg: pp_andn(0b01110) => 0b01000]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t pp_andn(const int_t value) noexcept
{
	using Bmi::andn;
	return andn<int_t>(value >> 1, value);
}
static_assert(pp_andn<std::uint32_t>(0b01110) == 0b01000);

/// @brief Computes a distance-1 parallel-suffix AND-NOT stage, retaining set bits whose adjacent bit to the left (low-bits) is clear. [eg: ps_andn(0b01110) => 0b00010]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t ps_andn(const int_t value) noexcept
{
	using Bmi::andn;
	return andn<int_t>(value << 1, value);
}
static_assert(ps_andn<std::uint32_t>(0b01110) == 0b00010);
static_assert(ps_andn<std::uint32_t>(0b001100) == 0b0000100);

/// @brief Computes an inverse distance-1 parallel-prefix AND-NOT stage, marking clear bits whose adjacent bit to the right (high-bits) is set. [eg: pp_andni(0b01110) => 0b00001]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t pp_andni(const int_t value) noexcept
{
	using Bmi::andn;
	return andn<int_t>(value, value >> 1);
}
static_assert(pp_andni<std::uint32_t>(0b01111) == 0b00000);
static_assert(pp_andni<std::uint32_t>(0b01110) == 0b00001);
static_assert(pp_andni<std::uint32_t>(0b01100) == 0b00010);
static_assert(pp_andni<std::uint32_t>(0b1011000) == 0b100100);

/// @brief Computes an inverse distance-1 parallel-suffix AND-NOT stage, marking clear bits whose adjacent bit to the left (low-bits) is set. [eg: ps_andni(0b01110) => 0b10000]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t ps_andni(const int_t value) noexcept
{
	using Bmi::andn;
	return andn<int_t>(value, value << 1);
}
static_assert(ps_andni<std::uint32_t>(0b01110) == 0b10000);
static_assert(ps_andni<std::uint32_t>(0b001100) == 0b010000);
static_assert(ps_andni<std::uint32_t>(0b101100) == 0b1010000);
#pragma endregion

#pragma region BMI Extended Operations
/// @brief Extract the highest set bit from source integer and set the corresponding bit in dst. All other bits in dst are zeroed, and all bits are zeroed if no
/// bits are set in source.
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t bmsi(const int_t value) noexcept
{
	using std::bit_floor;
	return bit_floor(value);
	// return value & ~(pp_or(value) >> 1);
	//  return value ^ bzhi(std::numeric_limits<int_t>::max(), bit_width(value));
	//  return !value ? int_t{0} : int_t{1} << (bit_width(value) - 1);
}
static_assert(bmsi<std::uint32_t>(0b10111) == 0b10000);
static_assert(bmsi<std::uint32_t>(0b0) == 0b0);

/// @brief Copy all bits from source to dst, and reset (set to 0) the bit in dst that corresponds to the highest set bit in source.
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t bmsr(const int_t value) noexcept
{
	using Bmi::bzhi;
	using std::bit_width;
	// return value ^ (int_t(1) << (bit_width(value) - 1));
	return bzhi(value, bit_width(value) - 1);
}
static_assert(bmsr<std::uint32_t>(0b1011) == 0b0011);
static_assert(bmsr<std::uint32_t>(0b1000) == 0b0000);

/// @brief Copy all bits from source to dst, and reset (set to 0) the bit in dst that corresponds to the highest set bit in source.
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t bmsr(const int_t value, int &out_msb_index) noexcept
{
	using std::bit_width;
	out_msb_index = bit_width(value) - 1;
	return bzhi(value, out_msb_index);
}

/// @brief Extract and reset the highest set bit in source.
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t bmse(const int_t value, int_t &out_msb) noexcept
{
	using std::bit_floor;
	out_msb = bit_floor(value);
	return value ^ out_msb;
}

/// @brief Extract and reset the highest set bit in source.
/// @return A tuple containing the source integer with the bits reset and the extracted bits.
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static std::tuple<int_t, int_t> bmse(const int_t value) noexcept
{
	using std::bit_floor;
	const int_t msb = bit_floor(value);
	return {value ^ msb, msb};
}
static_assert(bmse<std::uint32_t>(0b10111) == std::make_tuple(std::uint32_t{0b00111}, std::uint32_t{0b10000}));

/**
 * @brief Clears every source bit whose bit index is less than index. [eg: bzlo(0b11111, 3) => 0b11000]
 *
 * Clears the index least-significant bits on the left (low-bits), while retaining the bit at index. An index of zero returns source unchanged; an index greater
 * than or equal to the word width returns zero.
 *
 * @param source The bit pattern to modify.
 * @param index The exclusive upper bound of the cleared bit-index range.
 * @return The source value with bits in [0, index) cleared.
 */
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t bzlo(const int_t source, unsigned index) noexcept
{
	return andn(bzhi(~int_t{0}, index), source);
}
static_assert(bzlo<std::uint32_t>(0b10111, 0) == 0b10111);
static_assert(bzlo<std::uint32_t>(0b10111, 1) == 0b10110);
static_assert(bzlo<std::uint32_t>(0b10111, 2) == 0b10100);
static_assert(bzlo<std::uint32_t>(0b10111, 3) == 0b10000);

/// @brief Set all the lower bits of dst up to and including the highest set bit in source.
template <std::integral int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t bmsmsk(const int_t source) noexcept
{
	return pp_or(source);
}
#pragma endregion

#pragma region Common Building Blocks
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t PartialSumBLSMSK(const int_t n) noexcept
{
	int_t sum = n;
	sum += (n & 0xAAAAAAAA);
	sum += (n & 0xCCCCCCCC) << 1;
	sum += (n & 0xF0F0F0F0) << 2;
	sum += (n & 0xFF00FF00) << 3;
	sum += (n & 0xFFFF0000) << 4;
	return sum;
}

template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t PartialSumBLSI(const int_t n) noexcept
{
	int_t sum = n;
	sum += (n & 0xAAAAAAAA) >> 1;
	sum += (n & 0xCCCCCCCC);
	sum += (n & 0xF0F0F0F0) << 1;
	sum += (n & 0xFF00FF00) << 2;
	sum += (n & 0xFFFF0000) << 3;
	return sum;
}
static_assert(PartialSumBLSI<std::uint32_t>(0b1011) == 24);
static_assert(PartialSumBLSI<std::uint32_t>(0b1100) == 28);
static_assert(PartialSumBLSI<std::uint32_t>(0b1110) == 31);
static_assert(PartialSumBLSI<std::uint32_t>(0b1111) == 32);

/// @brief Sets the least significant, leftmost (low-bits) unset bit. [eg: 01011 => 01111]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t flipr_unset(const int_t value) noexcept
{
	return value | (value + 1);
}
static_assert(flipr_unset<std::uint32_t>(0b01011) == 0b01111);

/// @brief Returns a single 1-bit at the position of the leftmost (low-bits) 0-bit, producing 0 if none. [eg: 01011 => 00100]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t maskr_unset(const int_t value) noexcept
{
	using Bmi::blsi;
	return blsi<int_t>(~value);
}
static_assert(maskr_unset<std::uint32_t>(0b01011) == 0b00100);

/// @brief Returns a single 1-bit at the position of the rightmost (high-bits) trailing 1-bit, producing 0 if none. [eg: 010111 => 00100]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t maskl_trailing_one(const int_t value) noexcept
{
	using Bmi::blsi;
	return blsi<int_t>(~value) >> 1;
}
static_assert(maskl_trailing_one<std::uint32_t>(0b010111) == 0b00100);
static_assert(maskl_trailing_one<std::uint32_t>(0b010110) == 0b0);

/// @brief Clears all least significant, leftmost (low-bits) trailing set bits. [eg: 1011 => 1000]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t clear_trailing_ones(const int_t value) noexcept
{
	return value & (value + 1);
}
static_assert(clear_trailing_ones<std::uint32_t>(0b1011) == 0b1000);

/// @brief Sets all least significant, leftmost (low-bits) trailing unset bits. [eg: 10100 => 10111]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t flip_trailing_zeros(const int_t value) noexcept
{
	return value | (value - 1);
}
static_assert(flip_trailing_zeros<std::uint32_t>(0b10100) == 0b10111);

/// @brief Returns a mask over the trailing 0-bits in the source integer. [eg: 10100 => 011]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mask_trailing_zeros(const int_t value) noexcept
{
	using Bmi::blsi;
	return blsi(value) - 1;
}
static_assert(mask_trailing_zeros<std::uint32_t>(0b101000) == 0b0111);

/// @brief Returns a mask over the trailing 0-bits in the source integer.
/// For value==0, returns 0 ("safe" variant; avoids the wraparound/all-ones behavior).
/// [eg: 10100 => 00011]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mask_trailing_zeros_or_zero(const int_t value) noexcept
{
	return boolmask<int_t>(value != 0) & mask_trailing_zeros(value);
}
static_assert(mask_trailing_zeros_or_zero<std::uint32_t>(0b101000) == 0b0111);
static_assert(mask_trailing_zeros_or_zero<std::uint32_t>(0) == 0u);

/// @brief Returns a mask of all bits strictly lower than the least-significant set bit (LSB).
/// For value==0, returns 0.
/// [eg: 101000 => 000111]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mask_bits_lower_than_lsb(const int_t value) noexcept
{
	// `ps_or(value)` sets bits from the LSB up to MSB (and higher) to 1; inverting yields exactly the bits below the LSB.
	// For value==0, ps_or(0)==0, so ~ps_or(0) would be all-ones; mask it out.
	return boolmask<int_t>(value != 0) & ~ps_or(value);
}
static_assert(mask_bits_lower_than_lsb<std::uint32_t>(0b101000) == 0b0111);
static_assert(mask_bits_lower_than_lsb<std::uint32_t>(0) == 0u);

/// @brief Returns a mask of all bits strictly lower than the least-significant set bit (LSB).
/// For value==0, returns all-ones (useful as a "no constraint" mask).
/// [eg: 101000 => 000111]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mask_bits_lower_than_lsb_or_all_ones(const int_t value) noexcept
{
	return ~ps_or(value);
}
static_assert(mask_bits_lower_than_lsb_or_all_ones<std::uint32_t>(0b101000) == 0b0111);
static_assert(mask_bits_lower_than_lsb_or_all_ones<std::uint32_t>(0) == 0xFFFF'FFFFu);

/// @brief Returns a mask over the trailing 1-bits in the source integer, producing 0 if none. [eg: 10111 => 00111]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mask_trailing_ones(const int_t value) noexcept
{
	using Bmi::blsi;
	return blsi<int_t>(~value) - int_t{1};
}
static_assert(mask_trailing_ones<std::uint32_t>(0b10111) == 0b00111);
static_assert(mask_trailing_ones<std::uint32_t>(0b10110) == 0b0);

/// @brief Returns a mask over the leading zeros in the source integer. [eg: 000101 => 111000]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mask_leading_zeros(const int_t value) noexcept
{
	/*using Bmi::bzhi;
	using std::bit_width;
	return ~bzhi(std::numeric_limits<int_t>::max(), bit_width(value));*/
	return ~pp_or(value);
}
static_assert(mask_leading_zeros<std::uint8_t>(0b000101) == 0xF8);
static_assert(mask_leading_zeros<std::uint16_t>(0b000101) == 0xFFF8);
static_assert(mask_leading_zeros<std::uint32_t>(0b000101) == 0xFFFFFFF8);

/// @brief Returns a mask over the leading ones in the source integer. [eg: 111011 => 111000]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mask_leading_ones(const int_t value) noexcept
{
	return ~pp_or(static_cast<int_t>(~value));
}
static_assert(mask_leading_ones<std::uint8_t>(0b11100000) == 0b11100000);
static_assert(mask_leading_ones<std::uint8_t>(0b11110101) == 0b11110000);
static_assert(mask_leading_ones<std::uint16_t>(0xFFF5) == 0xFFF0);
static_assert(mask_leading_ones<std::uint32_t>(0xFFFFFFFFU) == 0xFFFFFFFFU);
static_assert(mask_leading_ones<std::uint32_t>(0xFFFFFFF5U) == 0xFFFFFFF0U);

/// @brief Clears all most significant, rightmost (high-bits) leading set bits. [eg: 110101 => 000101]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t clear_leading_ones(const int_t value) noexcept
{
	return pp_or(static_cast<int_t>(~value)) & value;
}
static_assert(clear_leading_ones<std::uint8_t>(0b11110000) == 0b00000000);
static_assert(clear_leading_ones<std::uint8_t>(0b11110101) == 0b00000101);
static_assert(clear_leading_ones<std::uint8_t>(0b11101011) == 0b00001011);
static_assert(clear_leading_ones<std::uint8_t>(0b10101111) == 0b00101111);
static_assert(clear_leading_ones<std::uint16_t>(0xFFF5) == 0b101);
static_assert(clear_leading_ones<std::uint32_t>(0xFFFFFFF5) == 0b101);

/// @brief Copy all bits from the source integer, and reset (set to 0) the leftmost (low-bits) string of contiguous set bits. [eg: 1011 => 1000]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t clear_lowest_set_bits(const int_t value) noexcept
{
	return value & ((value | (value - int_t{1})) + int_t{1});
}
static_assert(clear_lowest_set_bits<std::uint32_t>(0b1011) == 0b1000);
static_assert(clear_lowest_set_bits<std::uint32_t>(0b10110) == 0b10000);

/// @brief Copy all bits from the source integer, and reset (set to 0) the leftmost (low-bits) string of contiguous set bits after copying said bits into the provided
/// integer address. [eg: 1011 => 1000]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t clear_lowest_set_bits(const int_t value, int_t &out_consumed) noexcept
{
	const int_t mask = ((value | (value - int_t{1})) + int_t{1});
	out_consumed = value ^ mask;
	return value & mask;
}

/// @brief Extracts and returns the leftmost (low-bits) string of contiguous set bits, said bits are also reset (set to 0) within the source integer. [eg: 1011 => 0011]
/// @return A tuple containing the source integer with the bits reset and the extracted bits.
template <integer_like int_t>
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::tuple<int_t, int_t> consume_bit_sequence_right(const int_t value) noexcept
{
	const int_t mask = ((value | (value - int_t{1})) + int_t{1});
	return {value & mask, value & ~mask};
}
static_assert(consume_bit_sequence_right<std::uint32_t>(0b1011) == std::make_tuple(std::uint32_t{0b1000}, std::uint32_t{0b0011}));
static_assert(consume_bit_sequence_right<std::uint32_t>(0b10110) == std::make_tuple(std::uint32_t{0b10000}, std::uint32_t{0b00110}));

/// @brief Extracts and returns the rightmost (high-bits) string of contiguous set bits, said bits are also reset (set to 0) within the source integer. [eg: 0110111 =>
/// 0110000]
/// @return A tuple containing the source integer with the bits reset and the extracted bits.
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::tuple<int_t, int_t> consume_bit_sequence_left(const int_t value) noexcept
{
	using Bmi::andn;
	const int_t thresholds = ps_andn(value);
	const int_t seq_mask = Bmi::bmsi(thresholds) - 1; // convert the thresholds msb to a mask over all the bits to the left (low-bits) of it.
	return {value & seq_mask, andn(seq_mask, value)};
}
static_assert(consume_bit_sequence_left<std::uint32_t>(0b0110111) == std::make_tuple(std::uint32_t{0b0000111}, std::uint32_t{0b0110000}));
static_assert(consume_bit_sequence_left<std::uint32_t>(0b01101110) == std::make_tuple(std::uint32_t{0b00001110}, std::uint32_t{0b01100000}));

/// @brief Copy all bits from the source integer, and reset (set to 0) the trailing bits up-to but excluding the rightmost (high-bits) trailing set bit. [eg: 10111 => 10100]
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t left_collapse_trailing_bits(const int_t value) noexcept
{
	using Bmi::andn;
	return andn(mask_trailing_ones(value) >> 1, value);
}
static_assert(left_collapse_trailing_bits<std::uint32_t>(0b10111) == 0b10100);
static_assert(left_collapse_trailing_bits<std::uint32_t>(0b10110) == 0b10110);

/// @brief Clears all bits lower than (not including) the given target-bit from the source integer. [eg: (10111, 100) => 10100]
template <integer_like int_t>
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t clear_bits_lower_than(const int_t value, const int_t target_bit) noexcept
{
	using Bmi::andn;
	return andn(target_bit - int_t{1}, value);
}
static_assert(clear_bits_lower_than<std::uint32_t>(0b10111, 0b00100) == 0b10100);
static_assert(clear_bits_lower_than<std::uint32_t>(0b10111, 0b00010) == 0b10110);

/// @brief Clears all bits higher than (not including) the given target-bit from the source integer. [eg: (10111, 100) => 00111]
template <integer_like int_t>
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t clear_bits_higher_than(const int_t value, const int_t target_bit) noexcept
{
	using Bmi::blsmsk;
	return blsmsk(target_bit) & value;
}

/// @brief Extracts all bits lower than (not including) the given target-bit from the source integer. [eg: (10111, 100) => 00011]
template <integer_like int_t>
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t extract_bits_lower_than(const int_t value, const int_t target_bit) noexcept
{
	return value & (target_bit - int_t{1});
}
static_assert(extract_bits_lower_than<std::uint32_t>(0b10111, 0b00100) == 0b00011);
static_assert(extract_bits_lower_than<std::uint32_t>(0b10111, 0b00010) == 0b00001);

/// @brief Extracts all bits higher than (not including) the given target-bit from the source integer. [eg: (10111, 001) => 10110]
template <integer_like int_t>
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t extract_bits_higher_than(const int_t value, const int_t target_bit) noexcept
{
	using Bmi::andn;
	using Bmi::blsmsk;
	return andn(blsmsk(target_bit), value);
}
static_assert(extract_bits_higher_than<std::uint32_t>(0b10111, 0b00100) == 0b10000);
static_assert(extract_bits_higher_than<std::uint32_t>(0b10111, 0b00010) == 0b10100);
static_assert(extract_bits_higher_than<std::uint32_t>(0b10111, 0b00001) == 0b10110);

#pragma endregion

#pragma region Bit Extract
namespace Detail
{
template <std::integral int_t>
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr int_t portable_bextr(const int_t source, const unsigned start, const unsigned len) noexcept
{
	using unsigned_type = unsigned_t<int_t>;
	constexpr unsigned bit_count = static_cast<unsigned>(sizeof(int_t) * 8u);
	if (len == 0 || start >= bit_count)
	{
		return int_t{0};
	}
	const unsigned extracted_count = len < (bit_count - start) ? len : (bit_count - start);
	const unsigned_type shifted = to_unsigned(source) >> start;
	if (extracted_count == bit_count)
	{
		return from_unsigned<int_t>(shifted);
	}
	const unsigned_type mask = static_cast<unsigned_type>((unsigned_type{1} << extracted_count) - unsigned_type{1});
	return from_unsigned<int_t>(shifted & mask);
}
} // namespace Detail

/// @brief Extract contiguous bits from source integer, and return them shifted to the LSB side of the output. Extract the number of bits specified by len,
/// starting at the bit specified by start.
template <std::integral int_t>
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr int_t bextr(const int_t source, const std::uint8_t len, const std::uint8_t start) noexcept
{
#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_BMI1
	if (!std::is_constant_evaluated())
	{
		#if SIMDLIB_TARGET_X64
		if constexpr (sizeof(int_t) == sizeof(std::uint64_t))
		{
			return static_cast<int_t>(_bextr_u64(static_cast<std::uint64_t>(source), start, len));
		}
		else
		#endif
		if constexpr (sizeof(int_t) == sizeof(std::uint32_t))
		{
			return static_cast<int_t>(_bextr_u32(static_cast<std::uint32_t>(source), start, len));
		}
	}
#endif
	return Detail::portable_bextr(source, start, len);
}

/// @brief Extract contiguous bits from source integer, and return them shifted to the LSB side of the output. Extract the number of bits specified by len,
/// starting at the bit specified by start.
template <std::integral int_t, std::size_t len> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr int_t bextr(const int_t source, const std::uint8_t start) noexcept
{
	static_assert(len <= 255, "BMI bit-extract length must fit the intrinsic control field");
	return bextr(source, static_cast<std::uint8_t>(len), start);
}

/// @brief Extract contiguous bits from source integer, and return them shifted to the LSB side of the output. Extract the number of bits specified by len,
/// starting at the bit specified by start.
template <std::integral int_t, std::size_t start, std::size_t len> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr int_t bextr(const int_t source) noexcept
{
	static_assert(start <= 255 && len <= 255, "BMI bit-extract controls must fit the intrinsic control fields");
	return bextr(source, static_cast<std::uint8_t>(len), static_cast<std::uint8_t>(start));
}
#pragma endregion

#pragma region PDEP
/**
 * @brief Performs a software-emulated parallel bit deposit for the width of int_t. [eg: _pdep_emulator<uint8_t>(0b101, 0b01010100) => 0b01000100]
 *
 * Deposits the lowest popcount(mask) source bits into the set-bit positions of mask in ascending bit-index order. All unselected result bits are zero.
 *
 * @tparam int_t The integral source, mask, and result type.
 * @param source The packed source bits.
 * @param mask The destination bit positions.
 * @return The deposited bit pattern.
 */
template <std::integral int_t> SIMDLIB_FORCE_INLINE constexpr static int_t _pdep_emulator(int_t source, int_t mask) noexcept
{
	using unsigned_type = std::make_unsigned_t<int_t>;
	constexpr unsigned int_width = static_cast<unsigned>(sizeof(int_t) * 8u);
	const unsigned_type unsigned_source = static_cast<unsigned_type>(source);
	const unsigned_type unsigned_mask = static_cast<unsigned_type>(mask);
	unsigned_type result = 0;
	unsigned bit_index = 0;

	// Iterate over each bit of the mask (m)
	for (unsigned m = 0; m < int_width; ++m)
	{
		// Check if the mask bit at position m is set
		if ((unsigned_mask & (unsigned_type{1} << m)) != 0)
		{
			// Deposit the bit from the source into the corresponding position in result
			if ((unsigned_source & (unsigned_type{1} << bit_index)) != 0)
			{
				result |= (unsigned_type{1} << m);
			}
			++bit_index; // Move to the next bit in the source
		}
	}

	return static_cast<int_t>(result);
}

/// @brief Note: This is a wrapper for the '_pdep_xxx' intrinsic providing compile-time emulation.
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::uint32_t pdep_u32(std::uint32_t source, std::uint32_t mask) noexcept
{
#if SIMDLIB_TARGET_X64 && SIMDLIB_HAS_BMI2
	if (!std::is_constant_evaluated())
		return _pdep_u32(source, mask);
#endif
	return _pdep_emulator<std::uint32_t>(source, mask);
}

/// @brief Note: This is a wrapper for the '_pdep_xxx' intrinsic providing compile-time emulation.
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::uint64_t pdep_u64(std::uint64_t source, std::uint64_t mask) noexcept
{
#if SIMDLIB_TARGET_X64 && SIMDLIB_HAS_BMI2
	if (!std::is_constant_evaluated())
		return _pdep_u64(source, mask);
#endif
	return _pdep_emulator<std::uint64_t>(source, mask);
}

/// @brief This is a "pdep, but from right (high-bits) to left (low-bits)" aka "expand left"
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::uint32_t pdepl_u32(std::uint32_t source, std::uint32_t mask) noexcept
{
	return pdep_u32(source >> (std::popcount(~mask) & 31), mask);
}

/// @brief This is a "pdep, but from right (high-bits) to left (low-bits)" aka "expand left"
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::uint64_t pdepl_u64(std::uint64_t source, std::uint64_t mask) noexcept
{
	return pdep_u64(source >> (std::popcount(~mask) & 63), mask);
}

#pragma endregion

#pragma region PEXT
template <std::integral int_t> SIMDLIB_FORCE_INLINE constexpr static int_t _pext_emulator(int_t source, int_t mask) noexcept
{
	using unsigned_type = std::make_unsigned_t<int_t>;
	constexpr unsigned int_width = static_cast<unsigned>(sizeof(int_t) * 8u);
	const unsigned_type unsigned_source = static_cast<unsigned_type>(source);
	const unsigned_type unsigned_mask = static_cast<unsigned_type>(mask);
	unsigned_type result = 0;
	unsigned bit_index = 0;

	// Iterate over each bit position in the mask (m)
	for (unsigned m = 0; m < int_width; ++m)
	{
		// Check if the mask bit at position m is set
		if ((unsigned_mask & (unsigned_type{1} << m)) != 0)
		{
			// Extract the bit from source at position m and place it in result at bit_index position
			if ((unsigned_source & (unsigned_type{1} << m)) != 0)
			{
				result |= (unsigned_type{1} << bit_index);
			}
			++bit_index; // Move to the next bit position in result
		}
	}

	return static_cast<int_t>(result);
}

/// @brief Note: This is a wrapper for the '_pext_xxx' intrinsic, providing compile-time emulation.
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::uint32_t pext_u32(std::uint32_t source, std::uint32_t mask) noexcept
{
#if SIMDLIB_TARGET_X64 && SIMDLIB_HAS_BMI2
	if (!std::is_constant_evaluated())
		return _pext_u32(source, mask);
#endif
	return _pext_emulator<std::uint32_t>(source, mask);
}

/// @brief Note: This is a wrapper for the '_pext_xxx' intrinsic, providing compile-time emulation.
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::uint64_t pext_u64(std::uint64_t source, std::uint64_t mask) noexcept
{
#if SIMDLIB_TARGET_X64 && SIMDLIB_HAS_BMI2
	if (!std::is_constant_evaluated())
		return _pext_u64(source, mask);
#endif
	return _pext_emulator<std::uint64_t>(source, mask);
}
#pragma endregion
} // namespace SimdLib::Bmi
