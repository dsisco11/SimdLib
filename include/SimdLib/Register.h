#pragma once

#include <SimdLib/Config.h>

#if !SIMDLIB_REGISTER_INTERFACE_AVAILABLE && !SIMDLIB_REQUIRE_REGISTER_INTERFACE
#error "SIMDLIB_REGISTER_HEADER_REQUIRES_CXX23: <SimdLib/Register.h> requires C++23 explicit object parameter support"
#endif

#include <SimdLib/IRegister.h>
#include <SimdLib/RegisterFwd.h>
#include <SimdLib/RegisterMask.h>

#include <array>
#include <concepts>
#include <cstddef>
#include <span>
#include <utility>

namespace SimdLib
{

/**
 * @brief Owns one complete SIMD register whose lanes are all active.
 * @tparam element_t Scalar interpretation of each register lane.
 * @tparam bits Width of the native register in bits.
 */
template <class element_t, std::size_t bits>
	requires RegisterAvailable<element_t, bits>
class Register final
{
  public:
	using element_type = element_t;
	using api_type = Api<bits, element_type>;
	using native_type = typename api_type::vector_t;
	using mask_type = RegisterMask<element_type, bits>;

	constexpr static inline std::size_t register_width = bits;
	constexpr static inline std::size_t byte_count = api_type::byte_count;
	constexpr static inline std::size_t lane_count = api_type::element_count;

	/** @brief Owns the complete native register value represented by this aggregate. */
	native_type native = api_type::setzero();

	/**
	 * @brief Returns a register with every active lane set to zero.
	 * @return Fully initialized zero register.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static Register zero() noexcept
	{
		return Register{api_type::setzero()};
	}

	/**
	 * @brief Broadcasts one scalar value to every active lane.
	 * @param value Scalar value to broadcast.
	 * @return Register containing `value` in every lane.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static Register broadcast(element_type value) noexcept
	{
		return Register{api_type::set1(value)};
	}

	/**
	 * @brief Constructs a register from exactly one complete logical lane list.
	 * @tparam lane_types Scalar argument types convertible to `element_type`.
	 * @param lanes Values in low-to-high logical lane order.
	 * @return Register containing all supplied lane values.
	 */
	template <std::convertible_to<element_type>... lane_types>
		requires(sizeof...(lane_types) == lane_count)
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static Register from_lanes(lane_types &&...lanes) noexcept
	{
		return Register{api_type::setr(static_cast<element_type>(std::forward<lane_types>(lanes))...)};
	}

	/**
	 * @brief Constructs a register from one complete fixed-size lane array.
	 * @param source Source containing every active lane in logical order.
	 * @return Register containing all source lane values.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr static Register from_array(
		const std::array<element_type, lane_count> &source) noexcept
	{
		return Register{api_type::construct(source)};
	}

	/**
	 * @brief Loads a complete register from potentially unaligned storage.
	 * @param source Source containing exactly one register of elements.
	 * @return Register loaded from `source`.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY static Register load(std::span<const element_type, lane_count> source) noexcept
	{
		return Register{api_type::load(source)};
	}

	/**
	 * @brief Loads a complete register from register-aligned storage.
	 * @param source Aligned source containing exactly one register of elements.
	 * @return Register loaded from `source`.
	 * @pre `source.data()` is aligned to `byte_count` bytes.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY static Register load_aligned(
		std::span<const element_type, lane_count> source) noexcept
	{
		return Register{api_type::load_aligned(source)};
	}

	/**
	 * @brief Loads one complete register bit pattern from raw bytes.
	 * @param source Source containing exactly one register of bytes.
	 * @return Register containing the source bit pattern.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY static Register load_bytes(std::span<const std::byte, byte_count> source) noexcept
	{
		return Register{api_type::load(source)};
	}

	/**
	 * @brief Stores every active lane to potentially unaligned storage.
	 * @param value Register to store.
	 * @param destination Destination for exactly one register of elements.
	 */
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE void VECTORCALL store(this Register value, std::span<element_type, lane_count> destination) noexcept
	{
		api_type::store(value.native, destination);
	}

