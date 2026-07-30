#pragma once
#include <SimdLib/Config.h>
#include <SimdLib/Detail/Extensions.h>
#include <SimdLib/IImpl.h>
#include <SimdLib/TemplateTools.h>
#include <array>
#include <bit>
#include <cmath>
#include <concepts>
#if SIMDLIB_COMPILER_MSVC && SIMDLIB_TARGET_X86
#include <intrin.h>
#endif
#include <span>
#include <utility>

/*
 * This file contains SIMD operation abstractions for 128-bit & 256-bit register types across all integer and floating-point numeric types.
 * SEE: http://www.alfredklomp.com/programming/sse-intrinsics/
 * SEE: https://agner.org/optimize/optimizing_assembly.pdf
 * SEE: https://software.intel.com/sites/landingpage/IntrinsicsGuide/
 */
namespace SimdLib::Detail
{
/// <summary> Provides a common interface of standard SIMD method alias names for different integer types. </summary>
template <std::size_t width, class element_t>
	requires std::is_arithmetic_v<element_t>
struct SimdMappings
{
	constexpr static inline std::size_t register_width = width;
};

template <class...> constexpr static inline bool dependent_false_v = false;

template <class element_t>
	requires std::is_integral_v<element_t>
constexpr static inline std::size_t promoted_integer_width =
	(std::numeric_limits<element_t>::digits * 2) < 64 ? (std::numeric_limits<element_t>::digits * 2) : 64;

template <class element_t>
	requires std::is_integral_v<element_t>
using promoted_signed_t = SimdLib::select_signed_integer_t<promoted_integer_width<element_t>>;

template <class element_t>
	requires std::is_integral_v<element_t>
using promoted_unsigned_t = SimdLib::select_unsigned_integer_t<promoted_integer_width<element_t>>;

/**
 * @brief Rounds the square root of a 64-bit square sum and corrects the floating estimate exactly.
 *
 * @param total The nonnegative square sum.
 * @param maximum The greatest magnitude representable by the destination element type.
 * @return The nearest integer square root, saturated to `maximum`.
 */
std::uint64_t SIMD_FLAGS(Neither, RegisterOnly, ForceInline) magnitude_round_sqrt_u64(const std::uint64_t total, const std::uint64_t maximum) noexcept
{
	const __m128d totalValue = _mm_set_sd(static_cast<double>(total));
	const double root = _mm_cvtsd_f64(_mm_sqrt_sd(_mm_setzero_pd(), totalValue));
	std::uint64_t candidate = static_cast<std::uint64_t>(root + 0.5);
	if (candidate > maximum)
		candidate = maximum;
	if (candidate > 0 && total < candidate * candidate - candidate + 1)
		--candidate;
	if (candidate < maximum && total > candidate * candidate + candidate)
		++candidate;
	return candidate;
}

/**
 * @brief Packs one checked magnitude and its canonical overflow mask into the first two lanes.
 *
 * @tparam element_t The signed or unsigned integer lane type.
 * @param magnitude The saturated magnitude stored in lane zero.
 * @param overflow Whether lane one should contain an all-ones mask.
 * @return A native register whose remaining lanes are unspecified.
 */
template <class element_t>
	requires std::is_integral_v<element_t>
__m128i SIMD_FLAGS(Out, RegisterOnly, ForceInline) magnitude_checked_result(const std::uint64_t magnitude, const bool overflow) noexcept
{
	constexpr std::uint64_t laneMask = []() constexpr
	{
		if constexpr (sizeof(element_t) == 8)
			return ~std::uint64_t{0};
		else
			return (std::uint64_t{1} << (sizeof(element_t) * 8)) - 1;
	}();
	const std::uint64_t low = magnitude & laneMask;
	if constexpr (sizeof(element_t) == 8)
		return _mm_set_epi64x(overflow ? -1 : 0, static_cast<std::int64_t>(low));
	else
		return _mm_cvtsi64_si128(static_cast<std::int64_t>(low | ((overflow ? laneMask : 0) << (sizeof(element_t) * 8))));
}
/**
 * @brief Squares one unsigned 64-bit value into low and high 64-bit register lanes.
 *
 * @param value The unsigned scalar value.
 * @return A register containing the 128-bit product as `[low, high]`.
 */
__m128i SIMD_FLAGS(Out, RegisterOnly, ForceInline) magnitude_square_u64(const std::uint64_t value) noexcept
{
#if SIMDLIB_COMPILER_MSVC
	std::uint64_t high = 0;
	const std::uint64_t low = _umul128(value, value, &high);
	return _mm_set_epi64x(static_cast<std::int64_t>(high), static_cast<std::int64_t>(low));
#else
	const std::uint64_t lowHalf = static_cast<std::uint32_t>(value);
	const std::uint64_t highHalf = value >> 32;
	const std::uint64_t lowSquare = lowHalf * lowHalf;
	const std::uint64_t cross = highHalf * lowHalf;
	const std::uint64_t low = lowSquare + (cross << 33);
	const std::uint64_t high = highHalf * highHalf + (cross >> 31) + static_cast<std::uint64_t>(low < lowSquare);
	return _mm_set_epi64x(static_cast<std::int64_t>(high), static_cast<std::int64_t>(low));
#endif
}

/**
 * @brief Converts an exact 128-bit square sum into a rounded, bounded 64-bit magnitude.
 *
 * @param low The low 64 bits of the square sum.
 * @param high The high 64 bits of the square sum.
 * @param maximum The greatest representable destination magnitude.
 * @return The floating estimate rounded to the nearest integer and bounded by `maximum`.
 */
std::uint64_t SIMD_FLAGS(Neither, RegisterOnly, ForceInline)
	magnitude_round_sqrt_u128(const std::uint64_t low, const std::uint64_t high, const std::uint64_t maximum) noexcept
{
	constexpr double twoTo64 = 18'446'744'073'709'551'616.0;
	constexpr double twoTo63 = 9'223'372'036'854'775'808.0;
	const __m128d highValue = _mm_set_sd(static_cast<double>(high));
	const __m128d lowValue = _mm_set_sd(static_cast<double>(low));
	const __m128d total = _mm_add_sd(_mm_mul_sd(highValue, _mm_set_sd(twoTo64)), lowValue);
	const double root = _mm_cvtsd_f64(_mm_sqrt_sd(_mm_setzero_pd(), total));
	const double maximumAsDouble = static_cast<double>(maximum);
	if (root >= maximumAsDouble)
		return maximum;
	const double rounded = root + 0.5;
	if (rounded >= maximumAsDouble)
		return maximum;
	if (rounded < twoTo63)
		return static_cast<std::uint64_t>(rounded);
	return static_cast<std::uint64_t>(rounded - twoTo63) + (std::uint64_t{1} << 63);
}

#if SIMDLIB_HAS_SSE42

#pragma region 128-bit Implementations

template <class element_t>
	requires std::is_arithmetic_v<element_t>
struct SimdImpl128
{
};

/**
 * @brief Encodes four logical 32-bit selectors in low-to-high result-lane order.
 * @tparam index0 Source lane for result lane zero.
 * @tparam index1 Source lane for result lane one.
 * @tparam index2 Source lane for result lane two.
 * @tparam index3 Source lane for result lane three.
 * @return Immediate accepted by the 128-bit four-lane shuffle intrinsics.
 */
template <std::size_t index0, std::size_t index1, std::size_t index2, std::size_t index3>
[[nodiscard]] consteval int encode_logical_shuffle_32_immediate() noexcept
{
	return static_cast<int>(index0 | (index1 << 2) | (index2 << 4) | (index3 << 6));
}

/**
 * @brief Encodes one byte of a selected logical 16-bit lane.
 * @param index Logical 16-bit source-lane selector.
 * @param byte Byte position within the selected lane.
 * @return Byte selector accepted by the 128-bit byte-shuffle intrinsic.
 */
[[nodiscard]] consteval int encode_logical_shuffle_16_byte(const std::size_t index, const std::size_t byte) noexcept
{
	return static_cast<int>((index * 2) + byte);
}

/**
 * @brief Builds the constant byte-control register for a logical 16-bit shuffle.
 * @tparam indices Logical 16-bit lane selectors.
 * @tparam byte_positions Byte positions in the resulting control register.
 * @return Native byte-control register for the 128-bit byte-shuffle intrinsic.
 */
template <auto indices, std::size_t... byte_positions>
static __m128i SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) make_logical_shuffle_16_control(std::index_sequence<byte_positions...>) noexcept
{
	return _mm_setr_epi8(encode_logical_shuffle_16_byte(indices[byte_positions / 2], byte_positions % 2)...);
}

/**
 * @brief Encodes two logical 64-bit selectors as inseparable 32-bit pairs.
 * @tparam index0 Source lane for result lane zero.
 * @tparam index1 Source lane for result lane one.
 * @return Immediate accepted by the 128-bit 32-bit-lane shuffle intrinsic.
 */
template <std::size_t index0, std::size_t index1> [[nodiscard]] consteval int encode_logical_shuffle_64_immediate() noexcept
{
	return encode_logical_shuffle_32_immediate<(index0 * 2), (index0 * 2) + 1, (index1 * 2), (index1 * 2) + 1>();
}

/**
 * @brief Encodes two logical 64-bit floating-point selectors in low-to-high result-lane order.
 * @tparam index0 Source lane for result lane zero.
 * @tparam index1 Source lane for result lane one.
 * @return Immediate accepted by the 128-bit two-lane floating-point shuffle intrinsic.
 */
template <std::size_t index0, std::size_t index1> [[nodiscard]] consteval int encode_logical_shuffle_double_immediate() noexcept
{
	return static_cast<int>(index0 | (index1 << 1));
}

template <> struct SimdImpl128<int8_t>
{
	/** @brief Selects bytes from two registers using a canonical predicate register. */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m128i condition, __m128i when_true, __m128i when_false) noexcept
	{
		return _mm_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical signed-byte lanes using compile-time source selectors.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 16 && ((indices < 16) && ...))
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m128i lhs) noexcept
	{
		return _mm_shuffle_epi8(lhs, _mm_setr_epi8(static_cast<int>(indices)...));
	}
	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm_add_epi8(lhs, rhs);
	}
	/** @brief Multiplies adjacent lanes and adds their products for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		const __m128i lhsWideLo = _mm_cvtepi8_epi16(lhs);
		const __m128i rhsWideLo = _mm_cvtepi8_epi16(rhs);
		const __m128i lhsWideHi = _mm_cvtepi8_epi16(_mm_srli_si128(lhs, 8));
		const __m128i rhsWideHi = _mm_cvtepi8_epi16(_mm_srli_si128(rhs, 8));
		return _mm_hadd_epi16(_mm_mullo_epi16(lhsWideLo, rhsWideLo), _mm_mullo_epi16(lhsWideHi, rhsWideHi));
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm_maddubs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _ext_mul_epi8(lhs, rhs);
	}
	/** @brief Divides corresponding signed 8-bit lanes with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext128_div_epi8(lhs, rhs);
	}
	/** @brief Computes corresponding signed 8-bit remainders with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext128_rem_epi8(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		auto sqrt16 = [](__m128i values) noexcept
		{
			const __m128i lo32 = _mm_cvtepi16_epi32(values);
			const __m128i hi32 = _mm_cvtepi16_epi32(_mm_srli_si128(values, 8));
			const __m128i loRoots = _mm_cvtps_epi32(_mm_sqrt_ps(_mm_cvtepi32_ps(lo32)));
			const __m128i hiRoots = _mm_cvtps_epi32(_mm_sqrt_ps(_mm_cvtepi32_ps(hi32)));
			return _mm_packs_epi32(loRoots, hiRoots);
		};

		const __m128i lo16 = sqrt16(_mm_cvtepi8_epi16(lhs));
		const __m128i hi16 = sqrt16(_mm_cvtepi8_epi16(_mm_srli_si128(lhs, 8)));
		return _mm_packs_epi16(lo16, hi16);
	}
	/** @brief Computes the unchecked group magnitude in lane zero; all other lanes are unspecified. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const __m128i low = _mm_cvtepi8_epi16(lhs);
		const __m128i high = _mm_cvtepi8_epi16(_mm_srli_si128(lhs, 8));
		__m128i total = _mm_add_epi16(_mm_mullo_epi16(low, low), _mm_mullo_epi16(high, high));
		total = _mm_hadd_epi16(total, total);
		total = _mm_hadd_epi16(total, total);
		total = _mm_hadd_epi16(total, total);
		const __m128 squareSum = _mm_cvtepi32_ps(_mm_cvtepu16_epi32(total));
		return _mm_cvtps_epi32(_mm_sqrt_ss(squareSum));
	}

	/** @brief Computes a saturated magnitude in lane zero and a canonical overflow mask in lane one. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		constexpr std::uint64_t maximum = static_cast<std::uint64_t>(std::numeric_limits<std::int8_t>::max());
		constexpr std::uint64_t threshold = maximum * maximum + maximum + 1;
		const __m128i low = _mm_cvtepi8_epi16(lhs);
		const __m128i high = _mm_cvtepi8_epi16(_mm_srli_si128(lhs, 8));
		__m128i pairSums = _mm_add_epi32(_mm_madd_epi16(low, low), _mm_madd_epi16(high, high));
		pairSums = _mm_hadd_epi32(pairSums, pairSums);
		pairSums = _mm_hadd_epi32(pairSums, pairSums);
		const std::uint64_t total = static_cast<std::uint32_t>(_mm_cvtsi128_si32(pairSums));
		const bool overflow = total >= threshold;
		const std::uint64_t result = overflow ? maximum : magnitude_round_sqrt_u64(total, maximum);
		return magnitude_checked_result<std::int8_t>(result, overflow);
	}

	/** @brief Computes minimum-value position metadata for this native register specialization. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		constexpr __m128i indices = register_from_values<__m128i, std::int8_t>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		__m128i values = lhs;
		__m128i positions = indices;
		auto reduce = [&]<int offset>() noexcept
		{
			const __m128i shiftedValues = _mm_bsrli_si128(values, offset);
			const __m128i shiftedIndices = _mm_bsrli_si128(positions, offset);
			const __m128i less = _mm_cmpgt_epi8(values, shiftedValues);
			values = _mm_blendv_epi8(values, shiftedValues, less);
			positions = _mm_blendv_epi8(positions, shiftedIndices, less);
		};
		reduce.template operator()<1>();
		reduce.template operator()<2>();
		reduce.template operator()<4>();
		reduce.template operator()<8>();
		return _mm_insert_epi8(values, _mm_extract_epi8(positions, 0), 1);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m128i lhs, __m128i rhs) noexcept
	{
		return _mm_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _mm_abs_epi8(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi8(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm_min_epi8(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm_max_epi8(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _ext_slli_epx8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _ext_srli_epx8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _ext_srai_epx8(lhs, rhs);
	}

	// arithmetic (saturated)
	/** @brief Adds lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm_adds_epi8(lhs, rhs);
	}
	/** @brief Subtracts lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm_subs_epi8(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm_set1_epi8(lhs);
	}

	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args... args) noexcept
	{
		return _mm_set_epi8(static_cast<char>(args)...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args... args) noexcept
	{
		return _mm_setr_epi8(static_cast<char>(args)...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpeq_epi8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpgt_epi8(lhs, rhs);
	}

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm_cvtepi8_epi16(lhs, rhs);
	}
	template <class target_simd> static typename target_simd::vector_t SIMD_FLAGS(InOut, ForceInline) widen(auto lhs) noexcept
	{
		using target_element_t = typename target_simd::element_type;
		if constexpr (target_simd::register_width == 128)
		{
			if constexpr (sizeof(target_element_t) == sizeof(int16_t))
				return _mm_cvtepi8_epi16(lhs);
			else if constexpr (sizeof(target_element_t) == sizeof(int32_t))
				return _mm_cvtepi8_epi32(lhs);
			else if constexpr (sizeof(target_element_t) == sizeof(int64_t))
				return _mm_cvtepi8_epi64(lhs);
			else
				static_assert(dependent_false_v<target_simd>,
							  "No direct widen mapping exists for SimdImpl128<int8_t> and the requested destination SIMD shape.");
		}
		else if constexpr (target_simd::register_width == 256)
		{
			if constexpr (sizeof(target_element_t) == sizeof(int16_t))
				return _mm256_cvtepi8_epi16(lhs);
			else if constexpr (sizeof(target_element_t) == sizeof(int32_t))
				return _mm256_cvtepi8_epi32(lhs);
			else if constexpr (sizeof(target_element_t) == sizeof(int64_t))
				return _mm256_cvtepi8_epi64(lhs);
			else
				static_assert(dependent_false_v<target_simd>,
							  "No direct widen mapping exists for SimdImpl128<int8_t> and the requested destination SIMD shape.");
		}
		else
		{
			static_assert(dependent_false_v<target_simd>, "No direct widen mapping exists for SimdImpl128<int8_t> and the requested destination SIMD shape.");
		}
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<int8_t>(_mm_extract_epi8(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected signed 8-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 16)`.
	 * @return Selected scalar lane.
	 */
	static int8_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m128i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 16, "Signed 8-bit extraction requires a valid 128-bit lane index");
		switch (index)
		{
		case 0:
			return extract<0>(lhs);
		case 1:
			return extract<1>(lhs);
		case 2:
			return extract<2>(lhs);
		case 3:
			return extract<3>(lhs);
		case 4:
			return extract<4>(lhs);
		case 5:
			return extract<5>(lhs);
		case 6:
			return extract<6>(lhs);
		case 7:
			return extract<7>(lhs);
		case 8:
			return extract<8>(lhs);
		case 9:
			return extract<9>(lhs);
		case 10:
			return extract<10>(lhs);
		case 11:
			return extract<11>(lhs);
		case 12:
			return extract<12>(lhs);
		case 13:
			return extract<13>(lhs);
		case 14:
			return extract<14>(lhs);
		case 15:
			return extract<15>(lhs);
		default:
			return extract<0>(lhs);
		}
	}
	/** @brief Replaces the compile-time-selected signed 8-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const int8_t rhs) noexcept
	{
		return register_insert_constexpr<int8_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected signed 8-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const int8_t rhs) noexcept
	{
		return _mm_insert_epi8(lhs, static_cast<int>(rhs), index);
	}
	/**
	 * @brief Replaces one runtime-selected signed 8-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 16)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m128i lhs, const int8_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 16, "Signed 8-bit insertion requires a valid 128-bit lane index");
		const __m128i lane_indices = _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		const __m128i selected_lane = _mm_cmpeq_epi8(lane_indices, _mm_set1_epi8(static_cast<char>(index)));
		return _mm_blendv_epi8(lhs, _mm_set1_epi8(rhs), selected_lane);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm_unpacklo_epi8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm_unpackhi_epi8(lhs, rhs);
	}

	// misc
	/** @brief Shuffles bytes through the native runtime selector-register instruction. */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle(auto lhs, auto rhs) noexcept
		requires(std::same_as<decltype(lhs), __m128i> && std::same_as<decltype(rhs), __m128i>)
	{
		return _mm_shuffle_epi8(lhs, rhs);
	}
	/** @brief Selects bytes through the native runtime mask-register operation. */
	SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY static __m128i VECTORCALL blend(const __m128i lhs, const __m128i rhs, const __m128i mask) noexcept
	{
		return register_blend_bytes(lhs, rhs, mask);
	}
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) movemask(auto lhs) noexcept
	{
		return _mm_movemask_epi8(lhs);
	}
};

template <> struct SimdImpl128<uint8_t>
{
	/** @brief Selects bytes from two registers using a canonical predicate register. */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m128i condition, __m128i when_true, __m128i when_false) noexcept
	{
		return _mm_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical unsigned-byte lanes using compile-time source selectors.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 16 && ((indices < 16) && ...))
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m128i lhs) noexcept
	{
		return _mm_shuffle_epi8(lhs, _mm_setr_epi8(static_cast<int>(indices)...));
	}
	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm_add_epi8(lhs, rhs);
	}
	/** @brief Multiplies adjacent lanes and adds their products for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		const __m128i lhsWideLo = _mm_cvtepu8_epi16(lhs);
		const __m128i rhsWideLo = _mm_cvtepu8_epi16(rhs);
		const __m128i lhsWideHi = _mm_cvtepu8_epi16(_mm_srli_si128(lhs, 8));
		const __m128i rhsWideHi = _mm_cvtepu8_epi16(_mm_srli_si128(rhs, 8));
		return _mm_hadd_epi16(_mm_mullo_epi16(lhsWideLo, rhsWideLo), _mm_mullo_epi16(lhsWideHi, rhsWideHi));
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm_maddubs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _ext_mul_epi8(lhs, rhs);
	}
	/** @brief Divides corresponding unsigned 8-bit lanes with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext128_div_epu8(lhs, rhs);
	}
	/** @brief Computes corresponding unsigned 8-bit remainders with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext128_rem_epu8(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		auto sqrt16 = [](__m128i values) noexcept
		{
			const __m128i lo32 = _mm_cvtepu16_epi32(values);
			const __m128i hi32 = _mm_cvtepu16_epi32(_mm_srli_si128(values, 8));
			const __m128i loRoots = _mm_cvtps_epi32(_mm_sqrt_ps(_ext_cvtepu32_ps(lo32)));
			const __m128i hiRoots = _mm_cvtps_epi32(_mm_sqrt_ps(_ext_cvtepu32_ps(hi32)));
			return _mm_packus_epi32(loRoots, hiRoots);
		};

		const __m128i lo16 = sqrt16(_mm_cvtepu8_epi16(lhs));
		const __m128i hi16 = sqrt16(_mm_cvtepu8_epi16(_mm_srli_si128(lhs, 8)));
		return _mm_packus_epi16(lo16, hi16);
	}
	/** @brief Computes the unchecked group magnitude in lane zero; all other lanes are unspecified. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const __m128i low = _mm_cvtepu8_epi16(lhs);
		const __m128i high = _mm_cvtepu8_epi16(_mm_srli_si128(lhs, 8));
		__m128i total = _mm_add_epi16(_mm_mullo_epi16(low, low), _mm_mullo_epi16(high, high));
		total = _mm_hadd_epi16(total, total);
		total = _mm_hadd_epi16(total, total);
		total = _mm_hadd_epi16(total, total);
		const __m128 squareSum = _mm_cvtepi32_ps(_mm_cvtepu16_epi32(total));
		return _mm_cvtps_epi32(_mm_sqrt_ss(squareSum));
	}

	/** @brief Computes a saturated magnitude in lane zero and a canonical overflow mask in lane one. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		constexpr std::uint64_t maximum = static_cast<std::uint64_t>(std::numeric_limits<std::uint8_t>::max());
		constexpr std::uint64_t threshold = maximum * maximum + maximum + 1;
		const __m128i low = _mm_cvtepu8_epi16(lhs);
		const __m128i high = _mm_cvtepu8_epi16(_mm_srli_si128(lhs, 8));
		__m128i pairSums = _mm_add_epi32(_mm_madd_epi16(low, low), _mm_madd_epi16(high, high));
		pairSums = _mm_hadd_epi32(pairSums, pairSums);
		pairSums = _mm_hadd_epi32(pairSums, pairSums);
		const std::uint64_t total = static_cast<std::uint32_t>(_mm_cvtsi128_si32(pairSums));
		const bool overflow = total >= threshold;
		const std::uint64_t result = overflow ? maximum : magnitude_round_sqrt_u64(total, maximum);
		return magnitude_checked_result<std::uint8_t>(result, overflow);
	}

	/** @brief Computes minimum-value position metadata for this native register specialization. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		constexpr __m128i indices = register_from_values<__m128i, std::uint8_t>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		const __m128i signBit = _mm_set1_epi8(static_cast<char>(0x80));
		__m128i values = lhs;
		__m128i positions = indices;
		auto reduce = [&]<int offset>() noexcept
		{
			const __m128i shiftedValues = _mm_bsrli_si128(values, offset);
			const __m128i shiftedIndices = _mm_bsrli_si128(positions, offset);
			const __m128i less = _mm_cmpgt_epi8(_mm_xor_si128(values, signBit), _mm_xor_si128(shiftedValues, signBit));
			values = _mm_blendv_epi8(values, shiftedValues, less);
			positions = _mm_blendv_epi8(positions, shiftedIndices, less);
		};
		reduce.template operator()<1>();
		reduce.template operator()<2>();
		reduce.template operator()<4>();
		reduce.template operator()<8>();
		return _mm_insert_epi8(values, _mm_extract_epi8(positions, 0), 1);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m128i lhs, __m128i rhs) noexcept
	{
		return _mm_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _mm_abs_epi8(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi8(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm_min_epu8(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm_max_epu8(lhs, rhs);
	}
	/** @brief Computes lane-wise averages for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) avg(auto lhs, auto rhs) noexcept
	{
		return _mm_avg_epu8(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _ext_slli_epx8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _ext_srli_epx8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _ext_srai_epx8(lhs, rhs);
	}

	// arithmetic (horizontal)
	// static SIMDLIB_FORCE_INLINE auto VECTORCALL hadd (auto lhs, auto rhs) noexcept { return _mm_hadd_epi8(lhs, rhs); }
	// static SIMDLIB_FORCE_INLINE auto VECTORCALL hsub (auto lhs, auto rhs) noexcept { return _mm_hsub_epi8(lhs, rhs); }

	// arithmetic (saturated)
	/** @brief Adds lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm_adds_epu8(lhs, rhs);
	}
	/** @brief Subtracts lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm_subs_epu8(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _ext_set1_epu8(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args &&...args) noexcept
	{
		return _mm_set_epi8(static_cast<char>(args)...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args &&...args) noexcept
	{
		return _mm_setr_epi8(static_cast<char>(args)...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpeq_epi8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _ext_cmpgt_epu8(lhs, rhs);
	}

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm_cvtepu8_epi16(lhs, rhs);
	}
	template <class target_simd> static typename target_simd::vector_t SIMD_FLAGS(InOut, ForceInline) widen(auto lhs) noexcept
	{
		using target_element_t = typename target_simd::element_type;
		if constexpr (target_simd::register_width == 128)
		{
			if constexpr (sizeof(target_element_t) == sizeof(uint16_t))
				return _mm_cvtepu8_epi16(lhs);
			else if constexpr (sizeof(target_element_t) == sizeof(uint32_t))
				return _mm_cvtepu8_epi32(lhs);
			else if constexpr (sizeof(target_element_t) == sizeof(uint64_t))
				return _mm_cvtepu8_epi64(lhs);
			else
				static_assert(dependent_false_v<target_simd>,
							  "No direct widen mapping exists for SimdImpl128<uint8_t> and the requested destination SIMD shape.");
		}
		else if constexpr (target_simd::register_width == 256)
		{
			if constexpr (sizeof(target_element_t) == sizeof(uint16_t))
				return _mm256_cvtepu8_epi16(lhs);
			else if constexpr (sizeof(target_element_t) == sizeof(uint32_t))
				return _mm256_cvtepu8_epi32(lhs);
			else if constexpr (sizeof(target_element_t) == sizeof(uint64_t))
				return _mm256_cvtepu8_epi64(lhs);
			else
				static_assert(dependent_false_v<target_simd>,
							  "No direct widen mapping exists for SimdImpl128<uint8_t> and the requested destination SIMD shape.");
		}
		else
		{
			static_assert(dependent_false_v<target_simd>, "No direct widen mapping exists for SimdImpl128<uint8_t> and the requested destination SIMD shape.");
		}
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<uint8_t>(_mm_extract_epi8(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected unsigned 8-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 16)`.
	 * @return Selected scalar lane.
	 */
	static uint8_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m128i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 16, "Unsigned 8-bit extraction requires a valid 128-bit lane index");
		switch (index)
		{
		case 0:
			return extract<0>(lhs);
		case 1:
			return extract<1>(lhs);
		case 2:
			return extract<2>(lhs);
		case 3:
			return extract<3>(lhs);
		case 4:
			return extract<4>(lhs);
		case 5:
			return extract<5>(lhs);
		case 6:
			return extract<6>(lhs);
		case 7:
			return extract<7>(lhs);
		case 8:
			return extract<8>(lhs);
		case 9:
			return extract<9>(lhs);
		case 10:
			return extract<10>(lhs);
		case 11:
			return extract<11>(lhs);
		case 12:
			return extract<12>(lhs);
		case 13:
			return extract<13>(lhs);
		case 14:
			return extract<14>(lhs);
		case 15:
			return extract<15>(lhs);
		default:
			return extract<0>(lhs);
		}
	}
	/** @brief Replaces the compile-time-selected unsigned 8-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const uint8_t rhs) noexcept
	{
		return register_insert_constexpr<uint8_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected unsigned 8-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const uint8_t rhs) noexcept
	{
		return _mm_insert_epi8(lhs, static_cast<int>(rhs), index);
	}
	/**
	 * @brief Replaces one runtime-selected unsigned 8-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 16)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m128i lhs, const uint8_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 16, "Unsigned 8-bit insertion requires a valid 128-bit lane index");
		const __m128i lane_indices = _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		const __m128i selected_lane = _mm_cmpeq_epi8(lane_indices, _mm_set1_epi8(static_cast<char>(index)));
		return _mm_blendv_epi8(lhs, _mm_set1_epi8(std::bit_cast<int8_t>(rhs)), selected_lane);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm_unpacklo_epi8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm_unpackhi_epi8(lhs, rhs);
	}

	// misc
	/** @brief Shuffles bytes through the native runtime selector-register instruction. */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle(auto lhs, auto rhs) noexcept
		requires(std::same_as<decltype(lhs), __m128i> && std::same_as<decltype(rhs), __m128i>)
	{
		return _mm_shuffle_epi8(lhs, rhs);
	}
	/** @brief Selects bytes through the native runtime mask-register operation. */
	SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY static __m128i VECTORCALL blend(const __m128i lhs, const __m128i rhs, const __m128i mask) noexcept
	{
		return register_blend_bytes(lhs, rhs, mask);
	}
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) movemask(auto lhs) noexcept
	{
		return _mm_movemask_epi8(lhs);
	}
};

