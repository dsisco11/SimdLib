#pragma once

#include <SimdLib/Api.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#ifndef SIMDLIB_API_PARTIAL_CODEGEN_WIDTH
#error "SIMDLIB_API_PARTIAL_CODEGEN_WIDTH must select the fixture register width"
#endif

namespace ApiPartialTransferCodegen
{

using api_t = SimdLib::Api<SIMDLIB_API_PARTIAL_CODEGEN_WIDTH, std::uint32_t>;
using vector_t = typename api_t::vector_t;
constexpr inline std::size_t active_count = api_t::element_count / 2;
constexpr inline std::size_t active_byte_count = api_t::byte_count / 2;
using partial_array_t = std::array<std::uint32_t, active_count>;

/** @brief Broadcasts one scalar into exactly three active lanes and zero-fills the suffix. */
extern "C" [[nodiscard]] vector_t simdlib_api_partial_codegen_broadcast(std::uint32_t value) noexcept
{
#if SIMDLIB_CODEGEN_USE_API
	return api_t::template broadcast_partial<3>(value);
#else
	return api_t::setr_partial(value, value, value);
#endif
}

/** @brief Loads the exact low half of one register and zero-fills its high half. */
extern "C" [[nodiscard]] vector_t simdlib_api_partial_codegen_load(const std::uint32_t *source) noexcept
{
#if SIMDLIB_CODEGEN_USE_API
	return api_t::template load_partial<active_count>(std::span<const std::uint32_t>{source, active_count});
#elif SIMDLIB_API_PARTIAL_CODEGEN_WIDTH == 128
	return _mm_loadl_epi64(reinterpret_cast<const __m128i *>(source));
#else
	return _mm256_inserti128_si256(_mm256_setzero_si256(), _mm_loadu_si128(reinterpret_cast<const __m128i *>(source)), 0);
#endif
}

/** @brief Loads the exact aligned low half of one register and zero-fills its high half. */
extern "C" [[nodiscard]] vector_t simdlib_api_partial_codegen_load_aligned(const std::uint32_t *source) noexcept
{
#if SIMDLIB_CODEGEN_USE_API
	return api_t::template load_partial_aligned<active_count>(std::span<const std::uint32_t>{source, active_count});
#elif SIMDLIB_API_PARTIAL_CODEGEN_WIDTH == 128
	return _mm_loadl_epi64(reinterpret_cast<const __m128i *>(source));
#else
	return _mm256_inserti128_si256(_mm256_setzero_si256(), _mm_load_si128(reinterpret_cast<const __m128i *>(source)), 0);
#endif
}

/** @brief Loads one full aligned register through the partial-load API's complete-width branch. */
extern "C" [[nodiscard]] vector_t simdlib_api_partial_codegen_load_aligned_full(const std::uint32_t *source) noexcept
{
#if SIMDLIB_CODEGEN_USE_API
	return api_t::template load_partial_aligned<api_t::element_count>(
		std::span<const std::uint32_t>{source, api_t::element_count});
#elif SIMDLIB_API_PARTIAL_CODEGEN_WIDTH == 128
	return _mm_load_si128(reinterpret_cast<const __m128i *>(source));
#else
	return _mm256_load_si256(reinterpret_cast<const __m256i *>(source));
#endif
}

/** @brief Loads the exact low half of one register's byte representation. */
extern "C" [[nodiscard]] vector_t simdlib_api_partial_codegen_load_bytes(const std::byte *source) noexcept
{
#if SIMDLIB_CODEGEN_USE_API
	return api_t::template load_bytes_partial<active_byte_count>(std::span<const std::byte>{source, active_byte_count});
#elif SIMDLIB_API_PARTIAL_CODEGEN_WIDTH == 128
	return _mm_loadl_epi64(reinterpret_cast<const __m128i *>(source));
#else
	return _mm256_inserti128_si256(_mm256_setzero_si256(), _mm_loadu_si128(reinterpret_cast<const __m128i *>(source)), 0);
#endif
}

/** @brief Stores the exact low half of one register without touching its suffix. */
extern "C" void simdlib_api_partial_codegen_store(vector_t value, std::uint32_t *destination) noexcept
{
#if SIMDLIB_CODEGEN_USE_API
	api_t::template store_partial<active_count>(value, std::span<std::uint32_t>{destination, active_count});
#elif SIMDLIB_API_PARTIAL_CODEGEN_WIDTH == 128
	_mm_storel_epi64(reinterpret_cast<__m128i *>(destination), value);
#else
	_mm_storeu_si128(reinterpret_cast<__m128i *>(destination), _mm256_castsi256_si128(value));
#endif
}

/** @brief Stores the exact aligned low half of one register without touching its suffix. */
extern "C" void simdlib_api_partial_codegen_store_aligned(vector_t value, std::uint32_t *destination) noexcept
{
#if SIMDLIB_CODEGEN_USE_API
	api_t::template store_partial_aligned<active_count>(value, std::span<std::uint32_t>{destination, active_count});
#elif SIMDLIB_API_PARTIAL_CODEGEN_WIDTH == 128
	_mm_storel_epi64(reinterpret_cast<__m128i *>(destination), value);
#else
	_mm_store_si128(reinterpret_cast<__m128i *>(destination), _mm256_castsi256_si128(value));
#endif
}

/** @brief Stores one full aligned register through the partial-store API's complete-width branch. */
extern "C" void simdlib_api_partial_codegen_store_aligned_full(vector_t value, std::uint32_t *destination) noexcept
{
#if SIMDLIB_CODEGEN_USE_API
	api_t::template store_partial_aligned<api_t::element_count>(
		value, std::span<std::uint32_t>{destination, api_t::element_count});
#elif SIMDLIB_API_PARTIAL_CODEGEN_WIDTH == 128
	_mm_store_si128(reinterpret_cast<__m128i *>(destination), value);
#else
	_mm256_store_si256(reinterpret_cast<__m256i *>(destination), value);
#endif
}

/** @brief Stores the exact low half of one register's byte representation. */
extern "C" void simdlib_api_partial_codegen_store_bytes(vector_t value, std::byte *destination) noexcept
{
#if SIMDLIB_CODEGEN_USE_API
	api_t::template store_bytes_partial<active_byte_count>(value, std::span<std::byte>{destination, active_byte_count});
#elif SIMDLIB_API_PARTIAL_CODEGEN_WIDTH == 128
	_mm_storel_epi64(reinterpret_cast<__m128i *>(destination), value);
#else
	_mm_storeu_si128(reinterpret_cast<__m128i *>(destination), _mm256_castsi256_si128(value));
#endif
}

/** @brief Returns exactly the low half of one register after observing it as an array. */
[[nodiscard]] partial_array_t simdlib_api_partial_codegen_to_array(vector_t value) noexcept
{
#if SIMDLIB_CODEGEN_USE_API
	return api_t::template to_array_partial<active_count>(value);
#else
	partial_array_t result{};
#if SIMDLIB_API_PARTIAL_CODEGEN_WIDTH == 128
	_mm_storel_epi64(reinterpret_cast<__m128i *>(result.data()), value);
#else
	_mm_storeu_si128(reinterpret_cast<__m128i *>(result.data()), _mm256_castsi256_si128(value));
#endif
	return result;
#endif
}

} // namespace ApiPartialTransferCodegen