	/**
	 * @brief Stores every active lane to register-aligned storage.
	 * @param value Register to store.
	 * @param destination Aligned destination for one complete register.
	 * @pre `destination.data()` is aligned to `byte_count` bytes.
	 */
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE void VECTORCALL store_aligned(this Register value, std::span<element_type, lane_count> destination) noexcept
	{
		api_type::store_aligned(value.native, destination);
	}

	/**
	 * @brief Stores the complete register bit pattern to raw bytes.
	 * @param value Register to store.
	 * @param destination Destination containing exactly one register of bytes.
	 */
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE void VECTORCALL store_bytes(this Register value, std::span<std::byte, byte_count> destination) noexcept
	{
		api_type::store(value.native, destination);
	}

	/**
	 * @brief Copies every active lane into a fixed-size array.
	 * @param value Register to copy.
	 * @return Array containing all lanes in low-to-high logical order.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr std::array<element_type, lane_count> VECTORCALL to_array(this Register value) noexcept
	{
		return api_type::to_array(value.native);
	}

	/**
	 * @brief Returns one compile-time-selected lane.
	 * @tparam index Logical lane index.
	 * @param value Register containing the selected lane.
	 * @return Copy of the selected lane.
	 */
	template <std::size_t index>
		requires(index < lane_count)
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr element_type VECTORCALL lane(this Register value) noexcept
	{
		if consteval
		{
			return lane_constexpr<index>(value);
		}
		else
		{
			return api_type::template extract<static_cast<int>(index)>(value.native);
		}
	}

	/**
	 * @brief Returns a copy with one compile-time-selected lane replaced.
	 * @tparam index Logical lane index.
	 * @param value Register containing the lanes to copy.
	 * @param replacement Replacement value for the selected lane.
	 * @return Register with lane `index` replaced.
	 */
	template <std::size_t index>
		requires(index < lane_count)
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL with_lane(this Register value,
																													 element_type replacement) noexcept
	{
		value.native = api_type::template insert<index>(value.native, replacement);
		return value;
	}

#pragma region Arithmetic Operations

	/** @brief Adds corresponding lanes. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL operator+(this Register lhs, Register rhs) noexcept
		requires IApi::Add<api_type>
	{
		return Register{api_type::add(lhs.native, rhs.native)};
	}

	/** @brief Subtracts corresponding lanes. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL operator-(this Register lhs, Register rhs) noexcept
		requires IApi::Subtract<api_type>
	{
		return Register{api_type::subtract(lhs.native, rhs.native)};
	}

	/** @brief Multiplies corresponding lanes. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL operator*(this Register lhs, Register rhs) noexcept
		requires IApi::Multiply<api_type>
	{
		return Register{api_type::multiply(lhs.native, rhs.native)};
	}

	/**
	 * @brief Divides corresponding lanes.
	 * @pre Every divisor lane is nonzero and signed minimum is not divided by negative one.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL operator/(this Register lhs, Register rhs) noexcept
		requires IApi::Divide<api_type>
	{
		return Register{api_type::divide(lhs.native, rhs.native)};
	}

	/**
	 * @brief Computes corresponding-lane remainders.
	 * @pre Every divisor lane is nonzero and signed minimum is not divided by negative one.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE Register VECTORCALL operator%(this Register lhs, Register rhs) noexcept
		requires IApi::Modulus<api_type>
	{
		return Register{api_type::modulus(lhs.native, rhs.native)};
	}

	/** @brief Negates every lane with the selected backend's edge behavior. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL operator-(this Register value) noexcept
		requires IApi::Negate<api_type>
	{
		return Register{api_type::negate(value.native)};
	}

	/*
	 * Disabled compound assignment operators: their convenience does not justify the mutable-reference API surface,
	 * and MSVC 19.44 emits a redundant 32-byte stack-alignment frame for 256-bit wrapper mutation through references.
	 * Prefer `lhs = lhs + rhs`, `lhs = lhs - rhs`, `lhs = lhs * rhs`, `lhs = lhs / rhs`, or `lhs = lhs % rhs`.
	 *
	/// @brief Adds another register into this register.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE Register &VECTORCALL operator+=(
		this Register &lhs,
		Register rhs) noexcept
		requires IApi::Add<api_type>
	{
		lhs.native = api_type::add(lhs.native, rhs.native);
		return lhs;
	}

	/// @brief Subtracts another register from this register.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE Register &VECTORCALL operator-=(
		this Register &lhs,
		Register rhs) noexcept
		requires IApi::Subtract<api_type>
	{
		lhs.native = api_type::subtract(lhs.native, rhs.native);
		return lhs;
	}

	/// @brief Multiplies this register by another register.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE Register &VECTORCALL operator*=(
		this Register &lhs,
		Register rhs) noexcept
		requires IApi::Multiply<api_type>
	{
		lhs.native = api_type::multiply(lhs.native, rhs.native);
		return lhs;
	}

	///
	/// @brief Divides this register by another register.
	/// @pre Every divisor lane is nonzero and signed minimum is not divided by negative one.
	///
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE Register &VECTORCALL operator/=(
		this Register &lhs,
		Register rhs) noexcept
		requires IApi::Divide<api_type>
	{
		lhs.native = api_type::divide(lhs.native, rhs.native);
		return lhs;
	}

	///
	/// @brief Replaces this register with corresponding-lane remainders.
	/// @pre Every divisor lane is nonzero and signed minimum is not divided by negative one.
	///
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE Register &VECTORCALL operator%=(
		this Register &lhs,
		Register rhs) noexcept
		requires IApi::Modulus<api_type>
	{
		lhs.native = api_type::modulus(lhs.native, rhs.native);
		return lhs;
	}
	 */
#pragma endregion

#pragma region Specialized Arithmetic and Reductions

