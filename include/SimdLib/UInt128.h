#pragma once

#include <SimdLib/Bmi.h>
#include <SimdLib/Config.h>
#include <SimdLib/Api.h>

#include <array>
#include <bit>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <span>
#include <type_traits>

#if SIMDLIB_COMPILER_MSVC && defined(_M_X64)
#include <intrin.h>
#endif

#ifndef SIMDLIB_USE_COMPILER_CARRY_INTRINSICS
#define SIMDLIB_USE_COMPILER_CARRY_INTRINSICS 1
#endif

namespace SimdLib
{
/** @brief Unsigned 128-bit integer stored as low and high 64-bit words. */
class uint128_t final
{
  private:
	alignas(16) std::array<std::uint64_t, 2> m_data{0, 0};

	template <class>
	struct simd_element
	{
		using type = std::uint64_t;
	};

	/** @brief Internal facade used whenever the 128-bit SIMD backend is available. */
	template <class Dependency = void>
	using simd = Api<128, typename simd_element<Dependency>::type>;

	template <class Dependency = void>
	inline static constexpr bool simd_available =
		is_api_available_v<128, typename simd_element<Dependency>::type>;

  public:
	using block_t = std::uint64_t;
	inline static constexpr std::size_t block_count = 2;
	inline static constexpr std::size_t block_width = std::numeric_limits<block_t>::digits;

	constexpr uint128_t() noexcept = default;
	constexpr uint128_t(const uint128_t&) noexcept = default;
	constexpr uint128_t(uint128_t&&) noexcept = default;
	constexpr uint128_t& operator=(const uint128_t&) noexcept = default;
	constexpr uint128_t& operator=(uint128_t&&) noexcept = default;
	constexpr ~uint128_t() = default;

	/** @brief Constructs a value from low and high words, in that order. */
	constexpr uint128_t(const std::uint64_t lower, const std::uint64_t upper) noexcept
		: m_data{lower, upper}
	{
	}

	template <std::integral T>
	constexpr uint128_t(const T value) noexcept
		: m_data{static_cast<std::uint64_t>(value), 0}
	{
	}

	constexpr uint128_t(const bool value) noexcept
		: m_data{static_cast<std::uint64_t>(value), 0}
	{
	}

	/** @brief Loads the stored words into a backend register through Api. */
	template <class Dependency = void>
		  requires(simd_available<Dependency>)
	[[nodiscard]] auto to_register() const noexcept -> typename simd<Dependency>::vector_t
	{
		return simd<Dependency>::load_aligned(std::span<const std::uint64_t, block_count>{m_data});
	}

	/** @brief Constructs a value by extracting both words from a backend register through Api. */
	template <class Dependency = void>
		  requires(simd_available<Dependency>)
	[[nodiscard]] static uint128_t from_register(const typename simd<Dependency>::vector_t value) noexcept
	{
		return uint128_t(
			static_cast<std::uint64_t>(simd<Dependency>::template extract<0>(value)),
			static_cast<std::uint64_t>(simd<Dependency>::template extract<1>(value)));
	}

	[[nodiscard]] constexpr uint128_t operator+(const uint128_t& rhs) const noexcept
	{
		const auto lowResult = add_with_carry(m_data[0], rhs.m_data[0]);
		const auto highResult = add_with_carry(m_data[1], rhs.m_data[1], lowResult.carry);
		return uint128_t(lowResult.value, highResult.value);
	}

	[[nodiscard]] constexpr uint128_t operator-(const uint128_t& rhs) const noexcept
	{
		const auto lowResult = subtract_with_borrow(m_data[0], rhs.m_data[0]);
		const auto highResult = subtract_with_borrow(m_data[1], rhs.m_data[1], lowResult.borrow);
		return uint128_t(lowResult.value, highResult.value);
	}

	constexpr uint128_t& operator+=(const uint128_t& rhs) noexcept
	{
		return *this = *this + rhs;
	}

	constexpr uint128_t& operator-=(const uint128_t& rhs) noexcept
	{
		return *this = *this - rhs;
	}

	[[nodiscard]] constexpr bool operator==(const uint128_t& rhs) const noexcept = default;

	[[nodiscard]] constexpr std::strong_ordering operator<=>(const uint128_t& rhs) const noexcept
	{
		if (m_data[1] != rhs.m_data[1])
		{
			return m_data[1] <=> rhs.m_data[1];
		}
		return m_data[0] <=> rhs.m_data[0];
	}