template <> struct SimdImpl128<int16_t>
{
	/** @brief Selects 16-bit lanes from two registers using a canonical predicate register. */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m128i condition, __m128i when_true, __m128i when_false) noexcept
	{
		return _mm_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical signed 16-bit lanes using compile-time source selectors.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 8 && ((indices < 8) && ...))
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m128i lhs) noexcept
	{
		return _mm_shuffle_epi8(lhs, make_logical_shuffle_16_control<std::array{indices...}>(std::make_index_sequence<16>{}));
	}
	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm_add_epi16(lhs, rhs);
	}
	/** @brief Multiplies adjacent lanes and adds their products for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		return _mm_madd_epi16(lhs, rhs);
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm_maddubs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _mm_mullo_epi16(lhs, rhs);
	}
	/** @brief Divides corresponding signed 16-bit lanes with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext128_div_epi16(lhs, rhs);
	}
	/** @brief Computes corresponding signed 16-bit remainders with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext128_rem_epi16(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		const __m128i lo32 = _mm_cvtepi16_epi32(lhs);
		const __m128i hi32 = _mm_cvtepi16_epi32(_mm_srli_si128(lhs, 8));
		const __m128i loRoots = _mm_cvtps_epi32(_mm_sqrt_ps(_mm_cvtepi32_ps(lo32)));
		const __m128i hiRoots = _mm_cvtps_epi32(_mm_sqrt_ps(_mm_cvtepi32_ps(hi32)));
		return _mm_packs_epi32(loRoots, hiRoots);
	}
	/** @brief Computes the unchecked group magnitude in lane zero; all other lanes are unspecified. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		__m128i total = _mm_madd_epi16(lhs, lhs);
		total = _mm_hadd_epi32(total, total);
		total = _mm_hadd_epi32(total, total);
		return _mm_cvtpd_epi32(_mm_sqrt_sd(_mm_cvtepi32_pd(total), _mm_cvtepi32_pd(total)));
	}

	/** @brief Computes a saturated magnitude in lane zero and a canonical overflow mask in lane one. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		constexpr std::uint64_t maximum = static_cast<std::uint64_t>(std::numeric_limits<std::int16_t>::max());
		constexpr std::uint64_t threshold = maximum * maximum + maximum + 1;
		const __m128i minimum = _mm_set1_epi16(std::numeric_limits<std::int16_t>::min());
		if (_mm_movemask_epi8(_mm_cmpeq_epi16(lhs, minimum)) != 0)
			return magnitude_checked_result<std::int16_t>(maximum, true);
		const __m128i pairSquares = _mm_madd_epi16(lhs, lhs);
		const __m128i lowPairs = _mm_cvtepu32_epi64(pairSquares);
		const __m128i highPairs = _mm_cvtepu32_epi64(_mm_srli_si128(pairSquares, 8));
		const __m128i pairTotals = _mm_add_epi64(lowPairs, highPairs);
		const __m128i totalVector = _mm_add_epi64(pairTotals, _mm_srli_si128(pairTotals, 8));
		const std::uint64_t total = static_cast<std::uint64_t>(_mm_cvtsi128_si64(totalVector));
		const bool overflow = total >= threshold;
		const std::uint64_t result = overflow ? maximum : magnitude_round_sqrt_u64(total, maximum);
		return magnitude_checked_result<std::int16_t>(result, overflow);
	}

	/** @brief Computes minimum-value position metadata for this native register specialization. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		constexpr __m128i indices = register_from_values<__m128i, std::int16_t>(0, 1, 2, 3, 4, 5, 6, 7);
		__m128i values = lhs;
		__m128i positions = indices;
		auto reduce = [&]<int offset>() noexcept
		{
			if constexpr (offset < 8)
			{
				const __m128i shiftedValues = _mm_bsrli_si128(values, offset * 2);
				const __m128i shiftedIndices = _mm_bsrli_si128(positions, offset * 2);
				const __m128i less = _mm_cmpgt_epi16(values, shiftedValues);
				values = _mm_blendv_epi8(values, shiftedValues, less);
				positions = _mm_blendv_epi8(positions, shiftedIndices, less);
			}
		};
		reduce.template operator()<1>();
		reduce.template operator()<2>();
		reduce.template operator()<4>();
		return _mm_insert_epi16(values, _mm_extract_epi16(positions, 0), 1);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m128i lhs, __m128i rhs) noexcept
	{
		return _mm_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _mm_abs_epi16(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi16(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm_min_epi16(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm_max_epi16(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _mm_slli_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _mm_srli_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _mm_srai_epi16(lhs, rhs);
	}

	// arithmetic (horizontal)
	/** @brief Horizontally adds adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm_hadd_epi16(lhs, rhs);
	}
	/** @brief Horizontally subtracts adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm_hsub_epi16(lhs, rhs);
	}
	/** @brief Horizontally adds lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) hadd_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm_hadds_epi16(lhs, rhs);
	}
	/** @brief Horizontally subtracts lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) hsubtract_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm_hsubs_epi16(lhs, rhs);
	}

	// arithmetic (saturated)
	/** @brief Adds lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm_adds_epi16(lhs, rhs);
	}
	/** @brief Subtracts lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm_subs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) multiply_saturated(auto lhs, auto rhs) noexcept
	{
		const __m128i lhsLo = _mm_cvtepi16_epi32(lhs);
		const __m128i rhsLo = _mm_cvtepi16_epi32(rhs);
		const __m128i lhsHi = _mm_cvtepi16_epi32(_mm_srli_si128(lhs, 8));
		const __m128i rhsHi = _mm_cvtepi16_epi32(_mm_srli_si128(rhs, 8));
		return _mm_packs_epi32(_mm_mullo_epi32(lhsLo, rhsLo), _mm_mullo_epi32(lhsHi, rhsHi));
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm_set1_epi16(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args &&...args) noexcept
	{
		return _mm_set_epi16(static_cast<short>(args)...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args &&...args) noexcept
	{
		return _mm_setr_epi16(static_cast<short>(args)...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpeq_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpgt_epi16(lhs, rhs);
	}

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm_cvtepi16_epi32(lhs, rhs);
	}
	template <class target_simd> static typename target_simd::vector_t SIMD_FLAGS(InOut, ForceInline) widen(auto lhs) noexcept
	{
		using target_element_t = typename target_simd::element_type;
		if constexpr (target_simd::register_width == 128)
		{
			if constexpr (sizeof(target_element_t) == sizeof(int32_t))
				return _mm_cvtepi16_epi32(lhs);
			else if constexpr (sizeof(target_element_t) == sizeof(int64_t))
				return _mm_cvtepi16_epi64(lhs);
			else
				static_assert(dependent_false_v<target_simd>,
							  "No direct widen mapping exists for SimdImpl128<int16_t> and the requested destination SIMD shape.");
		}
		else if constexpr (target_simd::register_width == 256)
		{
			if constexpr (sizeof(target_element_t) == sizeof(int32_t))
				return _mm256_cvtepi16_epi32(lhs);
			else if constexpr (sizeof(target_element_t) == sizeof(int64_t))
				return _mm256_cvtepi16_epi64(lhs);
			else
				static_assert(dependent_false_v<target_simd>,
							  "No direct widen mapping exists for SimdImpl128<int16_t> and the requested destination SIMD shape.");
		}
		else
		{
			static_assert(dependent_false_v<target_simd>, "No direct widen mapping exists for SimdImpl128<int16_t> and the requested destination SIMD shape.");
		}
	}
	static auto SIMD_FLAGS(InOut, ForceInline) compress(auto lhs, auto rhs) noexcept
	{
		return _mm_packs_epi16(lhs, rhs);
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<int16_t>(_mm_extract_epi16(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected signed 16-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 8)`.
	 * @return Selected scalar lane.
	 */
	static int16_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m128i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 8, "Signed 16-bit extraction requires a valid 128-bit lane index");
		switch (index)
		{
		case 0:
			return extract<0>(lhs);
		case 1:
			return extract<1>(lhs);
		case 2:
			return extract<2>(lhs);
		case 3:
			return extract<3>(lhs);
		case 4:
			return extract<4>(lhs);
		case 5:
			return extract<5>(lhs);
		case 6:
			return extract<6>(lhs);
		case 7:
			return extract<7>(lhs);
		default:
			return extract<0>(lhs);
		}
	}
	/** @brief Replaces the compile-time-selected signed 16-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const int16_t rhs) noexcept
	{
		return register_insert_constexpr<int16_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected signed 16-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const int16_t rhs) noexcept
	{
		return _mm_insert_epi16(lhs, static_cast<int>(rhs), index);
	}
	/**
	 * @brief Replaces one runtime-selected signed 16-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 8)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m128i lhs, const int16_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 8, "Signed 16-bit insertion requires a valid 128-bit lane index");
		const __m128i lane_indices = _mm_setr_epi16(0, 1, 2, 3, 4, 5, 6, 7);
		const __m128i selected_lane = _mm_cmpeq_epi16(lane_indices, _mm_set1_epi16(static_cast<int16_t>(index)));
		return _mm_blendv_epi8(lhs, _mm_set1_epi16(rhs), selected_lane);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm_unpacklo_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm_unpackhi_epi16(lhs, rhs);
	}

	// misc
	/** @brief Emulates an immediate-controlled low-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each low four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_lo_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_half_16_slow(lhs, static_cast<unsigned int>(rhs), false);
	}
	/** @brief Shuffles the low four 16-bit lanes in each 128-bit group with an immediate control. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle_lo(__m128i lhs) noexcept
	{
		return _mm_shufflelo_epi16(lhs, imm8);
	}
	/** @brief Emulates an immediate-controlled high-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each high four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_hi_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_half_16_slow(lhs, static_cast<unsigned int>(rhs), true);
	}
	/** @brief Shuffles the high four 16-bit lanes in each 128-bit group with an immediate control. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle_hi(__m128i lhs) noexcept
	{
		return _mm_shufflehi_epi16(lhs, imm8);
	}
	/** @brief Emulates an immediate-controlled blend with a runtime scalar mask.
	 *  @param lhs Source for lanes whose control bits are clear.
	 *  @param rhs Source for lanes whose control bits are set.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the selected lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) blend_slow(auto lhs, auto rhs, const int imm8) noexcept
	{
		return register_blend_slow<std::int16_t>(lhs, rhs, static_cast<unsigned int>(imm8));
	}
	/** @brief Selects signed 16-bit lanes from two registers with an immediate control. */
	template <int imm8> SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static auto VECTORCALL blend(auto lhs, auto rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return register_blend_slow<std::int16_t>(lhs, rhs, static_cast<unsigned int>(imm8));
		return _mm_blend_epi16(lhs, rhs, imm8);
	}
};

template <> struct SimdImpl128<uint16_t>
{
	/** @brief Selects 16-bit lanes from two registers using a canonical predicate register. */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m128i condition, __m128i when_true, __m128i when_false) noexcept
	{
		return _mm_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical unsigned 16-bit lanes using compile-time source selectors.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 8 && ((indices < 8) && ...))
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m128i lhs) noexcept
	{
		return _mm_shuffle_epi8(lhs, make_logical_shuffle_16_control<std::array{indices...}>(std::make_index_sequence<16>{}));
	}
	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm_add_epi16(lhs, rhs);
	}
	/** @brief Multiplies adjacent lanes and adds their products for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		const __m128i lhsLo = _mm_cvtepu16_epi32(lhs);
		const __m128i rhsLo = _mm_cvtepu16_epi32(rhs);
		const __m128i lhsHi = _mm_cvtepu16_epi32(_mm_srli_si128(lhs, 8));
		const __m128i rhsHi = _mm_cvtepu16_epi32(_mm_srli_si128(rhs, 8));
		return _mm_hadd_epi32(_mm_mullo_epi32(lhsLo, rhsLo), _mm_mullo_epi32(lhsHi, rhsHi));
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm_maddubs_epi16(lhs, rhs);
	}
	/** @brief Computes the unchecked group magnitude in lane zero; all other lanes are unspecified. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const __m128i lowProducts = _mm_mullo_epi16(lhs, lhs);
		const __m128i highProducts = _mm_mulhi_epu16(lhs, lhs);
		const __m128i lowSquares = _mm_unpacklo_epi16(lowProducts, highProducts);
		const __m128i highSquares = _mm_unpackhi_epi16(lowProducts, highProducts);
		__m128i total = _mm_add_epi32(lowSquares, highSquares);
		total = _mm_hadd_epi32(total, total);
		total = _mm_hadd_epi32(total, total);
		const std::uint32_t squareSum = static_cast<std::uint32_t>(_mm_cvtsi128_si32(total));
		const __m128d root = _mm_sqrt_sd(_mm_setzero_pd(), _mm_set_sd(static_cast<double>(squareSum)));
		return _mm_cvtsi32_si128(static_cast<int>(_mm_cvtsd_si32(root)));
	}

	/** @brief Computes a saturated magnitude in lane zero and a canonical overflow mask in lane one. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		constexpr std::uint64_t maximum = static_cast<std::uint64_t>(std::numeric_limits<std::uint16_t>::max());
		constexpr std::uint64_t threshold = maximum * maximum + maximum + 1;
		const __m128i lowProducts = _mm_mullo_epi16(lhs, lhs);
		const __m128i highProducts = _mm_mulhi_epu16(lhs, lhs);
		const __m128i lowSquares = _mm_unpacklo_epi16(lowProducts, highProducts);
		const __m128i highSquares = _mm_unpackhi_epi16(lowProducts, highProducts);
		const __m128i lowPairs = _mm_add_epi64(_mm_cvtepu32_epi64(lowSquares), _mm_cvtepu32_epi64(_mm_srli_si128(lowSquares, 8)));
		const __m128i highPairs = _mm_add_epi64(_mm_cvtepu32_epi64(highSquares), _mm_cvtepu32_epi64(_mm_srli_si128(highSquares, 8)));
		const __m128i pairTotals = _mm_add_epi64(lowPairs, highPairs);
		const __m128i totalVector = _mm_add_epi64(pairTotals, _mm_srli_si128(pairTotals, 8));
		const std::uint64_t total = static_cast<std::uint64_t>(_mm_cvtsi128_si64(totalVector));
		const bool overflow = total >= threshold;
		const std::uint64_t result = overflow ? maximum : magnitude_round_sqrt_u64(total, maximum);
		return magnitude_checked_result<std::uint16_t>(result, overflow);
	}

	/** @brief Computes minimum-value position metadata for this native register specialization. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		return _mm_minpos_epu16(lhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _mm_mullo_epi16(lhs, rhs);
	}
	/** @brief Divides corresponding unsigned 16-bit lanes with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext128_div_epu16(lhs, rhs);
	}
	/** @brief Computes corresponding unsigned 16-bit remainders with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext128_rem_epu16(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		const __m128i lo32 = _mm_cvtepu16_epi32(lhs);
		const __m128i hi32 = _mm_cvtepu16_epi32(_mm_srli_si128(lhs, 8));
		const __m128i loRoots = _mm_cvtps_epi32(_mm_sqrt_ps(_ext_cvtepu32_ps(lo32)));
		const __m128i hiRoots = _mm_cvtps_epi32(_mm_sqrt_ps(_ext_cvtepu32_ps(hi32)));
		return _mm_packus_epi32(loRoots, hiRoots);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m128i lhs, __m128i rhs) noexcept
	{
		return _mm_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _mm_abs_epi16(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi16(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm_min_epu16(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm_max_epu16(lhs, rhs);
	}
	/** @brief Computes lane-wise averages for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) avg(auto lhs, auto rhs) noexcept
	{
		return _mm_avg_epu16(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _mm_slli_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _mm_srli_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _mm_srai_epi16(lhs, rhs);
	}

	// arithmetic (horizontal)
	/** @brief Horizontally adds adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm_hadd_epi16(lhs, rhs);
	}
	/** @brief Horizontally subtracts adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm_hsub_epi16(lhs, rhs);
	}
	/** @brief Horizontally adds unsigned 16-bit lanes with unsigned saturation. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) hadd_saturated(auto lhs, auto rhs) noexcept
	{
		const __m128i zero = _mm_setzero_si128();
		const __m128i lhsPairs = _mm_adds_epu16(lhs, _mm_srli_epi32(lhs, 16));
		const __m128i rhsPairs = _mm_adds_epu16(rhs, _mm_srli_epi32(rhs, 16));
		return _mm_packus_epi32(_mm_blend_epi16(lhsPairs, zero, 0xAA), _mm_blend_epi16(rhsPairs, zero, 0xAA));
	}
	/** @brief Horizontally subtracts unsigned 16-bit lanes with unsigned saturation. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) hsubtract_saturated(auto lhs, auto rhs) noexcept
	{
		const __m128i zero = _mm_setzero_si128();
		const __m128i lhsPairs = _mm_subs_epu16(lhs, _mm_srli_epi32(lhs, 16));
		const __m128i rhsPairs = _mm_subs_epu16(rhs, _mm_srli_epi32(rhs, 16));
		return _mm_packus_epi32(_mm_blend_epi16(lhsPairs, zero, 0xAA), _mm_blend_epi16(rhsPairs, zero, 0xAA));
	}

	// arithmetic (saturated)
	/** @brief Adds lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm_adds_epu16(lhs, rhs);
	}
	/** @brief Subtracts lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm_subs_epu16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) multiply_saturated(auto lhs, auto rhs) noexcept
	{
		const __m128i lhsLo = _mm_cvtepu16_epi32(lhs);
		const __m128i rhsLo = _mm_cvtepu16_epi32(rhs);
		const __m128i lhsHi = _mm_cvtepu16_epi32(_mm_srli_si128(lhs, 8));
		const __m128i rhsHi = _mm_cvtepu16_epi32(_mm_srli_si128(rhs, 8));
		return _mm_packus_epi32(_mm_mullo_epi32(lhsLo, rhsLo), _mm_mullo_epi32(lhsHi, rhsHi));
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm_set1_epi16(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args &&...args) noexcept
	{
		return _mm_set_epi16(static_cast<short>(args)...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args &&...args) noexcept
	{
		return _mm_setr_epi16(static_cast<short>(args)...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpeq_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _ext_cmpgt_epu16(lhs, rhs);
	}

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm_cvtepu16_epi32(lhs, rhs);
	}
	template <class target_simd> static typename target_simd::vector_t SIMD_FLAGS(InOut, ForceInline) widen(auto lhs) noexcept
	{
		using target_element_t = typename target_simd::element_type;
		if constexpr (target_simd::register_width == 128)
		{
			if constexpr (sizeof(target_element_t) == sizeof(uint32_t))
				return _mm_cvtepu16_epi32(lhs);
			else if constexpr (sizeof(target_element_t) == sizeof(uint64_t))
				return _mm_cvtepu16_epi64(lhs);
			else
				static_assert(dependent_false_v<target_simd>,
							  "No direct widen mapping exists for SimdImpl128<uint16_t> and the requested destination SIMD shape.");
		}
		else if constexpr (target_simd::register_width == 256)
		{
			if constexpr (sizeof(target_element_t) == sizeof(uint32_t))
				return _mm256_cvtepu16_epi32(lhs);
			else if constexpr (sizeof(target_element_t) == sizeof(uint64_t))
				return _mm256_cvtepu16_epi64(lhs);
			else
				static_assert(dependent_false_v<target_simd>,
							  "No direct widen mapping exists for SimdImpl128<uint16_t> and the requested destination SIMD shape.");
		}
		else
		{
			static_assert(dependent_false_v<target_simd>, "No direct widen mapping exists for SimdImpl128<uint16_t> and the requested destination SIMD shape.");
		}
	}
	static auto SIMD_FLAGS(InOut, ForceInline) compress(auto lhs, auto rhs) noexcept
	{
		return _mm_packus_epi16(lhs, rhs);
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<uint16_t>(_mm_extract_epi16(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected unsigned 16-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 8)`.
	 * @return Selected scalar lane.
	 */
	static uint16_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m128i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 8, "Unsigned 16-bit extraction requires a valid 128-bit lane index");
		switch (index)
		{
		case 0:
			return extract<0>(lhs);
		case 1:
			return extract<1>(lhs);
		case 2:
			return extract<2>(lhs);
		case 3:
			return extract<3>(lhs);
		case 4:
			return extract<4>(lhs);
		case 5:
			return extract<5>(lhs);
		case 6:
			return extract<6>(lhs);
		case 7:
			return extract<7>(lhs);
		default:
			return extract<0>(lhs);
		}
	}
	/** @brief Replaces the compile-time-selected unsigned 16-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const uint16_t rhs) noexcept
	{
		return register_insert_constexpr<uint16_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected unsigned 16-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const uint16_t rhs) noexcept
	{
		return _mm_insert_epi16(lhs, static_cast<int>(rhs), index);
	}
	/**
	 * @brief Replaces one runtime-selected unsigned 16-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 8)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m128i lhs, const uint16_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 8, "Unsigned 16-bit insertion requires a valid 128-bit lane index");
		const __m128i lane_indices = _mm_setr_epi16(0, 1, 2, 3, 4, 5, 6, 7);
		const __m128i selected_lane = _mm_cmpeq_epi16(lane_indices, _mm_set1_epi16(static_cast<int16_t>(index)));
		return _mm_blendv_epi8(lhs, _mm_set1_epi16(std::bit_cast<int16_t>(rhs)), selected_lane);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm_unpacklo_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm_unpackhi_epi16(lhs, rhs);
	}

	// misc
	/** @brief Emulates an immediate-controlled low-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each low four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_lo_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_half_16_slow(lhs, static_cast<unsigned int>(rhs), false);
	}
	/** @brief Shuffles the low four unsigned 16-bit lanes in each 128-bit group with an immediate control. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle_lo(__m128i lhs) noexcept
	{
		return _mm_shufflelo_epi16(lhs, imm8);
	}
	/** @brief Emulates an immediate-controlled high-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each high four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_hi_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_half_16_slow(lhs, static_cast<unsigned int>(rhs), true);
	}
	/** @brief Shuffles the high four unsigned 16-bit lanes in each 128-bit group with an immediate control. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle_hi(__m128i lhs) noexcept
	{
		return _mm_shufflehi_epi16(lhs, imm8);
	}
	/** @brief Emulates an immediate-controlled blend with a runtime scalar mask.
	 *  @param lhs Source for lanes whose control bits are clear.
	 *  @param rhs Source for lanes whose control bits are set.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the selected lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) blend_slow(auto lhs, auto rhs, const int imm8) noexcept
	{
		return register_blend_slow<std::int16_t>(lhs, rhs, static_cast<unsigned int>(imm8));
	}
	/** @brief Selects unsigned 16-bit lanes from two registers with an immediate control. */
	template <int imm8> SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static auto VECTORCALL blend(auto lhs, auto rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return register_blend_slow<std::uint16_t>(lhs, rhs, static_cast<unsigned int>(imm8));
		return _mm_blend_epi16(lhs, rhs, imm8);
	}
};