	/** @brief Selects the minimum value from each corresponding lane. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL min(this Register lhs, Register rhs) noexcept
		requires IApi::Min<api_type>
	{
		return Register{api_type::min(lhs.native, rhs.native)};
	}

	/** @brief Selects the maximum value from each corresponding lane. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL max(this Register lhs, Register rhs) noexcept
		requires IApi::Max<api_type>
	{
		return Register{api_type::max(lhs.native, rhs.native)};
	}

	/** @brief Computes the absolute value of every lane with the selected backend's edge behavior. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL absolute(this Register value) noexcept
		requires IApi::Absolute<api_type>
	{
		return Register{api_type::absolute(value.native)};
	}

	/** @brief Computes the square root of every lane where supported. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL sqrt(this Register value) noexcept
		requires IApi::Sqrt<api_type>
	{
		return Register{api_type::sqrt(value.native)};
	}

	/** @brief Computes the backend-defined average of corresponding lanes. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL average(this Register lhs, Register rhs) noexcept
		requires IApi::Average<api_type>
	{
		return Register{api_type::avg(lhs.native, rhs.native)};
	}

	/** @brief Multiplies corresponding lanes and adds a third register. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL multiply_add(this Register lhs, Register rhs,
																											  Register addend) noexcept
		requires IApi::MultiplyAdd<api_type>
	{
		return Register{api_type::multiply_add(lhs.native, rhs.native, addend.native)};
	}

	/** @brief Computes broadcast floating magnitudes or sparse unchecked integer magnitudes for each 128-bit group. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL magnitude(this Register value) noexcept
		requires IApi::Magnitude<api_type>
	{
		return Register{api_type::magnitude(value.native)};
	}

	/** @brief Computes saturated integer magnitudes with each overflow mask stored in the following lane. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL magnitude_checked(this Register value) noexcept
		requires IApi::MagnitudeChecked<api_type>
	{
		return Register{api_type::magnitude_checked(value.native)};
	}

	/** @brief Normalizes each floating-point 128-bit lane group by its magnitude. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL normalize(this Register value) noexcept
		requires IApi::Normalize<api_type>
	{
		return Register{api_type::normalize(value.native)};
	}

	/** @brief Adds adjacent lane pairs within each 128-bit lane of two registers. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL horizontal_add(this Register lhs, Register rhs) noexcept
		requires IApi::HorizontalAdd<api_type>
	{
		return Register{api_type::add_horizontal(lhs.native, rhs.native)};
	}

	/** @brief Subtracts adjacent lane pairs within each 128-bit lane of two registers. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL horizontal_subtract(this Register lhs, Register rhs) noexcept
		requires IApi::HorizontalSubtract<api_type>
	{
		return Register{api_type::subtract_horizontal(lhs.native, rhs.native)};
	}

	/**
	 * @brief Multiplies adjacent integral lane pairs and returns the explicitly promoted Register type.
	 * @tparam source_element_t Deferred source type used to constrain result-alias availability.
	 */
	template <class source_element_t = element_type>
		requires std::same_as<source_element_t, element_type> && std::is_integral_v<source_element_t> && IApi::MultiplyAddAdjacent<api_type>
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY multiply_add_adjacent_result_t<source_element_t, register_width> VECTORCALL
	multiply_add_adjacent(this Register lhs, Register rhs) noexcept
	{
		return multiply_add_adjacent_result_t<source_element_t, register_width>{api_type::multiply_add_adjacent(lhs.native, rhs.native)};
	}