	template <std::integral T>
		  requires(std::numeric_limits<T>::digits <= 64)
	[[nodiscard]] constexpr bool operator==(const T rhs) const noexcept
	{
		if constexpr (std::is_signed_v<T>)
		{
			return rhs >= 0 && m_data[1] == 0 && m_data[0] == static_cast<std::uint64_t>(rhs);
		}
		return m_data[1] == 0 && m_data[0] == static_cast<std::uint64_t>(rhs);
	}

	template <std::integral T>
		  requires(std::numeric_limits<T>::digits <= 64)
	[[nodiscard]] constexpr std::strong_ordering operator<=>(const T rhs) const noexcept
	{
		if constexpr (std::is_signed_v<T>)
		{
			if (rhs < 0)
			{
				return std::strong_ordering::greater;
			}
		}
		return *this <=> uint128_t(static_cast<std::uint64_t>(rhs));
	}

	[[nodiscard]] constexpr uint128_t operator&(const uint128_t& rhs) const noexcept
	{
#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_SSE42
		if (!std::is_constant_evaluated())
		{
			return simd_bitwise_binary<0>(rhs);
		}
#endif
		return uint128_t(m_data[0] & rhs.m_data[0], m_data[1] & rhs.m_data[1]);
	}

	[[nodiscard]] constexpr uint128_t operator|(const uint128_t& rhs) const noexcept
	{
#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_SSE42
		if (!std::is_constant_evaluated())
		{
			return simd_bitwise_binary<1>(rhs);
		}
#endif
		return uint128_t(m_data[0] | rhs.m_data[0], m_data[1] | rhs.m_data[1]);
	}

	[[nodiscard]] constexpr uint128_t operator^(const uint128_t& rhs) const noexcept
	{
#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_SSE42
		if (!std::is_constant_evaluated())
		{
			return simd_bitwise_binary<2>(rhs);
		}
#endif
		return uint128_t(m_data[0] ^ rhs.m_data[0], m_data[1] ^ rhs.m_data[1]);
	}

	[[nodiscard]] constexpr uint128_t operator~() const noexcept
	{
#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_SSE42
		if (!std::is_constant_evaluated())
		{
			return simd_bitwise_not();
		}
#endif
		return uint128_t(~m_data[0], ~m_data[1]);
	}

	constexpr uint128_t& operator&=(const uint128_t& rhs) noexcept
	{
		return *this = *this & rhs;
	}

	constexpr uint128_t& operator|=(const uint128_t& rhs) noexcept
	{
		return *this = *this | rhs;
	}

	constexpr uint128_t& operator^=(const uint128_t& rhs) noexcept
	{
		return *this = *this ^ rhs;
	}

	/** @brief Extracts a contiguous bit range and shifts it to bit zero. */
	[[deprecated("Prefer SimdLib::Bmi::bextr")]]
	[[nodiscard]] constexpr uint128_t extract(const std::uint8_t len, const std::uint8_t start) const noexcept
	{
		if (len == 0 || start >= 128)
		{
			return {};
		}
		const unsigned retained = static_cast<unsigned>(len) > 128u - start ? 128u - start : len;
		return (*this >> start) & create_mask(static_cast<int>(retained));
	}

	template <std::size_t len>
		  requires(len <= 64)
	[[deprecated("Prefer SimdLib::Bmi::bextr")]]
	[[nodiscard]] constexpr std::uint64_t extract(const std::uint8_t start) const noexcept
	{
		return static_cast<std::uint64_t>(extract(static_cast<std::uint8_t>(len), start));
	}

	template <std::size_t start, std::size_t len>
		  requires(start <= 128 && len <= 128)
	[[deprecated("Prefer SimdLib::Bmi::bextr")]]
	[[nodiscard]] constexpr uint128_t extract() const noexcept
	{
		return extract(static_cast<std::uint8_t>(len), static_cast<std::uint8_t>(start));
	}

	/** @brief Computes the absolute difference between two unsigned 128-bit values. */
	[[nodiscard]] constexpr uint128_t abs_diff(const uint128_t& other) const noexcept
	{
		return *this > other ? *this - other : other - *this;
	}

	/** @brief Creates a mask containing `bitCount` low one bits. */
	[[nodiscard]] static constexpr uint128_t create_mask(const int bitCount) noexcept
	{
		if (bitCount <= 0)
		{
			return {};
		}
		if (bitCount >= 128)
		{
			return uint128_t(~std::uint64_t{0}, ~std::uint64_t{0});
		}
		if (bitCount >= 64)
		{
			const int highBits = bitCount - 64;
			return uint128_t(~std::uint64_t{0}, highBits == 0 ? 0 : (std::uint64_t{1} << highBits) - 1);
		}
		return uint128_t((std::uint64_t{1} << bitCount) - 1, 0);
	}

