#pragma once
#include <SimdLib/Config.h>
#include <SimdLib/Detail/Implementations.h>
#include <SimdLib/TemplateTools.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <functional>
#include <immintrin.h>
#include <intrin.h>
#include <limits>
#include <span>
#include <type_traits>
#include <utility>

// This file contains SIMD implementations for 128-bit and 256-bit integer and floating-point types.
// REFERENCE: http://www.alfredklomp.com/programming/sse-intrinsics/

namespace SimdLib
{

namespace Detail
{
enum class comparison_operation
{
	less,
	greater,
	equivalent,
	unordered,
};
} // namespace Detail

template <std::size_t register_width, class element_t>
inline constexpr bool is_api_available_v =
	(std::same_as<element_t, std::int8_t> || std::same_as<element_t, std::uint8_t> ||
	 std::same_as<element_t, std::int16_t> || std::same_as<element_t, std::uint16_t> ||
	 std::same_as<element_t, std::int32_t> || std::same_as<element_t, std::uint32_t> ||
	 std::same_as<element_t, std::int64_t> || std::same_as<element_t, std::uint64_t> ||
	 std::same_as<element_t, float> || std::same_as<element_t, double>) &&
	Config::target_x86 &&
	((register_width == 128 && Config::has_sse42) || (register_width == 256 && Config::has_sse42 && Config::has_avx2));

template <std::size_t register_width, class element_t>
concept ApiAvailable = is_api_available_v<register_width, element_t>;

/**
 * @brief Primary API for SIMD operations, parameterized by register width and element type.
 * @tparam register_width The width of SIMD registers in bits (e.g. 128, 256).
 * @tparam element_t The type of elements contained in the SIMD register; must be an arithmetic type.
 * This class serves as the main interface for SIMD operations, providing type definitions and forwarding to the appropriate implementation based on the
 * specified register width and element type. It also includes utilities for determining promoted types for integer operations.
 */
template <std::size_t register_width, class element_t>
	requires ApiAvailable<register_width, element_t>
struct Api : public Detail::SimdMappings<register_width, element_t>
{
  protected:
	using impl = SimdLib::Detail::SimdMappings<register_width, element_t>;

  public:
	template <class ty> using Mappings = SimdLib::Detail::SimdMappings<register_width, ty>;
	template <class ty> using mapped_vector_t = typename Mappings<ty>::vector_t;
	template <class ty>
		requires std::is_integral_v<ty>
	using promoted_signed_t = SimdLib::Detail::promoted_signed_t<ty>;
	template <class ty>
		requires std::is_integral_v<ty>
	using promoted_unsigned_t = SimdLib::Detail::promoted_unsigned_t<ty>;
	template <class ty>
		requires std::is_integral_v<ty>
	using promoted_signed_vector_t = mapped_vector_t<promoted_signed_t<ty>>;
	template <class ty>
		requires std::is_integral_v<ty>
	using promoted_unsigned_vector_t = mapped_vector_t<promoted_unsigned_t<ty>>;

	using impl::add;
	using impl::divide;
	using impl::max;
	using impl::min;
	using impl::multiply;
	using impl::subtract;

	using element_type = element_t;
	using mask_t = impl::mask_t;
	using vector_t = impl::vector_t;
	using int_vector_t = impl::int_vector_t;
	using float_vector_t = impl::float_vector_t;
	//
	constexpr static inline bool using_int = std::is_integral_v<element_t>;
	constexpr static inline bool using_unsigned = std::is_unsigned_v<element_t>;
	constexpr static inline bool using_float = std::is_floating_point_v<element_t>;
	constexpr static inline std::size_t element_width = sizeof(element_t) * 8;
	constexpr static inline std::size_t byte_count = register_width / 8;
	constexpr static inline std::size_t byte_count_half = byte_count / 2;
	constexpr static inline std::size_t element_count = register_width / (sizeof(element_t) * 8);
	constexpr static inline std::size_t element_count_half = element_count / 2;
	template <std::size_t result_bit_width> using packed_element_t = select_unsigned_integer_t<result_bit_width>;
	template <std::size_t result_bit_width, std::size_t source_count>
	constexpr static inline std::size_t packed_element_count =
		(source_count * result_bit_width + std::numeric_limits<packed_element_t<result_bit_width>>::digits - 1) /
		std::numeric_limits<packed_element_t<result_bit_width>>::digits;

	template <class target_simd>
	constexpr static inline bool is_widen_target_v = requires {
		typename target_simd::element_type;
		typename target_simd::vector_t;
		{ target_simd::register_width } -> std::convertible_to<const std::size_t &>;
	};

#pragma region Data Transfer
	/** @brief Loads element data into a SIMD register.
	 *  @param data Source elements matching the full register width.
	 *  @return Register populated with the provided elements.
	 */
	SIMDLIB_FORCE_INLINE static vector_t VECTORCALL load(std::span<const element_t, element_count> data) noexcept
	{
		return impl::load_unaligned(data.data());
	}

	/** @brief Loads a full register from storage aligned to the register byte width. */
	SIMDLIB_FORCE_INLINE static vector_t VECTORCALL load_aligned(std::span<const element_t, element_count> data) noexcept
	{
		SIMDLIB_PRECONDITION(reinterpret_cast<std::uintptr_t>(data.data()) % byte_count == 0, "Aligned SIMD load requires register-width alignment");
		return impl::load(data.data());
	}

	/** @brief Explicit spelling for an unaligned full-register load. */
	SIMDLIB_FORCE_INLINE static vector_t VECTORCALL load_unaligned(std::span<const element_t, element_count> data) noexcept
	{
		return impl::load_unaligned(data.data());
	}

	/** @brief Loads a logical prefix of elements into a SIMD register and zero-fills the remaining lanes.
	 *  @tparam active_count Number of leading elements to load.
	 *  @param data Source span whose first `active_count` values are loaded.
	 *  @return Register containing the requested active values followed by zero-filled inactive lanes.
	 */
	template <std::size_t active_count>
	SIMDLIB_FORCE_INLINE constexpr static vector_t VECTORCALL load_partial(std::span<const element_t> data) noexcept
		requires(active_count <= element_count)
	{
		if (!std::is_constant_evaluated())
		{
			SIMDLIB_PRECONDITION(data.size() >= active_count, "Data span must contain at least the requested active element count");
		}

		if constexpr (active_count == element_count)
		{
			return impl::load_unaligned(data.data());
		}
		else
		{
			return [&data]<std::size_t... Indices>(std::index_sequence<Indices...>) constexpr noexcept -> vector_t
			{ return setr_partial(static_cast<element_t>(data[Indices])...); }(std::make_index_sequence<active_count>{});
		}
	}

	/** @brief Loads element data into a SIMD register without enforcing a fixed extent.
	 *  @param data Source span whose leading elements are read into the register.
	 *  @return Register populated from the provided span.
	 */
	SIMDLIB_FORCE_INLINE static vector_t VECTORCALL load_unsafe(std::span<const element_t> data) noexcept
	{
		return impl::load_unaligned(data.data());
	}

	/** @brief Stores a SIMD register into an element span.
	 *  @param vector Register value to store.
	 *  @param data Destination span that receives all register elements.
	 *  @return None.
	 */
	SIMDLIB_FORCE_INLINE static void VECTORCALL store(vector_t vector, std::span<element_t, element_count> data) noexcept
	{
		impl::store_unaligned(vector, data.data());
	}