	/**
	 * @brief Multiplies unsigned and signed byte pairs and returns signed 16-bit sums.
	 * @tparam source_element_t Deferred source type used to constrain result-alias availability.
	 */
	template <class source_element_t = element_type>
		requires std::same_as<source_element_t, element_type> && std::is_integral_v<source_element_t> && IApi::ByteMultiplyAdd<api_type>
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY byte_multiply_add_result_t<source_element_t, register_width> VECTORCALL
	multiply_add_unsigned_signed_bytes(this Register lhs, Register rhs) noexcept
	{
		return byte_multiply_add_result_t<source_element_t, register_width>{api_type::multiply_add_unsigned_signed_bytes(lhs.native, rhs.native)};
	}

	/**
	 * @brief Sums byte-wise absolute differences into unsigned 64-bit result lanes.
	 * @tparam source_element_t Deferred source type used to constrain result-alias availability.
	 */
	template <class source_element_t = element_type>
		requires std::same_as<source_element_t, element_type> && std::is_integral_v<source_element_t> && IApi::Sad<api_type>
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY sad_result_t<source_element_t, register_width> VECTORCALL
	sum_absolute_byte_differences(this Register lhs, Register rhs) noexcept
	{
		return sad_result_t<source_element_t, register_width>{api_type::sum_absolute_byte_differences(lhs.native, rhs.native)};
	}

	/**
	 * @brief Computes immediate-controlled byte-window absolute-difference sums.
	 * @tparam imm8 Immediate control value in the intrinsic range `0..255`.
	 * @tparam source_element_t Deferred source type used to constrain result-alias availability.
	 */
	template <int imm8, class source_element_t = element_type>
		requires(imm8 >= 0 && imm8 <= 255 && std::same_as<source_element_t, element_type> && std::is_integral_v<source_element_t> &&
				 IApi::MultiSad<api_type, imm8>)
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY multi_sad_result_t<source_element_t, register_width> VECTORCALL
	multi_sum_absolute_byte_differences(this Register lhs, Register rhs) noexcept
	{
		return multi_sad_result_t<source_element_t, register_width>{api_type::template multi_sum_absolute_byte_differences<imm8>(lhs.native, rhs.native)};
	}

	/** @brief Returns the first logical position containing the minimum integral value. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr std::size_t VECTORCALL min_position(this Register value) noexcept
		requires IApi::MinPosition<api_type>
	{
		return api_type::min_position(value.native);
	}

	/** @brief Returns the first logical position containing the maximum integral value. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr std::size_t VECTORCALL max_position(this Register value) noexcept
		requires IApi::MaxPosition<api_type>
	{
		return api_type::max_position(value.native);
	}

	/** @brief Adds corresponding lanes with saturation where supported. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL add_saturated(this Register lhs, Register rhs) noexcept
		requires IApi::AddSaturated<api_type>
	{
		return Register{api_type::add_saturated(lhs.native, rhs.native)};
	}

	/** @brief Subtracts corresponding lanes with saturation where supported. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL subtract_saturated(this Register lhs, Register rhs) noexcept
		requires IApi::SubtractSaturated<api_type>
	{
		return Register{api_type::subtract_saturated(lhs.native, rhs.native)};
	}

	/** @brief Adds adjacent lane pairs with saturation where supported. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL horizontal_add_saturated(this Register lhs,
																														  Register rhs) noexcept
		requires IApi::HorizontalAddSaturated<api_type>
	{
		return Register{api_type::hadd_saturated(lhs.native, rhs.native)};
	}

	/** @brief Subtracts adjacent lane pairs with saturation where supported. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL horizontal_subtract_saturated(this Register lhs,
																															   Register rhs) noexcept
		requires IApi::HorizontalSubtractSaturated<api_type>
	{
		return Register{api_type::hsubtract_saturated(lhs.native, rhs.native)};
	}

	/** @brief Alternates subtraction and addition across floating-point lanes. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL add_subtract(this Register lhs, Register rhs) noexcept
		requires IApi::AddSubtract<api_type>
	{
		return Register{api_type::add_subtract(lhs.native, rhs.native)};
	}

	/**
	 * @brief Computes a masked floating-point dot product with intrinsic-selected output lanes.
	 * @tparam imm8 Immediate control value in the intrinsic range `0..255`.
	 */
	template <int imm8>
		requires IApi::DotProduct<api_type, imm8>
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY Register VECTORCALL dot_product(this Register lhs, Register rhs) noexcept
	{
		return Register{api_type::template dot_product<imm8>(lhs.native, rhs.native)};
	}

#pragma endregion
#pragma region Bitwise Operations