	template <int width>
	[[nodiscard]] static constexpr uint128_t create_mask(const int offset) noexcept
	{
		static_assert(width >= 0 && width <= 128);
		if constexpr (width == 0)
		{
			return {};
		}
		if (offset <= 0)
		{
			return create_mask(width);
		}
		if (offset >= 128)
		{
			return {};
		}
		return create_mask(width) << offset;
	}

	/** @brief Whole-value left shift. Negative counts are treated as zero; counts of 128 or more produce zero. */
	template <std::integral T>
	[[nodiscard]] constexpr uint128_t operator<<(const T count) const noexcept
	{
		uint128_t result(*this);
		result.shift_left(normalize_shift(count));
		return result;
	}

	/** @brief Whole-value right shift. Negative counts are treated as zero; counts of 128 or more produce zero. */
	template <std::integral T>
	[[nodiscard]] constexpr uint128_t operator>>(const T count) const noexcept
	{
		uint128_t result(*this);
		result.shift_right(normalize_shift(count));
		return result;
	}

	template <std::integral T>
	constexpr uint128_t& operator<<=(const T count) noexcept
	{
		shift_left(normalize_shift(count));
		return *this;
	}

	template <std::integral T>
	constexpr uint128_t& operator>>=(const T count) noexcept
	{
		shift_right(normalize_shift(count));
		return *this;
	}

	[[nodiscard]] constexpr uint128_t operator-() const noexcept
	{
		return uint128_t{} - *this;
	}

	constexpr uint128_t& operator++() noexcept
	{
		return *this += uint128_t{1};
	}

	constexpr uint128_t& operator--() noexcept
	{
		return *this -= uint128_t{1};
	}

	constexpr uint128_t operator++(int) noexcept
	{
		const uint128_t previous(*this);
		++*this;
		return previous;
	}

	constexpr uint128_t operator--(int) noexcept
	{
		const uint128_t previous(*this);
		--*this;
		return previous;
	}

	[[nodiscard]] constexpr std::uint64_t& low() noexcept { return m_data[0]; }
	[[nodiscard]] constexpr std::uint64_t& high() noexcept { return m_data[1]; }
	[[nodiscard]] constexpr std::uint64_t low() const noexcept { return m_data[0]; }
	[[nodiscard]] constexpr std::uint64_t high() const noexcept { return m_data[1]; }

	/** @brief Returns the backing word at index zero (low) or one (high). */
	[[nodiscard]] constexpr std::uint64_t getBlock(const int index) const noexcept
	{
		return m_data[static_cast<std::size_t>(index)];
	}

	template <std::integral T>
	[[nodiscard]] constexpr explicit operator T() const noexcept
	{
		return static_cast<T>(m_data[0]);
	}

	[[nodiscard]] constexpr explicit operator bool() const noexcept
	{
		return m_data[0] != 0 || m_data[1] != 0;
	}

  private:
	struct add_carry_result final
	{
		std::uint64_t value;
		bool carry;
	};

	struct subtract_borrow_result final
	{
		std::uint64_t value;
		bool borrow;
	};

	[[nodiscard]] static constexpr add_carry_result portable_add_with_carry(
		const std::uint64_t lhs,
		const std::uint64_t rhs,
		const bool carryIn = false) noexcept
	{
		const std::uint64_t partial = lhs + rhs;
		const bool firstCarry = partial < lhs;
		const std::uint64_t result = partial + static_cast<std::uint64_t>(carryIn);
		return {result, firstCarry || result < partial};
	}

	[[nodiscard]] static constexpr subtract_borrow_result portable_subtract_with_borrow(
		const std::uint64_t lhs,
		const std::uint64_t rhs,
		const bool borrowIn = false) noexcept
	{
		const std::uint64_t partial = lhs - rhs;
		const bool firstBorrow = lhs < rhs;
		const std::uint64_t result = partial - static_cast<std::uint64_t>(borrowIn);
		return {result, firstBorrow || partial < static_cast<std::uint64_t>(borrowIn)};
	}