	/** @brief Stores a full register to storage aligned to the register byte width. */
	SIMDLIB_FORCE_INLINE static void VECTORCALL store_aligned(vector_t vector, std::span<element_t, element_count> data) noexcept
	{
		SIMDLIB_PRECONDITION(reinterpret_cast<std::uintptr_t>(data.data()) % byte_count == 0, "Aligned SIMD store requires register-width alignment");
		impl::store(vector, data.data());
	}

	/** @brief Explicit spelling for an unaligned full-register store. */
	SIMDLIB_FORCE_INLINE static void VECTORCALL store_unaligned(vector_t vector, std::span<element_t, element_count> data) noexcept
	{
		impl::store_unaligned(vector, data.data());
	}

	/** @brief Stores a SIMD register into a raw byte span.
	 *  @param vector Register value to store.
	 *  @param data Destination byte span with capacity for the full register payload.
	 *  @return None.
	 */
	SIMDLIB_FORCE_INLINE static void VECTORCALL store(vector_t vector, std::span<std::byte> data) noexcept
	{
		SIMDLIB_PRECONDITION(data.size() >= byte_count, "Data byte span must be at least the byte size of the register");
		impl::store_unaligned(vector, data.data());
	}

	/** @brief Constructs a SIMD register from a fixed array.
	 *  @param data Source array containing one full register worth of elements.
	 *  @return Register populated with the provided array contents.
	 */
	SIMDLIB_FORCE_INLINE constexpr static vector_t VECTORCALL construct(const std::array<element_t, element_count> &data) noexcept
	{
		return impl::construct(data);
	}

	/** @brief Converts a SIMD register into a fixed array of elements.
	 *  @param vector Register value to unpack.
	 *  @return Array containing the register elements in lane order.
	 */
	SIMDLIB_FORCE_INLINE constexpr static std::array<element_t, element_count> VECTORCALL to_array(const vector_t vector) noexcept
	{
		std::array<element_t, element_count> result{};
		if (std::is_constant_evaluated())
		{
			for (std::size_t index = 0; index < element_count; ++index)
			{
				result[index] = impl::get_element(vector, static_cast<int>(index));
			}
		}
		else
		{
			impl::store_unaligned(vector, result.data());
		}
		return result;
	}

	/** @brief Finishes integer magnitude by summing the SIMD-produced pairwise squares per 128-bit lane and broadcasting the root.
	 *  @tparam partial_element_t Integer lane type produced by the first pairwise square-and-sum step.
	 *  @param pairSums Register containing `x*x + y*y` style partial sums for each 128-bit lane group.
	 *  @return Register containing the lane-local magnitudes broadcast to every source lane.
	 */
	template <class partial_element_t>
	SIMDLIB_FORCE_INLINE static vector_t VECTORCALL
	FinishIntegerMagnitudeFromPairSums(typename Api<register_width, partial_element_t>::vector_t pairSums) noexcept
	{
		using partial_simd = Api<register_width, partial_element_t>;
		using accumulation_t = std::conditional_t<std::is_signed_v<partial_element_t>, int64_t, uint64_t>;
		constexpr std::size_t LaneGroupCount = register_width / 128;
		constexpr std::size_t SourceLaneWidth = element_count / LaneGroupCount;
		constexpr std::size_t PartialLaneWidth = partial_simd::element_count / LaneGroupCount;

		const auto partialValues = partial_simd::to_array(pairSums);
		std::array<element_t, element_count> output{};
		for (std::size_t groupIndex = 0; groupIndex < LaneGroupCount; ++groupIndex)
		{
			accumulation_t total{};
			const std::size_t partialStart = groupIndex * PartialLaneWidth;
			for (std::size_t partialOffset = 0; partialOffset < PartialLaneWidth; ++partialOffset)
			{
				total += static_cast<accumulation_t>(partialValues[partialStart + partialOffset]);
			}

			const element_t laneMagnitude = static_cast<element_t>(std::round(std::sqrt(static_cast<long double>(total))));
			const std::size_t laneStart = groupIndex * SourceLaneWidth;
			for (std::size_t laneOffset = 0; laneOffset < SourceLaneWidth; ++laneOffset)
			{
				output[laneStart + laneOffset] = laneMagnitude;
			}
		}

		return construct(output);
	}
#pragma endregion

#pragma region Arithmetic Operations

	/** @brief Returns a zero-initialized SIMD register.
	 *  @return Register with every lane initialized to zero.
	 */
	SIMDLIB_FORCE_INLINE constexpr static vector_t VECTORCALL setzero() noexcept
		requires requires { impl::setzero(); }
	{
		return impl::setzero();
	}

	/** @brief Broadcasts one scalar value to every lane in the register.
	 *  @param value Scalar value to broadcast.
	 *  @return Register with every lane initialized to `value`.
	 */
	SIMDLIB_FORCE_INLINE constexpr static vector_t VECTORCALL set1(const element_t value) noexcept
		requires requires(element_t scalar) { impl::set1(scalar); }
	{
		return impl::set1(value);
	}

	/** @brief Constructs a register from lane values in native argument order.
	 *  @tparam Args Argument pack matching the register lane count.
	 *  @param args Lane values in native intrinsic order.
	 *  @return Register containing the provided lane values.
	 */
	template <class... Args>
	SIMDLIB_FORCE_INLINE constexpr static auto VECTORCALL set(Args &&...args) noexcept
		requires requires(Args &&...values) { impl::set(std::forward<Args>(values)...); }
	{
		return impl::set(std::forward<Args>(args)...);
	}

	/** @brief Constructs a register from a partial native-order lane list and zero-fills the remaining lanes.
	 *  @tparam Args Argument pack containing up to one full register worth of lanes.
	 *  @param args Lane values in native intrinsic argument order.
	 *  @return Register containing the provided lanes with any remaining lanes initialized to zero.
	 */
	template <class... Args>
	SIMDLIB_FORCE_INLINE constexpr static auto VECTORCALL set_partial(Args &&...args) noexcept
		requires(sizeof...(Args) <= element_count)
	{
		return []<std::size_t... ZeroIndices>(std::index_sequence<ZeroIndices...>, Args &&...values) constexpr noexcept
			requires requires(Args &&...forwardedValues) { impl::set(std::forward<Args>(forwardedValues)..., ((void)ZeroIndices, element_t{})...); }
		{ return impl::set(std::forward<Args>(values)..., ((void)ZeroIndices, element_t{})...); }(std::make_index_sequence<element_count - sizeof...(Args)>{},
																								  std::forward<Args>(args)...);
	}

	/** @brief Constructs a register from lane values in forward lane order.
	 *  @tparam Args Argument pack matching the register lane count.
	 *  @param args Lane values in logical low-to-high order.
	 *  @return Register containing the provided lane values.
	 */
	template <class... Args>
	SIMDLIB_FORCE_INLINE constexpr static auto VECTORCALL setr(Args &&...args) noexcept
		requires requires(Args &&...values) { impl::setr(std::forward<Args>(values)...); }
	{
		return impl::setr(std::forward<Args>(args)...);
	}