	/** @brief Computes the bitwise intersection of two registers. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL operator&(this Register lhs, Register rhs) noexcept
	{
		return Register{api_type::bitwise_and(lhs.native, rhs.native)};
	}

	/** @brief Computes the bitwise union of two registers. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL operator|(this Register lhs, Register rhs) noexcept
	{
		return Register{api_type::bitwise_or(lhs.native, rhs.native)};
	}

	/** @brief Computes the bitwise exclusive union of two registers. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL operator^(this Register lhs, Register rhs) noexcept
	{
		return Register{api_type::bitwise_xor(lhs.native, rhs.native)};
	}

	/** @brief Complements every bit in a register. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL operator~(this Register value) noexcept
	{
		return Register{api_type::bitwise_not(value.native)};
	}

	/** @brief Computes `(~lhs) & rhs` with the existing backend operand polarity. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL andnot(this Register lhs, Register rhs) noexcept
	{
		return Register{api_type::bitwise_andnot(lhs.native, rhs.native)};
	}

	/*
	 * Disabled compound assignment operators: their convenience does not justify the mutable-reference API surface,
	 * and MSVC 19.44 emits a redundant 32-byte stack-alignment frame for 256-bit wrapper mutation through references.
	 * Prefer `lhs = lhs & rhs`, `lhs = lhs | rhs`, or `lhs = lhs ^ rhs`.
	 *
	/// @brief Intersects this register with another register.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register &VECTORCALL operator&=(
		this Register &lhs,
		Register rhs) noexcept
	{
		lhs.native = api_type::bitwise_and(lhs.native, rhs.native);
		return lhs;
	}

	/// @brief Unites this register with another register.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register &VECTORCALL operator|=(
		this Register &lhs,
		Register rhs) noexcept
	{
		lhs.native = api_type::bitwise_or(lhs.native, rhs.native);
		return lhs;
	}

	/// @brief Exclusively combines this register with another register.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register &VECTORCALL operator^=(
		this Register &lhs,
		Register rhs) noexcept
	{
		lhs.native = api_type::bitwise_xor(lhs.native, rhs.native);
		return lhs;
	}
	 */
	/** @brief Returns the selected intrinsic's native-granularity sign-bit mask. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr typename api_type::mask_t VECTORCALL
	movemask(this Register value) noexcept
	{
		return api_type::movemask(value.native);
	}

	/** @brief Returns one scalar sign bit for every logical lane. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr typename api_type::mask_t VECTORCALL
	lane_sign_bits(this Register value) noexcept
	{
		return api_type::movemask_slim(value.native);
	}

#pragma endregion

#pragma region Shifting Operations

	/**
	 * @brief Left-shifts every integral lane.
	 * @pre `count >= 0`; counts at least the lane width produce zero lanes.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL operator<<(this Register value, int count) noexcept
		requires std::is_integral_v<element_type>
	{
		return Register{api_type::shift_left(value.native, count)};
	}

	/**
	 * @brief Right-shifts every integral lane with zero fill.
	 * @pre `count >= 0`; counts at least the lane width produce zero lanes.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL logical_shift_right(this Register value,
																															   int count) noexcept
		requires std::is_integral_v<element_type>
	{
		return Register{api_type::shift_right(value.native, count)};
	}

	/**
	 * @brief Right-shifts unsigned lanes logically and signed lanes arithmetically.
	 * @pre `count >= 0`; oversized signed counts clamp and unsigned counts produce zero lanes.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL operator>>(this Register value, int count) noexcept
		requires std::is_integral_v<element_type>
	{
		if constexpr (std::is_signed_v<element_type>)
			return Register{api_type::shift_right_arithmetic(value.native, count)};
		else
			return Register{api_type::shift_right(value.native, count)};
	}

	/*
	 * Disabled compound assignment operators: their convenience does not justify the mutable-reference API surface,
	 * and MSVC 19.44 emits a redundant 32-byte stack-alignment frame for 256-bit wrapper mutation through references.
	 * Prefer `value = value << count` or `value = value >> count`.
	 *
	/// @brief Left-shifts every integral lane in this register.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register &VECTORCALL operator<<=(
		this Register &value,
		int count) noexcept
		requires std::is_integral_v<element_type>
	{
		value.native = api_type::shift_left(value.native, count);
		return value;
	}

	/// @brief Right-shifts every integral lane in this register using its signedness.
	SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register &VECTORCALL operator>>=(
		this Register &value,
		int count) noexcept
		requires std::is_integral_v<element_type>
	{
		if constexpr (std::is_signed_v<element_type>)
			value.native = api_type::shift_right_arithmetic(value.native, count);
		else
			value.native = api_type::shift_right(value.native, count);
		return value;
	}
	 */
	/** @brief Byte-shifts a complete 128-bit integral register toward higher byte indices. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register VECTORCALL byte_shift_left(this Register value, int count) noexcept
		requires(std::is_integral_v<element_type> && register_width == 128)
	{
		return Register{api_type::byte_shift_left(value.native, count)};
	}

	/** @brief Byte-shifts a complete 128-bit integral register toward lower byte indices. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register VECTORCALL byte_shift_right(this Register value, int count) noexcept
		requires(std::is_integral_v<element_type> && register_width == 128)
	{
		return Register{api_type::byte_shift_right(value.native, count)};
	}

	/** @brief Shifts a complete 128-bit integral register left as one bit string. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register VECTORCALL bit_shift_left(this Register value, int count) noexcept
		requires(std::is_integral_v<element_type> && register_width == 128)
	{
		return Register{api_type::bit_shift_left(value.native, count)};
	}

	/** @brief Shifts a complete 128-bit integral register right as one bit string. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register VECTORCALL bit_shift_right(this Register value, int count) noexcept
		requires(std::is_integral_v<element_type> && register_width == 128)
	{
		return Register{api_type::bit_shift_right(value.native, count)};
	}

	/** @brief Compile-time shifts a complete 128-bit integral register left as one bit string. */
	template <int count>
		requires(std::is_integral_v<element_type> && register_width == 128 && count >= 0)
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register VECTORCALL bit_shift_left(this Register value) noexcept
	{
		return Register{api_type::template bit_shift_left<count>(value.native)};
	}