	[[nodiscard]] static constexpr add_carry_result add_with_carry(
		const std::uint64_t lhs,
		const std::uint64_t rhs,
		const bool carryIn = false) noexcept
	{
#if SIMDLIB_USE_COMPILER_CARRY_INTRINSICS && SIMDLIB_COMPILER_MSVC && defined(_M_X64)
		if (!std::is_constant_evaluated())
		{
			std::uint64_t result = 0;
			const unsigned char carry = _addcarry_u64(
				static_cast<unsigned char>(carryIn), lhs, rhs, &result);
			return {result, carry != 0};
		}
#elif SIMDLIB_USE_COMPILER_CARRY_INTRINSICS && (SIMDLIB_COMPILER_CLANG || SIMDLIB_COMPILER_GCC)
		if (!std::is_constant_evaluated())
		{
			std::uint64_t partial = 0;
			std::uint64_t result = 0;
			const bool firstCarry = __builtin_add_overflow(lhs, rhs, &partial);
			const bool secondCarry = __builtin_add_overflow(
				partial, static_cast<std::uint64_t>(carryIn), &result);
			return {result, firstCarry || secondCarry};
		}
#endif
		return portable_add_with_carry(lhs, rhs, carryIn);
	}

	[[nodiscard]] static constexpr subtract_borrow_result subtract_with_borrow(
		const std::uint64_t lhs,
		const std::uint64_t rhs,
		const bool borrowIn = false) noexcept
	{
#if SIMDLIB_USE_COMPILER_CARRY_INTRINSICS && SIMDLIB_COMPILER_MSVC && defined(_M_X64)
		if (!std::is_constant_evaluated())
		{
			std::uint64_t result = 0;
			const unsigned char borrow = _subborrow_u64(
				static_cast<unsigned char>(borrowIn), lhs, rhs, &result);
			return {result, borrow != 0};
		}
#elif SIMDLIB_USE_COMPILER_CARRY_INTRINSICS && (SIMDLIB_COMPILER_CLANG || SIMDLIB_COMPILER_GCC)
		if (!std::is_constant_evaluated())
		{
			std::uint64_t partial = 0;
			std::uint64_t result = 0;
			const bool firstBorrow = __builtin_sub_overflow(lhs, rhs, &partial);
			const bool secondBorrow = __builtin_sub_overflow(
				partial, static_cast<std::uint64_t>(borrowIn), &result);
			return {result, firstBorrow || secondBorrow};
		}
#endif
		return portable_subtract_with_borrow(lhs, rhs, borrowIn);
	}

	template <std::integral T>
	[[nodiscard]] static constexpr int normalize_shift(const T count) noexcept
	{
		if constexpr (std::same_as<std::remove_cv_t<T>, bool>)
		{
			return count ? 1 : 0;
		}
		else
		{
			if constexpr (std::is_signed_v<T>)
			{
				if (count <= 0)
				{
					return 0;
				}
			}
			using unsigned_t = std::make_unsigned_t<T>;
			const unsigned_t unsignedCount = static_cast<unsigned_t>(count);
			return unsignedCount >= static_cast<unsigned_t>(128) ? 128 : static_cast<int>(unsignedCount);
		}
	}

	constexpr void shift_left(const int count) noexcept
	{
		if (count == 0)
		{
			return;
		}
		if (count >= 128)
		{
			m_data = {0, 0};
			return;
		}
#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_SSE42
		if (!std::is_constant_evaluated())
		{
			*this = simd_shift_left(count);
			return;
		}
#endif
		if (count >= 64)
		{
			m_data = {0, m_data[0] << (count - 64)};
			return;
		}
		m_data = {
			m_data[0] << count,
			(m_data[1] << count) | (m_data[0] >> (64 - count))};
	}

	constexpr void shift_right(const int count) noexcept
	{
		if (count == 0)
		{
			return;
		}
		if (count >= 128)
		{
			m_data = {0, 0};
			return;
		}
#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_SSE42
		if (!std::is_constant_evaluated())
		{
			*this = simd_shift_right(count);
			return;
		}
#endif
		if (count >= 64)
		{
			m_data = {m_data[1] >> (count - 64), 0};
			return;
		}
		m_data = {
			(m_data[0] >> count) | (m_data[1] << (64 - count)),
			m_data[1] >> count};
	}

	template <class Dependency = void>
		  requires(simd_available<Dependency>)
	[[nodiscard]] static uint128_t store_register(const typename simd<Dependency>::vector_t value) noexcept
	{
		uint128_t result;
		simd<Dependency>::store_aligned(value, std::span<std::uint64_t, block_count>{result.m_data});
		return result;
	}