	/** @brief Constructs a register from a partial forward-order lane list and zero-fills the remaining lanes.
	 *  @tparam Args Argument pack containing up to one full register worth of lanes.
	 *  @param args Lane values in low-to-high logical lane order.
	 *  @return Register containing the provided lanes with any remaining lanes initialized to zero.
	 */
	template <class... Args>
	SIMDLIB_FORCE_INLINE constexpr static auto VECTORCALL setr_partial(Args &&...args) noexcept
		requires(sizeof...(Args) <= element_count)
	{
		return []<std::size_t... ZeroIndices>(std::index_sequence<ZeroIndices...>, Args &&...values) constexpr noexcept
			requires requires(Args &&...forwardedValues) { impl::setr(std::forward<Args>(forwardedValues)..., ((void)ZeroIndices, element_t{})...); }
		{ return impl::setr(std::forward<Args>(values)..., ((void)ZeroIndices, element_t{})...); }(std::make_index_sequence<element_count - sizeof...(Args)>{},
																								   std::forward<Args>(args)...);
	}

	/** @brief Computes a fused multiply-add where the implementation supports it, or a multiply followed by add otherwise.
	 *  @param lhs Left-hand multiplicand register.
	 *  @param rhs Right-hand multiplicand register.
	 *  @param addend Register added to the product.
	 *  @return Register containing the multiply-add result.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL multiply_add(const vector_t lhs, const vector_t rhs, const vector_t addend) noexcept
		requires requires(vector_t left, vector_t right, vector_t sum) { impl::multiply_add(left, right, sum); }
	{
		return impl::multiply_add(lhs, rhs, addend);
	}

	/** @brief Widens this SIMD register into the specified destination SIMD shape.
	 *  @tparam target_simd Destination SIMD wrapper type carrying the target register width and element type.
	 *  @param lhs Source register to widen.
	 *  @return Destination register widened according to the source element signedness.
	 */
	template <class target_simd> SIMDLIB_FORCE_INLINE static typename target_simd::vector_t VECTORCALL widen(const vector_t lhs) noexcept
	{
		static_assert(is_widen_target_v<target_simd>,
					  "Api::widen<target_simd> requires a destination SIMD type with element_type, vector_t, and register_width.");
		static_assert(using_int, "Api::widen only supports integral source SIMD specializations.");
		static_assert(std::is_integral_v<typename target_simd::element_type>, "Api::widen only supports integral destination SIMD specializations.");
		static_assert(sizeof(element_t) < sizeof(typename target_simd::element_type),
					  "Api::widen requires the destination element type to be wider than the source element type.");
		static_assert(target_simd::register_width == 128 || target_simd::register_width == 256,
					  "Api::widen currently supports only 128-bit or 256-bit destination SIMD widths.");

		if constexpr (requires(vector_t value) { impl::template widen<target_simd>(value); })
		{
			return impl::template widen<target_simd>(lhs);
		}
		else
		{
			static_assert(
				requires(vector_t value) { impl::template widen<target_simd>(value); },
				"Api::widen does not yet have a backend mapping for this source/destination SIMD pair.");
		}
	}

	/** @brief Computes the remainder of each lhs element divided by the corresponding rhs element.
	 *  @param lhs Dividend register.
	 *  @param rhs Divisor register.
	 *  @return Register containing per-lane remainder results.
	 */
	SIMDLIB_FORCE_INLINE static vector_t VECTORCALL modulus(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::modulus(left, right); }
	{
		return impl::modulus(lhs, rhs);
	}

	/** @brief Negates each element in the register.
	 *  @param lhs Input register.
	 *  @return Register containing the negated element values.
	 */
	SIMDLIB_FORCE_INLINE static vector_t VECTORCALL negate(const vector_t lhs) noexcept
		requires requires(vector_t value) { impl::negate(value); }
	{
		return impl::negate(lhs);
	}

	/** @brief Computes the absolute value of each element in the register.
	 *  @param lhs Input register.
	 *  @return Register containing per-lane absolute values.
	 */
	SIMDLIB_FORCE_INLINE static vector_t VECTORCALL absolute(const vector_t lhs) noexcept
		requires requires(vector_t value) { impl::absolute(value); }
	{
		return impl::absolute(lhs);
	}

	/** @brief Computes the square root of each element in the register.
	 *  @param lhs Input register.
	 *  @return Register containing per-lane square roots.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL sqrt(const vector_t lhs) noexcept
		requires requires(vector_t value) { impl::sqrt(value); }
	{
		return impl::sqrt(lhs);
	}

	/** @brief Computes the vector magnitude per 128-bit lane.
	 *  @param lhs Input register.
	 *  @return Register containing the lane-local magnitudes broadcast within each 128-bit lane.
	 */
	SIMDLIB_FORCE_INLINE static vector_t VECTORCALL magnitude(const vector_t lhs) noexcept
		requires((std::is_floating_point_v<element_t> && requires(vector_t left, vector_t right) {
					 impl::sqrt(left);
					 impl::template dot_product<0x11>(left, right);
				 }) || (using_int && requires(vector_t value) {
					 impl::sqrt(value);
					 impl::multiply_add_adjacent(value, value);
				 }))
	{
		if constexpr (std::is_floating_point_v<element_t>)
		{
			if constexpr (std::same_as<element_t, float>)
			{
				return sqrt(dot_product<0xFF>(lhs, lhs));
			}
			else
			{
				return sqrt(dot_product<0x33>(lhs, lhs));
			}
		}
		else
		{
			if constexpr (sizeof(element_t) == 1)
			{
				using partial_element_t = std::conditional_t<using_unsigned, uint16_t, int16_t>;
				return FinishIntegerMagnitudeFromPairSums<partial_element_t>(multiply_add_adjacent(lhs, lhs));
			}
			else if constexpr (sizeof(element_t) == 2)
			{
				using partial_element_t = std::conditional_t<using_unsigned, uint32_t, int32_t>;
				return FinishIntegerMagnitudeFromPairSums<partial_element_t>(multiply_add_adjacent(lhs, lhs));
			}
			else if constexpr (sizeof(element_t) == 4)
			{
				using partial_element_t = std::conditional_t<using_unsigned, uint64_t, int64_t>;
				return FinishIntegerMagnitudeFromPairSums<partial_element_t>(multiply_add_adjacent(lhs, lhs));
			}
			else
			{
				return FinishIntegerMagnitudeFromPairSums<element_t>(multiply_add_adjacent(lhs, lhs));
			}
		}
	}

	/** @brief Normalizes floating-point lanes using the vector length computed per 128-bit lane.
	 *  @param lhs Input floating-point register.
	 *  @return Register containing the normalized per-lane values.
	 */
	SIMDLIB_FORCE_INLINE static vector_t VECTORCALL normalize(const vector_t lhs) noexcept
		requires(std::is_floating_point_v<element_t> && requires(vector_t left, vector_t right) {
			magnitude(left);
			impl::divide(left, right);
		})
	{
		return divide(lhs, magnitude(lhs));
	}

	/** @brief Computes the average of corresponding lanes where the specialization supports it.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing per-lane averages.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL avg(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::avg(left, right); }
	{
		return impl::avg(lhs, rhs);
	}

	/** @brief Adds adjacent element pairs within each 128-bit lane of two registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing pairwise horizontal sums.
	 */
	SIMDLIB_FORCE_INLINE static vector_t VECTORCALL add_horizontal(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::add_horizontal(left, right); }
	{
		return impl::add_horizontal(lhs, rhs);
	}