template <> struct SimdImpl128<int32_t>
{
	/** @brief Selects 32-bit lanes from two registers using a canonical predicate register. */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m128i condition, __m128i when_true, __m128i when_false) noexcept
	{
		return _mm_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical signed 32-bit lanes using compile-time source selectors.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 4 && ((indices < 4) && ...))
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m128i lhs) noexcept
	{
		return _mm_shuffle_epi32(lhs, encode_logical_shuffle_32_immediate<indices...>());
	}
	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm_add_epi32(lhs, rhs);
	}
	/** @brief Multiplies adjacent lanes and adds their products for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		const __m128i evenProducts = _mm_mul_epi32(lhs, rhs);
		const __m128i oddProducts = _mm_mul_epi32(_mm_srli_si128(lhs, 4), _mm_srli_si128(rhs, 4));
		return _mm_add_epi64(evenProducts, oddProducts);
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm_maddubs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _mm_mullo_epi32(lhs, rhs);
	}
	/** @brief Divides corresponding signed 32-bit lanes with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext128_div_epi32(lhs, rhs);
	}
	/** @brief Computes corresponding signed 32-bit remainders with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext128_rem_epi32(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		const __m128 roots = _mm_sqrt_ps(_mm_cvtepi32_ps(lhs));
		return _mm_cvtps_epi32(roots);
	}
	/** @brief Computes the unchecked group magnitude in lane zero; all other lanes are unspecified. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const __m128i pairSums = multiply_add_adjacent(lhs, lhs);
		const __m128i totalVector = _mm_add_epi64(pairSums, _mm_srli_si128(pairSums, 8));
		const std::int64_t total = _mm_cvtsi128_si64(totalVector);
		const __m128d root = _mm_sqrt_sd(_mm_setzero_pd(), _mm_cvtsi64_sd(_mm_setzero_pd(), total));
		return _mm_cvtsi64_si128(_mm_cvtsd_si64(root));
	}

	/** @brief Computes a saturated magnitude in lane zero and a canonical overflow mask in lane one. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		constexpr std::uint64_t maximum = static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max());
		constexpr std::uint64_t threshold = maximum * maximum + maximum + 1;
		const __m128i minimum = _mm_set1_epi32(std::numeric_limits<std::int32_t>::min());
		if (_mm_movemask_epi8(_mm_cmpeq_epi32(lhs, minimum)) != 0)
			return magnitude_checked_result<std::int32_t>(maximum, true);
		const __m128i evenSquares = _mm_mul_epi32(lhs, lhs);
		const __m128i shifted = _mm_srli_si128(lhs, 4);
		const __m128i oddSquares = _mm_mul_epi32(shifted, shifted);
		const __m128i pairTotals = _mm_add_epi64(evenSquares, oddSquares);
		const __m128i totalVector = _mm_add_epi64(pairTotals, _mm_srli_si128(pairTotals, 8));
		const std::uint64_t total = static_cast<std::uint64_t>(_mm_cvtsi128_si64(totalVector));
		const bool overflow = total >= threshold;
		const std::uint64_t result = overflow ? maximum : magnitude_round_sqrt_u64(total, maximum);
		return magnitude_checked_result<std::int32_t>(result, overflow);
	}

	/** @brief Computes minimum-value position metadata for this native register specialization. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		constexpr __m128i indices = register_from_values<__m128i, std::int32_t>(0, 1, 2, 3);
		__m128i values = lhs;
		__m128i positions = indices;
		const __m128i shifted1Values = _mm_bsrli_si128(values, 4);
		const __m128i shifted1Indices = _mm_bsrli_si128(positions, 4);
		const __m128i less1 = _mm_cmpgt_epi32(values, shifted1Values);
		values = _mm_blendv_epi8(values, shifted1Values, less1);
		positions = _mm_blendv_epi8(positions, shifted1Indices, less1);
		const __m128i shifted2Values = _mm_bsrli_si128(values, 8);
		const __m128i shifted2Indices = _mm_bsrli_si128(positions, 8);
		const __m128i less2 = _mm_cmpgt_epi32(values, shifted2Values);
		values = _mm_blendv_epi8(values, shifted2Values, less2);
		positions = _mm_blendv_epi8(positions, shifted2Indices, less2);
		return _mm_insert_epi32(values, _mm_extract_epi32(positions, 0), 1);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m128i lhs, __m128i rhs) noexcept
	{
		return _mm_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _mm_abs_epi32(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi32(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm_min_epi32(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm_max_epi32(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _mm_slli_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _mm_srli_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _mm_srai_epi32(lhs, rhs);
	}

	// arithmetic (horizontal)
	/** @brief Horizontally adds adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm_hadd_epi32(lhs, rhs);
	}
	/** @brief Horizontally subtracts adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm_hsub_epi32(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm_set1_epi32(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args &&...args) noexcept
	{
		return _mm_set_epi32(args...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args &&...args) noexcept
	{
		return _mm_setr_epi32(args...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpeq_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpgt_epi32(lhs, rhs);
	}

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm_cvtepi32_epi64(lhs, rhs);
	}
	template <class target_simd> static typename target_simd::vector_t SIMD_FLAGS(InOut, ForceInline) widen(auto lhs) noexcept
	{
		using target_element_t = typename target_simd::element_type;
		if constexpr (sizeof(target_element_t) == sizeof(int64_t))
		{
			if constexpr (target_simd::register_width == 128)
				return _mm_cvtepi32_epi64(lhs);
			else if constexpr (target_simd::register_width == 256)
				return _mm256_cvtepi32_epi64(lhs);
			else
				static_assert(dependent_false_v<target_simd>,
							  "No direct widen mapping exists for SimdImpl128<int32_t> and the requested destination SIMD shape.");
		}
		else
		{
			static_assert(dependent_false_v<target_simd>, "No direct widen mapping exists for SimdImpl128<int32_t> and the requested destination SIMD shape.");
		}
	}
	static auto SIMD_FLAGS(InOut, ForceInline) compress(auto lhs, auto rhs) noexcept
	{
		return _mm_packs_epi32(lhs, rhs);
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<int32_t>(_mm_extract_epi32(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected signed 32-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 4)`.
	 * @return Selected scalar lane.
	 */
	static int32_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m128i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 4, "Signed 32-bit extraction requires a valid 128-bit lane index");
		switch (index)
		{
		case 0:
			return extract<0>(lhs);
		case 1:
			return extract<1>(lhs);
		case 2:
			return extract<2>(lhs);
		case 3:
			return extract<3>(lhs);
		default:
			return extract<0>(lhs);
		}
	}
	/** @brief Replaces the compile-time-selected signed 32-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const int32_t rhs) noexcept
	{
		return register_insert_constexpr<int32_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected signed 32-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const int32_t rhs) noexcept
	{
		return _mm_insert_epi32(lhs, rhs, index);
	}
	/**
	 * @brief Replaces one runtime-selected signed 32-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 4)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m128i lhs, const int32_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 4, "Signed 32-bit insertion requires a valid 128-bit lane index");
		const __m128i lane_indices = _mm_setr_epi32(0, 1, 2, 3);
		const __m128i selected_lane = _mm_cmpeq_epi32(lane_indices, _mm_set1_epi32(index));
		return _mm_blendv_epi8(lhs, _mm_set1_epi32(rhs), selected_lane);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm_unpacklo_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm_unpackhi_epi32(lhs, rhs);
	}

	// misc
	/** @brief Emulates an immediate-controlled low-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each low four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_lo_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_half_16_slow(lhs, static_cast<unsigned int>(rhs), false);
	}
	/** @brief Emulates an immediate-controlled high-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each high four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_hi_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_half_16_slow(lhs, static_cast<unsigned int>(rhs), true);
	}
	/** @brief Emulates an immediate-controlled blend with a runtime scalar mask.
	 *  @param lhs Source for lanes whose control bits are clear.
	 *  @param rhs Source for lanes whose control bits are set.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the selected lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) blend_slow(auto lhs, auto rhs, const int imm8) noexcept
	{
		return register_blend_slow<std::int32_t>(lhs, rhs, static_cast<unsigned int>(imm8));
	}
	/** @brief Selects signed 32-bit lanes from two registers with an immediate control. */
	template <int imm8> SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static auto VECTORCALL blend(auto lhs, auto rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return register_blend_slow<std::int32_t>(lhs, rhs, static_cast<unsigned int>(imm8));
		return _mm_castps_si128(_mm_blend_ps(_mm_castsi128_ps(lhs), _mm_castsi128_ps(rhs), imm8 & 0x0F));
	}
};

template <> struct SimdImpl128<uint32_t>
{
	/** @brief Selects 32-bit lanes from two registers using a canonical predicate register. */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m128i condition, __m128i when_true, __m128i when_false) noexcept
	{
		return _mm_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical unsigned 32-bit lanes using compile-time source selectors.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 4 && ((indices < 4) && ...))
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m128i lhs) noexcept
	{
		return _mm_shuffle_epi32(lhs, encode_logical_shuffle_32_immediate<indices...>());
	}
	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm_add_epi32(lhs, rhs);
	}
	/**
	 * @brief Converts unsigned 32-bit lanes to floating-point lanes.
	 *
	 * @param lhs The unsigned integer lanes.
	 * @return The converted floating-point lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) convert_to_float(const __m128i lhs) noexcept
	{
		return _ext_cvtepu32_ps(lhs);
	}
	/** @brief Multiplies adjacent lanes and adds their products for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		const __m128i evenProducts = _mm_mul_epu32(lhs, rhs);
		const __m128i oddProducts = _mm_mul_epu32(_mm_srli_si128(lhs, 4), _mm_srli_si128(rhs, 4));
		return _mm_add_epi64(evenProducts, oddProducts);
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm_maddubs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _mm_mullo_epi32(lhs, rhs);
	}
	/**
	 * @brief Divides corresponding unsigned 32-bit lanes exactly.
	 *
	 * @param lhs The dividend lanes.
	 * @param rhs The nonzero divisor lanes.
	 * @return The truncating integer quotients.
	 */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext128_div_epu32(lhs, rhs);
	}
	/** @brief Computes corresponding unsigned 32-bit remainders with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext128_rem_epu32(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		const __m128 roots = _mm_sqrt_ps(_ext_cvtepu32_ps(lhs));
		return _mm_cvtps_epi32(roots);
	}
	/** @brief Computes the unchecked group magnitude in lane zero; all other lanes are unspecified. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const __m128i pairSums = multiply_add_adjacent(lhs, lhs);
		const __m128i totalVector = _mm_add_epi64(pairSums, _mm_srli_si128(pairSums, 8));
		const std::uint64_t total = static_cast<std::uint64_t>(_mm_cvtsi128_si64(totalVector));
		const __m128d root = _mm_sqrt_sd(_mm_setzero_pd(), _mm_set_sd(static_cast<double>(total)));
		return _mm_cvtsi64_si128(_mm_cvtsd_si64(root));
	}

	/** @brief Computes a saturated magnitude in lane zero and a canonical overflow mask in lane one. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		constexpr std::uint64_t maximum = static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max());
		constexpr std::uint64_t threshold = maximum * maximum + maximum + 1;
		const __m128i evenSquares = _mm_mul_epu32(lhs, lhs);
		const __m128i shifted = _mm_srli_si128(lhs, 4);
		const __m128i oddSquares = _mm_mul_epu32(shifted, shifted);
		const __m128i pairTotals = _mm_add_epi64(evenSquares, oddSquares);
		const __m128i signBit = _mm_set1_epi64x(std::numeric_limits<std::int64_t>::min());
		const __m128i pairCarries = _mm_cmpgt_epi64(_mm_xor_si128(evenSquares, signBit), _mm_xor_si128(pairTotals, signBit));
		const std::uint64_t lowTotal = static_cast<std::uint64_t>(_mm_cvtsi128_si64(pairTotals));
		const std::uint64_t highTotal = static_cast<std::uint64_t>(_mm_extract_epi64(pairTotals, 1));
		const std::uint64_t total = lowTotal + highTotal;
		const bool overflow = _mm_movemask_pd(_mm_castsi128_pd(pairCarries)) != 0 || total < lowTotal || total >= threshold;
		const std::uint64_t result = overflow ? maximum : magnitude_round_sqrt_u64(total, maximum);
		return magnitude_checked_result<std::uint32_t>(result, overflow);
	}

	/** @brief Computes minimum-value position metadata for this native register specialization. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		constexpr __m128i indices = register_from_values<__m128i, std::uint32_t>(0u, 1u, 2u, 3u);
		const __m128i signBit = _mm_set1_epi32(static_cast<int>(0x80000000u));
		__m128i values = lhs;
		__m128i positions = indices;
		const __m128i shifted1Values = _mm_bsrli_si128(values, 4);
		const __m128i shifted1Indices = _mm_bsrli_si128(positions, 4);
		const __m128i less1 = _mm_cmpgt_epi32(_mm_xor_si128(values, signBit), _mm_xor_si128(shifted1Values, signBit));
		values = _mm_blendv_epi8(values, shifted1Values, less1);
		positions = _mm_blendv_epi8(positions, shifted1Indices, less1);
		const __m128i shifted2Values = _mm_bsrli_si128(values, 8);
		const __m128i shifted2Indices = _mm_bsrli_si128(positions, 8);
		const __m128i less2 = _mm_cmpgt_epi32(_mm_xor_si128(values, signBit), _mm_xor_si128(shifted2Values, signBit));
		values = _mm_blendv_epi8(values, shifted2Values, less2);
		positions = _mm_blendv_epi8(positions, shifted2Indices, less2);
		return _mm_insert_epi32(values, _mm_extract_epi32(positions, 0), 1);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m128i lhs, __m128i rhs) noexcept
	{
		return _mm_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _mm_abs_epi32(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi32(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm_min_epu32(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm_max_epu32(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _mm_slli_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _mm_srli_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _mm_srai_epi32(lhs, rhs);
	}

	// arithmetic (horizontal)
	/** @brief Horizontally adds adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm_hadd_epi32(lhs, rhs);
	}
	/** @brief Horizontally subtracts adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm_hsub_epi32(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm_set1_epi32(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args... args) noexcept
	{
		return _mm_set_epi32(args...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args... args) noexcept
	{
		return _mm_setr_epi32(args...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpeq_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _ext_cmpgt_epu32(lhs, rhs);
	}

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm_cvtepu32_epi64(lhs, rhs);
	}
	template <class target_simd> static typename target_simd::vector_t SIMD_FLAGS(InOut, ForceInline) widen(auto lhs) noexcept
	{
		using target_element_t = typename target_simd::element_type;
		if constexpr (sizeof(target_element_t) == sizeof(uint64_t))
		{
			if constexpr (target_simd::register_width == 128)
				return _mm_cvtepu32_epi64(lhs);
			else if constexpr (target_simd::register_width == 256)
				return _mm256_cvtepu32_epi64(lhs);
			else
				static_assert(dependent_false_v<target_simd>,
							  "No direct widen mapping exists for SimdImpl128<uint32_t> and the requested destination SIMD shape.");
		}
		else
		{
			static_assert(dependent_false_v<target_simd>, "No direct widen mapping exists for SimdImpl128<uint32_t> and the requested destination SIMD shape.");
		}
	}
	static auto SIMD_FLAGS(InOut, ForceInline) compress(auto lhs, auto rhs) noexcept
	{
		return _mm_packus_epi32(lhs, rhs);
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<uint32_t>(_mm_extract_epi32(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected unsigned 32-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 4)`.
	 * @return Selected scalar lane.
	 */
	static uint32_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m128i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 4, "Unsigned 32-bit extraction requires a valid 128-bit lane index");
		switch (index)
		{
		case 0:
			return extract<0>(lhs);
		case 1:
			return extract<1>(lhs);
		case 2:
			return extract<2>(lhs);
		case 3:
			return extract<3>(lhs);
		default:
			return extract<0>(lhs);
		}
	}
	/** @brief Replaces the compile-time-selected unsigned 32-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const uint32_t rhs) noexcept
	{
		return register_insert_constexpr<uint32_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected unsigned 32-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const uint32_t rhs) noexcept
	{
		return _mm_insert_epi32(lhs, std::bit_cast<int32_t>(rhs), index);
	}
	/**
	 * @brief Replaces one runtime-selected unsigned 32-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 4)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m128i lhs, const uint32_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 4, "Unsigned 32-bit insertion requires a valid 128-bit lane index");
		const __m128i lane_indices = _mm_setr_epi32(0, 1, 2, 3);
		const __m128i selected_lane = _mm_cmpeq_epi32(lane_indices, _mm_set1_epi32(index));
		return _mm_blendv_epi8(lhs, _mm_set1_epi32(std::bit_cast<int32_t>(rhs)), selected_lane);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm_unpacklo_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm_unpackhi_epi32(lhs, rhs);
	}

	// misc
	/** @brief Emulates an immediate-controlled low-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each low four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_lo_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_half_16_slow(lhs, static_cast<unsigned int>(rhs), false);
	}
	/** @brief Emulates an immediate-controlled high-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each high four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_hi_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_half_16_slow(lhs, static_cast<unsigned int>(rhs), true);
	}
	/** @brief Emulates an immediate-controlled blend with a runtime scalar mask.
	 *  @param lhs Source for lanes whose control bits are clear.
	 *  @param rhs Source for lanes whose control bits are set.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the selected lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) blend_slow(auto lhs, auto rhs, const int imm8) noexcept
	{
		return register_blend_slow<std::int32_t>(lhs, rhs, static_cast<unsigned int>(imm8));
	}
	/** @brief Selects unsigned 32-bit lanes from two registers with an immediate control. */
	template <int imm8> SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static auto VECTORCALL blend(auto lhs, auto rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return register_blend_slow<std::uint32_t>(lhs, rhs, static_cast<unsigned int>(imm8));
		return _mm_castps_si128(_mm_blend_ps(_mm_castsi128_ps(lhs), _mm_castsi128_ps(rhs), imm8 & 0x0F));
	}
};

template <> struct SimdImpl128<int64_t>
{
	/** @brief Selects 64-bit lanes from two registers using a canonical predicate register. */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m128i condition, __m128i when_true, __m128i when_false) noexcept
	{
		return _mm_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical signed 64-bit lanes using compile-time source selectors.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 2 && ((indices < 2) && ...))
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m128i lhs) noexcept
	{
		return _mm_shuffle_epi32(lhs, encode_logical_shuffle_64_immediate<indices...>());
	}
	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm_add_epi64(lhs, rhs);
	}
	/** @brief Multiplies adjacent lanes and adds their products for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		const __m128i productLow = _mm_mul_epu32(lhs, rhs);
		const __m128i lhsHigh = _mm_srli_epi64(lhs, 32);
		const __m128i rhsHigh = _mm_srli_epi64(rhs, 32);
		const __m128i cross = _mm_add_epi64(_mm_mul_epu32(lhsHigh, rhs), _mm_mul_epu32(lhs, rhsHigh));
		const __m128i products = _mm_add_epi64(productLow, _mm_slli_epi64(cross, 32));
		const __m128i shifted = _mm_bsrli_si128(products, 8);
		const __m128i sum = _mm_add_epi64(products, shifted);
		return _mm_unpacklo_epi64(sum, _mm_setzero_si128());
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm_maddubs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _ext_mullo_epi64(lhs, rhs);
	}
	/** @brief Divides corresponding signed 64-bit lanes with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext128_div_epi64(lhs, rhs);
	}
	/** @brief Computes corresponding signed 64-bit remainders with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext128_rem_epi64(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		const __m128d roots = _mm_sqrt_pd(_mm_setr_pd(static_cast<double>(_mm_cvtsi128_si64(lhs)), static_cast<double>(_mm_extract_epi64(lhs, 1))));
		const auto lowRoot = static_cast<std::int64_t>(_mm_cvtsd_f64(roots));
		const auto highRoot = static_cast<std::int64_t>(_mm_cvtsd_f64(_mm_unpackhi_pd(roots, roots)));
		return _mm_set_epi64x(highRoot, lowRoot);
	}
	/** @brief Computes the unchecked group magnitude in lane zero; lane one is unspecified. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const std::uint64_t rawLow = static_cast<std::uint64_t>(_mm_cvtsi128_si64(lhs));
		const std::uint64_t rawHigh = static_cast<std::uint64_t>(_mm_extract_epi64(lhs, 1));
		const std::uint64_t lowSign = rawLow >> 63;
		const std::uint64_t highSign = rawHigh >> 63;
		const std::uint64_t lowValue = (rawLow ^ (std::uint64_t{0} - lowSign)) + lowSign;
		const std::uint64_t highValue = (rawHigh ^ (std::uint64_t{0} - highSign)) + highSign;
		const __m128i lowSquare = magnitude_square_u64(lowValue);
		const __m128i highSquare = magnitude_square_u64(highValue);
		const std::uint64_t lowWord0 = static_cast<std::uint64_t>(_mm_cvtsi128_si64(lowSquare));
		const std::uint64_t lowWord1 = static_cast<std::uint64_t>(_mm_cvtsi128_si64(highSquare));
		const std::uint64_t highWord0 = static_cast<std::uint64_t>(_mm_extract_epi64(lowSquare, 1));
		const std::uint64_t highWord1 = static_cast<std::uint64_t>(_mm_extract_epi64(highSquare, 1));
		const std::uint64_t lowWord = lowWord0 + lowWord1;
		const std::uint64_t highWord = highWord0 + highWord1 + static_cast<std::uint64_t>(lowWord < lowWord0);
		const std::uint64_t result = magnitude_round_sqrt_u128(lowWord, highWord, static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()));
		return _mm_cvtsi64_si128(static_cast<std::int64_t>(result));
	}

	/** @brief Computes a saturated magnitude in lane zero and a canonical overflow mask in lane one. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		constexpr std::uint64_t maximum = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
		constexpr std::uint64_t thresholdLow = 0x8000'0000'0000'0001ULL;
		constexpr std::uint64_t thresholdHigh = 0x3FFF'FFFF'FFFF'FFFFULL;
		const std::uint64_t rawLow = static_cast<std::uint64_t>(_mm_cvtsi128_si64(lhs));
		const std::uint64_t rawHigh = static_cast<std::uint64_t>(_mm_extract_epi64(lhs, 1));
		const std::uint64_t lowSign = rawLow >> 63;
		const std::uint64_t highSign = rawHigh >> 63;
		const std::uint64_t lowValue = (rawLow ^ (std::uint64_t{0} - lowSign)) + lowSign;
		const std::uint64_t highValue = (rawHigh ^ (std::uint64_t{0} - highSign)) + highSign;
		const __m128i lowSquare = magnitude_square_u64(lowValue);
		const __m128i highSquare = magnitude_square_u64(highValue);
		const std::uint64_t lowWord0 = static_cast<std::uint64_t>(_mm_cvtsi128_si64(lowSquare));
		const std::uint64_t lowWord1 = static_cast<std::uint64_t>(_mm_cvtsi128_si64(highSquare));
		const std::uint64_t highWord0 = static_cast<std::uint64_t>(_mm_extract_epi64(lowSquare, 1));
		const std::uint64_t highWord1 = static_cast<std::uint64_t>(_mm_extract_epi64(highSquare, 1));
		const std::uint64_t lowWord = lowWord0 + lowWord1;
		const std::uint64_t carry = static_cast<std::uint64_t>(lowWord < lowWord0);
		const std::uint64_t highPartial = highWord0 + highWord1;
		const bool highOverflow = highPartial < highWord0 || highPartial + carry < highPartial;
		const std::uint64_t highWord = highPartial + carry;
		const bool overflow = highOverflow || highWord > thresholdHigh || (highWord == thresholdHigh && lowWord >= thresholdLow);
		const std::uint64_t result = overflow ? maximum : magnitude_round_sqrt_u128(lowWord, highWord, maximum);
		return magnitude_checked_result<std::int64_t>(result, overflow);
	}

	/** @brief Computes minimum-value position metadata for this native register specialization. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		constexpr __m128i indices = register_from_values<__m128i, std::int64_t>(0, 1);
		const __m128i shiftedValues = _mm_bsrli_si128(lhs, 8);
		const __m128i shiftedIndices = _mm_bsrli_si128(indices, 8);
		const __m128i less = _mm_cmpgt_epi64(lhs, shiftedValues);
		const __m128i values = _mm_blendv_epi8(lhs, shiftedValues, less);
		const __m128i positions = _mm_blendv_epi8(indices, shiftedIndices, less);
		return _mm_insert_epi64(values, _mm_extract_epi64(positions, 0), 1);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m128i lhs, __m128i rhs) noexcept
	{
		return _mm_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _ext_abs_epi64(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi64(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _ext_min_epi64(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _ext_max_epi64(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _mm_slli_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _mm_srli_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _ext_srai_epi64(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm_set1_epi64x(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args... args) noexcept
	{
		return _mm_set_epi64x(args...);
	}
	/**
	 * @brief Constructs two signed 64-bit lanes in low-to-high logical order.
	 * @param low Value for lane zero.
	 * @param high Value for lane one.
	 * @return Register containing low followed by high.
	 */
	static __m128i SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(const std::int64_t low, const std::int64_t high) noexcept
	{
		return _mm_set_epi64x(high, low);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpeq_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpgt_epi64(lhs, rhs);
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<int64_t>(_mm_extract_epi64(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected signed 64-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 2)`.
	 * @return Selected scalar lane.
	 */
	static int64_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m128i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 2, "Signed 64-bit extraction requires a valid 128-bit lane index");
		switch (index)
		{
		case 0:
			return extract<0>(lhs);
		case 1:
			return extract<1>(lhs);
		default:
			return extract<0>(lhs);
		}
	}
	/** @brief Replaces the compile-time-selected signed 64-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const int64_t rhs) noexcept
	{
		return register_insert_constexpr<int64_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected signed 64-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const int64_t rhs) noexcept
	{
		return _mm_insert_epi64(lhs, rhs, index);
	}
	/**
	 * @brief Replaces one runtime-selected signed 64-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 2)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m128i lhs, const int64_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 2, "Signed 64-bit insertion requires a valid 128-bit lane index");
		const __m128i lane_indices = _mm_set_epi64x(1, 0);
		const __m128i selected_lane = _mm_cmpeq_epi64(lane_indices, _mm_set1_epi64x(index));
		return _mm_blendv_epi8(lhs, _mm_set1_epi64x(rhs), selected_lane);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm_unpacklo_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm_unpackhi_epi64(lhs, rhs);
	}
};

template <> struct SimdImpl128<uint64_t>
{
	/** @brief Selects 64-bit lanes from two registers using a canonical predicate register. */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m128i condition, __m128i when_true, __m128i when_false) noexcept
	{
		return _mm_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical unsigned 64-bit lanes using compile-time source selectors.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 2 && ((indices < 2) && ...))
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m128i lhs) noexcept
	{
		return _mm_shuffle_epi32(lhs, encode_logical_shuffle_64_immediate<indices...>());
	}
	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm_add_epi64(lhs, rhs);
	}
	/** @brief Multiplies adjacent lanes and adds their products for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		const __m128i productLow = _mm_mul_epu32(lhs, rhs);
		const __m128i lhsHigh = _mm_srli_epi64(lhs, 32);
		const __m128i rhsHigh = _mm_srli_epi64(rhs, 32);
		const __m128i cross = _mm_add_epi64(_mm_mul_epu32(lhsHigh, rhs), _mm_mul_epu32(lhs, rhsHigh));
		const __m128i products = _mm_add_epi64(productLow, _mm_slli_epi64(cross, 32));
		const __m128i shifted = _mm_bsrli_si128(products, 8);
		const __m128i sum = _mm_add_epi64(products, shifted);
		return _mm_unpacklo_epi64(sum, _mm_setzero_si128());
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm_maddubs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _ext_mullo_epi64(lhs, rhs);
	}
	/** @brief Divides corresponding unsigned 64-bit lanes with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext128_div_epu64(lhs, rhs);
	}
	/** @brief Computes corresponding unsigned 64-bit remainders with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext128_rem_epu64(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		const __m128d roots = _mm_sqrt_pd(_mm_setr_pd(static_cast<double>(static_cast<uint64_t>(_mm_cvtsi128_si64(lhs))),
													  static_cast<double>(static_cast<uint64_t>(_mm_extract_epi64(lhs, 1)))));
		const auto lowRoot = static_cast<std::int64_t>(_mm_cvtsd_f64(roots));
		const auto highRoot = static_cast<std::int64_t>(_mm_cvtsd_f64(_mm_unpackhi_pd(roots, roots)));
		return _mm_set_epi64x(highRoot, lowRoot);
	}
	/** @brief Computes the unchecked group magnitude in lane zero; lane one is unspecified. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const std::uint64_t lowValue = static_cast<std::uint64_t>(_mm_cvtsi128_si64(lhs));
		const std::uint64_t highValue = static_cast<std::uint64_t>(_mm_extract_epi64(lhs, 1));
		const __m128i lowSquare = magnitude_square_u64(lowValue);
		const __m128i highSquare = magnitude_square_u64(highValue);
		const std::uint64_t lowWord0 = static_cast<std::uint64_t>(_mm_cvtsi128_si64(lowSquare));
		const std::uint64_t lowWord1 = static_cast<std::uint64_t>(_mm_cvtsi128_si64(highSquare));
		const std::uint64_t highWord0 = static_cast<std::uint64_t>(_mm_extract_epi64(lowSquare, 1));
		const std::uint64_t highWord1 = static_cast<std::uint64_t>(_mm_extract_epi64(highSquare, 1));
		const std::uint64_t lowWord = lowWord0 + lowWord1;
		const std::uint64_t highWord = highWord0 + highWord1 + static_cast<std::uint64_t>(lowWord < lowWord0);
		const std::uint64_t result = magnitude_round_sqrt_u128(lowWord, highWord, std::numeric_limits<std::uint64_t>::max());
		return _mm_cvtsi64_si128(static_cast<std::int64_t>(result));
	}

	/** @brief Computes a saturated magnitude in lane zero and a canonical overflow mask in lane one. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		constexpr std::uint64_t maximum = std::numeric_limits<std::uint64_t>::max();
		constexpr std::uint64_t thresholdLow = 1;
		constexpr std::uint64_t thresholdHigh = maximum;
		const std::uint64_t lowValue = static_cast<std::uint64_t>(_mm_cvtsi128_si64(lhs));
		const std::uint64_t highValue = static_cast<std::uint64_t>(_mm_extract_epi64(lhs, 1));
		const __m128i lowSquare = magnitude_square_u64(lowValue);
		const __m128i highSquare = magnitude_square_u64(highValue);
		const std::uint64_t lowWord0 = static_cast<std::uint64_t>(_mm_cvtsi128_si64(lowSquare));
		const std::uint64_t lowWord1 = static_cast<std::uint64_t>(_mm_cvtsi128_si64(highSquare));
		const std::uint64_t highWord0 = static_cast<std::uint64_t>(_mm_extract_epi64(lowSquare, 1));
		const std::uint64_t highWord1 = static_cast<std::uint64_t>(_mm_extract_epi64(highSquare, 1));
		const std::uint64_t lowWord = lowWord0 + lowWord1;
		const std::uint64_t carry = static_cast<std::uint64_t>(lowWord < lowWord0);
		const std::uint64_t highPartial = highWord0 + highWord1;
		const bool highOverflow = highPartial < highWord0 || highPartial + carry < highPartial;
		const std::uint64_t highWord = highPartial + carry;
		const bool overflow = highOverflow || highWord > thresholdHigh || (highWord == thresholdHigh && lowWord >= thresholdLow);
		const std::uint64_t result = overflow ? maximum : magnitude_round_sqrt_u128(lowWord, highWord, maximum);
		return magnitude_checked_result<std::uint64_t>(result, overflow);
	}

	/** @brief Computes minimum-value position metadata for this native register specialization. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		constexpr __m128i indices = register_from_values<__m128i, std::uint64_t>(0ull, 1ull);
		const __m128i signBit = _mm_set1_epi64x(std::numeric_limits<std::int64_t>::min());
		const __m128i shiftedValues = _mm_bsrli_si128(lhs, 8);
		const __m128i shiftedIndices = _mm_bsrli_si128(indices, 8);
		const __m128i less = _mm_cmpgt_epi64(_mm_xor_si128(lhs, signBit), _mm_xor_si128(shiftedValues, signBit));
		const __m128i values = _mm_blendv_epi8(lhs, shiftedValues, less);
		const __m128i positions = _mm_blendv_epi8(indices, shiftedIndices, less);
		return _mm_insert_epi64(values, _mm_extract_epi64(positions, 0), 1);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m128i lhs, __m128i rhs) noexcept
	{
		return _mm_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return lhs;
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_epi64(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _ext_min_epu64(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _ext_max_epu64(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _mm_slli_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _mm_srli_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _ext_srai_epi64(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm_set1_epi64x(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args &&...args) noexcept
	{
		return _mm_set_epi64x(args...);
	}
	/**
	 * @brief Constructs two unsigned 64-bit lanes in low-to-high logical order.
	 * @param low Value for lane zero.
	 * @param high Value for lane one.
	 * @return Register containing the exact low and high lane bit patterns.
	 */
	static __m128i SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(const std::uint64_t low, const std::uint64_t high) noexcept
	{
		return _mm_set_epi64x(std::bit_cast<std::int64_t>(high), std::bit_cast<std::int64_t>(low));
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpeq_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _ext_cmpgt_epu64(lhs, rhs);
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<uint64_t>(_mm_extract_epi64(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected unsigned 64-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 2)`.
	 * @return Selected scalar lane.
	 */
	static uint64_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m128i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 2, "Unsigned 64-bit extraction requires a valid 128-bit lane index");
		switch (index)
		{
		case 0:
			return extract<0>(lhs);
		case 1:
			return extract<1>(lhs);
		default:
			return extract<0>(lhs);
		}
	}
	/** @brief Replaces the compile-time-selected unsigned 64-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const uint64_t rhs) noexcept
	{
		return register_insert_constexpr<uint64_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected unsigned 64-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const uint64_t rhs) noexcept
	{
		return _mm_insert_epi64(lhs, std::bit_cast<int64_t>(rhs), index);
	}
	/**
	 * @brief Replaces one runtime-selected unsigned 64-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 2)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m128i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m128i lhs, const uint64_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 2, "Unsigned 64-bit insertion requires a valid 128-bit lane index");
		const __m128i lane_indices = _mm_set_epi64x(1, 0);
		const __m128i selected_lane = _mm_cmpeq_epi64(lane_indices, _mm_set1_epi64x(index));
		return _mm_blendv_epi8(lhs, _mm_set1_epi64x(std::bit_cast<int64_t>(rhs)), selected_lane);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm_unpacklo_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm_unpackhi_epi64(lhs, rhs);
	}
};

template <> struct SimdImpl128<float>
{
	/** @brief Selects float lanes from two registers using a canonical predicate register. */
	static __m128 SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m128 condition, __m128 when_true, __m128 when_false) noexcept
	{
		return _mm_blendv_ps(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical floating-point lanes using compile-time source selectors.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 4 && ((indices < 4) && ...))
	static __m128 SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m128 lhs) noexcept
	{
		return _mm_shuffle_ps(lhs, lhs, encode_logical_shuffle_32_immediate<indices...>());
	}
	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm_add_ps(lhs, rhs);
	}
	/** @brief Alternates lane subtraction and addition for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_subtract(auto lhs, auto rhs) noexcept
	{
		return _mm_addsub_ps(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_ps(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _mm_mul_ps(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _mm_div_ps(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		return _mm_sqrt_ps(lhs);
	}
	/** @brief Computes and broadcasts the 128-bit floating-point magnitude. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		return _mm_sqrt_ps(_mm_dp_ps(lhs, lhs, 0xFF));
	}
	/** @brief Multiplies lanes and adds a third register for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add(auto lhs, auto rhs, auto addend) noexcept
	{
#if SIMDLIB_HAS_FMA
		return _mm_fmadd_ps(lhs, rhs, addend);
#else
		return _mm_add_ps(_mm_mul_ps(lhs, rhs), addend);
#endif
	}
	/** @brief Computes an immediate-controlled dot product for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) dot_product(__m128 lhs, __m128 rhs) noexcept
	{
		return _mm_dp_ps(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _ext_abs_ps(lhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm_min_ps(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm_max_ps(lhs, rhs);
	}

	// arithmetic (horizontal)
	/** @brief Horizontally adds adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm_hadd_ps(lhs, rhs);
	}
	/** @brief Horizontally subtracts adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm_hsub_ps(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm_set_ps1(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args... args) noexcept
	{
		return _mm_set_ps(args...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args... args) noexcept
	{
		return _mm_setr_ps(args...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpeq_ps(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpgt_ps(lhs, rhs);
	}

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm_cvtps_epi32(lhs, rhs);
	}
	// static SIMDLIB_FORCE_INLINE auto VECTORCALL compress (auto lhs, auto rhs) noexcept { return _mm_cvtepi32_ps(lhs, rhs); }

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return _mm_cvtss_f32(_mm_shuffle_ps(lhs, lhs, index));
	}

	/**
	 * @brief Extracts one runtime-selected 32-bit floating-point lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 4)`.
	 * @return Selected scalar lane.
	 */
	static float SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m128 lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 4, "32-bit floating-point extraction requires a valid 128-bit lane index");
		switch (index)
		{
		case 0:
			return extract<0>(lhs);
		case 1:
			return extract<1>(lhs);
		case 2:
			return extract<2>(lhs);
		case 3:
			return extract<3>(lhs);
		default:
			return extract<0>(lhs);
		}
	}
	/** @brief Replaces the compile-time-selected 32-bit floating-point lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const float rhs) noexcept
	{
		return register_insert_constexpr<float>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected 32-bit floating-point lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const float rhs) noexcept
	{
		return _mm_insert_ps(lhs, _mm_set_ss(rhs), index << 4);
	}
	/**
	 * @brief Replaces one runtime-selected 32-bit floating-point lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane index.
	 * @return Register with the selected lane replaced.
	 */
	static __m128 SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m128 lhs, const float rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 4, "32-bit floating-point insertion requires a valid 128-bit lane index");
		switch (index)
		{
		case 1:
			return insert<1>(lhs, rhs);
		case 2:
			return insert<2>(lhs, rhs);
		case 3:
			return insert<3>(lhs, rhs);
		default:
			return insert<0>(lhs, rhs);
		}
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm_unpacklo_ps(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm_unpackhi_ps(lhs, rhs);
	}

	// misc
	/** @brief Emulates an immediate-controlled floating shuffle with a runtime scalar control.
	 *  @param lhs Source for the lower selected lanes in each group.
	 *  @param rhs Source for the upper selected lanes in each group.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the shuffled lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_slow(auto lhs, auto rhs, unsigned int imm8) noexcept
	{
		return register_shuffle_float_slow(lhs, rhs, imm8);
	}
	/** @brief Emulates an immediate-controlled blend with a runtime scalar mask.
	 *  @param lhs Source for lanes whose control bits are clear.
	 *  @param rhs Source for lanes whose control bits are set.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the selected lanes.
	 */
	SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY static auto VECTORCALL blend_slow(auto lhs, auto rhs, const int imm8) noexcept
	{
		return register_blend_slow<float>(lhs, rhs, static_cast<unsigned int>(imm8));
	}
	/** @brief Selects 32-bit floating-point lanes from two registers with an immediate control. */
	template <int imm8> SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static auto VECTORCALL blend(auto lhs, auto rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return register_blend_slow<float>(lhs, rhs, static_cast<unsigned int>(imm8));
		return _mm_blend_ps(lhs, rhs, imm8 & 0x0F);
	}
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) movemask(auto lhs) noexcept
	{
		return _mm_movemask_ps(lhs);
	}
};