	/** @brief Compile-time shifts a complete 128-bit integral register right as one bit string. */
	template <int count>
		requires(std::is_integral_v<element_type> && register_width == 128 && count >= 0)
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE constexpr Register VECTORCALL bit_shift_right(this Register value) noexcept
	{
		return Register{api_type::template bit_shift_right<count>(value.native)};
	}

#pragma endregion

#pragma region Rearrangement and Conversion Operations

	/** @brief Returns the low 128-bit half of a 256-bit register.
	 *  @param value Source register in logical low-to-high lane order.
	 *  @return `Register<element_type, 128>` containing the lowest source lanes.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register<element_type, 128> VECTORCALL
	lower_half(this Register value) noexcept
		requires(register_width == 256 && IApi::LowerHalf<api_type>)
	{
		return Register<element_type, 128>{api_type::lower_half(value.native)};
	}

	/** @brief Interleaves the low half of each 128-bit lane group from two registers.
	 *  @param lhs Supplies even-numbered result lanes in every 128-bit group.
	 *  @param rhs Supplies odd-numbered result lanes in every 128-bit group.
	 *  @return Register containing `lhs[0], rhs[0], lhs[1], rhs[1], ...` independently in each 128-bit group.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL unpack_low(this Register lhs, Register rhs) noexcept
		requires IApi::UnpackLow<api_type>
	{
		return Register{api_type::unpack_lo(lhs.native, rhs.native)};
	}

	/** @brief Interleaves the high half of each 128-bit lane group from two registers.
	 *  @param lhs Supplies even-numbered result lanes in every 128-bit group.
	 *  @param rhs Supplies odd-numbered result lanes in every 128-bit group.
	 *  @return Register containing interleaved lanes from each source group's high half.
	 */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL unpack_high(this Register lhs, Register rhs) noexcept
		requires IApi::UnpackHigh<api_type>
	{
		return Register{api_type::unpack_hi(lhs.native, rhs.native)};
	}