	/** @brief Subtracts adjacent element pairs within each 128-bit lane of two registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing pairwise horizontal differences.
	 */
	SIMDLIB_FORCE_INLINE static vector_t VECTORCALL subtract_horizontal(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::subtract_horizontal(left, right); }
	{
		return impl::subtract_horizontal(lhs, rhs);
	}

	/** @brief Multiplies adjacent element pairs and accumulates them into promoted result lanes.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register whose lane type follows the promoted integer mapping rather than `vector_t`.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL multiply_add_adjacent(const vector_t lhs, const vector_t rhs) noexcept
		requires(using_int && requires(vector_t left, vector_t right) { impl::multiply_add_adjacent(left, right); })
	{
		return impl::multiply_add_adjacent(lhs, rhs);
	}

	/** @brief Multiplies raw register bytes as unsigned and signed pairs and accumulates them into signed 16-bit lanes.
	 *  @param lhs Left-hand input register whose bytes are interpreted as unsigned.
	 *  @param rhs Right-hand input register whose bytes are interpreted as signed.
	 *  @return Register containing signed 16-bit accumulation results derived from the raw register bytes.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL multiply_add_unsigned_signed_bytes(const vector_t lhs, const vector_t rhs) noexcept
		requires(using_int && requires(vector_t left, vector_t right) { impl::multiply_add_unsigned_signed_bytes(left, right); })
	{
		return impl::multiply_add_unsigned_signed_bytes(lhs, rhs);
	}

	/** @brief Computes byte-wise absolute differences and accumulates them into 64-bit result lanes.
	 *  @param lhs Left-hand input register interpreted byte-wise.
	 *  @param rhs Right-hand input register interpreted byte-wise.
	 *  @return Register containing 64-bit absolute-difference accumulations derived from the raw register bytes.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL sum_absolute_byte_differences(const vector_t lhs, const vector_t rhs) noexcept
		requires(using_int && requires(vector_t left, vector_t right) { impl::sum_absolute_byte_differences(left, right); })
	{
		return impl::sum_absolute_byte_differences(lhs, rhs);
	}

	/** @brief Computes byte-window absolute-difference sums selected by an immediate control mask.
	 *  @tparam imm8 Immediate control mask selecting the source windows.
	 *  @param lhs Left-hand input register interpreted byte-wise.
	 *  @param rhs Right-hand input register interpreted byte-wise.
	 *  @return Register containing byte-window absolute-difference accumulations derived from the raw register bytes.
	 */
	template <int imm8>
	SIMDLIB_FORCE_INLINE static auto VECTORCALL multi_sum_absolute_byte_differences(const vector_t lhs, const vector_t rhs) noexcept
		requires(using_int && requires(vector_t left, vector_t right) { impl::template multi_sum_absolute_byte_differences<imm8>(left, right); })
	{
		return impl::template multi_sum_absolute_byte_differences<imm8>(lhs, rhs);
	}

	/** @brief Returns the first index of the minimum value in the register.
	 *  @param lhs Input register.
	 *  @return Zero-based index of the first minimum element across the full SIMD register.
	 */
	SIMDLIB_FORCE_INLINE constexpr static std::size_t VECTORCALL min_position(const vector_t lhs) noexcept
		requires(using_int && requires(vector_t value) {
			impl::min_position(value);
			impl::template extract<1>(value);
		})
	{
		if (std::is_constant_evaluated())
		{
			const auto values = to_array(lhs);
			return static_cast<std::size_t>(std::min_element(values.begin(), values.end()) - values.begin());
		}

		return static_cast<std::size_t>(impl::template extract<1>(impl::min_position(lhs)));
	}

	/** @brief Returns the first index of the maximum value in the register.
	 *  @param lhs Input register.
	 *  @return Zero-based index of the first maximum element across the full SIMD register.
	 */
	SIMDLIB_FORCE_INLINE constexpr static std::size_t VECTORCALL max_position(const vector_t lhs) noexcept
		requires(using_int && requires(vector_t value) {
			impl::min_position(value);
			impl::template extract<1>(value);
		})
	{
		if (std::is_constant_evaluated())
		{
			const auto values = to_array(lhs);
			return static_cast<std::size_t>(std::max_element(values.begin(), values.end()) - values.begin());
		}

		if constexpr (using_unsigned)
		{
			return static_cast<std::size_t>(impl::template extract<1>(impl::min_position(TransformForMaxPosition(lhs))));
		}
		else
		{
			using unsigned_simd = SimdLib::Api<register_width, std::make_unsigned_t<element_t>>;
			return unsigned_simd::min_position(TransformForMaxPosition(lhs));
		}
	}

	/** @brief Adds corresponding lanes with saturation where the specialization supports it.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing saturated sums.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL add_saturated(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::add_saturated(left, right); }
	{
		return impl::add_saturated(lhs, rhs);
	}

	/** @brief Subtracts corresponding lanes with saturation where the specialization supports it.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing saturated differences.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL subtract_saturated(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::subtract_saturated(left, right); }
	{
		return impl::subtract_saturated(lhs, rhs);
	}

	/** @brief Adds adjacent pairs with saturation where the specialization supports it.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing saturated horizontal sums.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL hadd_saturated(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::hadd_saturated(left, right); }
	{
		return impl::hadd_saturated(lhs, rhs);
	}

	/** @brief Subtracts adjacent pairs with saturation where the specialization supports it.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing saturated horizontal differences.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL hsubtract_saturated(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::hsubtract_saturated(left, right); }
	{
		return impl::hsubtract_saturated(lhs, rhs);
	}

	/** @brief Alternates subtraction and addition across lanes for floating-point SIMD families.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing alternating subtract/add results.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL add_subtract(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::add_subtract(left, right); }
	{
		return impl::add_subtract(lhs, rhs);
	}

	/** @brief Computes a dot product using a compile-time immediate mask where the specialization supports it.
	 *  @tparam imm8 Immediate mask controlling source participation and destination writeback.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the masked dot-product result.
	 */
	template <int imm8>
	SIMDLIB_FORCE_INLINE static auto VECTORCALL dot_product(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::template dot_product<imm8>(left, right); }
	{
		return impl::template dot_product<imm8>(lhs, rhs);
	}

#pragma endregion

#pragma region Bitwise Operations

	/** @brief Computes a bitwise AND of two registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the bitwise AND result.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL bitwise_and(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::bitwise_and(left, right); }
	{
		return impl::bitwise_and(lhs, rhs);
	}

	/** @brief Computes a bitwise OR of two registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the bitwise OR result.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL bitwise_or(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::bitwise_or(left, right); }
	{
		return impl::bitwise_or(lhs, rhs);
	}

	/** @brief Computes a bitwise XOR of two registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the bitwise XOR result.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL bitwise_xor(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::bitwise_xor(left, right); }
	{
		return impl::bitwise_xor(lhs, rhs);
	}

	/** @brief Computes a bitwise AND-NOT of two registers.
	 *  @param lhs Left-hand input register whose bits are inverted before the AND.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the bitwise AND-NOT result.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL bitwise_andnot(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::bitwise_andnot(left, right); }
	{
		return impl::bitwise_andnot(lhs, rhs);
	}

	/** @brief Computes a bitwise NOT of a register.
	 *  @param lhs Input register.
	 *  @return Register containing the bitwise NOT result.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL bitwise_not(const vector_t lhs) noexcept
		requires requires(vector_t value) { impl::bitwise_not(value); }
	{
		return impl::bitwise_not(lhs);
	}

#pragma endregion

#pragma region Comparison Operations

	/** @brief Returns a mask composed from the most significant bit of each byte in the register.
	 *  @param lhs Input register.
	 *  @return Byte-granular movemask for the register contents.
	 */
	SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL movemask(const vector_t lhs) noexcept
	{
		if (std::is_constant_evaluated())
		{
			const auto lanes = to_array(lhs);
			mask_t result = 0;
			for (std::size_t laneIndex = 0; laneIndex < element_count; ++laneIndex)
			{
				const auto laneBytes = std::bit_cast<std::array<std::uint8_t, sizeof(element_t)>>(lanes[laneIndex]);
				for (std::size_t byteIndex = 0; byteIndex < sizeof(element_t); ++byteIndex)
				{
					const std::size_t maskIndex = laneIndex * sizeof(element_t) + byteIndex;
					result |= static_cast<mask_t>((laneBytes[byteIndex] >> 7) & 1u) << maskIndex;
				}
			}
			return result;
		}
		else
		{
			return impl::movemask(lhs);
		}
	}

