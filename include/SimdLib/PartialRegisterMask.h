#pragma once

#include <SimdLib/Config.h>

#if !SIMDLIB_REGISTER_INTERFACE_AVAILABLE && !SIMDLIB_REQUIRE_REGISTER_INTERFACE
#error "SIMDLIB_PARTIAL_REGISTER_MASK_HEADER_REQUIRES_CXX23: <SimdLib/PartialRegisterMask.h> requires C++23 explicit object parameter support"
#endif

#include <SimdLib/IPartialRegisterMask.h>
#include <SimdLib/PartialRegister.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace SimdLib
{

/**
 * @brief Owns one canonical predicate register for a PartialRegister geometry.
 * @tparam element_t Scalar geometry associated with each predicate lane.
 * @tparam register_bits Width of the associated native register in bits.
 * @tparam active_lane_count Number of active low predicate lanes.
 * @invariant Active lanes are canonical all-zero or all-one predicates; inactive lanes are all-bits zero.
 */
template <class element_t, std::size_t register_bits, std::size_t active_lane_count>
	requires PartialRegisterAvailable<element_t, register_bits, active_lane_count>
class PartialRegisterMask final
{
  public:
	using element_type = element_t;
	using api_type = Api<register_bits, element_type>;
	using native_type = typename api_type::vector_t;
	using register_type = PartialRegister<element_type, register_bits, active_lane_count>;
	using bits_type = std::conditional_t<(api_type::element_count <= 32), std::uint32_t, std::uint64_t>;

	constexpr static inline std::size_t register_width = register_bits;
	constexpr static inline std::size_t byte_count = api_type::byte_count;
	constexpr static inline std::size_t native_lane_count = api_type::element_count;
	constexpr static inline std::size_t lane_count = active_lane_count;
	constexpr static inline std::size_t inactive_lane_count = native_lane_count - lane_count;

	/**
	 * @brief Owns the partial native predicate represented by this aggregate.
	 * @pre Direct aggregate initialization must provide canonical active predicates and a bitwise-zero inactive suffix.
	 */
	native_type native = api_type::setzero();

	/**
	 * @brief Imports canonical active predicate lanes and clears every inactive lane.
	 * @param native Native predicate whose active lanes are all-zero or all-one.
	 * @return A predicate with a bitwise-zero inactive suffix.
	 * @pre Active lanes are canonical Boolean predicate representations.
	 */
	[[nodiscard]] constexpr static PartialRegisterMask SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) from_native(native_type native) noexcept
		requires IApi::BitwiseAnd<api_type>
	{
		return PartialRegisterMask{normalize_native(validate_import_native(native))};
	}

	/** @brief Exports the native predicate by value with a bitwise-zero inactive suffix. */
	[[nodiscard]] constexpr native_type SIMD_FLAGS(In, ForceInline, Flatten) to_native(this PartialRegisterMask value) noexcept
	{
		return validate_native(value.native);
	}

	/** @brief Tests whether any active predicate lane is true. */
	[[nodiscard]] constexpr bool SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) any(this PartialRegisterMask value) noexcept
		requires IApi::MovemaskSlim<api_type>
	{
		return value.bits() != 0;
	}

	/** @brief Tests whether every active predicate lane is true, ignoring false inactive lanes. */
	[[nodiscard]] constexpr bool SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) all(this PartialRegisterMask value) noexcept
		requires IApi::MovemaskSlim<api_type>
	{
		return value.bits() == active_bits;
	}

	/** @brief Tests whether every active predicate lane is false. */
	[[nodiscard]] constexpr bool SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) none(this PartialRegisterMask value) noexcept
		requires IApi::MovemaskSlim<api_type>
	{
		return value.bits() == 0;
	}

	/** @brief Returns one compact Boolean bit for each active predicate lane. */
	[[nodiscard]] constexpr bits_type SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) bits(this PartialRegisterMask value) noexcept
		requires IApi::MovemaskSlim<api_type>
	{
		return static_cast<bits_type>(api_type::movemask_slim(value.to_native())) & active_bits;
	}

	/**
	 * @brief Selects between matching PartialRegister operands for every active predicate lane.
	 * @param condition Canonical predicate selecting the source of each active lane.
	 * @param when_true Value supplying true-selected active lanes.
	 * @param when_false Value supplying false-selected active lanes.
	 * @return A PartialRegister with selected active lanes and a bitwise-zero inactive suffix.
	 */
	[[nodiscard]] constexpr register_type SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		select(this PartialRegisterMask condition, register_type when_true, register_type when_false) noexcept
		requires IApi::Select<api_type>
	{
		return register_type{api_type::select(condition.to_native(), when_true.to_native(), when_false.to_native())};
	}

	/** @brief Computes the intersection of two partial predicates. */
	[[nodiscard]] constexpr PartialRegisterMask SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator&(this PartialRegisterMask lhs,
																												PartialRegisterMask rhs) noexcept
		requires IApi::BitwiseAnd<api_type>
	{
		return PartialRegisterMask{api_type::bitwise_and(lhs.to_native(), rhs.to_native())};
	}

	/** @brief Computes the union of two partial predicates. */
	[[nodiscard]] constexpr PartialRegisterMask SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator|(this PartialRegisterMask lhs,
																												PartialRegisterMask rhs) noexcept
		requires IApi::BitwiseOr<api_type>
	{
		return PartialRegisterMask{api_type::bitwise_or(lhs.to_native(), rhs.to_native())};
	}

	/** @brief Computes the exclusive union of two partial predicates. */
	[[nodiscard]] constexpr PartialRegisterMask SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator^(this PartialRegisterMask lhs,
																												PartialRegisterMask rhs) noexcept
		requires IApi::BitwiseXor<api_type>
	{
		return PartialRegisterMask{api_type::bitwise_xor(lhs.to_native(), rhs.to_native())};
	}

	/** @brief Inverts active predicate lanes and explicitly clears the inactive predicate suffix. */
	[[nodiscard]] constexpr PartialRegisterMask SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator~(this PartialRegisterMask value) noexcept
		requires IApi::BitwiseNot<api_type> && IApi::BitwiseAnd<api_type>
	{
		return PartialRegisterMask{normalize_native(api_type::bitwise_not(value.to_native()))};
	}

  private:
	/** @brief Compact mask with one set bit for each active predicate lane. */
	constexpr static inline bits_type active_bits = (bits_type{1} << lane_count) - 1;

	/** @brief Compile-time all-bits-one active prefix and all-bits-zero inactive suffix. */
	constexpr static inline std::array<element_type, native_lane_count> active_lane_filter = []() constexpr
	{
		std::array<element_type, native_lane_count> result{};
		std::array<std::byte, sizeof(element_type)> one_bytes{};
		for (auto &byte : one_bytes)
			byte = std::byte{0xff};
		const auto one = std::bit_cast<element_type>(one_bytes);
		for (std::size_t lane = 0; lane < lane_count; ++lane)
			result[lane] = one;
		return result;
	}();

	/** @brief Clears inactive predicate lanes from a native value while retaining the active prefix. */
	[[nodiscard]] constexpr static native_type SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) normalize_native(native_type native) noexcept
		requires IApi::BitwiseAnd<api_type>
	{
		if consteval
		{
			auto lanes = api_type::to_array(native);
			for (std::size_t lane = lane_count; lane < native_lane_count; ++lane)
				lanes[lane] = element_type{};
			return api_type::construct(lanes);
		}
		return api_type::bitwise_and(native, api_type::construct(active_lane_filter));
	}

	/** @brief Validates canonical active predicates at the native import boundary when checks are enabled. */
	[[nodiscard]] constexpr static native_type validate_import_native(native_type native) noexcept
	{
#if SIMDLIB_ENABLE_CHECKS
		validate_predicate_bytes(native, false);
#endif
		return native;
	}

	/** @brief Validates canonical active predicates and a bitwise-zero inactive suffix when checks are enabled. */
	[[nodiscard]] constexpr static native_type SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) validate_native(native_type native) noexcept
	{
#if SIMDLIB_ENABLE_CHECKS
		validate_predicate_bytes(native, true);
#endif
		return native;
	}

	/**
	 * @brief Validates predicate representations through SIMD byte comparisons.
	 * @param native Native predicate to inspect.
	 * @param validate_inactive Whether the inactive suffix must also be bitwise zero.
	 */
	constexpr static void validate_predicate_bytes(native_type native, bool validate_inactive) noexcept
	{
		using byte_api_type = Api<register_width, std::uint8_t>;
		const auto bytes = api_type::template bit_cast<std::uint8_t>(native);
		const auto zero_bits = byte_api_type::movemask_slim(byte_api_type::compare_equal(bytes, byte_api_type::setzero()));
		const auto one_bits = byte_api_type::movemask_slim(byte_api_type::compare_equal(bytes, byte_api_type::set1(0xffU)));
		for (std::size_t lane = 0; lane < lane_count; ++lane)
		{
			bool is_zero = true;
			bool is_one = true;
			for (std::size_t byte = lane * sizeof(element_type); byte < (lane + 1) * sizeof(element_type); ++byte)
			{
				is_zero = is_zero && (zero_bits & (typename byte_api_type::mask_t{1} << byte)) != 0;
				is_one = is_one && (one_bits & (typename byte_api_type::mask_t{1} << byte)) != 0;
			}
			SIMDLIB_PRECONDITION(is_zero || is_one, "PartialRegisterMask active lanes must be canonical predicates");
		}
		if (validate_inactive)
			for (std::size_t byte = lane_count * sizeof(element_type); byte < byte_count; ++byte)
				SIMDLIB_PRECONDITION((zero_bits & (typename byte_api_type::mask_t{1} << byte)) != 0,
									 "PartialRegisterMask inactive lanes must have an all-bits-zero representation");
	}
};

} // namespace SimdLib