template <> struct SimdImpl128<double>
{
	/** @brief Selects double lanes from two registers using a canonical predicate register. */
	static __m128d SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m128d condition, __m128d when_true, __m128d when_false) noexcept
	{
		return _mm_blendv_pd(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical double-precision floating-point lanes using compile-time source selectors.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 2 && ((indices < 2) && ...))
	static __m128d SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m128d lhs) noexcept
	{
		return _mm_shuffle_pd(lhs, lhs, encode_logical_shuffle_double_immediate<indices...>());
	}
	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm_add_pd(lhs, rhs);
	}
	/** @brief Alternates lane subtraction and addition for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_subtract(auto lhs, auto rhs) noexcept
	{
		return _mm_addsub_pd(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm_sub_pd(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _mm_mul_pd(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _mm_div_pd(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		return _mm_sqrt_pd(lhs);
	}
	/** @brief Computes and broadcasts the 128-bit floating-point magnitude. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		return _mm_sqrt_pd(_mm_dp_pd(lhs, lhs, 0x33));
	}
	/** @brief Multiplies lanes and adds a third register for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add(auto lhs, auto rhs, auto addend) noexcept
	{
#if SIMDLIB_HAS_FMA
		return _mm_fmadd_pd(lhs, rhs, addend);
#else
		return _mm_add_pd(_mm_mul_pd(lhs, rhs), addend);
#endif
	}
	/** @brief Computes an immediate-controlled dot product for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) dot_product(__m128d lhs, __m128d rhs) noexcept
	{
		return _mm_dp_pd(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _ext_abs_pd(lhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm_min_pd(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm_max_pd(lhs, rhs);
	}

	// arithmetic (horizontal)
	/** @brief Horizontally adds adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm_hadd_pd(lhs, rhs);
	}
	/** @brief Horizontally subtracts adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm_hsub_pd(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm_set1_pd(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args... args) noexcept
	{
		return _mm_set_pd(args...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args... args) noexcept
	{
		return _mm_setr_pd(args...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpeq_pd(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _mm_cmpgt_pd(lhs, rhs);
	}
	// static SIMDLIB_FORCE_INLINE auto VECTORCALL cmplt (auto lhs, auto rhs) noexcept { return _mm_cmplt_pd(lhs, rhs); }

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm_cvtps_epi32(lhs, rhs);
	}
	// static SIMDLIB_FORCE_INLINE auto VECTORCALL compress (auto lhs, auto rhs) noexcept { return _mm_cvtepi32_pd(lhs, rhs); }

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		if constexpr (index == 0)
			return _mm_cvtsd_f64(lhs);
		else
			return _mm_cvtsd_f64(_mm_unpackhi_pd(lhs, lhs));
	}
	/**
	 * @brief Extracts one runtime-selected 64-bit floating-point lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 2)`.
	 * @return Selected scalar lane.
	 */
	static double SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m128d lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 2, "64-bit floating-point extraction requires a valid 128-bit lane index");
		switch (index)
		{
		case 0:
			return extract<0>(lhs);
		case 1:
			return extract<1>(lhs);
		default:
			return extract<0>(lhs);
		}
	}
	/** @brief Replaces the compile-time-selected 64-bit floating-point lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const double rhs) noexcept
	{
		return register_insert_constexpr<double>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected 64-bit floating-point lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const double rhs) noexcept
	{
		const __m128d replacement = _mm_set_sd(rhs);
		if constexpr (index == 0)
			return _mm_move_sd(lhs, replacement);
		else
			return _mm_unpacklo_pd(lhs, replacement);
	}
	/**
	 * @brief Replaces one runtime-selected 64-bit floating-point lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane index.
	 * @return Register with the selected lane replaced.
	 */
	static __m128d SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m128d lhs, const double rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 2, "64-bit floating-point insertion requires a valid 128-bit lane index");
		switch (index)
		{
		case 1:
			return insert<1>(lhs, rhs);
		default:
			return insert<0>(lhs, rhs);
		}
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm_unpacklo_pd(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm_unpackhi_pd(lhs, rhs);
	}

	// misc
	/** @brief Emulates an immediate-controlled floating shuffle with a runtime scalar control.
	 *  @param lhs Source for the lower selected lanes in each group.
	 *  @param rhs Source for the upper selected lanes in each group.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the shuffled lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_slow(auto lhs, auto rhs, unsigned int imm8) noexcept
	{
		return register_shuffle_double_slow(lhs, rhs, imm8);
	}
	/** @brief Emulates an immediate-controlled blend with a runtime scalar mask.
	 *  @param lhs Source for lanes whose control bits are clear.
	 *  @param rhs Source for lanes whose control bits are set.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the selected lanes.
	 */
	SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY static auto VECTORCALL blend_slow(auto lhs, auto rhs, const int imm8) noexcept
	{
		return register_blend_slow<double>(lhs, rhs, static_cast<unsigned int>(imm8));
	}
	/** @brief Selects 64-bit floating-point lanes from two registers with an immediate control. */
	template <int imm8> SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static auto VECTORCALL blend(auto lhs, auto rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return register_blend_slow<double>(lhs, rhs, static_cast<unsigned int>(imm8));
		return _mm_blend_pd(lhs, rhs, imm8 & 0x03);
	}
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) movemask(auto lhs) noexcept
	{
		return _mm_movemask_pd(lhs);
	}
};
#pragma endregion

#pragma region 128-bit Mappings

template <class element_t> struct SimdMappings<128, element_t> : public SimdImpl128<element_t>
{
  private:
	using impl = SimdImpl128<element_t>;

  public:
	using impl::extract;
	using impl::shuffle;

	template <class ty> using Mappings = SimdMappings<128, ty>;
	template <class ty> using mapped_vector_t = typename Mappings<ty>::vector_t;
	template <class ty>
		requires std::is_integral_v<ty>
	using promoted_signed_vector_t = mapped_vector_t<promoted_signed_t<ty>>;
	template <class ty>
		requires std::is_integral_v<ty>
	using promoted_unsigned_vector_t = mapped_vector_t<promoted_unsigned_t<ty>>;
	using mask_t = uint32_t;
	using int_vector_t = __m128i;
	using float_vector_t = __m128;
	using double_vector_t = __m128d;
	using vector_t = std::conditional_t<std::is_integral_v<element_t>, __m128i, std::conditional_t<std::is_same_v<element_t, float>, __m128, __m128d>>;
	constexpr static inline mask_t CMP_MASK = 0xFFFF;
	constexpr static inline std::size_t register_width = 128;
	constexpr static inline std::size_t element_count = register_width / (sizeof(element_t) * 8);
	constexpr static inline int_vector_t vector0 = register_from_values<int_vector_t, std::uint64_t>(0, 0);

#pragma region Set
	constexpr static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) setzero() noexcept
	{
		if (std::is_constant_evaluated())
		{
			return set1(element_t{0});
		}
		else
		{
			if constexpr (std::is_integral_v<element_t>)
				return _mm_setzero_si128();
			else if constexpr (std::is_same_v<element_t, float>)
				return _mm_setzero_ps();
			else if constexpr (std::is_same_v<element_t, double>)
				return _mm_setzero_pd();
		}
	}

	template <std::convertible_to<element_t>... Args>
		requires(sizeof...(Args) == element_count)
	constexpr static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) setr(Args &&...args) noexcept
	{
		if (std::is_constant_evaluated())
		{
			return setr_constexpr(std::forward<Args>(args)...);
		}
		else
		{
			return impl::setr(std::forward<Args>(args)...);
		}
	}

	constexpr static vector_t SIMD_FLAGS(Out, ForceInline, Flatten) construct(const std::array<element_t, element_count> &data) noexcept
	{
		if (std::is_constant_evaluated())
		{
			return register_from_array<vector_t>(data);
		}
		else
		{
			return load_unaligned(data.data());
		}
	}

	constexpr static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) set1(const element_t value) noexcept
	{
		if (std::is_constant_evaluated())
			return set1_constexpr(value);
		else
		{
			return impl::set1(value);
		}
	}

	/** @brief Broadcasts one value through the portable compile-time register representation. */
	constexpr static vector_t set1_constexpr(const element_t value) noexcept
	{
		return register_from_repeated_value<vector_t>(value);
	}

	/** @brief Constructs a register from forward-order lanes during constant evaluation. */
	template <std::convertible_to<element_t>... Args>
		requires(sizeof...(Args) == element_count)
	constexpr static vector_t setr_constexpr(Args &&...args) noexcept
	{
		return register_from_values<vector_t, element_t>(static_cast<element_t>(args)...);
	}

	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add(const vector_t lhs, const vector_t rhs, const vector_t addend) noexcept
	{
		if constexpr (requires(vector_t left, vector_t right, vector_t sum) { impl::multiply_add(left, right, sum); })
			return impl::multiply_add(lhs, rhs, addend);
		else
			return impl::add(impl::multiply(lhs, rhs), addend);
	}

	/// <summary>Broadcasts a 128-bit integer vector into both 128-bit lanes of a 256-bit integer vector.</summary>
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) broadcast_128(const typename SimdMappings<128, element_t>::int_vector_t v) noexcept
		requires std::is_integral_v<element_t>
	{
		return _mm256_broadcastsi128_si256(v);
	}

	static std::span<element_t, element_count> SIMD_FLAGS(Neither, ForceInline, Flatten) view_data(vector_t &vec) noexcept
	{
		return std::span<element_t, element_count>{register_data<element_t>(vec), element_count};
	}

	static std::span<const element_t, element_count> SIMD_FLAGS(Neither, ForceInline, Flatten) view_data(const vector_t &vec) noexcept
	{
		return std::span<const element_t, element_count>{register_data<element_t>(vec), element_count};
	}
#pragma endregion

#pragma region Load
	/**
	 * @brief Loads a complete 128-bit object representation without an alignment requirement.
	 * @param ptr Source containing at least 16 accessible bytes.
	 * @return Native register preserving every source bit.
	 */
	static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load_bytes(const void *ptr) noexcept
	{
		const int_vector_t bits = _mm_loadu_si128(reinterpret_cast<const int_vector_t *>(ptr));
		if constexpr (std::is_integral_v<element_t>)
			return bits;
		else if constexpr (std::is_same_v<element_t, float>)
			return _mm_castsi128_ps(bits);
		else
			return _mm_castsi128_pd(bits);
	}

	/// <summary>Loads a full register from memory. Pointer must be appropriately aligned for the register width.</summary>
	static int_vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load(const element_t *ptr) noexcept
		requires std::is_integral_v<element_t>
	{
		return _mm_load_si128(reinterpret_cast<const int_vector_t *>(ptr));
	}

	/// <summary>Loads a full register from memory without requiring alignment.</summary>
	static int_vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load_unaligned(const element_t *ptr) noexcept
		requires std::is_integral_v<element_t>
	{
		return _mm_loadu_si128(reinterpret_cast<const int_vector_t *>(ptr));
	}

	/// <summary>
	/// Loads the lower half of the register from memory (in bytes), zeroing the upper half.
	/// Intended for safe tail handling without over-reading past the end of a buffer.
	/// </summary>
	static int_vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load_half(const element_t *ptr) noexcept
		requires std::is_integral_v<element_t>
	{
		return _mm_loadl_epi64(reinterpret_cast<const int_vector_t *>(ptr));
	}

	/// <summary>Loads a full register from memory. Pointer must be appropriately aligned for the register width.</summary>
	static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load(const element_t *ptr) noexcept
		requires std::is_floating_point_v<element_t>
	{
		if constexpr (std::is_same_v<element_t, float>)
			return _mm_load_ps(ptr);
		else if constexpr (std::is_same_v<element_t, double>)
			return _mm_load_pd(ptr);
	}

	/// <summary>Loads a full register from memory without requiring alignment.</summary>
	static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load_unaligned(const element_t *ptr) noexcept
		requires std::is_floating_point_v<element_t>
	{
		if constexpr (std::is_same_v<element_t, float>)
			return _mm_loadu_ps(ptr);
		else if constexpr (std::is_same_v<element_t, double>)
			return _mm_loadu_pd(ptr);
	}
#pragma endregion

#pragma region Store
	/// <summary>Stores a full register to memory. Pointer must be appropriately aligned for the register width.</summary>
	static void SIMD_FLAGS(In, ForceInline, Flatten) store(int_vector_t lhs, void *ptr) noexcept
		requires std::is_integral_v<element_t>
	{
		_mm_store_si128(reinterpret_cast<int_vector_t *>(ptr), lhs);
	}

	/// <summary>Stores a full register to memory without requiring alignment.</summary>
	static void SIMD_FLAGS(In, ForceInline, Flatten) store_unaligned(int_vector_t lhs, void *ptr) noexcept
		requires std::is_integral_v<element_t>
	{
		_mm_storeu_si128(reinterpret_cast<int_vector_t *>(ptr), lhs);
	}

	/// <summary>
	/// Stores the lower half of the register to memory (in bytes).
	/// Intended for safe tail handling without over-writing past the end of a buffer.
	/// </summary>
	static void SIMD_FLAGS(In, ForceInline, Flatten) store_half(int_vector_t lhs, void *ptr) noexcept
		requires std::is_integral_v<element_t>
	{
		_mm_storel_epi64(reinterpret_cast<int_vector_t *>(ptr), lhs);
	}

	/// <summary>Stores a full register to memory. Pointer must be appropriately aligned for the register width.</summary>
	static void SIMD_FLAGS(In, ForceInline, Flatten) store(vector_t lhs, void *ptr) noexcept
		requires std::is_floating_point_v<element_t>
	{
		if constexpr (std::is_same_v<element_t, float>)
			_mm_store_ps(reinterpret_cast<float *>(ptr), lhs);
		else if constexpr (std::is_same_v<element_t, double>)
			_mm_store_pd(reinterpret_cast<double *>(ptr), lhs);
	}

	/// <summary>Stores a full register to memory without requiring alignment.</summary>
	static void SIMD_FLAGS(In, ForceInline, Flatten) store_unaligned(vector_t lhs, void *ptr) noexcept
		requires std::is_floating_point_v<element_t>
	{
		if constexpr (std::is_same_v<element_t, float>)
			_mm_storeu_ps(reinterpret_cast<float *>(ptr), lhs);
		else if constexpr (std::is_same_v<element_t, double>)
			_mm_storeu_pd(reinterpret_cast<double *>(ptr), lhs);
	}
#pragma endregion