	/** @brief Returns a mask composed from the most significant bit of each element in the register.
	 *  @param lhs Input register.
	 *  @return Element-granular movemask for the register contents.
	 */
	SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL movemask_slim(const vector_t lhs) noexcept
	{
		if (std::is_constant_evaluated())
		{
			const auto lanes = to_array(lhs);
			mask_t result = 0;
			for (std::size_t laneIndex = 0; laneIndex < element_count; ++laneIndex)
			{
				const auto laneBytes = std::bit_cast<std::array<std::uint8_t, sizeof(element_t)>>(lanes[laneIndex]);
				result |= static_cast<mask_t>((laneBytes.back() >> 7) & 1u) << laneIndex;
			}
			return result;
		}
		else
		{
			return impl::movemask_slim(lhs);
		}
	}

	/** @brief Computes an equality comparison mask for two integer registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with bits set where corresponding elements are equal.
	 */
	SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL cmp_eq(const int_vector_t lhs, const int_vector_t rhs) noexcept
		requires(using_int)
	{
		if (std::is_constant_evaluated())
		{
			const auto lhsValues = to_array(lhs);
			const auto rhsValues = to_array(rhs);
			mask_t result = 0;
			constexpr mask_t laneMask = static_cast<mask_t>((mask_t{1} << sizeof(element_t)) - 1);
			for (std::size_t index = 0; index < element_count; ++index)
				if (lhsValues[index] == rhsValues[index])
					result |= laneMask << (index * sizeof(element_t));
			return result;
		}
		else
		{
			return impl::movemask(impl::cmpeq(lhs, rhs));
		}
	}

	/** @brief Computes a byte-granular equality comparison mask for two registers of this SIMD shape.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with bits set where the underlying compare produced all-one bytes.
	 */
	SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL cmp_eq_mask(const vector_t lhs, const vector_t rhs) noexcept
	{
		if (std::is_constant_evaluated())
		{
			const auto lhsValues = to_array(lhs);
			const auto rhsValues = to_array(rhs);
			mask_t result = 0;
			constexpr mask_t laneMask = static_cast<mask_t>((mask_t{1} << sizeof(element_t)) - 1);
			for (std::size_t index = 0; index < element_count; ++index)
				if (lhsValues[index] == rhsValues[index])
					result |= laneMask << (index * sizeof(element_t));
			return result;
		}
		else
		{
			return impl::movemask(impl::cmpeq(lhs, rhs));
		}
	}

	/** @brief Computes a greater-than comparison mask for two integer registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with bits set where lhs elements are greater than rhs elements.
	 */
	SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL cmp_gt(const int_vector_t lhs, const int_vector_t rhs) noexcept
		requires(using_int)
	{
		if (std::is_constant_evaluated())
		{
			return movemask(compare_each_element<Detail::comparison_operation::greater>(lhs, rhs));
		}
		else
		{
			return impl::movemask(impl::cmpgt(lhs, rhs));
		}
	}

	/** @brief Computes a greater-than-or-equal comparison mask for two integer registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with bits set where lhs elements are greater than or equal to rhs elements.
	 */
	SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL cmp_ge(const int_vector_t lhs, const int_vector_t rhs) noexcept
		requires(using_int)
	{
		return cmp_eq(lhs, rhs) | cmp_gt(lhs, rhs);
	}

	/** @brief Computes a less-than comparison mask for two integer registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with bits set where lhs elements are less than rhs elements.
	 */
	SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL cmp_lt(const int_vector_t lhs, const int_vector_t rhs) noexcept
		requires(using_int)
	{
		if (std::is_constant_evaluated())
		{
			return movemask(compare_each_element<Detail::comparison_operation::less>(lhs, rhs));
		}
		else
		{
			return impl::movemask(impl::cmpgt(rhs, lhs));
		}
	}

	/** @brief Computes a less-than-or-equal comparison mask for two integer registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with bits set where lhs elements are less than or equal to rhs elements.
	 */
	SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL cmp_le(const int_vector_t lhs, const int_vector_t rhs) noexcept
		requires(using_int)
	{
		return cmp_eq(lhs, rhs) | cmp_lt(lhs, rhs);
	}

#pragma endregion

#pragma region Rearrangement Operations

	/** @brief Expands a register into a wider-lane register where the specialization supports it.
	 *  @note `expand` remains the legacy low-level fixed-shape widening alias. Prefer `widen<target_simd>(...)` from the curated `SimdLib::Api` surface for
	 * new code.
	 *  @param lhs Source register.
	 *  @param rhs Auxiliary source register when required by the implementation.
	 *  @return Expanded register value.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL expand(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::expand(left, right); }
	{
		return impl::expand(lhs, rhs);
	}

	/** @brief Compresses two registers into a narrower-lane register where the specialization supports it.
	 *  @param lhs Left-hand source register.
	 *  @param rhs Right-hand source register.
	 *  @return Compressed register value.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL compress(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::compress(left, right); }
	{
		return impl::compress(lhs, rhs);
	}

	/** @brief Extracts a lane or subvalue from a register.
	 *  @param lhs Source register.
	 *  @param rhs Extract selector.
	 *  @return Extracted value as defined by the specialization.
	 */
	template <int index>
	SIMDLIB_FORCE_INLINE static auto VECTORCALL extract(const vector_t lhs) noexcept
		requires requires(vector_t value) { impl::template extract<index>(value); }
	{
		return impl::template extract<index>(lhs);
	}

	/** @brief Extracts a lane or subvalue from a register using a runtime selector.
	 *  @param lhs Source register.
	 *  @param rhs Extract selector.
	 *  @return Extracted value as defined by the specialization.
	 */
	template <class selector_t>
	SIMDLIB_FORCE_INLINE static auto VECTORCALL extract(const vector_t lhs, selector_t rhs) noexcept
		requires requires(vector_t left, selector_t selector) { impl::extract(left, selector); }
	{
		return impl::extract(lhs, rhs);
	}

	/** @brief Returns the low 128-bit half of a 256-bit register when the specialization supports it.
	 *  @param lhs Source register.
	 *  @return Register containing the low 128-bit half in the corresponding 128-bit SIMD family.
	 */
	SIMDLIB_FORCE_INLINE static typename SimdLib::Detail::SimdMappings<128, element_t>::vector_t VECTORCALL lower_half(const vector_t lhs) noexcept
		requires(register_width == 256 && requires(vector_t value) { impl::lower_half(value); })
	{
		return impl::lower_half(lhs);
	}

