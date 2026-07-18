#pragma once

#include <SimdLib/Config.h>
#include <SimdLib/Api.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

namespace SimdLib::SimdResample
{
/// Resamples packed-bit masks where each byte represents eight logical elements.
///
/// Reduce operations require `src.size() == dst.size() * 8`. Output bit `i`
/// corresponds to input byte `src[group * 8 + i]`. Expand requires
/// `dst.size() == src.size() * 8` and maps each input bit to `0xFF` or `0x00`.

/// Packs one bit per source byte, set when the byte is nonzero.
inline void ReduceBytesToBitsBy8_Any(
    const std::span<const std::uint8_t> src,
    const std::span<std::uint8_t> dst) noexcept
{
	SIMDLIB_PRECONDITION(src.size() == dst.size() * 8, "ReduceBytesToBitsBy8_Any requires src.size() == dst.size() * 8");

#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_SSE42
	using U8x16 = Api<128, std::uint8_t>;
	const auto zero = U8x16::setzero();
	const std::size_t pairCount = dst.size() / 2;
	for (std::size_t pair = 0; pair < pairCount; ++pair)
	{
		const auto value = U8x16::load_unaligned(std::span<const std::uint8_t, 16>(src.data() + pair * 16, 16));
		const auto equalZero = U8x16::cmpeq(value, zero);
		const auto mask = static_cast<std::uint16_t>(~U8x16::movemask(equalZero));
		dst[pair * 2] = static_cast<std::uint8_t>(mask);
		dst[pair * 2 + 1] = static_cast<std::uint8_t>(mask >> 8);
	}
	if ((dst.size() & 1u) != 0)
	{
		const auto value = U8x16::load_half(src.data() + pairCount * 16);
		const auto equalZero = U8x16::cmpeq(value, zero);
		dst.back() = static_cast<std::uint8_t>(~U8x16::movemask(equalZero));
	}
#else
	for (std::size_t group = 0; group < dst.size(); ++group)
	{
		std::uint8_t result = 0;
		for (std::size_t lane = 0; lane < 8; ++lane)
			result |= static_cast<std::uint8_t>((src[group * 8 + lane] != 0 ? 1u : 0u) << lane);
		dst[group] = result;
	}
#endif
}

/// Packs one bit per source byte, set when the byte is exactly `0xFF`.
inline void ReduceBytesToBitsBy8_All(
    const std::span<const std::uint8_t> src,
    const std::span<std::uint8_t> dst) noexcept
{
	SIMDLIB_PRECONDITION(src.size() == dst.size() * 8, "ReduceBytesToBitsBy8_All requires src.size() == dst.size() * 8");

#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_SSE42
	using U8x16 = Api<128, std::uint8_t>;
	const auto allOnes = U8x16::set1(std::uint8_t{0xFF});
	const std::size_t pairCount = dst.size() / 2;
	for (std::size_t pair = 0; pair < pairCount; ++pair)
	{
		const auto value = U8x16::load_unaligned(std::span<const std::uint8_t, 16>(src.data() + pair * 16, 16));
		const auto mask = static_cast<std::uint16_t>(U8x16::movemask(U8x16::cmpeq(value, allOnes)));
		dst[pair * 2] = static_cast<std::uint8_t>(mask);
		dst[pair * 2 + 1] = static_cast<std::uint8_t>(mask >> 8);
	}
	if ((dst.size() & 1u) != 0)
	{
		const auto value = U8x16::load_half(src.data() + pairCount * 16);
		dst.back() = static_cast<std::uint8_t>(U8x16::movemask(U8x16::cmpeq(value, allOnes)));
	}
#else
	for (std::size_t group = 0; group < dst.size(); ++group)
	{
		std::uint8_t result = 0;
		for (std::size_t lane = 0; lane < 8; ++lane)
			result |= static_cast<std::uint8_t>((src[group * 8 + lane] == 0xFF ? 1u : 0u) << lane);
		dst[group] = result;
	}
#endif
}

/// Packs one bit per source byte, set when the byte has odd parity.
inline void ReduceBytesToBitsBy8_Parity(
    const std::span<const std::uint8_t> src,
    const std::span<std::uint8_t> dst) noexcept
{
	SIMDLIB_PRECONDITION(src.size() == dst.size() * 8, "ReduceBytesToBitsBy8_Parity requires src.size() == dst.size() * 8");

#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_SSE42
	using U8x16 = Api<128, std::uint8_t>;
	using U16x8 = Api<128, std::uint16_t>;
	const auto lowNibbleMask = U8x16::set1(std::uint8_t{0x0F});
	const auto one = U8x16::set1(std::uint8_t{1});
	const auto parityLut = U8x16::setr(
	    std::uint8_t{0}, std::uint8_t{1}, std::uint8_t{1}, std::uint8_t{0},
	    std::uint8_t{1}, std::uint8_t{0}, std::uint8_t{0}, std::uint8_t{1},
	    std::uint8_t{1}, std::uint8_t{0}, std::uint8_t{0}, std::uint8_t{1},
	    std::uint8_t{0}, std::uint8_t{1}, std::uint8_t{1}, std::uint8_t{0});

	const auto parityMask = [&](const typename U8x16::vector_t value) noexcept
	{
		const auto low = U8x16::bitwise_and(value, lowNibbleMask);
		const auto high = U8x16::bitwise_and(U16x8::shift_right(value, 4), lowNibbleMask);
		const auto parity = U8x16::bitwise_xor(U8x16::shuffle(parityLut, low), U8x16::shuffle(parityLut, high));
		return U8x16::movemask(U8x16::cmpeq(parity, one));
	};

	const std::size_t pairCount = dst.size() / 2;
	for (std::size_t pair = 0; pair < pairCount; ++pair)
	{
		const auto value = U8x16::load_unaligned(std::span<const std::uint8_t, 16>(src.data() + pair * 16, 16));
		const auto mask = static_cast<std::uint16_t>(parityMask(value));
		dst[pair * 2] = static_cast<std::uint8_t>(mask);
		dst[pair * 2 + 1] = static_cast<std::uint8_t>(mask >> 8);
	}
	if ((dst.size() & 1u) != 0)
		dst.back() = static_cast<std::uint8_t>(parityMask(U8x16::load_half(src.data() + pairCount * 16)));
#else
	for (std::size_t group = 0; group < dst.size(); ++group)
	{
		std::uint8_t result = 0;
		for (std::size_t lane = 0; lane < 8; ++lane)
		{
			const auto parity = std::popcount(static_cast<unsigned>(src[group * 8 + lane])) & 1;
			result |= static_cast<std::uint8_t>(parity << lane);
		}
		dst[group] = result;
	}
#endif
}

/// Expands each packed source bit to one byte (`1 -> 0xFF`, `0 -> 0x00`).
inline void ExpandBitsToBytesBy8(
    const std::span<const std::uint8_t> src,
    const std::span<std::uint8_t> dst) noexcept
{
	SIMDLIB_PRECONDITION(dst.size() == src.size() * 8, "ExpandBitsToBytesBy8 requires dst.size() == src.size() * 8");

#if SIMDLIB_TARGET_X86 && SIMDLIB_HAS_SSE42
	using U8x16 = Api<128, std::uint8_t>;
	const auto zero = U8x16::setzero();
	const auto allOnes = U8x16::set1(std::uint8_t{0xFF});
	const auto laneMasks = U8x16::setr(
	    std::uint8_t{1}, std::uint8_t{2}, std::uint8_t{4}, std::uint8_t{8},
	    std::uint8_t{16}, std::uint8_t{32}, std::uint8_t{64}, std::uint8_t{0x80},
	    std::uint8_t{1}, std::uint8_t{2}, std::uint8_t{4}, std::uint8_t{8},
	    std::uint8_t{16}, std::uint8_t{32}, std::uint8_t{64}, std::uint8_t{0x80});
	const auto expandPair = [&](const std::uint8_t low, const std::uint8_t high) noexcept
	{
		const auto bits = U8x16::setr(
		    low, low, low, low, low, low, low, low,
		    high, high, high, high, high, high, high, high);
		const auto equalZero = U8x16::cmpeq(U8x16::bitwise_and(bits, laneMasks), zero);
		return U8x16::bitwise_andnot(equalZero, allOnes);
	};

	const std::size_t pairCount = src.size() / 2;
	for (std::size_t pair = 0; pair < pairCount; ++pair)
		U8x16::store_unaligned(
		    expandPair(src[pair * 2], src[pair * 2 + 1]),
		    std::span<std::uint8_t, 16>(dst.data() + pair * 16, 16));
	if ((src.size() & 1u) != 0)
		U8x16::store_half(expandPair(src.back(), 0), dst.data() + pairCount * 16);
#else
	for (std::size_t group = 0; group < src.size(); ++group)
		for (std::size_t lane = 0; lane < 8; ++lane)
			dst[group * 8 + lane] = (src[group] & (1u << lane)) != 0 ? 0xFF : 0;
#endif
}
} // namespace SimdLib::SimdResample