#pragma region Bitwise Operations
	/**
	 * @brief Computes the bitwise AND of two mapped registers.
	 *
	 * @param lhs The first register.
	 * @param rhs The second register.
	 * @return The resulting mapped register.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_and(vector_t lhs, vector_t rhs) noexcept
	{
		if constexpr (std::is_integral_v<element_t>)
			return _mm_and_si128(lhs, rhs);
		else if constexpr (std::same_as<element_t, float>)
			return _mm_and_ps(lhs, rhs);
		else
			return _mm_and_pd(lhs, rhs);
	}
	/**
	 * @brief Computes the bitwise OR of two mapped registers.
	 *
	 * @param lhs The first register.
	 * @param rhs The second register.
	 * @return The resulting mapped register.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_or(vector_t lhs, vector_t rhs) noexcept
	{
		if constexpr (std::is_integral_v<element_t>)
			return _mm_or_si128(lhs, rhs);
		else if constexpr (std::same_as<element_t, float>)
			return _mm_or_ps(lhs, rhs);
		else
			return _mm_or_pd(lhs, rhs);
	}
	/**
	 * @brief Computes the bitwise XOR of two mapped registers.
	 *
	 * @param lhs The first register.
	 * @param rhs The second register.
	 * @return The resulting mapped register.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_xor(vector_t lhs, vector_t rhs) noexcept
	{
		if constexpr (std::is_integral_v<element_t>)
			return _mm_xor_si128(lhs, rhs);
		else if constexpr (std::same_as<element_t, float>)
			return _mm_xor_ps(lhs, rhs);
		else
			return _mm_xor_pd(lhs, rhs);
	}
	/**
	 * @brief Computes the bitwise complement of a mapped register.
	 *
	 * @param lhs The source register.
	 * @return The resulting mapped register.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_not(vector_t lhs) noexcept
	{
		if constexpr (std::is_integral_v<element_t>)
			return _mm_xor_si128(lhs, _mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128()));
		else if constexpr (std::same_as<element_t, float>)
			return _mm_xor_ps(lhs, _mm_cmpeq_ps(_mm_setzero_ps(), _mm_setzero_ps()));
		else
			return _mm_xor_pd(lhs, _mm_castsi128_pd(_mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128())));
	}
	/**
	 * @brief Computes the bitwise AND of the complemented first register and the second register.
	 *
	 * @param lhs The register to complement.
	 * @param rhs The register to combine with the complement.
	 * @return The resulting mapped register.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_andnot(vector_t lhs, vector_t rhs) noexcept
	{
		if constexpr (std::is_integral_v<element_t>)
			return _mm_andnot_si128(lhs, rhs);
		else if constexpr (std::same_as<element_t, float>)
			return _mm_andnot_ps(lhs, rhs);
		else
			return _mm_andnot_pd(lhs, rhs);
	}
#pragma endregion

#pragma region Arithmetic Operations
	static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) negate(int_vector_t lhs) noexcept
		requires std::is_integral_v<element_t>
	{
		if constexpr (sizeof(element_t) == 8)
			return _mm_sub_epi64(_mm_setzero_si128(), lhs);
		else if constexpr (sizeof(element_t) == 4)
			return _mm_sub_epi32(_mm_setzero_si128(), lhs);
		else if constexpr (sizeof(element_t) == 2)
			return _mm_sub_epi16(_mm_setzero_si128(), lhs);
		else
			return _mm_sub_epi8(_mm_setzero_si128(), lhs);
	}

	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) negate(vector_t lhs) noexcept
		requires std::is_floating_point_v<element_t>
	{
		if constexpr (std::same_as<element_t, float>)
			return _mm_sub_ps(_mm_setzero_ps(), lhs);
		else
			return _mm_sub_pd(_mm_setzero_pd(), lhs);
	}
#pragma endregion

#pragma region 128-bit Shifting

	/**
	 * @brief Shifts a complete register toward higher byte indices.
	 * @param lhs Source register.
	 * @param shift Runtime byte count.
	 * @return Shifted register with zero-filled low bytes.
	 */
	static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) byte_shift_left_slow(int_vector_t lhs, int shift) noexcept
	{
		return _ext128_byte_shift_left_slow(lhs, shift);
	}

	/**
	 * @brief Shifts a complete register toward lower byte indices.
	 * @param lhs Source register.
	 * @param shift Runtime byte count.
	 * @return Shifted register with zero-filled high bytes.
	 */
	static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) byte_shift_right_slow(int_vector_t lhs, int shift) noexcept
	{
		return _ext128_byte_shift_right_slow(lhs, shift);
	}

	/**
	 * @brief Shifts a complete 128-bit register left by a runtime bit count.
	 * @param lhs Source register interpreted as one unsigned 128-bit bit string.
	 * @param shift Runtime count; nonpositive counts are identity and counts of at least 128 produce zero.
	 * @return Shifted register with zero-filled low bits.
	 */
	static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bit_shift_left_slow(const int_vector_t lhs, const int shift) noexcept
	{
		return _ext128_shift_left_bits_slow(lhs, shift);
	}

	/**
	 * @brief Shifts a complete 128-bit register right by a runtime bit count.
	 * @param lhs Source register interpreted as one unsigned 128-bit bit string.
	 * @param shift Runtime count; nonpositive counts are identity and counts of at least 128 produce zero.
	 * @return Shifted register with zero-filled high bits.
	 */
	static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bit_shift_right_slow(const int_vector_t lhs, const int shift) noexcept
	{
		return _ext128_shift_right_bits_slow(lhs, shift);
	}

	/**
	 * @brief Shifts a complete 128-bit register left by a compile-time bit count.
	 * @tparam shift Nonnegative bit count; counts of at least 128 produce zero.
	 * @param lhs Source register interpreted as one unsigned 128-bit bit string.
	 * @return Shifted register with zero-filled low bits.
	 */
	template <int shift> static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bit_shift_left(const int_vector_t lhs) noexcept
	{
		return _ext128_shift_left_bits_static<shift>(lhs);
	}

	/**
	 * @brief Shifts a complete 128-bit register right by a compile-time bit count.
	 * @tparam shift Nonnegative bit count; counts of at least 128 produce zero.
	 * @param lhs Source register interpreted as one unsigned 128-bit bit string.
	 * @return Shifted register with zero-filled high bits.
	 */
	template <int shift> static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bit_shift_right(const int_vector_t lhs) noexcept
	{
		return _ext128_shift_right_bits_static<shift>(lhs);
	}

#pragma endregion

#pragma region Shuffling
	/** @brief Emulates an immediate-controlled 32-bit shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param imm8 Runtime control byte.
	 *  @return Register with each four-lane group shuffled.
	 */
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY static int_vector_t VECTORCALL shuffle_32_slow(int_vector_t lhs, std::uint32_t imm8) noexcept
		requires std::is_integral_v<element_t>
	{
		return register_shuffle_32_slow(lhs, imm8);
	}

	/// <summary> Shuffles the 32-bit integers in the vector using a compile-time control mask. </summary>
	template <std::uint32_t imm8>
	static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle_32(int_vector_t lhs) noexcept
		requires std::is_integral_v<element_t>
	{
		return _mm_shuffle_epi32(lhs, imm8);
	}

	/// <summary> Shuffles the bytes in the vector using the indexes in the second vector. </summary>
	static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(int_vector_t lhs, int_vector_t indices) noexcept
		requires std::is_integral_v<element_t>
	{
		return _mm_shuffle_epi8(lhs, indices);
	}

#pragma endregion

#pragma region Miscellaneous Operations

	/// <summary> Returns a mask of the most significant BIT of each BYTE in each element. </summary>
	static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) movemask(const vector_t lhs) noexcept
	{
		if constexpr (std::is_integral_v<element_t>)
			return _mm_movemask_epi8(lhs);
		else if constexpr (std::is_same_v<element_t, float>)
			return _mm_movemask_epi8(_mm_castps_si128(lhs));
		else if constexpr (std::is_same_v<element_t, double>)
			return _mm_movemask_epi8(_mm_castpd_si128(lhs));
	}

	/// <summary> Returns a mask of the most significant BIT of each element. </summary>
	static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) movemask_slim(const vector_t lhs) noexcept
	{
		if constexpr (std::is_integral_v<element_t>)
			return movemask(swizzle_msb(lhs));
		else if constexpr (std::is_same_v<element_t, float>)
			return _mm_movemask_ps(lhs);
		else if constexpr (std::is_same_v<element_t, double>)
			return _mm_movemask_pd(lhs);
	}

	/// <summary> Compute the bitwise AND of 128 bits (representing integer data) in a and b, and set ZF to 1 if the result is zero, otherwise set ZF to 0.
	/// Compute the bitwise NOT of a and then AND with b, and set CF to 1 if the result is zero, otherwise set CF to 0. Return the CF value. </summary>
	static int SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) test(int_vector_t lhs, int_vector_t rhs) noexcept
	{
		return _mm_testc_si128(lhs, rhs);
	}

	/// <summary> Compute the bitwise AND of 128 bits (representing integer data) in a and b, and set ZF to 1 if the result is zero, otherwise set ZF to 0.
	/// Compute the bitwise NOT of a and then AND with b, and set CF to 1 if the result is zero, otherwise set CF to 0. Return the ZF value. </summary>
	static int SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) testz(int_vector_t lhs, int_vector_t rhs) noexcept
	{
		return _mm_testz_si128(lhs, rhs);
	}

	/// <summary> Compute the bitwise AND of 128 bits (representing integer data) in a and b, and set ZF to 1 if the result is zero, otherwise set ZF to 0.
	/// Compute the bitwise NOT of a and then AND with b, and set CF to 1 if the result is zero, otherwise set CF to 0. Return 1 if both the ZF and CF values
	/// are zero, otherwise return 0. </summary>
	static int SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) testnzc(int_vector_t lhs, int_vector_t rhs) noexcept
	{
		return _mm_testnzc_si128(lhs, rhs);
	}

	/// <summary> Returns the shuffle order to move the most significant byte of each element into the least significant bytes. </summary>
	constexpr static int_vector_t get_msb_swizzle_order() noexcept
	{
		int_vector_t seq{};
		constexpr const auto elem_size = sizeof(element_t);
		constexpr const auto elem_count = 16 / elem_size;

		for (std::size_t i = 0; i < 16; ++i)
		{
			if (i < elem_count)
			{
				// Select the MSB byte of each element, packing them into the low bytes.
				register_set_constexpr<std::uint8_t>(seq, i, static_cast<std::uint8_t>((i * elem_size) + (elem_size - 1)));
			}
			else
			{
				// Zero out the rest (PSHUFB: high bit set => 0).
				register_set_constexpr<std::uint8_t>(seq, i, 0x80);
			}
		}
		return seq;
	}

	/// <summary> Swizzle the vector to only contain the most significant bit of each byte. </summary>
	static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) swizzle_msb(int_vector_t lhs) noexcept
	{
		return shuffle(lhs, get_msb_swizzle_order());
	}
#pragma endregion
};
#pragma endregion

#endif // SIMDLIB_HAS_SSE42

#if SIMDLIB_HAS_AVX2 && SIMDLIB_HAS_SSE42

#pragma region 256-bit Implementations

template <class element_t>
	requires std::is_arithmetic_v<element_t>
struct SimdImpl256
{
};

/**
 * @brief Reports whether a 256-bit logical shuffle selects any lane from the opposite 128-bit half.
 * @tparam lanes_per_half Logical lanes in one 128-bit half.
 * @tparam indices Complete logical selector array.
 * @return True when at least one output lane crosses the 128-bit boundary.
 */
template <std::size_t lanes_per_half, auto indices> [[nodiscard]] consteval bool logical_shuffle_256_has_cross_half_selector() noexcept
{
	for (std::size_t output = 0; output < indices.size(); ++output)
		if (output / lanes_per_half != indices[output] / lanes_per_half)
			return true;
	return false;
}

/**
 * @brief Reports whether a 256-bit logical shuffle selects any lane from its original 128-bit half.
 * @tparam lanes_per_half Logical lanes in one 128-bit half.
 * @tparam indices Complete logical selector array.
 * @return True when at least one output lane remains within its original 128-bit half.
 */
template <std::size_t lanes_per_half, auto indices> [[nodiscard]] consteval bool logical_shuffle_256_has_local_half_selector() noexcept
{
	for (std::size_t output = 0; output < indices.size(); ++output)
		if (output / lanes_per_half == indices[output] / lanes_per_half)
			return true;
	return false;
}

/**
 * @brief Encodes one byte of a full-width 256-bit byte or word shuffle control.
 * @param element_bytes Bytes in each logical lane.
 * @param select_cross_half Whether this control selects cross-half or local-half lanes.
 * @param output_lane Logical output lane containing the byte.
 * @param source_lane Logical source lane selected for the output lane.
 * @param byte_in_lane Byte position within the logical output lane.
 * @return Lane-relative VPSHUFB selector or the zeroing sentinel when handled by the other control.
 */
[[nodiscard]] consteval int encode_logical_shuffle_256_byte(const std::size_t element_bytes, const bool select_cross_half, const std::size_t output_lane,
															const std::size_t source_lane, const std::size_t byte_in_lane) noexcept
{
	const std::size_t lanes_per_half = 16 / element_bytes;
	const bool crosses_half = output_lane / lanes_per_half != source_lane / lanes_per_half;
	if (crosses_half != select_cross_half)
		return 0x80;
	return static_cast<int>((source_lane % lanes_per_half) * element_bytes + byte_in_lane);
}

/**
 * @brief Builds one constant VPSHUFB control for a full-width 256-bit byte or word shuffle.
 * @tparam element_bytes Bytes in each logical lane.
 * @tparam select_cross_half Whether this control selects cross-half or local-half lanes.
 * @tparam indices Complete logical selector array.
 * @tparam byte_positions Output byte positions.
 * @return Native AVX2 byte-control register.
 */
template <std::size_t element_bytes, bool select_cross_half, auto indices, std::size_t... byte_positions>
static __m256i SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) make_logical_shuffle_256_byte_control(std::index_sequence<byte_positions...>) noexcept
{
	return _mm256_setr_epi8(static_cast<char>(encode_logical_shuffle_256_byte(element_bytes, select_cross_half, byte_positions / element_bytes,
																			  indices[byte_positions / element_bytes], byte_positions % element_bytes))...);
}

template <> struct SimdImpl256<int8_t>
{
	/** @brief Selects bytes from two registers using a canonical predicate register. */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m256i condition, __m256i when_true, __m256i when_false) noexcept
	{
		return _mm256_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical signed-byte lanes across the complete 256-bit register.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 32 && ((indices < 32) && ...))
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m256i lhs) noexcept
	{
		constexpr auto selectors = std::array{indices...};
		if constexpr (!logical_shuffle_256_has_cross_half_selector<16, selectors>())
		{
			return _mm256_shuffle_epi8(lhs, make_logical_shuffle_256_byte_control<1, false, selectors>(std::make_index_sequence<32>{}));
		}
		else
		{
			const __m256i swapped = _mm256_permute2x128_si256(lhs, lhs, 0x01);
			if constexpr (!logical_shuffle_256_has_local_half_selector<16, selectors>())
				return _mm256_shuffle_epi8(swapped, make_logical_shuffle_256_byte_control<1, true, selectors>(std::make_index_sequence<32>{}));
			else
				return _mm256_or_si256(_mm256_shuffle_epi8(lhs, make_logical_shuffle_256_byte_control<1, false, selectors>(std::make_index_sequence<32>{})),
									   _mm256_shuffle_epi8(swapped, make_logical_shuffle_256_byte_control<1, true, selectors>(std::make_index_sequence<32>{})));
		}
	}

	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm256_add_epi8(lhs, rhs);
	}
	/** @brief Multiplies signed byte lanes and adds adjacent products into signed 16-bit lanes. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		const __m256i lowProducts = _mm256_mullo_epi16(_mm256_cvtepi8_epi16(_mm256_castsi256_si128(lhs)), _mm256_cvtepi8_epi16(_mm256_castsi256_si128(rhs)));
		const __m256i highProducts =
			_mm256_mullo_epi16(_mm256_cvtepi8_epi16(_mm256_extracti128_si256(lhs, 1)), _mm256_cvtepi8_epi16(_mm256_extracti128_si256(rhs, 1)));
		const __m256i interleavedSums = _mm256_hadd_epi16(lowProducts, highProducts);
		return _mm256_permute4x64_epi64(interleavedSums, _MM_SHUFFLE(3, 1, 2, 0));
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm256_maddubs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _ext256_mul_epi8(lhs, rhs);
	}
	/** @brief Divides corresponding signed 8-bit lanes with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext256_div_epi8(lhs, rhs);
	}
	/** @brief Computes scalar-equivalent signed 8-bit remainders with register-only extraction and reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext256_rem_epi8(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		auto sqrt16x16 = [](__m256i values) noexcept
		{
			const __m128i low16 = _mm256_castsi256_si128(values);
			const __m128i high16 = _mm256_extracti128_si256(values, 1);
			const __m256i low32 = _mm256_cvtepi16_epi32(low16);
			const __m256i high32 = _mm256_cvtepi16_epi32(high16);
			const __m256i lowRoots = _mm256_cvtps_epi32(_mm256_sqrt_ps(_mm256_cvtepi32_ps(low32)));
			const __m256i highRoots = _mm256_cvtps_epi32(_mm256_sqrt_ps(_mm256_cvtepi32_ps(high32)));
			const __m128i packedLow = _mm_packs_epi32(_mm256_castsi256_si128(lowRoots), _mm256_extracti128_si256(lowRoots, 1));
			const __m128i packedHigh = _mm_packs_epi32(_mm256_castsi256_si128(highRoots), _mm256_extracti128_si256(highRoots, 1));
			return _mm256_inserti128_si256(_mm256_castsi128_si256(packedLow), packedHigh, 1);
		};

		const __m128i low8 = _mm256_castsi256_si128(lhs);
		const __m128i high8 = _mm256_extracti128_si256(lhs, 1);
		const __m256i widenedLow = _mm256_cvtepi8_epi16(low8);
		const __m256i widenedHigh = _mm256_cvtepi8_epi16(high8);
		const __m256i rootsLow16 = sqrt16x16(widenedLow);
		const __m256i rootsHigh16 = sqrt16x16(widenedHigh);
		const __m128i packedLow = _mm_packs_epi16(_mm256_castsi256_si128(rootsLow16), _mm256_extracti128_si256(rootsLow16, 1));
		const __m128i packedHigh = _mm_packs_epi16(_mm256_castsi256_si128(rootsHigh16), _mm256_extracti128_si256(rootsHigh16, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(packedLow), packedHigh, 1);
	}
	/** @brief Computes one unchecked magnitude in lane zero of each 128-bit group. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<int8_t>::magnitude(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<int8_t>::magnitude(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Computes saturated magnitudes and adjacent overflow masks for both 128-bit groups. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<int8_t>::magnitude_checked(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<int8_t>::magnitude_checked(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Returns the minimum value and its first lane position without materializing register data in memory. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		const __m128i lowMeta = SimdImpl128<int8_t>::min_position(_mm256_castsi256_si128(lhs));
		const __m128i highMeta = SimdImpl128<int8_t>::min_position(_mm256_extracti128_si256(lhs, 1));
		const auto lowValue = static_cast<std::int8_t>(_mm_extract_epi8(lowMeta, 0));
		const auto highValue = static_cast<std::int8_t>(_mm_extract_epi8(highMeta, 0));
		const bool chooseHigh = highValue < lowValue;
		const __m128i selectedMeta = chooseHigh ? highMeta : lowMeta;
		const int position = static_cast<int>(_mm_extract_epi8(selectedMeta, 1)) + (chooseHigh ? 16 : 0);
		__m128i output = _mm_setzero_si128();
		output = _mm_insert_epi8(output, _mm_extract_epi8(selectedMeta, 0), 0);
		output = _mm_insert_epi8(output, position, 1);
		return _mm256_zextsi128_si256(output);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm256_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m256i lhs, __m256i rhs) noexcept
	{
		return _mm256_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _mm256_abs_epi8(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi8(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm256_min_epi8(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm256_max_epi8(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _ext256_slli_epx8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _ext256_srli_epx8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _ext256_srai_epx8(lhs, rhs);
	}

	// arithmetic (saturated)
	/** @brief Adds lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm256_adds_epi8(lhs, rhs);
	}
	/** @brief Subtracts lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm256_subs_epi8(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm256_set1_epi8(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args &&...args) noexcept
	{
		return _mm256_set_epi8(args...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args &&...args) noexcept
	{
		return _mm256_setr_epi8(args...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm256_cmpeq_epi8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _mm256_cmpgt_epi8(lhs, rhs);
	}

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm256_cvtepi8_epi16(lhs, rhs);
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<int8_t>(_mm256_extract_epi8(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected signed 8-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 32)`.
	 * @return Selected scalar lane.
	 */
	static int8_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m256i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 32, "Signed 8-bit extraction requires a valid 256-bit lane index");
		const __m256i selected = _mm256_permutevar8x32_epi32(lhs, _mm256_set1_epi32(index >> 2));
		const uint32_t selected_dword = static_cast<uint32_t>(_mm_cvtsi128_si32(_mm256_castsi256_si128(selected)));
		return static_cast<int8_t>(selected_dword >> ((index & 3) * 8));
	}
	/** @brief Replaces the compile-time-selected signed 8-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const int8_t rhs) noexcept
	{
		return register_insert_constexpr<int8_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected signed 8-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const int8_t rhs) noexcept
	{
		return _mm256_insert_epi8(lhs, static_cast<int>(rhs), index);
	}
	/**
	 * @brief Replaces one runtime-selected signed 8-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 32)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m256i lhs, const int8_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 32, "Signed 8-bit insertion requires a valid 256-bit lane index");
		if (index < 16)
		{
			const __m128i lower = SimdImpl128<int8_t>::insert_slow(_mm256_castsi256_si128(lhs), rhs, index);
			return _mm256_inserti128_si256(lhs, lower, 0);
		}
		const __m128i upper = SimdImpl128<int8_t>::insert_slow(_mm256_extracti128_si256(lhs, 1), rhs, index - 16);
		return _mm256_inserti128_si256(lhs, upper, 1);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpacklo_epi8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpackhi_epi8(lhs, rhs);
	}

	// misc
	/** @brief Shuffles bytes through the native runtime selector-register instruction. */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle(auto lhs, auto rhs) noexcept
		requires(std::same_as<decltype(lhs), __m256i> && std::same_as<decltype(rhs), __m256i>)
	{
		return _mm256_shuffle_epi8(lhs, rhs);
	}
	/** @brief Selects bytes through the native runtime mask-register operation. */
	SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY static __m256i VECTORCALL blend(const __m256i lhs, const __m256i rhs, const __m256i mask) noexcept
	{
		return register_blend_bytes(lhs, rhs, mask);
	}
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) movemask(auto lhs) noexcept
	{
		return _mm256_movemask_epi8(lhs);
	}
};

template <> struct SimdImpl256<uint8_t>
{
	/** @brief Selects bytes from two registers using a canonical predicate register. */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m256i condition, __m256i when_true, __m256i when_false) noexcept
	{
		return _mm256_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical unsigned-byte lanes across the complete 256-bit register.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 32 && ((indices < 32) && ...))
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m256i lhs) noexcept
	{
		constexpr auto selectors = std::array{indices...};
		if constexpr (!logical_shuffle_256_has_cross_half_selector<16, selectors>())
		{
			return _mm256_shuffle_epi8(lhs, make_logical_shuffle_256_byte_control<1, false, selectors>(std::make_index_sequence<32>{}));
		}
		else
		{
			const __m256i swapped = _mm256_permute2x128_si256(lhs, lhs, 0x01);
			if constexpr (!logical_shuffle_256_has_local_half_selector<16, selectors>())
				return _mm256_shuffle_epi8(swapped, make_logical_shuffle_256_byte_control<1, true, selectors>(std::make_index_sequence<32>{}));
			else
				return _mm256_or_si256(_mm256_shuffle_epi8(lhs, make_logical_shuffle_256_byte_control<1, false, selectors>(std::make_index_sequence<32>{})),
									   _mm256_shuffle_epi8(swapped, make_logical_shuffle_256_byte_control<1, true, selectors>(std::make_index_sequence<32>{})));
		}
	}

	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm256_add_epi8(lhs, rhs);
	}
	/** @brief Multiplies unsigned byte lanes and adds adjacent products into unsigned 16-bit lanes. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		const __m256i lowProducts = _mm256_mullo_epi16(_mm256_cvtepu8_epi16(_mm256_castsi256_si128(lhs)), _mm256_cvtepu8_epi16(_mm256_castsi256_si128(rhs)));
		const __m256i highProducts =
			_mm256_mullo_epi16(_mm256_cvtepu8_epi16(_mm256_extracti128_si256(lhs, 1)), _mm256_cvtepu8_epi16(_mm256_extracti128_si256(rhs, 1)));
		const __m256i interleavedSums = _mm256_hadd_epi16(lowProducts, highProducts);
		return _mm256_permute4x64_epi64(interleavedSums, _MM_SHUFFLE(3, 1, 2, 0));
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm256_maddubs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _ext256_mul_epi8(lhs, rhs);
	}
	/** @brief Divides corresponding unsigned 8-bit lanes with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext256_div_epu8(lhs, rhs);
	}
	/** @brief Computes scalar-equivalent unsigned 8-bit remainders with register-only extraction and reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext256_rem_epu8(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		auto sqrt16x16 = [](__m256i values) noexcept
		{
			const __m128i low16 = _mm256_castsi256_si128(values);
			const __m128i high16 = _mm256_extracti128_si256(values, 1);
			const __m256i low32 = _mm256_cvtepu16_epi32(low16);
			const __m256i high32 = _mm256_cvtepu16_epi32(high16);
			const __m256i lowRoots = _mm256_cvtps_epi32(_mm256_sqrt_ps(_ext256_cvtepu32_ps(low32)));
			const __m256i highRoots = _mm256_cvtps_epi32(_mm256_sqrt_ps(_ext256_cvtepu32_ps(high32)));
			const __m128i packedLow = _mm_packus_epi32(_mm256_castsi256_si128(lowRoots), _mm256_extracti128_si256(lowRoots, 1));
			const __m128i packedHigh = _mm_packus_epi32(_mm256_castsi256_si128(highRoots), _mm256_extracti128_si256(highRoots, 1));
			return _mm256_inserti128_si256(_mm256_castsi128_si256(packedLow), packedHigh, 1);
		};

		const __m128i low8 = _mm256_castsi256_si128(lhs);
		const __m128i high8 = _mm256_extracti128_si256(lhs, 1);
		const __m256i widenedLow = _mm256_cvtepu8_epi16(low8);
		const __m256i widenedHigh = _mm256_cvtepu8_epi16(high8);
		const __m256i rootsLow16 = sqrt16x16(widenedLow);
		const __m256i rootsHigh16 = sqrt16x16(widenedHigh);
		const __m128i packedLow = _mm_packus_epi16(_mm256_castsi256_si128(rootsLow16), _mm256_extracti128_si256(rootsLow16, 1));
		const __m128i packedHigh = _mm_packus_epi16(_mm256_castsi256_si128(rootsHigh16), _mm256_extracti128_si256(rootsHigh16, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(packedLow), packedHigh, 1);
	}
	/** @brief Computes one unchecked magnitude in lane zero of each 128-bit group. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<uint8_t>::magnitude(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<uint8_t>::magnitude(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Computes saturated magnitudes and adjacent overflow masks for both 128-bit groups. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<uint8_t>::magnitude_checked(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<uint8_t>::magnitude_checked(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Returns the minimum value and its first lane position without materializing register data in memory. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		const __m128i lowMeta = SimdImpl128<uint8_t>::min_position(_mm256_castsi256_si128(lhs));
		const __m128i highMeta = SimdImpl128<uint8_t>::min_position(_mm256_extracti128_si256(lhs, 1));
		const auto lowValue = static_cast<std::uint8_t>(_mm_extract_epi8(lowMeta, 0));
		const auto highValue = static_cast<std::uint8_t>(_mm_extract_epi8(highMeta, 0));
		const bool chooseHigh = highValue < lowValue;
		const __m128i selectedMeta = chooseHigh ? highMeta : lowMeta;
		const int position = static_cast<int>(_mm_extract_epi8(selectedMeta, 1)) + (chooseHigh ? 16 : 0);
		__m128i output = _mm_setzero_si128();
		output = _mm_insert_epi8(output, _mm_extract_epi8(selectedMeta, 0), 0);
		output = _mm_insert_epi8(output, position, 1);
		return _mm256_zextsi128_si256(output);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm256_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m256i lhs, __m256i rhs) noexcept
	{
		return _mm256_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _mm256_abs_epi8(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi8(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm256_min_epu8(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm256_max_epu8(lhs, rhs);
	}
	/** @brief Computes lane-wise averages for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) avg(auto lhs, auto rhs) noexcept
	{
		return _mm256_avg_epu8(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _ext256_slli_epx8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _ext256_srli_epx8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _ext256_srai_epx8(lhs, rhs);
	}

	// arithmetic (saturated)
	/** @brief Adds lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm256_adds_epu8(lhs, rhs);
	}
	/** @brief Subtracts lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm256_subs_epu8(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _ext256_set1_epu8(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args &&...args) noexcept
	{
		return _mm256_set_epi8(args...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args &&...args) noexcept
	{
		return _mm256_setr_epi8(args...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm256_cmpeq_epi8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _ext256_cmpgt_epu8(lhs, rhs);
	}

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm256_cvtepu8_epi16(lhs, rhs);
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<uint8_t>(_mm256_extract_epi8(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected unsigned 8-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 32)`.
	 * @return Selected scalar lane.
	 */
	static uint8_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m256i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 32, "Unsigned 8-bit extraction requires a valid 256-bit lane index");
		const __m256i selected = _mm256_permutevar8x32_epi32(lhs, _mm256_set1_epi32(index >> 2));
		const uint32_t selected_dword = static_cast<uint32_t>(_mm_cvtsi128_si32(_mm256_castsi256_si128(selected)));
		return static_cast<uint8_t>(selected_dword >> ((index & 3) * 8));
	}
	/** @brief Replaces the compile-time-selected unsigned 8-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const uint8_t rhs) noexcept
	{
		return register_insert_constexpr<uint8_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected unsigned 8-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const uint8_t rhs) noexcept
	{
		return _mm256_insert_epi8(lhs, static_cast<int>(rhs), index);
	}
	/**
	 * @brief Replaces one runtime-selected unsigned 8-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 32)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m256i lhs, const uint8_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 32, "Unsigned 8-bit insertion requires a valid 256-bit lane index");
		if (index < 16)
		{
			const __m128i lower = SimdImpl128<uint8_t>::insert_slow(_mm256_castsi256_si128(lhs), rhs, index);
			return _mm256_inserti128_si256(lhs, lower, 0);
		}
		const __m128i upper = SimdImpl128<uint8_t>::insert_slow(_mm256_extracti128_si256(lhs, 1), rhs, index - 16);
		return _mm256_inserti128_si256(lhs, upper, 1);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpacklo_epi8(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpackhi_epi8(lhs, rhs);
	}

	// misc
	/** @brief Shuffles bytes through the native runtime selector-register instruction. */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle(auto lhs, auto rhs) noexcept
		requires(std::same_as<decltype(lhs), __m256i> && std::same_as<decltype(rhs), __m256i>)
	{
		return _mm256_shuffle_epi8(lhs, rhs);
	}
	/** @brief Selects bytes through the native runtime mask-register operation. */
	SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY static __m256i VECTORCALL blend(const __m256i lhs, const __m256i rhs, const __m256i mask) noexcept
	{
		return register_blend_bytes(lhs, rhs, mask);
	}
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) movemask(auto lhs) noexcept
	{
		return _mm256_movemask_epi8(lhs);
	}
};

