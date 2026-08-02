#pragma once

#include <SimdLib/Config.h>

#if !SIMDLIB_REGISTER_INTERFACE_AVAILABLE && !SIMDLIB_REQUIRE_REGISTER_INTERFACE
#error "SIMDLIB_PARTIAL_REGISTER_HEADER_REQUIRES_CXX23: <SimdLib/PartialRegister.h> requires C++23 explicit object parameter support"
#endif

#include <SimdLib/PartialRegisterFwd.h>

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

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
	 * @brief Broadcasts one scalar value to every active lane and clears every inactive lane.
	 * @param value Scalar value to broadcast.
	 * @return PartialRegister containing `value` in every active lane and positive zero elsewhere.
	 * @remarks Available exactly when `IApi::BroadcastPartial<api_type, lane_count>` is satisfied.
	 */
	[[nodiscard]] constexpr static PartialRegister SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) broadcast(element_type value) noexcept
		requires IApi::BroadcastPartial<api_type, lane_count>
	{
		return PartialRegister{api_type::template broadcast_partial<lane_count>(value)};
	}

	/**
	 * @brief Constructs a partial register from exactly its active logical lane list.
	 * @tparam lane_types Scalar argument types convertible to `element_type`.
	 * @param lanes Values in low-to-high logical lane order.
	 * @return PartialRegister containing the supplied active values and a zero inactive suffix.
	 */
	template <std::convertible_to<element_type>... lane_types>
		requires(sizeof...(lane_types) == lane_count) && IApi::SetReversePartial<api_type, lane_types...>
	[[nodiscard]] constexpr static PartialRegister SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) from_lanes(lane_types &&...lanes) noexcept
	{
		return PartialRegister{api_type::setr_partial(static_cast<element_type>(std::forward<lane_types>(lanes))...)};
	}

	/**
	 * @brief Constructs a partial register from exactly its active fixed-size lane array.
	 * @param source Active values in low-to-high logical order.
	 * @return PartialRegister containing the supplied active values and a zero inactive suffix.
	 */
	[[nodiscard]] constexpr static PartialRegister SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten)
		from_array(const std::array<element_type, lane_count> &source) noexcept
		requires IApi::LoadPartial<api_type, lane_count>
	{
		return PartialRegister{api_type::template load_partial<lane_count>(std::span<const element_type, lane_count>{source})};
	}

	/**
	 * @brief Loads exactly the active logical elements from potentially unaligned storage.
	 * @param source Source containing exactly the active logical extent.
	 * @return PartialRegister containing the source values and a zero inactive suffix.
	 * @remarks The operation does not read beyond `source`.
	 */
	[[nodiscard]] static PartialRegister SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten)
		load(std::span<const element_type, lane_count> source) noexcept
		requires IApi::LoadPartial<api_type, lane_count>
	{
		return PartialRegister{api_type::template load_partial<lane_count>(source)};
	}

	/**
	 * @brief Loads exactly the active logical elements from register-aligned storage.
	 * @param source Register-aligned source containing exactly the active logical extent.
	 * @return PartialRegister containing the source values and a zero inactive suffix.
	 * @pre `source.data()` is aligned to `byte_count` bytes.
	 * @remarks The operation does not read beyond `source`.
	 */
	[[nodiscard]] static PartialRegister SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten)
		load_aligned(std::span<const element_type, lane_count> source) noexcept
		requires IApi::LoadPartialAligned<api_type, lane_count>
	{
		return PartialRegister{api_type::template load_partial_aligned<lane_count>(source)};
	}

	/**
	 * @brief Loads exactly the active logical byte representation from potentially unaligned storage.
	 * @param source Source containing exactly `active_byte_count` bytes.
	 * @return PartialRegister containing the source bit pattern and a zero inactive suffix.
	 * @remarks The operation does not read beyond `source`.
	 */
	[[nodiscard]] static PartialRegister SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten)
		load_bytes(std::span<const std::byte, active_byte_count> source) noexcept
		requires IApi::LoadBytesPartial<api_type, active_byte_count>
	{
		return PartialRegister{api_type::template load_bytes_partial<active_byte_count>(source)};
	}

	/**
	 * @brief Stores exactly the active logical elements to potentially unaligned storage.
	 * @param value Partial register to store.
	 * @param destination Destination containing exactly the active logical extent.
	 * @remarks The operation does not write beyond `destination`.
	 */
	void SIMD_FLAGS(In, ForceInline, Flatten) store(this PartialRegister value, std::span<element_type, lane_count> destination) noexcept
		requires IApi::StorePartial<api_type, lane_count>
	{
		api_type::template store_partial<lane_count>(value.to_native(), destination);
	}

	/**
	 * @brief Stores exactly the active logical elements to register-aligned storage.
	 * @param value Partial register to store.
	 * @param destination Register-aligned destination containing exactly the active logical extent.
	 * @pre `destination.data()` is aligned to `byte_count` bytes.
	 * @remarks The operation does not write beyond `destination`.
	 */
	void SIMD_FLAGS(In, ForceInline, Flatten) store_aligned(this PartialRegister value, std::span<element_type, lane_count> destination) noexcept
		requires IApi::StorePartialAligned<api_type, lane_count>
	{
		api_type::template store_partial_aligned<lane_count>(value.to_native(), destination);
	}

	/**
	 * @brief Stores exactly the active logical byte representation to potentially unaligned storage.
	 * @param value Partial register to store.
	 * @param destination Destination containing exactly `active_byte_count` bytes.
	 * @remarks The operation does not write beyond `destination`.
	 */
	void SIMD_FLAGS(In, ForceInline, Flatten) store_bytes(this PartialRegister value, std::span<std::byte, active_byte_count> destination) noexcept
		requires IApi::StoreBytesPartial<api_type, active_byte_count>
	{
		api_type::template store_bytes_partial<active_byte_count>(value.to_native(), destination);
	}

	/**
	 * @brief Copies exactly the active logical lanes into a fixed-size array.
	 * @param value Partial register to observe.
	 * @return Active lanes in low-to-high logical order.
	 */
	[[nodiscard]] constexpr std::array<element_type, lane_count> SIMD_FLAGS(In, ForceInline, Flatten)
		to_array(this PartialRegister value) noexcept
		requires IApi::ToArrayPartial<api_type, lane_count>
	{
		return api_type::template to_array_partial<lane_count>(value.to_native());
	}

	/**
	 * @brief Returns one compile-time-selected active lane.
	 * @tparam index Active logical lane index.
	 * @param value Partial register containing the selected lane.
	 * @return Copy of the selected active lane.
	 * @remarks Available exactly when `index < lane_count` and `IApi::Extract<api_type, index>` are satisfied.
	 */
	template <std::size_t index>
		requires(index < lane_count) && IApi::Extract<api_type, index>
	[[nodiscard]] constexpr element_type SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) lane(this PartialRegister value) noexcept
	{
		if consteval
		{
			return value.to_array()[index];
		}
		return api_type::template extract<static_cast<int>(index)>(value.to_native());
	}

	/**
	 * @brief Returns a copy with one compile-time-selected active lane replaced.
	 * @tparam index Active logical lane index.
	 * @param value Partial register containing the lanes to copy.
	 * @param replacement Replacement value for the selected active lane.
	 * @return PartialRegister with lane `index` replaced and the inactive suffix unchanged.
	 * @remarks Available exactly when `index < lane_count` and `IApi::Insert<api_type, index>` are satisfied.
	 */
	template <std::size_t index>
		requires(index < lane_count) && IApi::Insert<api_type, index>
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		with_lane(this PartialRegister value, element_type replacement) noexcept
	{
		value.native = api_type::template insert<index>(value.to_native(), replacement);
		return value;
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
