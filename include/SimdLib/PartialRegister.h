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
	 * @brief Initializes every native lane to all-bits zero.
	 * @remarks The logical prefix and inactive suffix are both zero after default construction.
	 */
	constexpr PartialRegister() noexcept = default;

	/**
	 * @brief Returns a value with every active and inactive lane set to all-bits zero.
	 * @return A fully initialized logical zero value.
	 */
	[[nodiscard]] constexpr static PartialRegister SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) zero() noexcept
	{
		return from_clean_native(api_type::setzero());
	}

	/**
	 * @brief Imports a native value after clearing its inactive high-lane suffix.
	 * @param native Native register value whose logical low prefix is retained.
	 * @return A PartialRegister with a bitwise-zero inactive suffix.
	 */
	[[nodiscard]] constexpr static PartialRegister SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) from_native(native_type native) noexcept
	{
		return from_clean_native(normalize_native(native));
	}

	/**
	 * @brief Exports the sole native register value by value.
	 * @return A native value whose inactive high-lane suffix is all-bits zero.
	 */
	[[nodiscard]] constexpr native_type SIMD_FLAGS(In, ForceInline, Flatten) to_native(this PartialRegister value) noexcept
	{
		return validate_native(value.native_);
	}

  private:
	/** @brief Tags native values whose inactive suffix is already known to be zero. */
	struct clean_native_t
	{
		/** @brief Creates the private clean-native construction tag. */
		explicit constexpr clean_native_t() noexcept = default;
	};

	/** @brief Compile-time logical-prefix mask with no per-object storage. */
	constexpr static inline std::array<bool, native_lane_count> active_lane_mask = []() constexpr {
		std::array<bool, native_lane_count> result{};
		for (std::size_t lane = 0; lane < lane_count; ++lane)
			result[lane] = true;
		return result;
	}();

	/** @brief Sole native register storage; never exposed as a writable public member. */
	native_type native_ = api_type::setzero();

	/**
	 * @brief Constructs a value from a native register that already satisfies the invariant.
	 * @param native Native value with a bitwise-zero inactive suffix.
	 * @param clean Private proof that normalization was performed or unnecessary.
	 */
	constexpr explicit PartialRegister(native_type native, clean_native_t clean) noexcept : native_(validate_native(native))
	{
		static_cast<void>(clean);
	}

	/**
	 * @brief Constructs a value at the invariant-preserving operation boundary.
	 * @param native Native value already proven clean for the inactive suffix.
	 * @return A PartialRegister that validates the supplied clean native value.
	 */
	[[nodiscard]] constexpr static PartialRegister from_clean_native(native_type native) noexcept
	{
		return PartialRegister{native, clean_native_t{}};
	}

	/**
	 * @brief Clears inactive high lanes from an arbitrary native value.
	 * @param native Native value whose logical low prefix is retained.
	 * @return Native value with an all-bits-zero inactive suffix.
	 */
	[[nodiscard]] constexpr static native_type normalize_native(native_type native) noexcept
	{
		auto lanes = api_type::to_array(native);
		for (std::size_t lane = 0; lane < native_lane_count; ++lane)
			if (!active_lane_mask[lane])
				lanes[lane] = element_type{};
		return api_type::construct(lanes);
	}

	/**
	 * @brief Validates the bit representation of every inactive lane when checks are enabled.
	 * @param native Native value expected to have an all-bits-zero inactive suffix.
	 * @return The unchanged native value.
	 */
	[[nodiscard]] constexpr static native_type validate_native(native_type native) noexcept
	{
#if SIMDLIB_ENABLE_CHECKS
		const auto lanes = api_type::to_array(native);
		for (std::size_t lane = lane_count; lane < native_lane_count; ++lane)
		{
			const auto bytes = std::bit_cast<std::array<std::byte, sizeof(element_type)>>(lanes[lane]);
			for (const std::byte byte : bytes)
				SIMDLIB_PRECONDITION(byte == std::byte{}, "PartialRegister inactive lanes must have an all-bits-zero representation");
		}
#endif
		return native;
	}
};

} // namespace SimdLib