template <> struct SimdImpl256<int16_t>
{
	/** @brief Selects 16-bit lanes from two registers using a canonical predicate register. */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m256i condition, __m256i when_true, __m256i when_false) noexcept
	{
		return _mm256_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical signed 16-bit lanes across the complete 256-bit register.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 16 && ((indices < 16) && ...))
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m256i lhs) noexcept
	{
		constexpr auto selectors = std::array{indices...};
		if constexpr (!logical_shuffle_256_has_cross_half_selector<8, selectors>())
		{
			return _mm256_shuffle_epi8(lhs, make_logical_shuffle_256_byte_control<2, false, selectors>(std::make_index_sequence<32>{}));
		}
		else
		{
			const __m256i swapped = _mm256_permute2x128_si256(lhs, lhs, 0x01);
			if constexpr (!logical_shuffle_256_has_local_half_selector<8, selectors>())
				return _mm256_shuffle_epi8(swapped, make_logical_shuffle_256_byte_control<2, true, selectors>(std::make_index_sequence<32>{}));
			else
				return _mm256_or_si256(_mm256_shuffle_epi8(lhs, make_logical_shuffle_256_byte_control<2, false, selectors>(std::make_index_sequence<32>{})),
									   _mm256_shuffle_epi8(swapped, make_logical_shuffle_256_byte_control<2, true, selectors>(std::make_index_sequence<32>{})));
		}
	}

	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm256_add_epi16(lhs, rhs);
	}
	/** @brief Multiplies adjacent lanes and adds their products for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		return _mm256_madd_epi16(lhs, rhs);
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm256_maddubs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _mm256_mullo_epi16(lhs, rhs);
	}
	/** @brief Divides corresponding signed 16-bit lanes with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext256_div_epi16(lhs, rhs);
	}
	/** @brief Computes scalar-equivalent signed 16-bit remainders with register-only extraction and reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext256_rem_epi16(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		auto sqrt16x8 = [](__m128i values) noexcept
		{
			const __m256i widened = _mm256_cvtepi16_epi32(values);
			const __m256 roots = _mm256_sqrt_ps(_mm256_cvtepi32_ps(widened));
			const __m256i integers = _mm256_cvtps_epi32(roots);
			return _mm_packs_epi32(_mm256_castsi256_si128(integers), _mm256_extracti128_si256(integers, 1));
		};

		const __m128i low16 = _mm256_castsi256_si128(lhs);
		const __m128i high16 = _mm256_extracti128_si256(lhs, 1);
		const __m128i rootsLow = sqrt16x8(low16);
		const __m128i rootsHigh = sqrt16x8(high16);
		return _mm256_inserti128_si256(_mm256_castsi128_si256(rootsLow), rootsHigh, 1);
	}
	/** @brief Computes one unchecked magnitude in lane zero of each 128-bit group. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<int16_t>::magnitude(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<int16_t>::magnitude(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Computes saturated magnitudes and adjacent overflow masks for both 128-bit groups. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<int16_t>::magnitude_checked(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<int16_t>::magnitude_checked(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Returns the minimum value and its first lane position without materializing register data in memory. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		const __m128i lowMeta = SimdImpl128<int16_t>::min_position(_mm256_castsi256_si128(lhs));
		const __m128i highMeta = SimdImpl128<int16_t>::min_position(_mm256_extracti128_si256(lhs, 1));
		const auto lowValue = static_cast<std::int16_t>(_mm_extract_epi16(lowMeta, 0));
		const auto highValue = static_cast<std::int16_t>(_mm_extract_epi16(highMeta, 0));
		const bool chooseHigh = highValue < lowValue;
		const __m128i selectedMeta = chooseHigh ? highMeta : lowMeta;
		const int position = static_cast<int>(_mm_extract_epi16(selectedMeta, 1)) + (chooseHigh ? 8 : 0);
		__m128i output = _mm_setzero_si128();
		output = _mm_insert_epi16(output, _mm_extract_epi16(selectedMeta, 0), 0);
		output = _mm_insert_epi16(output, position, 1);
		return _mm256_zextsi128_si256(output);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm256_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m256i lhs, __m256i rhs) noexcept
	{
		return _mm256_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _mm256_abs_epi16(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi16(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm256_min_epi16(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm256_max_epi16(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _mm256_slli_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _mm256_srli_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _mm256_srai_epi16(lhs, rhs);
	}

	// arithmetic (horizontal)
	/** @brief Horizontally adds adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm256_hadd_epi16(lhs, rhs);
	}
	/** @brief Horizontally subtracts adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm256_hsub_epi16(lhs, rhs);
	}
	/** @brief Horizontally adds lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) hadd_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm256_hadds_epi16(lhs, rhs);
	}
	/** @brief Horizontally subtracts lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) hsubtract_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm256_hsubs_epi16(lhs, rhs);
	}

	// arithmetic (saturated)
	/** @brief Adds lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm256_adds_epi16(lhs, rhs);
	}
	/** @brief Subtracts lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm256_subs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) multiply_saturated(auto lhs, auto rhs) noexcept
	{
		const __m128i lhsLo128 = _mm256_castsi256_si128(lhs);
		const __m128i rhsLo128 = _mm256_castsi256_si128(rhs);
		const __m128i lhsHi128 = _mm256_extracti128_si256(lhs, 1);
		const __m128i rhsHi128 = _mm256_extracti128_si256(rhs, 1);

		const __m256i loProducts = _mm256_mullo_epi32(_mm256_cvtepi16_epi32(lhsLo128), _mm256_cvtepi16_epi32(rhsLo128));
		const __m256i hiProducts = _mm256_mullo_epi32(_mm256_cvtepi16_epi32(lhsHi128), _mm256_cvtepi16_epi32(rhsHi128));

		const __m128i loPacked = _mm_packs_epi32(_mm256_castsi256_si128(loProducts), _mm256_extracti128_si256(loProducts, 1));
		const __m128i hiPacked = _mm_packs_epi32(_mm256_castsi256_si128(hiProducts), _mm256_extracti128_si256(hiProducts, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(loPacked), hiPacked, 1);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm256_set1_epi16(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args &&...args) noexcept
	{
		return _mm256_set_epi16(args...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args &&...args) noexcept
	{
		return _mm256_setr_epi16(args...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm256_cmpeq_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _mm256_cmpgt_epi16(lhs, rhs);
	}

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm256_cvtepi16_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) compress(auto lhs, auto rhs) noexcept
	{
		return _mm256_packs_epi16(lhs, rhs);
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<int16_t>(_mm256_extract_epi16(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected signed 16-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 16)`.
	 * @return Selected scalar lane.
	 */
	static int16_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m256i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 16, "Signed 16-bit extraction requires a valid 256-bit lane index");
		const __m256i selected = _mm256_permutevar8x32_epi32(lhs, _mm256_set1_epi32(index >> 1));
		const uint32_t selected_dword = static_cast<uint32_t>(_mm_cvtsi128_si32(_mm256_castsi256_si128(selected)));
		return static_cast<int16_t>(selected_dword >> ((index & 1) * 16));
	}
	/** @brief Replaces the compile-time-selected signed 16-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const int16_t rhs) noexcept
	{
		return register_insert_constexpr<int16_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected signed 16-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const int16_t rhs) noexcept
	{
		return _mm256_insert_epi16(lhs, static_cast<int>(rhs), index);
	}
	/**
	 * @brief Replaces one runtime-selected signed 16-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 16)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m256i lhs, const int16_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 16, "Signed 16-bit insertion requires a valid 256-bit lane index");
		if (index < 8)
		{
			const __m128i lower = SimdImpl128<int16_t>::insert_slow(_mm256_castsi256_si128(lhs), rhs, index);
			return _mm256_inserti128_si256(lhs, lower, 0);
		}
		const __m128i upper = SimdImpl128<int16_t>::insert_slow(_mm256_extracti128_si256(lhs, 1), rhs, index - 8);
		return _mm256_inserti128_si256(lhs, upper, 1);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpacklo_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpackhi_epi16(lhs, rhs);
	}

	// misc
	/** @brief Emulates an immediate-controlled low-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each low four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_lo_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_half_16_slow(lhs, static_cast<unsigned int>(rhs), false);
	}
	/** @brief Shuffles the low four signed 16-bit lanes in each 128-bit group with an immediate control. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle_lo(__m256i lhs) noexcept
	{
		return _mm256_shufflelo_epi16(lhs, imm8);
	}
	/** @brief Emulates an immediate-controlled high-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each high four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_hi_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_half_16_slow(lhs, static_cast<unsigned int>(rhs), true);
	}
	/** @brief Shuffles the high four signed 16-bit lanes in each 128-bit group with an immediate control. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle_hi(__m256i lhs) noexcept
	{
		return _mm256_shufflehi_epi16(lhs, imm8);
	}
	/** @brief Emulates an immediate-controlled blend with a runtime scalar mask.
	 *  @param lhs Source for lanes whose control bits are clear.
	 *  @param rhs Source for lanes whose control bits are set.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the selected lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) blend_slow(auto lhs, auto rhs, const int imm8) noexcept
	{
		return register_blend_slow<std::int16_t>(lhs, rhs, static_cast<unsigned int>(imm8));
	}
	/** @brief Selects signed 16-bit lanes from two 256-bit registers with a repeated immediate control. */
	template <int imm8> SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static auto VECTORCALL blend(auto lhs, auto rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return register_blend_slow<std::int16_t>(lhs, rhs, static_cast<unsigned int>(imm8));
		return _mm256_blend_epi16(lhs, rhs, imm8);
	}
};

template <> struct SimdImpl256<uint16_t>
{
	/** @brief Selects 16-bit lanes from two registers using a canonical predicate register. */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m256i condition, __m256i when_true, __m256i when_false) noexcept
	{
		return _mm256_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical unsigned 16-bit lanes across the complete 256-bit register.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 16 && ((indices < 16) && ...))
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m256i lhs) noexcept
	{
		constexpr auto selectors = std::array{indices...};
		if constexpr (!logical_shuffle_256_has_cross_half_selector<8, selectors>())
		{
			return _mm256_shuffle_epi8(lhs, make_logical_shuffle_256_byte_control<2, false, selectors>(std::make_index_sequence<32>{}));
		}
		else
		{
			const __m256i swapped = _mm256_permute2x128_si256(lhs, lhs, 0x01);
			if constexpr (!logical_shuffle_256_has_local_half_selector<8, selectors>())
				return _mm256_shuffle_epi8(swapped, make_logical_shuffle_256_byte_control<2, true, selectors>(std::make_index_sequence<32>{}));
			else
				return _mm256_or_si256(_mm256_shuffle_epi8(lhs, make_logical_shuffle_256_byte_control<2, false, selectors>(std::make_index_sequence<32>{})),
									   _mm256_shuffle_epi8(swapped, make_logical_shuffle_256_byte_control<2, true, selectors>(std::make_index_sequence<32>{})));
		}
	}

	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm256_add_epi16(lhs, rhs);
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm256_maddubs_epi16(lhs, rhs);
	}
	/** @brief Multiplies adjacent unsigned 16-bit lanes and adds their products into unsigned 32-bit lanes. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		const __m256i lowProducts = _mm256_mullo_epi32(_mm256_cvtepu16_epi32(_mm256_castsi256_si128(lhs)), _mm256_cvtepu16_epi32(_mm256_castsi256_si128(rhs)));
		const __m256i highProducts =
			_mm256_mullo_epi32(_mm256_cvtepu16_epi32(_mm256_extracti128_si256(lhs, 1)), _mm256_cvtepu16_epi32(_mm256_extracti128_si256(rhs, 1)));
		const __m256i interleavedSums = _mm256_hadd_epi32(lowProducts, highProducts);
		return _mm256_permute4x64_epi64(interleavedSums, _MM_SHUFFLE(3, 1, 2, 0));
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _mm256_mullo_epi16(lhs, rhs);
	}
	/** @brief Divides corresponding unsigned 16-bit lanes with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext256_div_epu16(lhs, rhs);
	}
	/** @brief Computes scalar-equivalent unsigned 16-bit remainders with register-only extraction and reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext256_rem_epu16(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		auto sqrt16x8 = [](__m128i values) noexcept
		{
			const __m256i widened = _mm256_cvtepu16_epi32(values);
			const __m256 roots = _mm256_sqrt_ps(_ext256_cvtepu32_ps(widened));
			const __m256i integers = _mm256_cvtps_epi32(roots);
			return _mm_packus_epi32(_mm256_castsi256_si128(integers), _mm256_extracti128_si256(integers, 1));
		};

		const __m128i low16 = _mm256_castsi256_si128(lhs);
		const __m128i high16 = _mm256_extracti128_si256(lhs, 1);
		const __m128i rootsLow = sqrt16x8(low16);
		const __m128i rootsHigh = sqrt16x8(high16);
		return _mm256_inserti128_si256(_mm256_castsi128_si256(rootsLow), rootsHigh, 1);
	}
	/** @brief Computes one unchecked magnitude in lane zero of each 128-bit group. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<uint16_t>::magnitude(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<uint16_t>::magnitude(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Computes saturated magnitudes and adjacent overflow masks for both 128-bit groups. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<uint16_t>::magnitude_checked(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<uint16_t>::magnitude_checked(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Returns the minimum value and its first lane position without materializing register data in memory. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		const __m128i lowMeta = SimdImpl128<uint16_t>::min_position(_mm256_castsi256_si128(lhs));
		const __m128i highMeta = SimdImpl128<uint16_t>::min_position(_mm256_extracti128_si256(lhs, 1));
		const auto lowValue = static_cast<std::uint16_t>(_mm_extract_epi16(lowMeta, 0));
		const auto highValue = static_cast<std::uint16_t>(_mm_extract_epi16(highMeta, 0));
		const bool chooseHigh = highValue < lowValue;
		const __m128i selectedMeta = chooseHigh ? highMeta : lowMeta;
		const int position = static_cast<int>(_mm_extract_epi16(selectedMeta, 1)) + (chooseHigh ? 8 : 0);
		__m128i output = _mm_setzero_si128();
		output = _mm_insert_epi16(output, _mm_extract_epi16(selectedMeta, 0), 0);
		output = _mm_insert_epi16(output, position, 1);
		return _mm256_zextsi128_si256(output);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm256_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m256i lhs, __m256i rhs) noexcept
	{
		return _mm256_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _mm256_abs_epi16(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi16(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm256_min_epu16(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm256_max_epu16(lhs, rhs);
	}
	/** @brief Computes lane-wise averages for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) avg(auto lhs, auto rhs) noexcept
	{
		return _mm256_avg_epu16(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _mm256_slli_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _mm256_srli_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _mm256_srai_epi16(lhs, rhs);
	}

	// arithmetic (horizontal)
	/** @brief Horizontally adds adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm256_hadd_epi16(lhs, rhs);
	}
	/** @brief Horizontally subtracts adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm256_hsub_epi16(lhs, rhs);
	}
	/** @brief Horizontally adds unsigned 16-bit lanes with unsigned saturation in each 128-bit group. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) hadd_saturated(auto lhs, auto rhs) noexcept
	{
		const __m256i zero = _mm256_setzero_si256();
		const __m256i lhsPairs = _mm256_adds_epu16(lhs, _mm256_srli_epi32(lhs, 16));
		const __m256i rhsPairs = _mm256_adds_epu16(rhs, _mm256_srli_epi32(rhs, 16));
		return _mm256_packus_epi32(_mm256_blend_epi16(lhsPairs, zero, 0xAA), _mm256_blend_epi16(rhsPairs, zero, 0xAA));
	}
	/** @brief Horizontally subtracts unsigned 16-bit lanes with unsigned saturation in each 128-bit group. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) hsubtract_saturated(auto lhs, auto rhs) noexcept
	{
		const __m256i zero = _mm256_setzero_si256();
		const __m256i lhsPairs = _mm256_subs_epu16(lhs, _mm256_srli_epi32(lhs, 16));
		const __m256i rhsPairs = _mm256_subs_epu16(rhs, _mm256_srli_epi32(rhs, 16));
		return _mm256_packus_epi32(_mm256_blend_epi16(lhsPairs, zero, 0xAA), _mm256_blend_epi16(rhsPairs, zero, 0xAA));
	}

	// arithmetic (saturated)
	/** @brief Adds lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm256_adds_epu16(lhs, rhs);
	}
	/** @brief Subtracts lanes with saturation for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_saturated(auto lhs, auto rhs) noexcept
	{
		return _mm256_subs_epu16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) multiply_saturated(auto lhs, auto rhs) noexcept
	{
		const __m128i lhsLo128 = _mm256_castsi256_si128(lhs);
		const __m128i rhsLo128 = _mm256_castsi256_si128(rhs);
		const __m128i lhsHi128 = _mm256_extracti128_si256(lhs, 1);
		const __m128i rhsHi128 = _mm256_extracti128_si256(rhs, 1);

		const __m256i loProducts = _mm256_mullo_epi32(_mm256_cvtepu16_epi32(lhsLo128), _mm256_cvtepu16_epi32(rhsLo128));
		const __m256i hiProducts = _mm256_mullo_epi32(_mm256_cvtepu16_epi32(lhsHi128), _mm256_cvtepu16_epi32(rhsHi128));

		const __m128i loPacked = _mm_packus_epi32(_mm256_castsi256_si128(loProducts), _mm256_extracti128_si256(loProducts, 1));
		const __m128i hiPacked = _mm_packus_epi32(_mm256_castsi256_si128(hiProducts), _mm256_extracti128_si256(hiProducts, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(loPacked), hiPacked, 1);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm256_set1_epi16(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args... args) noexcept
	{
		return _mm256_set_epi16(args...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args... args) noexcept
	{
		return _mm256_setr_epi16(args...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm256_cmpeq_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _ext256_cmpgt_epu16(lhs, rhs);
	}

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm256_cvtepu16_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) compress(auto lhs, auto rhs) noexcept
	{
		return _mm256_packus_epi16(lhs, rhs);
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<uint16_t>(_mm256_extract_epi16(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected unsigned 16-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 16)`.
	 * @return Selected scalar lane.
	 */
	static uint16_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m256i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 16, "Unsigned 16-bit extraction requires a valid 256-bit lane index");
		const __m256i selected = _mm256_permutevar8x32_epi32(lhs, _mm256_set1_epi32(index >> 1));
		const uint32_t selected_dword = static_cast<uint32_t>(_mm_cvtsi128_si32(_mm256_castsi256_si128(selected)));
		return static_cast<uint16_t>(selected_dword >> ((index & 1) * 16));
	}
	/** @brief Replaces the compile-time-selected unsigned 16-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const uint16_t rhs) noexcept
	{
		return register_insert_constexpr<uint16_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected unsigned 16-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const uint16_t rhs) noexcept
	{
		return _mm256_insert_epi16(lhs, static_cast<int>(rhs), index);
	}
	/**
	 * @brief Replaces one runtime-selected unsigned 16-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 16)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m256i lhs, const uint16_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 16, "Unsigned 16-bit insertion requires a valid 256-bit lane index");
		if (index < 8)
		{
			const __m128i lower = SimdImpl128<uint16_t>::insert_slow(_mm256_castsi256_si128(lhs), rhs, index);
			return _mm256_inserti128_si256(lhs, lower, 0);
		}
		const __m128i upper = SimdImpl128<uint16_t>::insert_slow(_mm256_extracti128_si256(lhs, 1), rhs, index - 8);
		return _mm256_inserti128_si256(lhs, upper, 1);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpacklo_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpackhi_epi16(lhs, rhs);
	}

	// misc
	/** @brief Emulates an immediate-controlled low-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each low four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_lo_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_half_16_slow(lhs, static_cast<unsigned int>(rhs), false);
	}
	/** @brief Shuffles the low four unsigned 16-bit lanes in each 128-bit group with an immediate control. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle_lo(__m256i lhs) noexcept
	{
		return _mm256_shufflelo_epi16(lhs, imm8);
	}
	/** @brief Emulates an immediate-controlled high-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each high four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_hi_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_half_16_slow(lhs, static_cast<unsigned int>(rhs), true);
	}
	/** @brief Shuffles the high four unsigned 16-bit lanes in each 128-bit group with an immediate control. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle_hi(__m256i lhs) noexcept
	{
		return _mm256_shufflehi_epi16(lhs, imm8);
	}
	/** @brief Emulates an immediate-controlled blend with a runtime scalar mask.
	 *  @param lhs Source for lanes whose control bits are clear.
	 *  @param rhs Source for lanes whose control bits are set.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the selected lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) blend_slow(auto lhs, auto rhs, const int imm8) noexcept
	{
		return register_blend_slow<std::int16_t>(lhs, rhs, static_cast<unsigned int>(imm8));
	}
	/** @brief Selects unsigned 16-bit lanes from two 256-bit registers with a repeated immediate control. */
	template <int imm8> SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static auto VECTORCALL blend(auto lhs, auto rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return register_blend_slow<std::uint16_t>(lhs, rhs, static_cast<unsigned int>(imm8));
		return _mm256_blend_epi16(lhs, rhs, imm8);
	}
};

template <> struct SimdImpl256<int32_t>
{
	/** @brief Selects 32-bit lanes from two registers using a canonical predicate register. */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m256i condition, __m256i when_true, __m256i when_false) noexcept
	{
		return _mm256_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical signed 32-bit lanes across the complete 256-bit register.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 8 && ((indices < 8) && ...))
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m256i lhs) noexcept
	{
		return _mm256_permutevar8x32_epi32(lhs, _mm256_setr_epi32(static_cast<int>(indices)...));
	}

	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm256_add_epi32(lhs, rhs);
	}
	/** @brief Multiplies adjacent lanes and adds their products for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		const __m256i evenProducts = _mm256_mul_epi32(lhs, rhs);
		const __m256i oddProducts = _mm256_mul_epi32(_mm256_srli_si256(lhs, 4), _mm256_srli_si256(rhs, 4));
		return _mm256_add_epi64(evenProducts, oddProducts);
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm256_maddubs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _mm256_mullo_epi32(lhs, rhs);
	}
	/** @brief Divides corresponding signed 32-bit lanes with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext256_div_epi32(lhs, rhs);
	}
	/** @brief Computes scalar-equivalent signed 32-bit remainders with register-only extraction and reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext256_rem_epi32(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		const __m256 roots = _mm256_sqrt_ps(_mm256_cvtepi32_ps(lhs));
		return _mm256_cvtps_epi32(roots);
	}
	/** @brief Computes one unchecked magnitude in lane zero of each 128-bit group. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<int32_t>::magnitude(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<int32_t>::magnitude(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Computes saturated magnitudes and adjacent overflow masks for both 128-bit groups. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<int32_t>::magnitude_checked(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<int32_t>::magnitude_checked(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Returns the minimum value and its first lane position without materializing register data in memory. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		const __m128i lowMeta = SimdImpl128<int32_t>::min_position(_mm256_castsi256_si128(lhs));
		const __m128i highMeta = SimdImpl128<int32_t>::min_position(_mm256_extracti128_si256(lhs, 1));
		const auto lowValue = static_cast<std::int32_t>(_mm_extract_epi32(lowMeta, 0));
		const auto highValue = static_cast<std::int32_t>(_mm_extract_epi32(highMeta, 0));
		const bool chooseHigh = highValue < lowValue;
		const __m128i selectedMeta = chooseHigh ? highMeta : lowMeta;
		const int position = static_cast<int>(_mm_extract_epi32(selectedMeta, 1)) + (chooseHigh ? 4 : 0);
		__m128i output = _mm_setzero_si128();
		output = _mm_insert_epi32(output, _mm_extract_epi32(selectedMeta, 0), 0);
		output = _mm_insert_epi32(output, position, 1);
		return _mm256_zextsi128_si256(output);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm256_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m256i lhs, __m256i rhs) noexcept
	{
		return _mm256_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _mm256_abs_epi32(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi32(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm256_min_epi32(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm256_max_epi32(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _mm256_slli_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _mm256_srli_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _mm256_srai_epi32(lhs, rhs);
	}

	// arithmetic (horizontal)
	/** @brief Horizontally adds adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm256_hadd_epi32(lhs, rhs);
	}
	/** @brief Horizontally subtracts adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm256_hsub_epi32(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm256_set1_epi32(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args... args) noexcept
	{
		return _mm256_set_epi32(args...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args... args) noexcept
	{
		return _mm256_setr_epi32(args...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm256_cmpeq_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _mm256_cmpgt_epi32(lhs, rhs);
	}

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm256_cvtepi32_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) compress(auto lhs, auto rhs) noexcept
	{
		return _mm256_packs_epi32(lhs, rhs);
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<int32_t>(_mm256_extract_epi32(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected signed 32-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 8)`.
	 * @return Selected scalar lane.
	 */
	static int32_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m256i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 8, "Signed 32-bit extraction requires a valid 256-bit lane index");
		const __m256i selected = _mm256_permutevar8x32_epi32(lhs, _mm256_set1_epi32(index));
		return _mm_cvtsi128_si32(_mm256_castsi256_si128(selected));
	}
	/** @brief Replaces the compile-time-selected signed 32-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const int32_t rhs) noexcept
	{
		return register_insert_constexpr<int32_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected signed 32-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const int32_t rhs) noexcept
	{
		return _mm256_insert_epi32(lhs, rhs, index);
	}
	/**
	 * @brief Replaces one runtime-selected signed 32-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 8)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m256i lhs, const int32_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 8, "Signed 32-bit insertion requires a valid 256-bit lane index");
		if (index < 4)
		{
			const __m128i lower = SimdImpl128<int32_t>::insert_slow(_mm256_castsi256_si128(lhs), rhs, index);
			return _mm256_inserti128_si256(lhs, lower, 0);
		}
		const __m128i upper = SimdImpl128<int32_t>::insert_slow(_mm256_extracti128_si256(lhs, 1), rhs, index - 4);
		return _mm256_inserti128_si256(lhs, upper, 1);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpacklo_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpackhi_epi32(lhs, rhs);
	}

	// misc
	/** @brief Emulates an immediate-controlled low-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each low four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_lo_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_32_slow(lhs, static_cast<unsigned int>(rhs));
	}
	/** @brief Emulates an immediate-controlled high-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each high four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_hi_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_32_slow(lhs, static_cast<unsigned int>(rhs));
	}
	/** @brief Emulates an immediate-controlled blend with a runtime scalar mask.
	 *  @param lhs Source for lanes whose control bits are clear.
	 *  @param rhs Source for lanes whose control bits are set.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the selected lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) blend_slow(auto lhs, auto rhs, const int imm8) noexcept
	{
		return register_blend_slow<std::int32_t>(lhs, rhs, static_cast<unsigned int>(imm8));
	}
	/** @brief Selects signed 32-bit lanes from two 256-bit registers with an immediate control. */
	template <int imm8> SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static auto VECTORCALL blend(auto lhs, auto rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return register_blend_slow<std::int32_t>(lhs, rhs, static_cast<unsigned int>(imm8));
		return _mm256_blend_epi32(lhs, rhs, imm8);
	}
};