	/** @brief Inserts a lane or subvalue into a register.
	 *  @tparam Args Argument pack matching the implementation-specific insert signature.
	 *  @param args Arguments forwarded to the specialization insert operation.
	 *  @return Register containing the inserted value.
	 */
	template <class... Args>
	SIMDLIB_FORCE_INLINE static auto VECTORCALL insert(Args &&...args) noexcept
		requires requires(Args &&...values) { impl::insert(std::forward<Args>(values)...); }
	{
		return impl::insert(std::forward<Args>(args)...);
	}

	/** @brief Unpacks the low lanes of two registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the unpacked low-lane interleave.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL unpack_lo(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::unpack_lo(left, right); }
	{
		return impl::unpack_lo(lhs, rhs);
	}

	/** @brief Unpacks the high lanes of two registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the unpacked high-lane interleave.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL unpack_hi(const vector_t lhs, const vector_t rhs) noexcept
		requires requires(vector_t left, vector_t right) { impl::unpack_hi(left, right); }
	{
		return impl::unpack_hi(lhs, rhs);
	}

	/** @brief Shuffles register contents according to the implementation-specific control form.
	 *  @tparam Args Argument pack matching the specialization shuffle signature.
	 *  @param args Arguments forwarded to the specialization shuffle operation.
	 *  @return Register containing the shuffled result.
	 */
	template <std::size_t... indices>
	SIMDLIB_FORCE_INLINE static auto VECTORCALL shuffle(const int_vector_t lhs) noexcept
		requires requires(int_vector_t value) { impl::template shuffle<indices...>(value); }
	{
		return impl::template shuffle<indices...>(lhs);
	}

	/** @brief Shuffles register contents according to the implementation-specific control form.
	 *  @tparam Args Argument pack matching the specialization shuffle signature.
	 *  @param args Arguments forwarded to the specialization shuffle operation.
	 *  @return Register containing the shuffled result.
	 */
	template <class... Args>
	SIMDLIB_FORCE_INLINE static auto VECTORCALL shuffle(Args &&...args) noexcept
		requires requires(Args &&...values) { impl::shuffle(std::forward<Args>(values)...); }
	{
		return impl::shuffle(std::forward<Args>(args)...);
	}

	/** @brief Shuffles the low half of a register where the specialization supports it.
	 *  @tparam Args Argument pack matching the specialization shuffle-low signature.
	 *  @param args Arguments forwarded to the specialization shuffle-low operation.
	 *  @return Register containing the shuffled low-half result.
	 */
	template <class... Args>
	SIMDLIB_FORCE_INLINE static auto VECTORCALL shuffle_lo(Args &&...args) noexcept
		requires requires(Args &&...values) { impl::shuffle_lo(std::forward<Args>(values)...); }
	{
		return impl::shuffle_lo(std::forward<Args>(args)...);
	}

	/** @brief Shuffles the high half of a register where the specialization supports it.
	 *  @tparam Args Argument pack matching the specialization shuffle-high signature.
	 *  @param args Arguments forwarded to the specialization shuffle-high operation.
	 *  @return Register containing the shuffled high-half result.
	 */
	template <class... Args>
	SIMDLIB_FORCE_INLINE static auto VECTORCALL shuffle_hi(Args &&...args) noexcept
		requires requires(Args &&...values) { impl::shuffle_hi(std::forward<Args>(values)...); }
	{
		return impl::shuffle_hi(std::forward<Args>(args)...);
	}

	/** @brief Blends two registers according to the implementation-specific control form.
	 *  @tparam Args Argument pack matching the specialization blend signature.
	 *  @param args Arguments forwarded to the specialization blend operation.
	 *  @return Register containing the blended result.
	 */
	template <class... Args>
	SIMDLIB_FORCE_INLINE static auto VECTORCALL blend(Args &&...args) noexcept
		requires requires(Args &&...values) { impl::blend(std::forward<Args>(values)...); }
	{
		return impl::blend(std::forward<Args>(args)...);
	}

#pragma endregion

#pragma region Shifting Operations

	/** @brief Shifts each integer lane left by the specified amount.
	 *  @param lhs Input integer register.
	 *  @param shift Shift count applied to each lane.
	 *  @return Register containing per-lane left-shifted values.
	 */
	SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL shift_left(const int_vector_t lhs, int shift) noexcept
		requires(using_int)
	{
		if (std::is_constant_evaluated())
		{
			std::array<element_t, element_count> results{};
			for (std::size_t index = 0; index < element_count; ++index)
			{
				results[index] = static_cast<element_t>(impl::get_element(lhs, static_cast<int>(index)) << shift);
			}
			return impl::construct(results);
		}

		return impl::shift_left(lhs, shift);
	}

	/** @brief Shifts each integer lane right by the specified amount.
	 *  @param lhs Input integer register.
	 *  @param shift Shift count applied to each lane.
	 *  @return Register containing per-lane right-shifted values.
	 */
	SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL shift_right(const int_vector_t lhs, int shift) noexcept
		requires(using_int)
	{
		if (std::is_constant_evaluated())
		{
			std::array<element_t, element_count> results{};
			for (std::size_t index = 0; index < element_count; ++index)
			{
				results[index] = static_cast<element_t>(impl::get_element(lhs, static_cast<int>(index)) >> shift);
			}
			return impl::construct(results);
		}

		return impl::shift_right(lhs, shift);
	}

	/** @brief Arithmetic-shifts each integer lane right by the specified amount.
	 *  @param lhs Input integer register.
	 *  @param shift Shift count applied to each lane.
	 *  @return Register containing per-lane arithmetic right-shifted values.
	 */
	SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL shift_right_arithmetic(const int_vector_t lhs, int shift) noexcept
		requires(using_int)
	{
		if (std::is_constant_evaluated())
		{
			std::array<element_t, element_count> results{};
			for (std::size_t index = 0; index < element_count; ++index)
			{
				results[index] = static_cast<element_t>(impl::get_element(lhs, index) >> shift);
			}
			return impl::construct(results);
		}

		return impl::shift_right_arithmetic(lhs, shift);
	}

	/** @brief Shifts the complete 128-bit register left, carrying bits across lane boundaries.
	 * Unlike `shift_left`, this treats the register as one
	 * unsigned 128-bit bit string.
	 * A zero or negative runtime count returns the input; counts of 128 or more return zero.
	 */
	SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL bit_shift_left(const int_vector_t lhs, const int shift) noexcept
		requires(using_int && register_width == 128)
	{
		return impl::bit_shift_left(lhs, shift);
	}

	/** @brief Compile-time complete-register left shift. Counts of 128 or more return zero. */
	template <int shift>
	SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL bit_shift_left(const int_vector_t lhs) noexcept
		requires(using_int && register_width == 128)
	{
		static_assert(shift >= 0, "Whole-register shifts require a non-negative count.");
		return impl::template bit_shift_left<shift>(lhs);
	}