	/** @brief Rearranges byte lanes with a complete compile-time logical selector list.
	 *  @tparam indices One source-lane index for every result lane.
	 *  @param value Source byte register.
	 *  @return Register containing the selected bytes in logical output order.
	 *  @note Every selector must stay in the same 128-bit group as its output lane because the selected intrinsic cannot cross groups.
	 */
	template <std::size_t... indices>
		requires IApi::Shuffle<api_type, indices...>
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL shuffle(this Register value) noexcept
	{
		return Register{api_type::template shuffle<indices...>(value.native)};
	}

	/** @brief Shuffles the low four 16-bit lanes in each 128-bit group.
	 *  @tparam imm8 Immediate control in the inclusive range `0..255`; every two-bit field selects one source lane.
	 *  @param value Source register.
	 *  @return Register with low lane groups shuffled and high lane groups preserved.
	 */
	template <int imm8>
		requires IApi::ShuffleLow<api_type, imm8>
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL shuffle_low(this Register value) noexcept
	{
		return Register{api_type::template shuffle_lo<imm8>(value.native)};
	}

	/** @brief Shuffles the high four 16-bit lanes in each 128-bit group.
	 *  @tparam imm8 Immediate control in the inclusive range `0..255`; every two-bit field selects one source lane.
	 *  @param value Source register.
	 *  @return Register with high lane groups shuffled and low lane groups preserved.
	 */
	template <int imm8>
		requires IApi::ShuffleHigh<api_type, imm8>
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL shuffle_high(this Register value) noexcept
	{
		return Register{api_type::template shuffle_hi<imm8>(value.native)};
	}

	/** @brief Selects corresponding lanes from two registers with an immediate control mask.
	 *  @tparam imm8 Immediate control in the inclusive range `0..255`; set applicable bits select `rhs`.
	 *  @param lhs Register selected by cleared applicable control bits.
	 *  @param rhs Register selected by set applicable control bits.
	 *  @return Register containing the intrinsic-defined immediate blend.
	 *  @note Unused immediate bits retain intrinsic behavior. A 256-bit 16-bit blend repeats the mask in each 128-bit group.
	 */
	template <int imm8>
		requires IApi::Blend<api_type, imm8>
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register VECTORCALL blend(this Register lhs, Register rhs) noexcept
	{
		return Register{api_type::template blend<imm8>(lhs.native, rhs.native)};
	}

	/** @brief Reinterprets every bit of this complete register as another supported lane type.
	 *  @tparam target_t Destination lane interpretation at the same register width.
	 *  @param value Source register whose complete bit pattern is preserved.
	 *  @return `Register<target_t, register_width>` containing exactly the source bits.
	 */
	template <class target_t>
		requires RegisterAvailable<target_t, register_width> && IApi::BitCast<api_type, target_t>
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register<target_t, register_width> VECTORCALL
	bit_cast(this Register value) noexcept
	{
		return Register<target_t, register_width>{api_type::template bit_cast<target_t>(value.native)};
	}

	/** @brief Numerically converts every lane into one complete destination register.
	 *  @tparam target_t Explicit numeric destination lane type.
	 *  @param value Source register.
	 *  @return `Register<target_t, register_width>` containing converted lane values.
	 *  @note The initial surface supports signed or unsigned 32-bit integers to `float`, and `float` to signed 32-bit integers.
	 */
	template <class target_t>
		requires RegisterAvailable<target_t, register_width> && IApi::Convert<api_type, target_t>
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register<target_t, register_width> VECTORCALL
	convert(this Register value) noexcept
	{
		return Register<target_t, register_width>{api_type::template convert<target_t>(value.native)};
	}