template <> struct SimdImpl256<uint32_t>
{
	/** @brief Selects 32-bit lanes from two registers using a canonical predicate register. */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m256i condition, __m256i when_true, __m256i when_false) noexcept
	{
		return _mm256_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical unsigned 32-bit lanes across the complete 256-bit register.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 8 && ((indices < 8) && ...))
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m256i lhs) noexcept
	{
		return _mm256_permutevar8x32_epi32(lhs, _mm256_setr_epi32(static_cast<int>(indices)...));
	}

	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm256_add_epi32(lhs, rhs);
	}
	/**
	 * @brief Converts unsigned 32-bit lanes to floating-point lanes.
	 *
	 * @param lhs The unsigned integer lanes.
	 * @return The converted floating-point lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) convert_to_float(const __m256i lhs) noexcept
	{
		return _ext256_cvtepu32_ps(lhs);
	}
	/** @brief Multiplies adjacent lanes and adds their products for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		const __m256i evenProducts = _mm256_mul_epu32(lhs, rhs);
		const __m256i oddProducts = _mm256_mul_epu32(_mm256_srli_si256(lhs, 4), _mm256_srli_si256(rhs, 4));
		return _mm256_add_epi64(evenProducts, oddProducts);
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm256_maddubs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _mm256_mullo_epi32(lhs, rhs);
	}
	/** @brief Divides corresponding unsigned 32-bit lanes with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext256_div_epu32(lhs, rhs);
	}
	/** @brief Computes scalar-equivalent unsigned 32-bit remainders with register-only extraction and reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext256_rem_epu32(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		const __m128i low = _mm256_castsi256_si128(lhs);
		const __m128i high = _mm256_extracti128_si256(lhs, 1);
		const __m128 lowRoots = _mm_sqrt_ps(_ext_cvtepu32_ps(low));
		const __m128 highRoots = _mm_sqrt_ps(_ext_cvtepu32_ps(high));
		const __m128i lowInts = _mm_cvtps_epi32(lowRoots);
		const __m128i highInts = _mm_cvtps_epi32(highRoots);
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowInts), highInts, 1);
	}
	/** @brief Computes one unchecked magnitude in lane zero of each 128-bit group. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<uint32_t>::magnitude(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<uint32_t>::magnitude(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Computes saturated magnitudes and adjacent overflow masks for both 128-bit groups. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<uint32_t>::magnitude_checked(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<uint32_t>::magnitude_checked(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Returns the minimum value and its first lane position without materializing register data in memory. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		const __m128i lowMeta = SimdImpl128<uint32_t>::min_position(_mm256_castsi256_si128(lhs));
		const __m128i highMeta = SimdImpl128<uint32_t>::min_position(_mm256_extracti128_si256(lhs, 1));
		const auto lowValue = static_cast<std::uint32_t>(_mm_extract_epi32(lowMeta, 0));
		const auto highValue = static_cast<std::uint32_t>(_mm_extract_epi32(highMeta, 0));
		const bool chooseHigh = highValue < lowValue;
		const __m128i selectedMeta = chooseHigh ? highMeta : lowMeta;
		const int position = static_cast<int>(_mm_extract_epi32(selectedMeta, 1)) + (chooseHigh ? 4 : 0);
		__m128i output = _mm_setzero_si128();
		output = _mm_insert_epi32(output, _mm_extract_epi32(selectedMeta, 0), 0);
		output = _mm_insert_epi32(output, position, 1);
		return _mm256_zextsi128_si256(output);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm256_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m256i lhs, __m256i rhs) noexcept
	{
		return _mm256_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _mm256_abs_epi32(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi32(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm256_min_epu32(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm256_max_epu32(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _mm256_slli_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _mm256_srli_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _mm256_srai_epi32(lhs, rhs);
	}

	// arithmetic (horizontal)
	/** @brief Horizontally adds adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm256_hadd_epi32(lhs, rhs);
	}
	/** @brief Horizontally subtracts adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm256_hsub_epi32(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm256_set1_epi32(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args... args) noexcept
	{
		return _mm256_set_epi32(args...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args... args) noexcept
	{
		return _mm256_setr_epi32(args...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm256_cmpeq_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _ext256_cmpgt_epu32(lhs, rhs);
	}

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm256_cvtepu32_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) compress(auto lhs, auto rhs) noexcept
	{
		return _mm256_packus_epi32(lhs, rhs);
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<uint32_t>(_mm256_extract_epi32(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected unsigned 32-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 8)`.
	 * @return Selected scalar lane.
	 */
	static uint32_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m256i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 8, "Unsigned 32-bit extraction requires a valid 256-bit lane index");
		const __m256i selected = _mm256_permutevar8x32_epi32(lhs, _mm256_set1_epi32(index));
		return static_cast<uint32_t>(_mm_cvtsi128_si32(_mm256_castsi256_si128(selected)));
	}
	/** @brief Replaces the compile-time-selected unsigned 32-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const uint32_t rhs) noexcept
	{
		return register_insert_constexpr<uint32_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected unsigned 32-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const uint32_t rhs) noexcept
	{
		return _mm256_insert_epi32(lhs, std::bit_cast<int32_t>(rhs), index);
	}
	/**
	 * @brief Replaces one runtime-selected unsigned 32-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 8)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m256i lhs, const uint32_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 8, "Unsigned 32-bit insertion requires a valid 256-bit lane index");
		if (index < 4)
		{
			const __m128i lower = SimdImpl128<uint32_t>::insert_slow(_mm256_castsi256_si128(lhs), rhs, index);
			return _mm256_inserti128_si256(lhs, lower, 0);
		}
		const __m128i upper = SimdImpl128<uint32_t>::insert_slow(_mm256_extracti128_si256(lhs, 1), rhs, index - 4);
		return _mm256_inserti128_si256(lhs, upper, 1);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpacklo_epi32(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpackhi_epi32(lhs, rhs);
	}

	// misc
	/** @brief Emulates an immediate-controlled low-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each low four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_lo_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_32_slow(lhs, static_cast<unsigned int>(rhs));
	}
	/** @brief Emulates an immediate-controlled high-half shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param rhs Runtime control byte.
	 *  @return Register with each high four-lane group shuffled.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_hi_slow(auto lhs, auto rhs) noexcept
	{
		return register_shuffle_32_slow(lhs, static_cast<unsigned int>(rhs));
	}
	/** @brief Emulates an immediate-controlled blend with a runtime scalar mask.
	 *  @param lhs Source for lanes whose control bits are clear.
	 *  @param rhs Source for lanes whose control bits are set.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the selected lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) blend_slow(auto lhs, auto rhs, const int imm8) noexcept
	{
		return register_blend_slow<std::int32_t>(lhs, rhs, static_cast<unsigned int>(imm8));
	}
	/** @brief Selects unsigned 32-bit lanes from two 256-bit registers with an immediate control. */
	template <int imm8> SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static auto VECTORCALL blend(auto lhs, auto rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return register_blend_slow<std::uint32_t>(lhs, rhs, static_cast<unsigned int>(imm8));
		return _mm256_blend_epi32(lhs, rhs, imm8);
	}
};

template <> struct SimdImpl256<int64_t>
{
	/** @brief Selects 64-bit lanes from two registers using a canonical predicate register. */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m256i condition, __m256i when_true, __m256i when_false) noexcept
	{
		return _mm256_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical signed 64-bit lanes across the complete 256-bit register.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 4 && ((indices < 4) && ...))
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m256i lhs) noexcept
	{
		return _mm256_permute4x64_epi64(lhs, encode_logical_shuffle_32_immediate<indices...>());
	}

	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm256_add_epi64(lhs, rhs);
	}
	/** @brief Multiplies adjacent lanes and adds their products for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		const __m128i low = SimdImpl128<int64_t>::multiply_add_adjacent(_mm256_castsi256_si128(lhs), _mm256_castsi256_si128(rhs));
		const __m128i high = SimdImpl128<int64_t>::multiply_add_adjacent(_mm256_extracti128_si256(lhs, 1), _mm256_extracti128_si256(rhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(low), high, 1);
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm256_maddubs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _ext256_mullo_epi64(lhs, rhs);
	}
	/** @brief Divides corresponding signed 64-bit lanes with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext256_div_epi64(lhs, rhs);
	}
	/** @brief Computes scalar-equivalent signed 64-bit remainders with register-only extraction and reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext256_rem_epi64(lhs, rhs);
	}
	/** @brief Computes integer square roots lane-wise using register extracts and reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		const __m128i lowRoots = SimdImpl128<int64_t>::sqrt(_mm256_castsi256_si128(lhs));
		const __m128i highRoots = SimdImpl128<int64_t>::sqrt(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowRoots), highRoots, 1);
	}
	/** @brief Computes one unchecked magnitude in lane zero of each 128-bit group. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<int64_t>::magnitude(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<int64_t>::magnitude(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Computes saturated magnitudes and adjacent overflow masks for both 128-bit groups. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<int64_t>::magnitude_checked(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<int64_t>::magnitude_checked(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Returns the minimum value and its first lane position without materializing register data in memory. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		const __m128i lowMeta = SimdImpl128<int64_t>::min_position(_mm256_castsi256_si128(lhs));
		const __m128i highMeta = SimdImpl128<int64_t>::min_position(_mm256_extracti128_si256(lhs, 1));
		const auto lowValue = static_cast<std::int64_t>(_mm_extract_epi64(lowMeta, 0));
		const auto highValue = static_cast<std::int64_t>(_mm_extract_epi64(highMeta, 0));
		const bool chooseHigh = highValue < lowValue;
		const __m128i selectedMeta = chooseHigh ? highMeta : lowMeta;
		const int position = static_cast<int>(_mm_extract_epi64(selectedMeta, 1)) + (chooseHigh ? 2 : 0);
		__m128i output = _mm_setzero_si128();
		output = _mm_insert_epi64(output, _mm_extract_epi64(selectedMeta, 0), 0);
		output = _mm_insert_epi64(output, position, 1);
		return _mm256_zextsi128_si256(output);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm256_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m256i lhs, __m256i rhs) noexcept
	{
		return _mm256_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _ext256_abs_epi64(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi64(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _ext256_min_epi64(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _ext256_max_epi64(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _mm256_slli_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _mm256_srli_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _ext256_srai_epi64(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm256_set1_epi64x(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args... args) noexcept
	{
		return _mm256_set_epi64x(args...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args... args) noexcept
	{
		return _mm256_setr_epi64x(args...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm256_cmpeq_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _mm256_cmpgt_epi64(lhs, rhs);
	}

	// conversion
	// static SIMDLIB_FORCE_INLINE auto VECTORCALL expand (auto lhs, auto rhs) noexcept { return _mm256_cvtepi64_epi128(lhs, rhs); }

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<int64_t>(_mm256_extract_epi64(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected signed 64-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 4)`.
	 * @return Selected scalar lane.
	 */
	static int64_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m256i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 4, "Signed 64-bit extraction requires a valid 256-bit lane index");
		const __m256i first_word = _mm256_set1_epi32(index * 2);
		const __m256i word_offsets = _mm256_setr_epi32(0, 1, 0, 1, 0, 1, 0, 1);
		const __m256i selected = _mm256_permutevar8x32_epi32(lhs, _mm256_add_epi32(first_word, word_offsets));
		return SimdImpl128<int64_t>::template extract<0>(_mm256_castsi256_si128(selected));
	}
	/** @brief Replaces the compile-time-selected signed 64-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const int64_t rhs) noexcept
	{
		return register_insert_constexpr<int64_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected signed 64-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const int64_t rhs) noexcept
	{
		return _mm256_insert_epi64(lhs, rhs, index);
	}
	/**
	 * @brief Replaces one runtime-selected signed 64-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 4)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m256i lhs, const int64_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 4, "Signed 64-bit insertion requires a valid 256-bit lane index");
		if (index < 2)
		{
			const __m128i lower = SimdImpl128<int64_t>::insert_slow(_mm256_castsi256_si128(lhs), rhs, index);
			return _mm256_inserti128_si256(lhs, lower, 0);
		}
		const __m128i upper = SimdImpl128<int64_t>::insert_slow(_mm256_extracti128_si256(lhs, 1), rhs, index - 2);
		return _mm256_inserti128_si256(lhs, upper, 1);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpacklo_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpackhi_epi64(lhs, rhs);
	}
};

template <> struct SimdImpl256<uint64_t>
{
	/** @brief Selects 64-bit lanes from two registers using a canonical predicate register. */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m256i condition, __m256i when_true, __m256i when_false) noexcept
	{
		return _mm256_blendv_epi8(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical unsigned 64-bit lanes across the complete 256-bit register.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 4 && ((indices < 4) && ...))
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m256i lhs) noexcept
	{
		return _mm256_permute4x64_epi64(lhs, encode_logical_shuffle_32_immediate<indices...>());
	}

	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm256_add_epi64(lhs, rhs);
	}
	/** @brief Multiplies adjacent lanes and adds their products for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(auto lhs, auto rhs) noexcept
	{
		const __m128i low = SimdImpl128<uint64_t>::multiply_add_adjacent(_mm256_castsi256_si128(lhs), _mm256_castsi256_si128(rhs));
		const __m128i high = SimdImpl128<uint64_t>::multiply_add_adjacent(_mm256_extracti128_si256(lhs, 1), _mm256_extracti128_si256(rhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(low), high, 1);
	}
	/** @brief Multiplies unsigned and signed byte pairs for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(auto lhs, auto rhs) noexcept
	{
		return _mm256_maddubs_epi16(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _ext256_mullo_epi64(lhs, rhs);
	}
	/** @brief Divides corresponding unsigned 64-bit lanes with scalar instructions and intrinsic reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _ext256_div_epu64(lhs, rhs);
	}
	/** @brief Computes scalar-equivalent unsigned 64-bit remainders with register-only extraction and reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(auto lhs, auto rhs) noexcept
	{
		return _ext256_rem_epu64(lhs, rhs);
	}
	/** @brief Computes integer square roots lane-wise using register extracts and reconstruction. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		const __m128i lowRoots = SimdImpl128<uint64_t>::sqrt(_mm256_castsi256_si128(lhs));
		const __m128i highRoots = SimdImpl128<uint64_t>::sqrt(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowRoots), highRoots, 1);
	}
	/** @brief Computes one unchecked magnitude in lane zero of each 128-bit group. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<uint64_t>::magnitude(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<uint64_t>::magnitude(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Computes saturated magnitudes and adjacent overflow masks for both 128-bit groups. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(auto lhs) noexcept
	{
		const __m128i lowMagnitude = SimdImpl128<uint64_t>::magnitude_checked(_mm256_castsi256_si128(lhs));
		const __m128i highMagnitude = SimdImpl128<uint64_t>::magnitude_checked(_mm256_extracti128_si256(lhs, 1));
		return _mm256_inserti128_si256(_mm256_castsi128_si256(lowMagnitude), highMagnitude, 1);
	}

	/** @brief Returns the minimum value and its first lane position without materializing register data in memory. */
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(auto lhs) noexcept
	{
		const __m128i lowMeta = SimdImpl128<uint64_t>::min_position(_mm256_castsi256_si128(lhs));
		const __m128i highMeta = SimdImpl128<uint64_t>::min_position(_mm256_extracti128_si256(lhs, 1));
		const auto lowValue = static_cast<std::uint64_t>(_mm_extract_epi64(lowMeta, 0));
		const auto highValue = static_cast<std::uint64_t>(_mm_extract_epi64(highMeta, 0));
		const bool chooseHigh = highValue < lowValue;
		const __m128i selectedMeta = chooseHigh ? highMeta : lowMeta;
		const int position = static_cast<int>(_mm_extract_epi64(selectedMeta, 1)) + (chooseHigh ? 2 : 0);
		__m128i output = _mm_setzero_si128();
		output = _mm_insert_epi64(output, _mm_extract_epi64(selectedMeta, 0), 0);
		output = _mm_insert_epi64(output, position, 1);
		return _mm256_zextsi128_si256(output);
	}
	/** @brief Computes byte-wise absolute-difference sums for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(auto lhs, auto rhs) noexcept
	{
		return _mm256_sad_epu8(lhs, rhs);
	}
	/** @brief Computes immediate-selected byte-window absolute-difference sums for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(__m256i lhs, __m256i rhs) noexcept
	{
		return _mm256_mpsadbw_epu8(lhs, rhs, imm8);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return lhs;
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_epi64(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _ext256_min_epu64(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _ext256_max_epu64(lhs, rhs);
	}

	// shifting
	static auto SIMD_FLAGS(InOut, ForceInline) shift_left(auto lhs, auto rhs) noexcept
	{
		return _mm256_slli_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right(auto lhs, auto rhs) noexcept
	{
		return _mm256_srli_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) shift_right_arithmetic(auto lhs, auto rhs) noexcept
	{
		return _ext256_srai_epi64(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm256_set1_epi64x(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args... args) noexcept
	{
		return _mm256_set_epi64x(args...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args... args) noexcept
	{
		return _mm256_setr_epi64x(args...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _mm256_cmpeq_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _ext256_cmpgt_epu64(lhs, rhs);
	}

	// conversion
	// static SIMDLIB_FORCE_INLINE auto VECTORCALL expand (auto lhs, auto rhs) noexcept { return _mm256_cvtepu64_epi128(lhs, rhs); }

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		return static_cast<uint64_t>(_mm256_extract_epi64(lhs, index));
	}
	/**
	 * @brief Extracts one runtime-selected unsigned 64-bit lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 4)`.
	 * @return Selected scalar lane.
	 */
	static uint64_t SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m256i lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 4, "Unsigned 64-bit extraction requires a valid 256-bit lane index");
		const __m256i first_word = _mm256_set1_epi32(index * 2);
		const __m256i word_offsets = _mm256_setr_epi32(0, 1, 0, 1, 0, 1, 0, 1);
		const __m256i selected = _mm256_permutevar8x32_epi32(lhs, _mm256_add_epi32(first_word, word_offsets));
		return SimdImpl128<uint64_t>::template extract<0>(_mm256_castsi256_si128(selected));
	}
	/** @brief Replaces the compile-time-selected unsigned 64-bit lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const uint64_t rhs) noexcept
	{
		return register_insert_constexpr<uint64_t>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected unsigned 64-bit lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const uint64_t rhs) noexcept
	{
		return _mm256_insert_epi64(lhs, std::bit_cast<int64_t>(rhs), index);
	}
	/**
	 * @brief Replaces one runtime-selected unsigned 64-bit lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane in the range `[0, 4)`.
	 * @return Register with the selected lane replaced.
	 */
	static __m256i SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m256i lhs, const uint64_t rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 4, "Unsigned 64-bit insertion requires a valid 256-bit lane index");
		if (index < 2)
		{
			const __m128i lower = SimdImpl128<uint64_t>::insert_slow(_mm256_castsi256_si128(lhs), rhs, index);
			return _mm256_inserti128_si256(lhs, lower, 0);
		}
		const __m128i upper = SimdImpl128<uint64_t>::insert_slow(_mm256_extracti128_si256(lhs, 1), rhs, index - 2);
		return _mm256_inserti128_si256(lhs, upper, 1);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpacklo_epi64(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpackhi_epi64(lhs, rhs);
	}
};

template <> struct SimdImpl256<float>
{
	/** @brief Selects float lanes from two registers using a canonical predicate register. */
	static __m256 SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m256 condition, __m256 when_true, __m256 when_false) noexcept
	{
		return _mm256_blendv_ps(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical floating-point lanes across the complete 256-bit register.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 8 && ((indices < 8) && ...))
	static __m256 SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m256 lhs) noexcept
	{
		return _mm256_permutevar8x32_ps(lhs, _mm256_setr_epi32(static_cast<int>(indices)...));
	}

	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm256_add_ps(lhs, rhs);
	}
	/** @brief Alternates lane subtraction and addition for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_subtract(auto lhs, auto rhs) noexcept
	{
		return _mm256_addsub_ps(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_ps(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _mm256_mul_ps(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _mm256_div_ps(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		return _mm256_sqrt_ps(lhs);
	}
	/** @brief Computes and broadcasts floating-point magnitudes independently in both 128-bit groups. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		return _mm256_sqrt_ps(_mm256_dp_ps(lhs, lhs, 0xFF));
	}
	/** @brief Multiplies lanes and adds a third register for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add(auto lhs, auto rhs, auto addend) noexcept
	{
#if SIMDLIB_HAS_FMA
		return _mm256_fmadd_ps(lhs, rhs, addend);
#else
		return _mm256_add_ps(_mm256_mul_ps(lhs, rhs), addend);
#endif
	}
	/** @brief Computes an immediate-controlled dot product for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) dot_product(__m256 lhs, __m256 rhs) noexcept
	{
		const __m128 lhsLow = _mm256_castps256_ps128(lhs);
		const __m128 lhsHigh = _mm256_extractf128_ps(lhs, 1);
		const __m128 rhsLow = _mm256_castps256_ps128(rhs);
		const __m128 rhsHigh = _mm256_extractf128_ps(rhs, 1);
		const __m128 dotLow = _mm_dp_ps(lhsLow, rhsLow, imm8);
		const __m128 dotHigh = _mm_dp_ps(lhsHigh, rhsHigh, imm8);
		return _mm256_insertf128_ps(_mm256_castps128_ps256(dotLow), dotHigh, 1);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _ext256_abs_ps(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_ps(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm256_min_ps(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm256_max_ps(lhs, rhs);
	}

	// arithmetic (horizontal)
	/** @brief Horizontally adds adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm256_hadd_ps(lhs, rhs);
	}
	/** @brief Horizontally subtracts adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm256_hsub_ps(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm256_set1_ps(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args... args) noexcept
	{
		return _mm256_set_ps(args...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args... args) noexcept
	{
		return _mm256_setr_ps(args...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _ext256_cmpeq_ps(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _ext256_cmpgt_ps(lhs, rhs);
	}

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm256_cvtps_epi32(lhs, rhs);
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		constexpr int half_index = index / 4;
		constexpr int lane_index = index % 4;
		const __m128 half = [&]()
		{
			if constexpr (half_index == 0)
				return _mm256_castps256_ps128(lhs);
			else
				return _mm256_extractf128_ps(lhs, half_index);
		}();
		return _mm_cvtss_f32(_mm_shuffle_ps(half, half, lane_index));
	}

	/**
	 * @brief Extracts one runtime-selected 32-bit floating-point lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 8)`.
	 * @return Selected scalar lane.
	 */
	static float SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m256 lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 8, "32-bit floating-point extraction requires a valid 256-bit lane index");
		const __m256 selected = _mm256_permutevar8x32_ps(lhs, _mm256_set1_epi32(index));
		return SimdImpl128<float>::template extract<0>(_mm256_castps256_ps128(selected));
	}
	/** @brief Replaces the compile-time-selected 32-bit floating-point lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const float rhs) noexcept
	{
		return register_insert_constexpr<float>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected 32-bit floating-point lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const float rhs) noexcept
	{
		constexpr int half_index = index / 4;
		constexpr int lane_index = index % 4;
		__m128 half;
		if constexpr (half_index == 0)
			half = _mm256_castps256_ps128(lhs);
		else
			half = _mm256_extractf128_ps(lhs, half_index);
		half = _mm_insert_ps(half, _mm_set_ss(rhs), lane_index << 4);
		return _mm256_insertf128_ps(lhs, half, half_index);
	}
	/**
	 * @brief Replaces one runtime-selected 32-bit floating-point lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane index.
	 * @return Register with the selected lane replaced.
	 */
	static __m256 SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m256 lhs, const float rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 8, "32-bit floating-point insertion requires a valid 256-bit lane index");
		if (index < 4)
		{
			const __m128 lower = SimdImpl128<float>::insert_slow(_mm256_castps256_ps128(lhs), rhs, index);
			return _mm256_insertf128_ps(lhs, lower, 0);
		}
		const __m128 upper = SimdImpl128<float>::insert_slow(_mm256_extractf128_ps(lhs, 1), rhs, index - 4);
		return _mm256_insertf128_ps(lhs, upper, 1);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpacklo_ps(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpackhi_ps(lhs, rhs);
	}

	// misc
	/** @brief Emulates an immediate-controlled floating shuffle with a runtime scalar control.
	 *  @param lhs Source for the lower selected lanes in each group.
	 *  @param rhs Source for the upper selected lanes in each group.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the shuffled lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_slow(auto lhs, auto rhs, const int imm8) noexcept
	{
		return register_shuffle_float_slow(lhs, rhs, imm8);
	}
	/** @brief Emulates an immediate-controlled blend with a runtime scalar mask.
	 *  @param lhs Source for lanes whose control bits are clear.
	 *  @param rhs Source for lanes whose control bits are set.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the selected lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) blend_slow(auto lhs, auto rhs, const int imm8) noexcept
	{
		return register_blend_slow<float>(lhs, rhs, static_cast<unsigned int>(imm8));
	}
	/** @brief Selects 32-bit floating-point lanes from two 256-bit registers with an immediate control. */
	template <int imm8> SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static auto VECTORCALL blend(auto lhs, auto rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return register_blend_slow<float>(lhs, rhs, static_cast<unsigned int>(imm8));
		return _mm256_blend_ps(lhs, rhs, imm8);
	}
};