	/** @brief Shifts the complete 128-bit register right, carrying bits across lane boundaries.
	 * Unlike `shift_right`, this treats the register as one
	 * unsigned 128-bit bit string.
	 * A zero or negative runtime count returns the input; counts of 128 or more return zero.
	 */
	SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL bit_shift_right(const int_vector_t lhs, const int shift) noexcept
		requires(using_int && register_width == 128)
	{
		return impl::bit_shift_right(lhs, shift);
	}

	/** @brief Compile-time complete-register right shift. Counts of 128 or more return zero. */
	template <int shift>
	SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL bit_shift_right(const int_vector_t lhs) noexcept
		requires(using_int && register_width == 128)
	{
		static_assert(shift >= 0, "Whole-register shifts require a non-negative count.");
		return impl::template bit_shift_right<shift>(lhs);
	}

#pragma endregion

#pragma region Conversion Operations

	/** @brief Converts 32-bit integer lanes into floating-point lanes.
	 *  @param vector Input integer register.
	 *  @return Floating-point register containing the converted lane values.
	 */
	SIMDLIB_FORCE_INLINE static float_vector_t VECTORCALL convert_to_float(int_vector_t vector) noexcept
		requires(element_width == 32)
	{
		static_assert(element_width == 32, "Only 32 bit integers can be converted to floats");
		if constexpr (register_width == 128)
		{
			return _mm_cvtepi32_ps(vector);
		}
		else if constexpr (register_width == 256)
		{
			return _mm256_cvtepi32_ps(vector);
		}
	}

	/** @brief Converts 32-bit floating-point lanes into integer lanes.
	 *  @param vector Input floating-point register.
	 *  @return Integer register containing the converted lane values.
	 */
	SIMDLIB_FORCE_INLINE static int_vector_t VECTORCALL convert_to_int(float_vector_t vector) noexcept
		requires(element_width == 32)
	{
		static_assert(element_width == 32, "Only 32 bit floats can be converted to integers");
		if constexpr (register_width == 128)
		{
			return _mm_cvtps_epi32(vector);
		}
		else if constexpr (register_width == 256)
		{
			return _mm256_cvtps_epi32(vector);
		}
	}

	/** @brief Converts between 32-bit integer and floating-point register representations.
	 *  @param vector Input register.
	 *  @return Register converted to the complementary 32-bit scalar representation.
	 */
	SIMDLIB_FORCE_INLINE static auto VECTORCALL convert(vector_t vector) noexcept
		requires(element_width == 32)
	{
		if constexpr (std::is_floating_point_v<element_t>)
			return convert_to_int(vector);
		else
			return convert_to_float(vector);
	}

#pragma endregion

#pragma region Transform

	/** @brief Applies a SIMD transform whose fixed-width lane results are packed contiguously into integer storage.
	 *  @tparam result_bit_width Number of logical result bits produced per source element.
	 *  @tparam count Number of source elements.
	 *  @tparam Func Callable that accepts `vector_t` and returns an unsigned integer containing packed lane results, with lane zero in the least-significant bits.
	 *  @param read Source elements to transform.
	 *  @param write Destination storage for the packed result bit stream.
	 *  @param func SIMD transformation that returns one packed result for each loaded register.
	 *  @return None.
	 */
	template <std::size_t result_bit_width, std::size_t count, std::invocable<vector_t> Func>
	SIMDLIB_FORCE_INLINE constexpr static void transform_pack(
		std::span<const element_t, count> read,
		std::span<packed_element_t<result_bit_width>, packed_element_count<result_bit_width, count>> write,
		Func &&func) noexcept
		requires(result_bit_width > 0 && result_bit_width <= 64)
	{
		using result_t = std::remove_cvref_t<std::invoke_result_t<Func, vector_t>>;
		using write_t = packed_element_t<result_bit_width>;
		// uintptr_t is the standard unsigned type that most closely represents the target's native integer register width.
		// Accumulating into it lets us write whole machine words instead of updating individual destination bytes.
		using native_word_t = std::uintptr_t;
		static_assert(std::unsigned_integral<result_t> && !std::same_as<result_t, bool>,
			"Packed SIMD transforms must return an unsigned integer");
		static_assert(element_count * result_bit_width <= 64,
			"A packed SIMD register result cannot exceed 64 bits");
		static_assert(element_count * result_bit_width <= std::numeric_limits<result_t>::digits,
			"The packed transform result type must contain every result bit for one SIMD register");
		// Callback results place the first SIMD lane in the least-significant bits. Copying the accumulator directly to
		// sequential storage preserves that lane order only when the least-significant byte is stored first.
		static_assert(std::endian::native == std::endian::little,
			"Packed SIMD transforms require little-endian integer storage");

		constexpr std::size_t native_word_width = std::numeric_limits<native_word_t>::digits;
		constexpr std::size_t total_result_bit_count = count * result_bit_width;
		constexpr std::size_t flushed_native_word_count = total_result_bit_count / native_word_width;
		// The destination rounds up to a complete write_t. After flushing every full native word, the final store may
		// therefore include both pending result bits and zero-valued padding, but can never exceed one native word.
		constexpr std::size_t remaining_storage_byte_count =
			packed_element_count<result_bit_width, count> * sizeof(write_t) - flushed_native_word_count * sizeof(native_word_t);
		static_assert(remaining_storage_byte_count <= sizeof(native_word_t));
		auto write_bytes = std::as_writable_bytes(write);
		// pending holds the next unwritten output bits in its least-significant positions. Staging them here avoids both
		// clearing the destination first and read-modify-write operations on partial destination elements.
		native_word_t pending = 0;
		std::size_t pending_bit_count = 0;
		std::size_t write_byte_offset = 0;

		// Append one SIMD register's packed result to the logical output stream. The loop normally executes once and only
		// needs another iteration when the result crosses a native-word boundary.
		const auto append = [&](const result_t packed_result, std::size_t result_bit_count) constexpr noexcept
		{
			std::uint64_t remaining = static_cast<std::uint64_t>(packed_result);
			while (result_bit_count != 0)
			{
				const std::size_t available_bit_count = native_word_width - pending_bit_count;
				const std::size_t consumed_bit_count = std::min(result_bit_count, available_bit_count);
				const std::uint64_t consumed_mask = consumed_bit_count == 64
					? std::numeric_limits<std::uint64_t>::max()
					: (std::uint64_t{1} << consumed_bit_count) - 1;

				pending |= static_cast<native_word_t>((remaining & consumed_mask) << pending_bit_count);
				remaining = consumed_bit_count == 64 ? 0 : remaining >> consumed_bit_count;
				pending_bit_count += consumed_bit_count;
				result_bit_count -= consumed_bit_count;

				// memcpy permits a native-width store without imposing alignment or aliasing requirements on write_t.
				if (pending_bit_count == native_word_width)
				{
					std::memcpy(write_bytes.data() + write_byte_offset, &pending, sizeof(pending));
					write_byte_offset += sizeof(pending);
					pending = 0;
					pending_bit_count = 0;
				}
			}
		};

		constexpr std::size_t full_batch_count = count / element_count;
		for (std::size_t batch_index = 0; batch_index < full_batch_count; ++batch_index)
		{
			const std::size_t read_index = batch_index * element_count;
			const vector_t value = load_unsafe(read.subspan(read_index, element_count));
			append(std::invoke(func, value), element_count * result_bit_width);
		}

		constexpr std::size_t tail_count = count % element_count;
		if constexpr (tail_count != 0)
		{
			constexpr std::size_t read_index = full_batch_count * element_count;
			// SIMD loads require a complete register. Zero-fill its inactive lanes, then append only the valid lanes' result
			// bits so callback output for the padded lanes cannot leak into the packed destination.
			std::array<element_t, element_count> staged{};
			std::copy_n(read.data() + read_index, tail_count, staged.data());
			const vector_t value = load(std::span<const element_t, element_count>(staged));
			append(std::invoke(func, value), tail_count * result_bit_width);
		}

		if constexpr (remaining_storage_byte_count != 0)
		{
			// pending began as zero, so unused high bits in the final write_t are deterministically cleared without a separate
			// destination-initialization pass.
			std::memcpy(write_bytes.data() + write_byte_offset, &pending, remaining_storage_byte_count);
		}
	}

