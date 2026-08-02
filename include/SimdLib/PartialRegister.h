#pragma once

#include <SimdLib/Config.h>

#if !SIMDLIB_REGISTER_INTERFACE_AVAILABLE && !SIMDLIB_REQUIRE_REGISTER_INTERFACE
#error "SIMDLIB_PARTIAL_REGISTER_HEADER_REQUIRES_CXX23: <SimdLib/PartialRegister.h> requires C++23 explicit object parameter support"
#endif

#include <SimdLib/PartialRegisterFwd.h>

#include <array>
#include <bit>
#include <cstddef>

namespace SimdLib
{

/**
 * @brief Owns one SIMD register with a fixed low prefix of logical lanes.
 * @tparam element_t Scalar interpretation of every lane.
 * @tparam bits Physical native-register width in bits.
 * @tparam active_lane_count Number of active low lanes.
 * @invariant The sole native value has all-bits-zero inactive high lanes.
 * @remarks The type is available only for a non-empty, non-complete active prefix.
 */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
	requires RegisterAvailable<element_t, bits> && (active_lane_count > 0) && (active_lane_count < Api<bits, element_t>::element_count)
class PartialRegister final
{
  public:
	using element_type = element_t;
	using api_type = Api<bits, element_type>;
	using native_type = typename api_type::vector_t;
	using mask_type = PartialRegisterMask<element_type, bits, active_lane_count>;

	constexpr static inline std::size_t register_width = bits;
	constexpr static inline std::size_t byte_count = api_type::byte_count;
	constexpr static inline std::size_t native_lane_count = api_type::element_count;
	constexpr static inline std::size_t lane_count = active_lane_count;
	constexpr static inline std::size_t active_byte_count = lane_count * sizeof(element_type);
	constexpr static inline std::size_t inactive_lane_count = native_lane_count - lane_count;

	/**
	 * @brief Owns the partial native register represented by this aggregate.
	 * @pre Direct aggregate initialization must supply a value with a bitwise-zero inactive suffix.
	 */
	native_type native = api_type::setzero();

	/**
	 * @brief Returns a value with every active and inactive lane set to all-bits zero.
	 * @return A fully initialized logical zero value.
	 */
	[[nodiscard]] constexpr static PartialRegister SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) zero() noexcept
	{
		return PartialRegister{api_type::setzero()};
	}

	/**
	 * @brief Imports a native value after clearing its inactive high-lane suffix.
	 * @param native Native register value whose logical low prefix is retained.
	 * @return A PartialRegister with a bitwise-zero inactive suffix.
	 */
	[[nodiscard]] constexpr static PartialRegister SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) from_native(native_type native) noexcept
		requires IApi::BitwiseAnd<api_type>
	{
		return PartialRegister{normalize_native(native)};
	}

	/**
	 * @brief Exports the sole native register value by value.
	 * @return A native value whose inactive high-lane suffix is all-bits zero.
	 */
	[[nodiscard]] constexpr native_type SIMD_FLAGS(In, ForceInline, Flatten) to_native(this PartialRegister value) noexcept
	{
		return validate_native(value.native);
	}

  private:
	/** @brief Compile-time all-bits-one active prefix and all-bits-zero inactive suffix. */
	constexpr static inline std::array<element_type, native_lane_count> active_lane_filter = []() constexpr {
		std::array<element_type, native_lane_count> result{};
		std::array<std::byte, sizeof(element_type)> one_bytes{};
		for (auto &byte : one_bytes)
			byte = std::byte{0xff};
		const auto one = std::bit_cast<element_type>(one_bytes);
		for (std::size_t lane = 0; lane < lane_count; ++lane)
			result[lane] = one;
		return result;
	}();

	/**
	 * @brief Clears inactive high lanes from an arbitrary native value.
	 * @param native Native value whose logical low prefix is retained.
	 * @return Native value with an all-bits-zero inactive suffix.
	 */
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

	/**
	 * @brief Validates the bit representation of every inactive lane when checks are enabled.
	 * @param native Native value expected to have an all-bits-zero inactive suffix.
	 * @return The unchanged native value.
	 */
	[[nodiscard]] constexpr static native_type validate_native(native_type native) noexcept
	{
#if SIMDLIB_ENABLE_CHECKS
		using byte_api_type = Api<register_width, std::uint8_t>;
		const auto bytes = api_type::template bit_cast<std::uint8_t>(native);
		const auto zero_bytes = byte_api_type::compare_equal(bytes, byte_api_type::setzero());
		const auto zero_bits = byte_api_type::movemask_slim(zero_bytes);
		for (std::size_t byte = active_byte_count; byte < byte_count; ++byte)
			SIMDLIB_PRECONDITION((zero_bits & (typename byte_api_type::mask_t{1} << byte)) != 0,
				"PartialRegister inactive lanes must have an all-bits-zero representation");
#endif
		return native;
	}
};

} // namespace SimdLib

#include <SimdLib/PartialRegisterMask.h>