	template <int operation, class Dependency = void>
		  requires(simd_available<Dependency>)
	[[nodiscard]] uint128_t simd_bitwise_binary(const uint128_t& rhs) const noexcept
	{
		const auto lhsRegister = to_register<Dependency>();
		const auto rhsRegister = rhs.template to_register<Dependency>();
		if constexpr (operation == 0)
		{
			return store_register<Dependency>(simd<Dependency>::bitwise_and(lhsRegister, rhsRegister));
		}
		else if constexpr (operation == 1)
		{
			return store_register<Dependency>(simd<Dependency>::bitwise_or(lhsRegister, rhsRegister));
		}
		else
		{
			return store_register<Dependency>(simd<Dependency>::bitwise_xor(lhsRegister, rhsRegister));
		}
	}

	template <class Dependency = void>
		  requires(simd_available<Dependency>)
	[[nodiscard]] uint128_t simd_bitwise_not() const noexcept
	{
		const auto value = simd<Dependency>::construct(m_data);
		return store_register<Dependency>(simd<Dependency>::bitwise_not(value));
	}

	template <class Dependency = void>
		  requires(simd_available<Dependency>)
	[[nodiscard]] uint128_t simd_shift_left(const int count) const noexcept
	{
		return store_register<Dependency>(simd<Dependency>::bit_shift_left(to_register<Dependency>(), count));
	}

	template <class Dependency = void>
		  requires(simd_available<Dependency>)
	[[nodiscard]] uint128_t simd_shift_right(const int count) const noexcept
	{
		return store_register<Dependency>(simd<Dependency>::bit_shift_right(to_register<Dependency>(), count));
	}
};

static_assert(sizeof(uint128_t) == 16);
static_assert(alignof(uint128_t) == 16);
static_assert(std::is_standard_layout_v<uint128_t>);
static_assert(std::is_trivially_copyable_v<uint128_t>);
} // namespace SimdLib

namespace std
{
template <>
class numeric_limits<SimdLib::uint128_t>
{
  public:
	static constexpr bool is_specialized = true;
	static constexpr int digits = 128;
	static constexpr int digits10 = 38;
	static constexpr int max_digits10 = 0;
	static constexpr bool is_signed = false;
	static constexpr bool is_integer = true;
	static constexpr bool is_exact = true;
	static constexpr int radix = 2;
	static constexpr int min_exponent = 0;
	static constexpr int min_exponent10 = 0;
	static constexpr int max_exponent = 0;
	static constexpr int max_exponent10 = 0;
	static constexpr bool has_infinity = false;
	static constexpr bool has_quiet_NaN = false;
	static constexpr bool has_signaling_NaN = false;
	static constexpr float_denorm_style has_denorm = denorm_absent;
	static constexpr bool has_denorm_loss = false;
	static constexpr bool is_iec559 = false;
	static constexpr bool is_bounded = true;
	static constexpr bool is_modulo = true;
	static constexpr bool traps = numeric_limits<std::uint64_t>::traps;
	static constexpr bool tinyness_before = false;
	static constexpr float_round_style round_style = round_toward_zero;

	[[nodiscard]] static constexpr SimdLib::uint128_t min() noexcept { return {}; }
	[[nodiscard]] static constexpr SimdLib::uint128_t lowest() noexcept { return {}; }
	[[nodiscard]] static constexpr SimdLib::uint128_t max() noexcept
	{
		return {numeric_limits<std::uint64_t>::max(), numeric_limits<std::uint64_t>::max()};
	}
	[[nodiscard]] static constexpr SimdLib::uint128_t epsilon() noexcept { return {}; }
	[[nodiscard]] static constexpr SimdLib::uint128_t round_error() noexcept { return {}; }
	[[nodiscard]] static constexpr SimdLib::uint128_t infinity() noexcept { return {}; }
	[[nodiscard]] static constexpr SimdLib::uint128_t quiet_NaN() noexcept { return {}; }
	[[nodiscard]] static constexpr SimdLib::uint128_t signaling_NaN() noexcept { return {}; }
	[[nodiscard]] static constexpr SimdLib::uint128_t denorm_min() noexcept { return {}; }
};

template <>
struct hash<SimdLib::uint128_t>
{
	[[nodiscard]] constexpr std::size_t operator()(const SimdLib::uint128_t& value) const noexcept
	{
		return static_cast<std::size_t>(value.low() ^ value.high());
	}
};
} // namespace std