	/** @brief Widens only the lowest source lanes needed to fill one complete target register.
	 *  @tparam target_t Wider integral destination lane type with the same signedness as `element_type`.
	 *  @tparam target_bits Destination register width, either 128 or 256 bits.
	 *  @param value Source 128-bit integral register.
	 *  @return Complete target register populated from the lowest `target_bits / (sizeof(target_t) * 8)` source lanes.
	 *  @note Source lanes above the returned register's lane count are intentionally not consumed.
	 */
	template <class target_t, std::size_t target_bits>
		requires RegisterAvailable<target_t, target_bits> && IApi::Widen<api_type, Api<target_bits, target_t>>
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register<target_t, target_bits> VECTORCALL
	widen_low(this Register value) noexcept
	{
		return Register<target_t, target_bits>{api_type::template widen<Api<target_bits, target_t>>(value.native)};
	}

#pragma endregion

#pragma region Comparison Operations

	/** @brief Compares corresponding lanes for ordered equality. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr mask_type VECTORCALL compare_equal(this Register lhs,
																														  Register rhs) noexcept
	{
		return mask_type{api_type::compare_equal(lhs.native, rhs.native)};
	}

	/** @brief Compares corresponding lanes for greater-than ordering. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr mask_type VECTORCALL compare_greater(this Register lhs,
																															Register rhs) noexcept
	{
		return mask_type{api_type::compare_greater(lhs.native, rhs.native)};
	}

	/** @brief Compares corresponding lanes for greater-than-or-equal ordering. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr mask_type VECTORCALL compare_greater_equal(this Register lhs,
																																  Register rhs) noexcept
	{
		return mask_type{api_type::compare_greater_equal(lhs.native, rhs.native)};
	}

	/** @brief Compares corresponding lanes for less-than ordering. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr mask_type VECTORCALL compare_less(this Register lhs,
																														 Register rhs) noexcept
	{
		return mask_type{api_type::compare_less(lhs.native, rhs.native)};
	}

	/** @brief Compares corresponding lanes for less-than-or-equal ordering. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr mask_type VECTORCALL compare_less_equal(this Register lhs,
																															   Register rhs) noexcept
	{
		return mask_type{api_type::compare_less_equal(lhs.native, rhs.native)};
	}

	/** @brief Tests whether every corresponding lane compares equal. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr bool VECTORCALL operator==(this Register lhs, Register rhs) noexcept
	{
		return lhs.compare_equal(rhs).all();
	}

	/** @brief Tests whether at least one corresponding lane compares unequal. */
	[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr bool VECTORCALL operator!=(this Register lhs, Register rhs) noexcept
	{
		return !lhs.compare_equal(rhs).all();
	}

#pragma endregion

  private:
	/**
	 * @brief Implements compile-time lane observation through the portable array representation.
	 * @tparam index Logical lane index to observe.
	 * @param value Register containing the selected lane.
	 * @return Copy of lane `index`.
	 */
	template <std::size_t index> [[nodiscard]] constexpr static element_type lane_constexpr(Register value) noexcept
	{
		return value.to_array()[index];
	}
};

/** @brief Selects true or false register lanes according to this predicate. */
template <class element_t, std::size_t register_bits>
	requires RegisterAvailable<element_t, register_bits>
[[nodiscard]] SIMDLIB_FLATTEN SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY constexpr Register<element_t, register_bits> VECTORCALL
RegisterMask<element_t, register_bits>::select(this RegisterMask condition, register_type when_true, register_type when_false) noexcept
{
	return register_type{condition.select_native(when_true.native, when_false.native)};
}

/**
 * @brief Selects the widest complete register available for an element type.
 * @tparam element_t Scalar interpretation of each register lane.
 */
template <class element_t>
	requires RegisterAvailable<element_t, 128>
using NativeRegister = Register<element_t, (is_register_available_v<element_t, 256> ? 256 : 128)>;

} // namespace SimdLib