template <> struct SimdImpl256<double>
{
	/** @brief Selects double lanes from two registers using a canonical predicate register. */
	static __m256d SIMD_FLAGS(InOut, RegisterOnly, ForceInline) select(__m256d condition, __m256d when_true, __m256d when_false) noexcept
	{
		return _mm256_blendv_pd(when_false, when_true, condition);
	}

	/**
	 * @brief Shuffles logical double-precision floating-point lanes across the complete 256-bit register.
	 * @tparam indices Source lane for each result lane in low-to-high order.
	 * @param lhs Source register.
	 * @return Register containing the selected logical lanes.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == 4 && ((indices < 4) && ...))
	static __m256d SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(__m256d lhs) noexcept
	{
		return _mm256_permute4x64_pd(lhs, encode_logical_shuffle_32_immediate<indices...>());
	}

	// arithmetic
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add(auto lhs, auto rhs) noexcept
	{
		return _mm256_add_pd(lhs, rhs);
	}
	/** @brief Alternates lane subtraction and addition for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_subtract(auto lhs, auto rhs) noexcept
	{
		return _mm256_addsub_pd(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_pd(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply(auto lhs, auto rhs) noexcept
	{
		return _mm256_mul_pd(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) divide(auto lhs, auto rhs) noexcept
	{
		return _mm256_div_pd(lhs, rhs);
	}
	/** @brief Computes lane-wise square roots for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(auto lhs) noexcept
	{
		return _mm256_sqrt_pd(lhs);
	}
	/** @brief Computes and broadcasts floating-point magnitudes independently in both 128-bit groups. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(auto lhs) noexcept
	{
		const __m256d squares = _mm256_mul_pd(lhs, lhs);
		return _mm256_sqrt_pd(_mm256_hadd_pd(squares, squares));
	}
	/** @brief Multiplies lanes and adds a third register for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add(auto lhs, auto rhs, auto addend) noexcept
	{
#if SIMDLIB_HAS_FMA
		return _mm256_fmadd_pd(lhs, rhs, addend);
#else
		return _mm256_add_pd(_mm256_mul_pd(lhs, rhs), addend);
#endif
	}
	/** @brief Computes an immediate-controlled dot product for this native register specialization. */
	template <int imm8> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) dot_product(__m256d lhs, __m256d rhs) noexcept
	{
		const __m128d lhsLow = _mm256_castpd256_pd128(lhs);
		const __m128d lhsHigh = _mm256_extractf128_pd(lhs, 1);
		const __m128d rhsLow = _mm256_castpd256_pd128(rhs);
		const __m128d rhsHigh = _mm256_extractf128_pd(rhs, 1);
		const __m128d dotLow = _mm_dp_pd(lhsLow, rhsLow, imm8);
		const __m128d dotHigh = _mm_dp_pd(lhsHigh, rhsHigh, imm8);
		return _mm256_insertf128_pd(_mm256_castpd128_pd256(dotLow), dotHigh, 1);
	}
	//
	/** @brief Computes lane-wise absolute values for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(auto lhs) noexcept
	{
		return _ext256_abs_pd(lhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) negate(auto lhs, auto rhs) noexcept
	{
		return _mm256_sub_pd(lhs, rhs);
	}
	/** @brief Computes lane-wise minima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(auto lhs, auto rhs) noexcept
	{
		return _mm256_min_pd(lhs, rhs);
	}
	/** @brief Computes lane-wise maxima for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(auto lhs, auto rhs) noexcept
	{
		return _mm256_max_pd(lhs, rhs);
	}

	// arithmetic (horizontal)
	/** @brief Horizontally adds adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm256_hadd_pd(lhs, rhs);
	}
	/** @brief Horizontally subtracts adjacent lanes for this native register specialization. */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_horizontal(auto lhs, auto rhs) noexcept
	{
		return _mm256_hsub_pd(lhs, rhs);
	}

	// loading
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set1(auto lhs) noexcept
	{
		return _mm256_set1_pd(lhs);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) set(Args... args) noexcept
	{
		return _mm256_set_pd(args...);
	}
	template <typename... Args> static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline) setr(Args... args) noexcept
	{
		return _mm256_setr_pd(args...);
	}

	// comparison
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpeq(auto lhs, auto rhs) noexcept
	{
		return _ext256_cmpeq_pd(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) cmpgt(auto lhs, auto rhs) noexcept
	{
		return _ext256_cmpgt_pd(lhs, rhs);
	}

	// conversion
	static auto SIMD_FLAGS(InOut, ForceInline) expand(auto lhs, auto rhs) noexcept
	{
		return _mm256_cvtps_epi32(lhs, rhs);
	}

	// extract / insert
	template <int index> static auto SIMD_FLAGS(In, RegisterOnly, ForceInline) extract(auto lhs) noexcept
	{
		constexpr int half_index = index / 2;
		constexpr int lane_index = index % 2;
		const __m128d half = [&]()
		{
			if constexpr (half_index == 0)
				return _mm256_castpd256_pd128(lhs);
			else
				return _mm256_extractf128_pd(lhs, half_index);
		}();
		if constexpr (lane_index == 0)
			return _mm_cvtsd_f64(half);
		else
			return _mm_cvtsd_f64(_mm_unpackhi_pd(half, half));
	}

	/**
	 * @brief Extracts one runtime-selected 64-bit floating-point lane.
	 * @param lhs Source register.
	 * @param index Selected lane in the range `[0, 4)`.
	 * @return Selected scalar lane.
	 */
	static double SIMD_FLAGS(In, RegisterOnly, ForceInline) extract_slow(const __m256d lhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 4, "64-bit floating-point extraction requires a valid 256-bit lane index");
		const __m256i first_word = _mm256_set1_epi32(index * 2);
		const __m256i word_offsets = _mm256_setr_epi32(0, 1, 0, 1, 0, 1, 0, 1);
		const __m256i selected = _mm256_permutevar8x32_epi32(_mm256_castpd_si256(lhs), _mm256_add_epi32(first_word, word_offsets));
		return SimdImpl128<double>::template extract<0>(_mm_castsi128_pd(_mm256_castsi256_si128(selected)));
	}
	/** @brief Replaces the compile-time-selected 64-bit floating-point lane during constant evaluation. */
	template <int index> [[nodiscard]] constexpr static auto insert_constexpr(auto lhs, const double rhs) noexcept
	{
		return register_insert_constexpr<double>(lhs, rhs, static_cast<std::size_t>(index));
	}
	/** @brief Replaces the compile-time-selected 64-bit floating-point lane. */
	template <int index> static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert(auto lhs, const double rhs) noexcept
	{
		constexpr int half_index = index / 2;
		constexpr int lane_index = index % 2;
		__m128d half;
		if constexpr (half_index == 0)
			half = _mm256_castpd256_pd128(lhs);
		else
			half = _mm256_extractf128_pd(lhs, half_index);
		const __m128d replacement = _mm_set_sd(rhs);
		if constexpr (lane_index == 0)
			half = _mm_move_sd(half, replacement);
		else
			half = _mm_unpacklo_pd(half, replacement);
		return _mm256_insertf128_pd(lhs, half, half_index);
	}
	/**
	 * @brief Replaces one runtime-selected 64-bit floating-point lane.
	 * @param lhs Source register.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane index.
	 * @return Register with the selected lane replaced.
	 */
	static __m256d SIMD_FLAGS(InOut, RegisterOnly, ForceInline) insert_slow(const __m256d lhs, const double rhs, const int index) noexcept
	{
		SIMDLIB_PRECONDITION(index >= 0 && index < 4, "64-bit floating-point insertion requires a valid 256-bit lane index");
		if (index < 2)
		{
			const __m128d lower = SimdImpl128<double>::insert_slow(_mm256_castpd256_pd128(lhs), rhs, index);
			return _mm256_insertf128_pd(lhs, lower, 0);
		}
		const __m128d upper = SimdImpl128<double>::insert_slow(_mm256_extractf128_pd(lhs, 1), rhs, index - 2);
		return _mm256_insertf128_pd(lhs, upper, 1);
	}

	// unpack / pack
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_lo(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpacklo_pd(lhs, rhs);
	}
	static auto SIMD_FLAGS(InOut, ForceInline) unpack_hi(auto lhs, auto rhs) noexcept
	{
		return _mm256_unpackhi_pd(lhs, rhs);
	}

	// misc
	/** @brief Emulates an immediate-controlled floating shuffle with a runtime scalar control.
	 *  @param lhs Source for the lower selected lanes in each group.
	 *  @param rhs Source for the upper selected lanes in each group.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the shuffled lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) shuffle_slow(auto lhs, auto rhs, const int imm8) noexcept
	{
		return register_shuffle_double_slow(lhs, rhs, imm8);
	}
	/** @brief Emulates an immediate-controlled blend with a runtime scalar mask.
	 *  @param lhs Source for lanes whose control bits are clear.
	 *  @param rhs Source for lanes whose control bits are set.
	 *  @param imm8 Runtime control byte.
	 *  @return Register containing the selected lanes.
	 */
	static auto SIMD_FLAGS(InOut, ForceInline) blend_slow(auto lhs, auto rhs, const int imm8) noexcept
	{
		return register_blend_slow<double>(lhs, rhs, static_cast<unsigned int>(imm8));
	}
	/** @brief Selects 64-bit floating-point lanes from two 256-bit registers with an immediate control. */
	template <int imm8> SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static auto VECTORCALL blend(auto lhs, auto rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return register_blend_slow<double>(lhs, rhs, static_cast<unsigned int>(imm8));
		return _mm256_blend_pd(lhs, rhs, imm8 & 0x0F);
	}
};

#pragma endregion

#pragma region 256-Bit Mappings

template <class element_t> struct SimdMappings<256, element_t> : public SimdImpl256<element_t>
{
  private:
	using impl = SimdImpl256<element_t>;

  public:
	using impl::extract;
	using impl::shuffle;

	template <class ty> using Mappings = SimdMappings<256, ty>;
	template <class ty> using mapped_vector_t = typename Mappings<ty>::vector_t;
	template <class ty>
		requires std::is_integral_v<ty>
	using promoted_signed_vector_t = mapped_vector_t<promoted_signed_t<ty>>;
	template <class ty>
		requires std::is_integral_v<ty>
	using promoted_unsigned_vector_t = mapped_vector_t<promoted_unsigned_t<ty>>;
	using mask_t = uint32_t;
	using int_vector_t = __m256i;
	using float_vector_t = __m256;
	using double_vector_t = __m256d;
	using vector_t = std::conditional_t<std::is_integral_v<element_t>, __m256i, std::conditional_t<std::is_same_v<element_t, float>, __m256, __m256d>>;
	constexpr static inline mask_t CMP_MASK = 0xFFFFFFFF;
	constexpr static inline std::size_t register_width = 256;
	constexpr static inline std::size_t element_count = register_width / (sizeof(element_t) * 8);
	constexpr static inline std::size_t element_size = sizeof(element_t);
	constexpr static inline std::size_t element_width = 8 * element_size;

	static typename SimdMappings<128, element_t>::vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) lower_half(const vector_t lhs) noexcept
	{
		if constexpr (std::is_integral_v<element_t>)
			return _mm256_castsi256_si128(lhs);
		else if constexpr (std::same_as<element_t, float>)
			return _mm256_castps256_ps128(lhs);
		else
			return _mm256_castpd256_pd128(lhs);
	}

#pragma region Set
	/// <summary> Set all elements of the register to 0 (often a noop). </summary>
	constexpr static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) setzero() noexcept
	{
		if (std::is_constant_evaluated())
		{
			return set1(element_t{0});
		}
		else
		{
			if constexpr (std::is_integral_v<element_t>)
				return _mm256_setzero_si256();
			else if constexpr (std::is_same_v<element_t, float>)
				return _mm256_setzero_ps();
			else if constexpr (std::is_same_v<element_t, double>)
				return _mm256_setzero_pd();
		}
	}

	template <std::convertible_to<element_t>... Args>
		requires(sizeof...(Args) == element_count)
	constexpr static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) setr(Args &&...args) noexcept
	{
		if (std::is_constant_evaluated())
		{
			return setr_constexpr(std::forward<Args>(args)...);
		}
		else
		{
			return impl::setr(std::forward<Args>(args)...);
		}
	}

	constexpr static vector_t SIMD_FLAGS(Out, ForceInline, Flatten) construct(const std::array<element_t, element_count> &data) noexcept
	{
		if (std::is_constant_evaluated())
		{
			return register_from_array<vector_t>(data);
		}
		else
		{
			return load_unaligned(data.data());
		}
	}

	constexpr static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) set1(const element_t value) noexcept
	{
		if (std::is_constant_evaluated())
			return set1_constexpr(value);
		else
		{
			return impl::set1(value);
		}
	}

	/** @brief Broadcasts one value through the portable compile-time register representation. */
	constexpr static vector_t set1_constexpr(const element_t value) noexcept
	{
		return register_from_repeated_value<vector_t>(value);
	}

	/** @brief Constructs a register from forward-order lanes during constant evaluation. */
	template <std::convertible_to<element_t>... Args>
		requires(sizeof...(Args) == element_count)
	constexpr static vector_t setr_constexpr(Args &&...args) noexcept
	{
		return register_from_values<vector_t, element_t>(static_cast<element_t>(args)...);
	}

	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add(const vector_t lhs, const vector_t rhs, const vector_t addend) noexcept
	{
		if constexpr (requires(vector_t left, vector_t right, vector_t sum) { impl::multiply_add(left, right, sum); })
			return impl::multiply_add(lhs, rhs, addend);
		else
			return impl::add(impl::multiply(lhs, rhs), addend);
	}

	static std::span<element_t, element_count> SIMD_FLAGS(Neither, ForceInline, Flatten) view_data(vector_t &vec) noexcept
	{
		return std::span<element_t, element_count>{register_data<element_t>(vec), element_count};
	}

	static std::span<const element_t, element_count> SIMD_FLAGS(Neither, ForceInline, Flatten) view_data(const vector_t &vec) noexcept
	{
		return std::span<const element_t, element_count>{register_data<element_t>(vec), element_count};
	}

#pragma endregion

#pragma region Load
	/**
	 * @brief Loads a complete 256-bit object representation without an alignment requirement.
	 * @param ptr Source containing at least 32 accessible bytes.
	 * @return Native register preserving every source bit.
	 */
	static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load_bytes(const void *ptr) noexcept
	{
		const int_vector_t bits = _mm256_loadu_si256(reinterpret_cast<const int_vector_t *>(ptr));
		if constexpr (std::is_integral_v<element_t>)
			return bits;
		else if constexpr (std::is_same_v<element_t, float>)
			return _mm256_castsi256_ps(bits);
		else
			return _mm256_castsi256_pd(bits);
	}

	/// <summary>Loads a full register from memory. Pointer must be appropriately aligned for the register width.</summary>
	static int_vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load(const element_t *ptr) noexcept
		requires std::is_integral_v<element_t>
	{
		return _mm256_load_si256(reinterpret_cast<const int_vector_t *>(ptr));
	}

	/// <summary>Loads a full register from memory without requiring alignment.</summary>
	static int_vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load_unaligned(const element_t *ptr) noexcept
		requires std::is_integral_v<element_t>
	{
		return _mm256_loadu_si256(reinterpret_cast<const int_vector_t *>(ptr));
	}

	/// <summary>
	/// Loads the lower half of the register from memory (in bytes), zeroing the upper half.
	/// For 256-bit registers this loads the low 128-bit lane and clears the high lane.
	/// Intended for safe tail handling without over-reading past the end of a buffer.
	/// </summary>
	static int_vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load_half(const element_t *ptr) noexcept
		requires std::is_integral_v<element_t>
	{
		const __m128i lo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr));
		return _mm256_inserti128_si256(_mm256_setzero_si256(), lo, 0);
	}

	/// <summary>Loads a full register from memory. Pointer must be appropriately aligned for the register width.</summary>
	static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load(const element_t *ptr) noexcept
		requires std::is_floating_point_v<element_t>
	{
		if constexpr (std::is_same_v<element_t, float>)
			return _mm256_load_ps(ptr);
		else if constexpr (std::is_same_v<element_t, double>)
			return _mm256_load_pd(ptr);
	}

	/// <summary>Loads a full register from memory without requiring alignment.</summary>
	static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load_unaligned(const element_t *ptr) noexcept
		requires std::is_floating_point_v<element_t>
	{
		if constexpr (std::is_same_v<element_t, float>)
			return _mm256_loadu_ps(ptr);
		else if constexpr (std::is_same_v<element_t, double>)
			return _mm256_loadu_pd(ptr);
	}
#pragma endregion

#pragma region Store
	/// <summary>Stores a full register to memory. Pointer must be appropriately aligned for the register width.</summary>
	static void SIMD_FLAGS(In, ForceInline, Flatten) store(int_vector_t lhs, void *ptr) noexcept
		requires std::is_integral_v<element_t>
	{
		_mm256_store_si256(reinterpret_cast<int_vector_t *>(ptr), lhs);
	}

	/// <summary>Stores a full register to memory without requiring alignment.</summary>
	static void SIMD_FLAGS(In, ForceInline, Flatten) store_unaligned(int_vector_t lhs, void *ptr) noexcept
		requires std::is_integral_v<element_t>
	{
		_mm256_storeu_si256(reinterpret_cast<int_vector_t *>(ptr), lhs);
	}

	/// <summary>
	/// Stores the lower half of the register to memory (in bytes).
	/// For 256-bit registers this stores only the low 128-bit lane.
	/// Intended for safe tail handling without over-writing past the end of a buffer.
	/// </summary>
	static void SIMD_FLAGS(In, ForceInline, Flatten) store_half(int_vector_t lhs, void *ptr) noexcept
		requires std::is_integral_v<element_t>
	{
		const __m128i lo = _mm256_castsi256_si128(lhs);
		_mm_storeu_si128(reinterpret_cast<__m128i *>(ptr), lo);
	}

	/// <summary>Stores a full register to memory. Pointer must be appropriately aligned for the register width.</summary>
	static void SIMD_FLAGS(In, ForceInline, Flatten) store(vector_t lhs, void *ptr) noexcept
		requires std::is_floating_point_v<element_t>
	{
		if constexpr (std::is_same_v<element_t, float>)
			_mm256_store_ps(reinterpret_cast<float *>(ptr), lhs);
		else if constexpr (std::is_same_v<element_t, double>)
			_mm256_store_pd(reinterpret_cast<double *>(ptr), lhs);
	}

	/// <summary>Stores a full register to memory without requiring alignment.</summary>
	static void SIMD_FLAGS(In, ForceInline, Flatten) store_unaligned(vector_t lhs, void *ptr) noexcept
		requires std::is_floating_point_v<element_t>
	{
		if constexpr (std::is_same_v<element_t, float>)
			_mm256_storeu_ps(reinterpret_cast<float *>(ptr), lhs);
		else if constexpr (std::is_same_v<element_t, double>)
			_mm256_storeu_pd(reinterpret_cast<double *>(ptr), lhs);
	}
#pragma endregion

#pragma region Bitwise Operations
	/**
	 * @brief Computes the bitwise AND of two mapped registers.
	 *
	 * @param lhs The first register.
	 * @param rhs The second register.
	 * @return The resulting mapped register.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_and(vector_t lhs, vector_t rhs) noexcept
	{
		if constexpr (std::is_integral_v<element_t>)
			return _mm256_and_si256(lhs, rhs);
		else if constexpr (std::same_as<element_t, float>)
			return _mm256_and_ps(lhs, rhs);
		else
			return _mm256_and_pd(lhs, rhs);
	}
	/**
	 * @brief Computes the bitwise OR of two mapped registers.
	 *
	 * @param lhs The first register.
	 * @param rhs The second register.
	 * @return The resulting mapped register.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_or(vector_t lhs, vector_t rhs) noexcept
	{
		if constexpr (std::is_integral_v<element_t>)
			return _mm256_or_si256(lhs, rhs);
		else if constexpr (std::same_as<element_t, float>)
			return _mm256_or_ps(lhs, rhs);
		else
			return _mm256_or_pd(lhs, rhs);
	}
	/**
	 * @brief Computes the bitwise XOR of two mapped registers.
	 *
	 * @param lhs The first register.
	 * @param rhs The second register.
	 * @return The resulting mapped register.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_xor(vector_t lhs, vector_t rhs) noexcept
	{
		if constexpr (std::is_integral_v<element_t>)
			return _mm256_xor_si256(lhs, rhs);
		else if constexpr (std::same_as<element_t, float>)
			return _mm256_xor_ps(lhs, rhs);
		else
			return _mm256_xor_pd(lhs, rhs);
	}
	/**
	 * @brief Computes the bitwise AND of the complemented first register and the second register.
	 *
	 * @param lhs The register to complement.
	 * @param rhs The register to combine with the complement.
	 * @return The resulting mapped register.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_andnot(vector_t lhs, vector_t rhs) noexcept
	{
		if constexpr (std::is_integral_v<element_t>)
			return _mm256_andnot_si256(lhs, rhs);
		else if constexpr (std::same_as<element_t, float>)
			return _mm256_andnot_ps(lhs, rhs);
		else
			return _mm256_andnot_pd(lhs, rhs);
	}
	/**
	 * @brief Computes the bitwise complement of a mapped register.
	 *
	 * @param lhs The source register.
	 * @return The resulting mapped register.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_not(vector_t lhs) noexcept
	{
		if constexpr (std::is_integral_v<element_t>)
			return _mm256_xor_si256(lhs, _mm256_cmpeq_epi32(_mm256_setzero_si256(), _mm256_setzero_si256()));
		else if constexpr (std::same_as<element_t, float>)
			return _mm256_xor_ps(lhs, _ext256_cmpeq_ps(_mm256_setzero_ps(), _mm256_setzero_ps()));
		else
			return _mm256_xor_pd(lhs, _mm256_castsi256_pd(_mm256_cmpeq_epi32(_mm256_setzero_si256(), _mm256_setzero_si256())));
	}
#pragma endregion

#pragma region Arithmetic Operations
	static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) negate(int_vector_t lhs) noexcept
		requires std::is_integral_v<element_t>
	{
		if constexpr (sizeof(element_t) == 8)
			return _mm256_sub_epi64(_mm256_setzero_si256(), lhs);
		else if constexpr (sizeof(element_t) == 4)
			return _mm256_sub_epi32(_mm256_setzero_si256(), lhs);
		else if constexpr (sizeof(element_t) == 2)
			return _mm256_sub_epi16(_mm256_setzero_si256(), lhs);
		else
			return _mm256_sub_epi8(_mm256_setzero_si256(), lhs);
	}

	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) negate(vector_t lhs) noexcept
		requires std::is_floating_point_v<element_t>
	{
		if constexpr (std::same_as<element_t, float>)
			return _mm256_sub_ps(_mm256_setzero_ps(), lhs);
		else
			return _mm256_sub_pd(_mm256_setzero_pd(), lhs);
	}
#pragma endregion

#pragma region Shuffling
	/** @brief Emulates an immediate-controlled 32-bit shuffle with a runtime scalar control.
	 *  @param lhs Source register.
	 *  @param imm8 Runtime control byte.
	 *  @return Register with each four-lane group shuffled.
	 */
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY static int_vector_t VECTORCALL shuffle_32_slow(int_vector_t lhs, std::uint32_t imm8) noexcept
		requires std::is_integral_v<element_t>
	{
		return register_shuffle_32_slow(lhs, imm8);
	}

	/// <summary> Shuffles the 32-bit integers in the vector using a compile-time control mask. </summary>
	template <std::uint32_t imm8>
	static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle_32(int_vector_t lhs) noexcept
		requires std::is_integral_v<element_t>
	{
		return _mm256_shuffle_epi32(lhs, imm8);
	}

	/// <summary> Shuffles the bytes in the vector using the indexes in the second vector. </summary>
	static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(int_vector_t lhs, int_vector_t rhs) noexcept
		requires std::is_integral_v<element_t>
	{
		return _mm256_shuffle_epi8(lhs, rhs);
	}

#pragma endregion

#pragma region Miscellaneous Operations
	/// <summary> Returns a mask of the most significant BIT of each BYTE in each element. </summary>
	static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) movemask(const vector_t lhs) noexcept
	{
		if constexpr (std::is_integral_v<element_t>)
			return _mm256_movemask_epi8(lhs);
		else if constexpr (std::is_same_v<element_t, float>)
			return _mm256_movemask_epi8(_mm256_castps_si256(lhs));
		else if constexpr (std::is_same_v<element_t, double>)
			return _mm256_movemask_epi8(_mm256_castpd_si256(lhs));
	}

	/// <summary> Returns a mask of the most significant BIT of each element. </summary>
	static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) movemask_slim(const vector_t lhs) noexcept
	{
		if constexpr (std::is_integral_v<element_t>)
		{
			// Note: _mm256_shuffle_epi8 is lane-local; compress the resulting byte-mask to element bits.
			const uint32_t raw = static_cast<uint32_t>(movemask(swizzle_msb(lhs)));

			if constexpr (sizeof(element_t) == 1)
			{
				return static_cast<mask_t>(raw);
			}
			else if constexpr (sizeof(element_t) == 2)
			{ // 8 elements per 128-bit lane => bits 0..7 and 16..23
				return static_cast<mask_t>((raw & 0x00FFu) | ((raw >> 8) & 0xFF00u));
			}
			else if constexpr (sizeof(element_t) == 4)
			{ // 4 elements per 128-bit lane => bits 0..3 and 16..19
				return static_cast<mask_t>((raw & 0x000Fu) | ((raw >> 12) & 0x00F0u));
			}
			else if constexpr (sizeof(element_t) == 8)
			{ // 2 elements per 128-bit lane => bits 0..1 and 16..17
				return static_cast<mask_t>((raw & 0x0003u) | ((raw >> 14) & 0x000Cu));
			}
			else
			{
				static_assert(sizeof(element_t) == 1 || sizeof(element_t) == 2 || sizeof(element_t) == 4 || sizeof(element_t) == 8, "Unsupported element size");
			}
		}
		else if constexpr (std::is_same_v<element_t, float>)
			return _mm256_movemask_ps(lhs);
		else if constexpr (std::is_same_v<element_t, double>)
			return _mm256_movemask_pd(lhs);
	}

	/// <summary> Compute the bitwise AND of 256 bits (representing integer data) in a and b, and set ZF to 1 if the result is zero, otherwise set ZF to 0.
	/// Compute the bitwise NOT of a and then AND with b, and set CF to 1 if the result is zero, otherwise set CF to 0. Return the CF value. </summary>
	static int SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) test(int_vector_t lhs, int_vector_t rhs) noexcept
	{
		return _mm256_testc_si256(lhs, rhs);
	}

	/// <summary> Compute the bitwise AND of 256 bits (representing integer data) in a and b, and set ZF to 1 if the result is zero, otherwise set ZF to 0.
	/// Compute the bitwise NOT of a and then AND with b, and set CF to 1 if the result is zero, otherwise set CF to 0. Return the ZF value. </summary>
	static int SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) testz(int_vector_t lhs, int_vector_t rhs) noexcept
	{
		return _mm256_testz_si256(lhs, rhs);
	}

	/// <summary> Compute the bitwise AND of 256 bits (representing integer data) in a and b, and set ZF to 1 if the result is zero, otherwise set ZF to 0.
	/// Compute the bitwise NOT of a and then AND with b, and set CF to 1 if the result is zero, otherwise set CF to 0. Return 1 if both the ZF and CF values
	/// are zero, otherwise return 0. </summary>
	static int SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) testnzc(int_vector_t lhs, int_vector_t rhs) noexcept
	{
		return _mm256_testnzc_si256(lhs, rhs);
	}

	/// <summary> Returns the shuffle order to move the most significant byte of each element into the least significant bytes. </summary>
	constexpr static int_vector_t get_msb_swizzle_order() noexcept
	{
		int_vector_t seq{};
		constexpr const auto elem_size = sizeof(element_t);
		constexpr const auto elems_per_lane = 16 / elem_size;

		for (std::size_t lane = 0; lane < 2; ++lane)
		{
			const std::size_t lane_base = lane * 16;
			for (std::size_t i = 0; i < 16; ++i)
			{
				if (i < elems_per_lane)
				{
					// Select the MSB byte of each element within the 128-bit lane.
					register_set_constexpr<std::uint8_t>(seq, lane_base + i, static_cast<std::uint8_t>((i * elem_size) + (elem_size - 1)));
				}
				else
				{
					// Zero out the rest (PSHUFB: high bit set => 0).
					register_set_constexpr<std::uint8_t>(seq, lane_base + i, 0x80);
				}
			}
		}
		return seq;
	}

	/// <summary> Swizzle the vector to only contain the most significant bit of each byte. </summary>
	static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) swizzle_msb(int_vector_t lhs) noexcept
	{
		return shuffle(lhs, get_msb_swizzle_order());
	}
#pragma endregion
};

#pragma endregion

#endif // SIMDLIB_HAS_AVX2 && SIMDLIB_HAS_SSE42

} // namespace SimdLib::Detail