namespace SimdLib::Bmi
{
/** @brief Extracts a contiguous bit range from a 128-bit value and shifts it to bit zero. */
[[nodiscard]] constexpr uint128_t bextr(
	const uint128_t value,
	const std::uint8_t len,
	const std::uint8_t start) noexcept
{
	if (len == 0 || start >= 128)
	{
		return {};
	}
	const unsigned retained = static_cast<unsigned>(len) > 128u - start ? 128u - start : len;
	return (value >> start) & uint128_t::create_mask(static_cast<int>(retained));
}

template <std::size_t len>
	requires(len <= 64)
[[nodiscard]] constexpr std::uint64_t bextr(const uint128_t value, const std::uint8_t start) noexcept
{
	return static_cast<std::uint64_t>(bextr(value, static_cast<std::uint8_t>(len), start));
}

template <std::size_t start, std::size_t len>
	requires(start <= 128 && len <= 128)
[[nodiscard]] constexpr uint128_t bextr(const uint128_t value) noexcept
{
	return bextr(value, static_cast<std::uint8_t>(len), static_cast<std::uint8_t>(start));
}
} // namespace SimdLib::Bmi

namespace SimdLib
{
/** @brief Constructs a 128-bit value from an unsigned long long literal. */
[[nodiscard]] constexpr uint128_t operator""_u128(const unsigned long long value) noexcept
{
	return uint128_t(value);
}

/** @brief Returns the number of one bits. */
[[nodiscard]] constexpr int popcount(const uint128_t value) noexcept
{
	return std::popcount(value.low()) + std::popcount(value.high());
}

/** @brief Returns the number of consecutive zero bits from the least-significant side. */
[[nodiscard]] constexpr int countr_zero(const uint128_t value) noexcept
{
	return value.low() == 0 ? 64 + std::countr_zero(value.high()) : std::countr_zero(value.low());
}

/** @brief Returns the number of consecutive one bits from the least-significant side. */
[[nodiscard]] constexpr int countr_one(const uint128_t value) noexcept
{
	return value.low() == std::numeric_limits<std::uint64_t>::max()
		? 64 + std::countr_one(value.high())
		: std::countr_one(value.low());
}

/** @brief Returns the number of consecutive zero bits from the most-significant side. */
[[nodiscard]] constexpr int countl_zero(const uint128_t value) noexcept
{
	return value.high() == 0 ? 64 + std::countl_zero(value.low()) : std::countl_zero(value.high());
}

/** @brief Returns the number of consecutive one bits from the most-significant side. */
[[nodiscard]] constexpr int countl_one(const uint128_t value) noexcept
{
	return value.high() == std::numeric_limits<std::uint64_t>::max()
		? 64 + std::countl_one(value.low())
		: std::countl_one(value.high());
}

/** @brief Returns the number of bits required to represent the value. */
[[nodiscard]] constexpr int bit_width(const uint128_t value) noexcept
{
	return value.high() == 0 ? std::bit_width(value.low()) : 64 + std::bit_width(value.high());
}

/** @brief Returns the smallest representable power of two not less than the value, or zero on overflow. */
[[nodiscard]] constexpr uint128_t bit_ceil(const uint128_t value) noexcept
{
	if (value <= uint128_t{1})
	{
		return uint128_t{1};
	}
	return uint128_t{1} << bit_width(value - uint128_t{1});
}

/** @brief Returns the greatest power of two not greater than the value. */
[[nodiscard]] constexpr uint128_t bit_floor(const uint128_t value) noexcept
{
	return value == uint128_t{} ? uint128_t{} : uint128_t{1} << (bit_width(value) - 1);
}

/** @brief Returns true when exactly one bit is set. */
[[nodiscard]] constexpr bool has_single_bit(const uint128_t value) noexcept
{
	return popcount(value) == 1;
}

static_assert(uint128_t{std::numeric_limits<std::uint64_t>::max(), 0} + uint128_t{1} == uint128_t{0, 1});
static_assert(uint128_t{0, 1} - uint128_t{1} == uint128_t{std::numeric_limits<std::uint64_t>::max(), 0});
static_assert((uint128_t{1} << 127) == uint128_t{0, std::uint64_t{1} << 63});
static_assert((uint128_t{1} << 128) == uint128_t{});
static_assert((uint128_t{0, std::uint64_t{1} << 63} >> 127) == uint128_t{1});
static_assert(popcount(std::numeric_limits<uint128_t>::max()) == 128);
} // namespace SimdLib