	/** @brief Applies a unary SIMD transform to an element span in place.
	 *  @tparam Func Callable that accepts and returns `vector_t`.
	 *  @param data Span transformed in place.
	 *  @param func Unary SIMD transform to apply.
	 *  @return None.
	 */
	template <std::invocable<vector_t> Func> static inline void transform(std::span<element_t> data, Func &&func) noexcept
	{
		const auto Length = data.size();
		for (std::size_t i = 0; i < Length / element_count; ++i)
		{
			const auto index = i * element_count;
			auto buf = data.subspan(index, element_count);
			const vector_t vec = load_unsafe(buf);
			store(std::invoke(func, vec), std::as_writable_bytes(buf));
		}
		// handle any remaining elements
		if (Length % element_count > 0)
		{
			const auto index = Length - (Length % element_count);
			const auto remainder = Length % element_count;
			std::array<element_t, element_count> staged{};
			std::memcpy(staged.data(), data.data() + index, remainder * sizeof(element_t));
			const auto vec = load(std::span<const element_t, element_count>(staged));
			const vector_t result = std::invoke(func, vec);
			std::memcpy(data.data() + index, &result, remainder * sizeof(element_t));
		}
	}

	/** @brief Applies a unary SIMD transform to a read span and writes the results to a separate output span.
	 *  @tparam Func Callable that accepts and returns `vector_t`.
	 *  @param lhs Source element span.
	 *  @param write Destination element span.
	 *  @param func Unary SIMD transform to apply.
	 *  @return None.
	 */
	template <std::invocable<vector_t> Func> static inline void transform(std::span<const element_t> lhs, std::span<element_t> write, Func &&func) noexcept
	{
		static_assert(std::is_invocable_r_v<vector_t, Func, vector_t>, "Function must return a value of vector_t");
		const auto Length = lhs.size();
		for (std::size_t i = 0; i < Length / element_count; ++i)
		{
			const auto index = i * element_count;
			const vector_t lhsVector = load_unsafe(lhs.subspan(index, element_count));
			store(std::invoke(func, lhsVector), std::as_writable_bytes(write.subspan(index, element_count)));
		}
		// handle any remaining elements
		if (Length % element_count > 0)
		{
			const auto index = Length - (Length % element_count);
			const auto remainder = Length % element_count;
			std::array<element_t, element_count> staged{};
			std::memcpy(staged.data(), lhs.data() + index, remainder * sizeof(element_t));
			const vector_t lhsVector = load(std::span<const element_t, element_count>(staged));
			const vector_t result = std::invoke(func, lhsVector);
			std::memcpy(write.data() + index, &result, remainder * sizeof(element_t));
		}
	}

	/** @brief Applies a binary SIMD transform to two source spans and writes the results to a destination span.
	 *  @tparam Func Callable that accepts two `vector_t` values and returns `vector_t`.
	 *  @param lhs Left-hand source span.
	 *  @param rhs Right-hand source span.
	 *  @param write Destination element span.
	 *  @param func Binary SIMD transform to apply.
	 *  @return None.
	 */
	template <std::invocable<vector_t, vector_t> Func>
	static inline void transform(std::span<const element_t> lhs, std::span<const element_t> rhs, std::span<element_t> write, Func &&func) noexcept
	{
		static_assert(std::is_invocable_r_v<vector_t, Func, vector_t, vector_t>, "Function must return an vector_t");
		const auto Length = lhs.size();
		for (std::size_t i = 0; i < Length / element_count; ++i)
		{
			const auto index = i * element_count;
			const vector_t lhsVector = load_unsafe(lhs.subspan(index, element_count));
			const vector_t rhsVector = load_unsafe(rhs.subspan(index, element_count));
			store(std::invoke(func, lhsVector, rhsVector), std::as_writable_bytes(write.subspan(index, element_count)));
		}
		// handle any remaining elements
		if (Length % element_count > 0)
		{
			const auto index = Length - (Length % element_count);
			const auto remainder = Length % element_count;
			std::array<element_t, element_count> stagedLhs{};
			std::array<element_t, element_count> stagedRhs{};
			std::memcpy(stagedLhs.data(), lhs.data() + index, remainder * sizeof(element_t));
			std::memcpy(stagedRhs.data(), rhs.data() + index, remainder * sizeof(element_t));
			const vector_t lhsVector = load(std::span<const element_t, element_count>(stagedLhs));
			const vector_t rhsVector = load(std::span<const element_t, element_count>(stagedRhs));
			const vector_t result = std::invoke(func, lhsVector, rhsVector);
			std::memcpy(write.data() + index, &result, remainder * sizeof(element_t));
		}
	}

#pragma endregion

#pragma region Internal
  protected:
	/** @brief Re-encodes integer lanes so a minimum-position backend yields the first maximum index.
	 *  @param lhs Input integer register.
	 *  @return Transformed register whose first minimum corresponds to the original first maximum.
	 */
	SIMDLIB_FORCE_INLINE static vector_t VECTORCALL TransformForMaxPosition(const vector_t lhs) noexcept
	{
		if constexpr (using_unsigned)
		{
			return impl::bitwise_not(lhs);
		}
		else
		{
			const vector_t signMask = impl::set1(std::numeric_limits<element_t>::min());
			return impl::bitwise_not(impl::bitwise_xor(lhs, signMask));
		}
	}

	/** @brief Compares each integer lane using the requested ordering and emits an all-bits-set or zero lane result.
	 *  @tparam op Comparison ordering to evaluate per lane.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Integer register containing comparison-mask lanes.
	 */
	template <Detail::comparison_operation op> constexpr static int_vector_t compare_each_element(const int_vector_t lhs, const int_vector_t rhs) noexcept
	{
		const auto lhsValues = to_array(lhs);
		const auto rhsValues = to_array(rhs);
		std::array<element_t, element_count> results{};
		std::transform(lhsValues.begin(), lhsValues.end(), rhsValues.begin(), results.begin(),
					   [](const element_t lhsValue, const element_t rhsValue) constexpr noexcept
					   {
						   bool comparison = false;
						   if constexpr (op == Detail::comparison_operation::less)
							   comparison = lhsValue < rhsValue;
						   else if constexpr (op == Detail::comparison_operation::greater)
							   comparison = lhsValue > rhsValue;
						   else if constexpr (op == Detail::comparison_operation::equivalent)
							   comparison = lhsValue == rhsValue;
						   else if constexpr (op == Detail::comparison_operation::unordered)
							   comparison = false;

						   return static_cast<element_t>(comparison ? -1 : 0);
					   });
		return impl::construct(results);
	}
#pragma endregion
};

} // namespace SimdLib
