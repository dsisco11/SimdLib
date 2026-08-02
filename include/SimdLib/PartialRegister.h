#pragma once

#include <SimdLib/Config.h>

#if !SIMDLIB_REGISTER_INTERFACE_AVAILABLE && !SIMDLIB_REQUIRE_REGISTER_INTERFACE
#error "SIMDLIB_PARTIAL_REGISTER_HEADER_REQUIRES_CXX23: <SimdLib/PartialRegister.h> requires C++23 explicit object parameter support"
#endif

#include <SimdLib/PartialRegisterFwd.h>
#include <SimdLib/Register.h>

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
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
	requires PartialRegisterAvailable<element_t, bits, active_lane_count>
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

  private:
	/** @brief Compile-time inactive-lane divisor identity used by division and modulus. */
	constexpr static inline std::array<element_type, native_lane_count> inactive_divisor_identity = []() constexpr
	{
		std::array<element_type, native_lane_count> result{};
		for (std::size_t lane = lane_count; lane < native_lane_count; ++lane)
			result[lane] = element_type{1};
		return result;
	}();

	/** @brief Compile-time inactive-lane maximum used to exclude the suffix from minimum-position searches. */
	constexpr static inline std::array<element_type, native_lane_count> inactive_min_position_identity = []() constexpr
	{
		std::array<element_type, native_lane_count> result{};
		for (std::size_t lane = lane_count; lane < native_lane_count; ++lane)
			result[lane] = std::numeric_limits<element_type>::max();
		return result;
	}();

	/** @brief Compile-time inactive-lane minimum used to exclude the suffix from maximum-position searches. */
	constexpr static inline std::array<element_type, native_lane_count> inactive_max_position_identity = []() constexpr
	{
		std::array<element_type, native_lane_count> result{};
		for (std::size_t lane = lane_count; lane < native_lane_count; ++lane)
			result[lane] = std::numeric_limits<element_type>::lowest();
		return result;
	}();

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

	/**
	 * @brief Wraps a lane-combining native result in its deliberate partial or complete result type.
	 * @tparam result_t Public result type selected by the corresponding result alias.
	 * @param native Native result produced by the source API operation.
	 * @return Complete result unchanged, or partial result with its inactive suffix cleared.
	 */
	template <class result_t, class result_native_t>
	[[nodiscard]] constexpr static result_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) make_specialized_result(result_native_t native) noexcept
	{
		if constexpr (result_t::lane_count == result_t::api_type::element_count)
			return result_t{native};
		else
			return result_t::from_native(native);
	}

	/**
	 * @brief Reports whether an API can shuffle an active selector list padded with one known-zero source lane.
	 * @tparam shuffle_api_t API whose logical lane type is being shuffled.
	 * @tparam active_count Number of meaningful low lanes or bytes in that API interpretation.
	 * @tparam indices Active-prefix source selectors.
	 * @tparam padding_indices Positions in the inactive result suffix.
	 * @return True when the expanded native-width selector list is supported by the API.
	 */
	template <class shuffle_api_t, std::size_t active_count, std::size_t... indices, std::size_t... padding_indices>
	[[nodiscard]] consteval static bool partial_shuffle_available(std::index_sequence<padding_indices...>) noexcept
	{
		return IApi::Shuffle<shuffle_api_t, indices..., ((void)padding_indices, active_count)...>;
	}

	/**
	 * @brief Applies an active-prefix shuffle while selecting the first inactive zero lane for every suffix output.
	 * @tparam shuffle_api_t API whose logical lane type is being shuffled.
	 * @tparam active_count Number of meaningful low lanes or bytes in that API interpretation.
	 * @tparam indices Active-prefix source selectors.
	 * @tparam padding_indices Positions in the inactive result suffix.
	 * @param value Native value with a zero suffix beginning at `active_count`.
	 * @return Native-width shuffled value whose inactive result suffix remains zero.
	 */
	template <class shuffle_api_t, std::size_t active_count, std::size_t... indices, std::size_t... padding_indices>
	[[nodiscard]] constexpr static typename shuffle_api_t::vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		partial_shuffle_native(typename shuffle_api_t::vector_t value, std::index_sequence<padding_indices...>) noexcept
		requires IApi::Shuffle<shuffle_api_t, indices..., ((void)padding_indices, active_count)...>
	{
		return shuffle_api_t::template shuffle<indices..., ((void)padding_indices, active_count)...>(value);
	}

	/**
	 * @brief Reports whether a dot-product immediate writes only active result lanes.
	 * @tparam imm8 Intrinsic dot-product control byte.
	 * @return True when every selected destination position is active in every physical 128-bit group.
	 */
	template <int imm8> [[nodiscard]] consteval static bool dot_product_outputs_are_active() noexcept
	{
		constexpr std::size_t group_lane_count = 128 / (sizeof(element_type) * 8);
		for (std::size_t lane = lane_count; lane < native_lane_count; ++lane)
			if ((imm8 & (1 << (lane % group_lane_count))) != 0)
				return false;
		return true;
	}

	/**
	 * @brief Validates division and modulus preconditions over active lanes only when checks are enabled.
	 * @param dividends Native dividend lanes with a zero inactive suffix.
	 * @param divisors Native divisor lanes with a zero inactive suffix.
	 */
	static void SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) validate_active_divisors(native_type dividends, native_type divisors) noexcept
	{
#if SIMDLIB_ENABLE_CHECKS
		using mask_t = typename api_type::mask_t;
		constexpr mask_t active_bits = (mask_t{1} << lane_count) - mask_t{1};
		const auto zero_divisor_bits = api_type::cmp_eq_slim(divisors, api_type::setzero()) & active_bits;
		SIMDLIB_PRECONDITION(zero_divisor_bits == 0, "PartialRegister active divisor lanes must be nonzero");
		if constexpr (std::is_integral_v<element_type> && std::is_signed_v<element_type>)
		{
			const auto minimum_bits = api_type::cmp_eq_slim(dividends, api_type::set1(std::numeric_limits<element_type>::lowest()));
			const auto negative_one_bits = api_type::cmp_eq_slim(divisors, api_type::set1(element_type{-1}));
			SIMDLIB_PRECONDITION((minimum_bits & negative_one_bits & active_bits) == 0, "PartialRegister signed minimum cannot be divided by negative one");
		}
#else
		(void)dividends;
		(void)divisors;
#endif
	}

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

  public:
	/**
	 * @brief Owns the partial native register represented by this aggregate.
	 * @pre Direct aggregate initialization must supply a value with a bitwise-zero inactive suffix.
	 */
	native_type native = api_type::setzero();

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
	[[nodiscard]] static PartialRegister SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load(std::span<const element_type, lane_count> source) noexcept
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
	[[nodiscard]] constexpr std::array<element_type, lane_count> SIMD_FLAGS(In, ForceInline, Flatten) to_array(this PartialRegister value) noexcept
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

#pragma region Arithmetic Operations

	/**
	 * @brief Adds corresponding active lanes and preserves the zero inactive suffix.
	 * @param lhs Left active-lane addends.
	 * @param rhs Right active-lane addends.
	 * @return Same-shaped partial register containing the active sums and inactive zeros.
	 * @remarks Available exactly when `IApi::Add<api_type>` is satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator+(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::Add<api_type>
	{
		return PartialRegister{api_type::add(lhs.native, rhs.native)};
	}

	/**
	 * @brief Subtracts corresponding active lanes and clears any inactive negative-zero representations.
	 * @param lhs Active-lane minuends.
	 * @param rhs Active-lane subtrahends.
	 * @return Same-shaped partial register containing the active differences and inactive bitwise zeros.
	 * @remarks Floating results require suffix normalization. Available when the listed `IApi` contracts are satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator-(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::Subtract<api_type> && (!std::is_floating_point_v<element_type> || IApi::BitwiseAnd<api_type>)
	{
		const auto result = api_type::subtract(lhs.native, rhs.native);
		if constexpr (std::is_floating_point_v<element_type>)
			return PartialRegister{normalize_native(result)};
		else
			return PartialRegister{result};
	}

	/**
	 * @brief Multiplies corresponding active lanes and preserves the zero inactive suffix.
	 * @param lhs Left active-lane factors.
	 * @param rhs Right active-lane factors.
	 * @return Same-shaped partial register containing the active products and inactive zeros.
	 * @remarks Available exactly when `IApi::Multiply<api_type>` is satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator*(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::Multiply<api_type>
	{
		return PartialRegister{api_type::multiply(lhs.native, rhs.native)};
	}

	/**
	 * @brief Divides corresponding active lanes after replacing inactive divisors with one.
	 * @param lhs Active-lane dividends.
	 * @param rhs Active-lane divisors.
	 * @return Same-shaped partial register containing active quotients and inactive bitwise zeros.
	 * @pre Every active divisor is nonzero and signed minimum is not divided by negative one.
	 * @remarks Available when the listed `IApi` contracts are satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator/(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::Divide<api_type> && IApi::BitwiseOr<api_type> && IApi::BitwiseAnd<api_type>
	{
		validate_active_divisors(lhs.native, rhs.native);
		const auto divisors = api_type::bitwise_or(rhs.native, api_type::construct(PartialRegister::inactive_divisor_identity));
		return PartialRegister{normalize_native(api_type::divide(lhs.native, divisors))};
	}

	/**
	 * @brief Computes corresponding active-lane remainders after replacing inactive divisors with one.
	 * @param lhs Active-lane dividends.
	 * @param rhs Active-lane divisors.
	 * @return Same-shaped partial register containing active remainders and inactive bitwise zeros.
	 * @pre Every active divisor is nonzero and signed minimum is not divided by negative one.
	 * @remarks Available when the listed `IApi` contracts are satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator%(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::Modulus<api_type> && IApi::BitwiseOr<api_type> && IApi::BitwiseAnd<api_type>
	{
		validate_active_divisors(lhs.native, rhs.native);
		const auto divisors = api_type::bitwise_or(rhs.native, api_type::construct(PartialRegister::inactive_divisor_identity));
		return PartialRegister{normalize_native(api_type::modulus(lhs.native, divisors))};
	}

	/**
	 * @brief Negates every active lane and clears negative-zero bit patterns from the inactive suffix.
	 * @param value Active lanes to negate.
	 * @return Same-shaped partial register containing active negations and inactive bitwise zeros.
	 * @remarks Available when `IApi::Negate<api_type>` and `IApi::BitwiseAnd<api_type>` are satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator-(this PartialRegister value) noexcept
		requires IApi::Negate<api_type> && IApi::BitwiseAnd<api_type>
	{
		return PartialRegister{normalize_native(api_type::negate(value.native))};
	}

#pragma endregion
#pragma region Specialized Arithmetic and Reductions

	/**
	 * @brief Selects the intrinsic-defined minimum for each active lane and clears the suffix.
	 * @param lhs First active-lane candidates.
	 * @param rhs Second active-lane candidates.
	 * @return Same-shaped partial register containing active minima and inactive bitwise zeros.
	 * @remarks Available when `IApi::Min<api_type>` and `IApi::BitwiseAnd<api_type>` are satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) min(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::Min<api_type>
	{
		return PartialRegister{api_type::min(lhs.native, rhs.native)};
	}

	/**
	 * @brief Selects the intrinsic-defined maximum for each active lane and clears the suffix.
	 * @param lhs First active-lane candidates.
	 * @param rhs Second active-lane candidates.
	 * @return Same-shaped partial register containing active maxima and inactive bitwise zeros.
	 * @remarks Available when `IApi::Max<api_type>` and `IApi::BitwiseAnd<api_type>` are satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) max(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::Max<api_type>
	{
		return PartialRegister{api_type::max(lhs.native, rhs.native)};
	}

	/**
	 * @brief Computes the absolute value of each active lane and preserves inactive zeros.
	 * @param value Active lanes whose absolute values are requested.
	 * @return Same-shaped partial register containing active absolute values and inactive zeros.
	 * @remarks Available exactly when `IApi::Absolute<api_type>` is satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(this PartialRegister value) noexcept
		requires IApi::Absolute<api_type>
	{
		return PartialRegister{api_type::absolute(value.native)};
	}

	/**
	 * @brief Computes the square root of each active lane and preserves inactive positive zeros.
	 * @param value Active lanes whose square roots are requested.
	 * @return Same-shaped partial register containing active square roots and inactive positive zeros.
	 * @remarks Exceptional active inputs follow the API contract. Available exactly when `IApi::Sqrt<api_type>` is satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(this PartialRegister value) noexcept
		requires IApi::Sqrt<api_type>
	{
		return PartialRegister{api_type::sqrt(value.native)};
	}

	/**
	 * @brief Computes the intrinsic-defined average of corresponding active lanes.
	 * @param lhs Left active-lane inputs.
	 * @param rhs Right active-lane inputs.
	 * @return Same-shaped partial register containing active averages and inactive zeros.
	 * @remarks Rounding follows the API contract. Available exactly when `IApi::Average<api_type>` is satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) average(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::Average<api_type>
	{
		return PartialRegister{api_type::avg(lhs.native, rhs.native)};
	}

	/**
	 * @brief Multiplies corresponding active lanes and adds the corresponding active addend.
	 * @param lhs Left active-lane multiplicands.
	 * @param rhs Right active-lane multiplicands.
	 * @param addend Active-lane addends.
	 * @return Same-shaped partial register containing active multiply-add results and inactive zeros.
	 * @remarks Fusion follows the API configuration. Available exactly when `IApi::MultiplyAdd<api_type>` is satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		multiply_add(this PartialRegister lhs, PartialRegister rhs, PartialRegister addend) noexcept
		requires IApi::MultiplyAdd<api_type>
	{
		return PartialRegister{api_type::multiply_add(lhs.native, rhs.native, addend.native)};
	}

	/**
	 * @brief Computes the Register-defined magnitude for each 128-bit group containing active lanes.
	 * @param value Active lanes contributing to each grouped magnitude; inactive lanes contribute zero.
	 * @return Same-shaped partial result with the API-defined active layout and inactive bitwise zeros.
	 * @pre Every active integer-group magnitude is representable in `element_type`.
	 * @remarks Available when `IApi::Magnitude<api_type>` and `IApi::BitwiseAnd<api_type>` are satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(this PartialRegister value) noexcept
		requires IApi::Magnitude<api_type> && IApi::BitwiseAnd<api_type>
	{
		return PartialRegister{normalize_native(api_type::magnitude(value.native))};
	}

	/**
	 * @brief Computes checked integer magnitudes and retains the overflow lane of every occupied 128-bit group.
	 * @tparam source_element_t Deferred source type used to constrain result availability.
	 * @param value Active lanes contributing to each grouped checked magnitude.
	 * @return A partial or complete result ending after the final occupied group's overflow lane.
	 * @remarks Available when `IApi::MagnitudeChecked<api_type>` and `IApi::BitwiseAnd<api_type>` are satisfied.
	 */
	template <class source_element_t = element_type>
		requires std::same_as<source_element_t, element_type> && IApi::MagnitudeChecked<api_type> && IApi::BitwiseAnd<api_type>
	[[nodiscard]] partial_magnitude_checked_result_t<source_element_t, register_width, lane_count> SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		magnitude_checked(this PartialRegister value) noexcept
	{
		using result_t = partial_magnitude_checked_result_t<source_element_t, register_width, lane_count>;
		return PartialRegister::template make_specialized_result<result_t>(api_type::magnitude_checked(value.native));
	}

	/**
	 * @brief Normalizes active floating lanes by their active 128-bit-group magnitude and clears the suffix.
	 * @param value Floating-point source whose active group prefixes determine each magnitude.
	 * @return Active lanes divided by their group magnitude with an all-bits-zero inactive suffix.
	 * @remarks Inactive divisors are neutralized at the abstract operation boundary. Floating-environment status follows the
	 * selected compiler model, matching Register. Available when the listed `IApi` contracts are satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) normalize(this PartialRegister value) noexcept
		requires IApi::Normalize<api_type> && IApi::BitwiseAnd<api_type>
	{
		return PartialRegister{normalize_native(api_type::normalize(value.native))};
	}

	/**
	 * @brief Returns the low logical prefix of intrinsic-ordered adjacent-pair sums.
	 * @param lhs Supplies the first intrinsic-ordered active results.
	 * @param rhs Supplies the remaining intrinsic-ordered active results.
	 * @return Same-shaped partial result with its inactive suffix cleared.
	 * @remarks Available when `IApi::HorizontalAdd<api_type>` and `IApi::BitwiseAnd<api_type>` are satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) horizontal_add(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::HorizontalAdd<api_type> && IApi::BitwiseAnd<api_type>
	{
		return PartialRegister{normalize_native(api_type::add_horizontal(lhs.native, rhs.native))};
	}

	/**
	 * @brief Returns the low logical prefix of intrinsic-ordered adjacent-pair differences.
	 * @param lhs Supplies the first intrinsic-ordered active results.
	 * @param rhs Supplies the remaining intrinsic-ordered active results.
	 * @return Same-shaped partial result with its inactive suffix cleared.
	 * @remarks Available when `IApi::HorizontalSubtract<api_type>` and `IApi::BitwiseAnd<api_type>` are satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		horizontal_subtract(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::HorizontalSubtract<api_type> && IApi::BitwiseAnd<api_type>
	{
		return PartialRegister{normalize_native(api_type::subtract_horizontal(lhs.native, rhs.native))};
	}

	/**
	 * @brief Multiplies adjacent active integral lane pairs, pairing an unmatched final lane with zero.
	 * @tparam source_element_t Deferred source type used to constrain result availability.
	 * @param lhs Left active-lane factors.
	 * @param rhs Right active-lane factors.
	 * @return Contiguous promoted partial or complete result with `ceil(lane_count / 2)` logical lanes.
	 * @remarks Available exactly when the listed source and `IApi::MultiplyAddAdjacent` constraints are satisfied.
	 */
	template <class source_element_t = element_type>
		requires std::same_as<source_element_t, element_type> && std::is_integral_v<source_element_t> && IApi::MultiplyAddAdjacent<api_type>
	[[nodiscard]] partial_multiply_add_adjacent_result_t<source_element_t, register_width, lane_count> SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten)
		multiply_add_adjacent(this PartialRegister lhs, PartialRegister rhs) noexcept
	{
		using result_t = partial_multiply_add_adjacent_result_t<source_element_t, register_width, lane_count>;
		return PartialRegister::template make_specialized_result<result_t>(api_type::multiply_add_adjacent(lhs.native, rhs.native));
	}

	/**
	 * @brief Multiplies unsigned and signed active byte pairs, pairing an unmatched final byte with zero.
	 * @tparam source_element_t Deferred source type used to constrain result availability.
	 * @param lhs Unsigned active-byte multiplicands.
	 * @param rhs Signed active-byte multiplicands.
	 * @return Contiguous signed 16-bit partial or complete result with `ceil(active_byte_count / 2)` logical lanes.
	 * @remarks Available exactly when the listed source and `IApi::ByteMultiplyAdd` constraints are satisfied.
	 */
	template <class source_element_t = element_type>
		requires std::same_as<source_element_t, element_type> && std::is_integral_v<source_element_t> && IApi::ByteMultiplyAdd<api_type>
	[[nodiscard]] partial_byte_multiply_add_result_t<source_element_t, register_width, lane_count> SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten)
		multiply_add_unsigned_signed_bytes(this PartialRegister lhs, PartialRegister rhs) noexcept
	{
		using result_t = partial_byte_multiply_add_result_t<source_element_t, register_width, lane_count>;
		return PartialRegister::template make_specialized_result<result_t>(api_type::multiply_add_unsigned_signed_bytes(lhs.native, rhs.native));
	}

	/**
	 * @brief Sums active byte-wise absolute differences into contiguous unsigned 64-bit groups.
	 * @tparam source_element_t Deferred source type used to constrain result availability.
	 * @param lhs Left active source bytes.
	 * @param rhs Right active source bytes.
	 * @return Contiguous unsigned 64-bit partial or complete result with `ceil(active_byte_count / 8)` logical lanes.
	 * @remarks Inactive source bytes contribute zero. Available exactly when the listed source and `IApi::Sad` constraints are satisfied.
	 */
	template <class source_element_t = element_type>
		requires std::same_as<source_element_t, element_type> && std::is_integral_v<source_element_t> && IApi::Sad<api_type>
	[[nodiscard]] partial_sad_result_t<source_element_t, register_width, lane_count> SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten)
		sum_absolute_byte_differences(this PartialRegister lhs, PartialRegister rhs) noexcept
	{
		using result_t = partial_sad_result_t<source_element_t, register_width, lane_count>;
		return PartialRegister::template make_specialized_result<result_t>(api_type::sum_absolute_byte_differences(lhs.native, rhs.native));
	}

	/**
	 * @brief Computes immediate-controlled multi-SAD with inactive source bytes fixed at zero.
	 * @tparam imm8 Immediate selector in the API-defined range.
	 * @tparam source_element_t Deferred source type used to constrain result availability.
	 * @param lhs Left active source bytes.
	 * @param rhs Right active source bytes.
	 * @return Complete Register preserving the API's noncontiguous output layout.
	 * @remarks Available exactly when the listed source and `IApi::MultiSad` constraints are satisfied.
	 */
	template <int imm8, class source_element_t = element_type>
		requires(imm8 >= 0 && imm8 <= 255 && std::same_as<source_element_t, element_type> && std::is_integral_v<source_element_t> &&
				 IApi::MultiSad<api_type, imm8>)
	[[nodiscard]] multi_sad_result_t<source_element_t, register_width> SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten)
		multi_sum_absolute_byte_differences(this PartialRegister lhs, PartialRegister rhs) noexcept
	{
		return multi_sad_result_t<source_element_t, register_width>{api_type::template multi_sum_absolute_byte_differences<imm8>(lhs.native, rhs.native)};
	}

	/**
	 * @brief Returns the first active logical lane containing the minimum integral value.
	 * @param value Active integral lanes to search; the inactive suffix is excluded.
	 * @return First minimum index in `[0, lane_count)`.
	 * @remarks Available when `IApi::MinPosition<api_type>` and `IApi::BitwiseOr<api_type>` are satisfied.
	 */
	[[nodiscard]] constexpr std::size_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(this PartialRegister value) noexcept
		requires IApi::MinPosition<api_type> && IApi::BitwiseOr<api_type>
	{
		return api_type::min_position(api_type::bitwise_or(value.native, api_type::construct(PartialRegister::inactive_min_position_identity)));
	}

	/**
	 * @brief Returns the first active logical lane containing the maximum integral value.
	 * @param value Active integral lanes to search; the inactive suffix is excluded.
	 * @return First maximum index in `[0, lane_count)`.
	 * @remarks Available when `IApi::MaxPosition<api_type>` and `IApi::BitwiseOr<api_type>` are satisfied.
	 */
	[[nodiscard]] constexpr std::size_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) max_position(this PartialRegister value) noexcept
		requires IApi::MaxPosition<api_type> && IApi::BitwiseOr<api_type>
	{
		return api_type::max_position(api_type::bitwise_or(value.native, api_type::construct(PartialRegister::inactive_max_position_identity)));
	}

	/**
	 * @brief Adds corresponding active lanes with intrinsic saturation.
	 * @param lhs Left active-lane addends.
	 * @param rhs Right active-lane addends.
	 * @return Same-shaped saturated result with an inactive zero suffix.
	 * @remarks Available exactly when `IApi::AddSaturated<api_type>` is satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_saturated(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::AddSaturated<api_type>
	{
		return PartialRegister{api_type::add_saturated(lhs.native, rhs.native)};
	}

	/**
	 * @brief Subtracts corresponding active lanes with intrinsic saturation.
	 * @param lhs Active-lane minuends.
	 * @param rhs Active-lane subtrahends.
	 * @return Same-shaped saturated result with an inactive zero suffix.
	 * @remarks Available exactly when `IApi::SubtractSaturated<api_type>` is satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		subtract_saturated(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::SubtractSaturated<api_type>
	{
		return PartialRegister{api_type::subtract_saturated(lhs.native, rhs.native)};
	}

	/**
	 * @brief Returns the low logical prefix of intrinsic-ordered saturated adjacent-pair sums.
	 * @param lhs Supplies the first intrinsic-ordered active results.
	 * @param rhs Supplies the remaining intrinsic-ordered active results.
	 * @return Same-shaped saturated partial result with its inactive suffix cleared.
	 * @remarks Available when `IApi::HorizontalAddSaturated<api_type>` and `IApi::BitwiseAnd<api_type>` are satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		horizontal_add_saturated(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::HorizontalAddSaturated<api_type> && IApi::BitwiseAnd<api_type>
	{
		return PartialRegister{normalize_native(api_type::hadd_saturated(lhs.native, rhs.native))};
	}

	/**
	 * @brief Returns the low logical prefix of intrinsic-ordered saturated adjacent-pair differences.
	 * @param lhs Supplies the first intrinsic-ordered active results.
	 * @param rhs Supplies the remaining intrinsic-ordered active results.
	 * @return Same-shaped saturated partial result with its inactive suffix cleared.
	 * @remarks Available when `IApi::HorizontalSubtractSaturated<api_type>` and `IApi::BitwiseAnd<api_type>` are satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		horizontal_subtract_saturated(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::HorizontalSubtractSaturated<api_type> && IApi::BitwiseAnd<api_type>
	{
		return PartialRegister{normalize_native(api_type::hsubtract_saturated(lhs.native, rhs.native))};
	}

	/**
	 * @brief Alternates subtraction and addition across active floating lanes and clears the suffix.
	 * @param lhs Left active-lane inputs.
	 * @param rhs Right active-lane inputs.
	 * @return Same-shaped intrinsic-ordered result with inactive bitwise-zero lanes.
	 * @remarks Lane polarity repeats per 128-bit group. Available when the listed `IApi` contracts are satisfied.
	 */
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_subtract(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::AddSubtract<api_type> && IApi::BitwiseAnd<api_type>
	{
		return PartialRegister{normalize_native(api_type::add_subtract(lhs.native, rhs.native))};
	}

	/**
	 * @brief Computes an immediate-controlled dot product and clears every inactive output lane.
	 * @tparam imm8 API-defined source and destination selection control.
	 * @param lhs Left active-lane factors.
	 * @param rhs Right active-lane factors.
	 * @return Same-shaped partial result when every selected destination lane is active.
	 * @remarks Inactive input lanes contribute zero. Available only when the listed API and active-output constraints are satisfied.
	 */
	template <int imm8>
		requires IApi::DotProduct<api_type, imm8> && (PartialRegister::template dot_product_outputs_are_active<imm8>())
	[[nodiscard]] PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) dot_product(this PartialRegister lhs, PartialRegister rhs) noexcept
	{
		return PartialRegister{api_type::template dot_product<imm8>(lhs.native, rhs.native)};
	}

#pragma endregion

#pragma region Bitwise Operations

	/**
	 * @brief Computes the bitwise intersection of corresponding active lanes.
	 * @param lhs Left active bit pattern.
	 * @param rhs Right active bit pattern.
	 * @return Same-shaped partial register containing `lhs & rhs` and an inactive zero suffix.
	 * @remarks Available exactly when `IApi::BitwiseAnd<api_type>` is satisfied.
	 */
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator&(this PartialRegister lhs,
																											PartialRegister rhs) noexcept
		requires IApi::BitwiseAnd<api_type>
	{
		return PartialRegister{api_type::bitwise_and(lhs.native, rhs.native)};
	}

	/**
	 * @brief Computes the bitwise union of corresponding active lanes.
	 * @param lhs Left active bit pattern.
	 * @param rhs Right active bit pattern.
	 * @return Same-shaped partial register containing `lhs | rhs` and an inactive zero suffix.
	 * @remarks Available exactly when `IApi::BitwiseOr<api_type>` is satisfied.
	 */
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator|(this PartialRegister lhs,
																											PartialRegister rhs) noexcept
		requires IApi::BitwiseOr<api_type>
	{
		return PartialRegister{api_type::bitwise_or(lhs.native, rhs.native)};
	}

	/**
	 * @brief Computes the bitwise exclusive union of corresponding active lanes.
	 * @param lhs Left active bit pattern.
	 * @param rhs Right active bit pattern.
	 * @return Same-shaped partial register containing `lhs ^ rhs` and an inactive zero suffix.
	 * @remarks Available exactly when `IApi::BitwiseXor<api_type>` is satisfied.
	 */
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator^(this PartialRegister lhs,
																											PartialRegister rhs) noexcept
		requires IApi::BitwiseXor<api_type>
	{
		return PartialRegister{api_type::bitwise_xor(lhs.native, rhs.native)};
	}

	/**
	 * @brief Complements every active bit and clears every inactive bit.
	 * @param value Active bit pattern to complement.
	 * @return Same-shaped complemented partial register with an inactive zero suffix.
	 * @remarks Available when `IApi::BitwiseNot<api_type>` and `IApi::BitwiseAnd<api_type>` are satisfied.
	 */
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator~(this PartialRegister value) noexcept
		requires IApi::BitwiseNot<api_type> && IApi::BitwiseAnd<api_type>
	{
		return PartialRegister{normalize_native(api_type::bitwise_not(value.native))};
	}

	/**
	 * @brief Computes `(~lhs) & rhs` for corresponding active bits.
	 * @param lhs Active bit pattern complemented before intersection.
	 * @param rhs Active bit pattern intersected with the complemented left operand.
	 * @return Same-shaped partial register containing `(~lhs) & rhs` and inactive zeros.
	 * @remarks Available exactly when `IApi::BitwiseAndNot<api_type>` is satisfied.
	 */
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) andnot(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::BitwiseAndNot<api_type>
	{
		return PartialRegister{api_type::bitwise_andnot(lhs.native, rhs.native)};
	}

	/**
	 * @brief Returns the API's native-granularity sign-bit mask for the active payload.
	 * @param value Partial register whose active sign bits are observed.
	 * @return Native-granularity scalar mask with no bits sourced from inactive bytes.
	 * @remarks Available exactly when `IApi::Movemask<api_type>` is satisfied.
	 */
	[[nodiscard]] constexpr typename api_type::mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) movemask(this PartialRegister value) noexcept
		requires IApi::Movemask<api_type>
	{
		return api_type::movemask(value.native);
	}

	/**
	 * @brief Returns one sign bit for every active logical lane.
	 * @param value Partial register whose active lane sign bits are observed.
	 * @return Compact scalar mask whose unused high bits are zero.
	 * @remarks Available exactly when `IApi::MovemaskSlim<api_type>` is satisfied.
	 */
	[[nodiscard]] constexpr typename api_type::mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) lane_sign_bits(this PartialRegister value) noexcept
		requires IApi::MovemaskSlim<api_type>
	{
		return api_type::movemask_slim(value.native);
	}

#pragma endregion

#pragma region Shifting Operations

	/**
	 * @brief Left-shifts every active integral lane with zero fill.
	 * @param value Active integral lanes to shift.
	 * @param count Runtime shift count applied to each active lane.
	 * @return Same-shaped shifted result with an inactive zero suffix.
	 * @pre `count >= 0`; counts at least the lane width produce zero lanes.
	 * @remarks Available exactly when `IApi::ShiftLeft<api_type>` is satisfied.
	 */
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator<<(this PartialRegister value, int count) noexcept
		requires IApi::ShiftLeft<api_type>
	{
		return PartialRegister{api_type::shift_left(value.native, count)};
	}

	/**
	 * @brief Right-shifts every active integral lane with zero fill.
	 * @param value Active integral lanes to shift.
	 * @param count Runtime shift count applied to each active lane.
	 * @return Same-shaped shifted result with an inactive zero suffix.
	 * @pre `count >= 0`; counts at least the lane width produce zero lanes.
	 * @remarks Available exactly when `IApi::ShiftRight<api_type>` is satisfied.
	 */
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		logical_shift_right(this PartialRegister value, int count) noexcept
		requires IApi::ShiftRight<api_type>
	{
		return PartialRegister{api_type::shift_right(value.native, count)};
	}

	/**
	 * @brief Right-shifts unsigned active lanes logically and signed active lanes arithmetically.
	 * @param value Active integral lanes to shift.
	 * @param count Runtime shift count applied to each active lane.
	 * @return Same-shaped signedness-selected result with an inactive zero suffix.
	 * @pre `count >= 0`; oversized signed counts clamp and unsigned counts produce zero lanes.
	 */
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) operator>>(this PartialRegister value, int count) noexcept
		requires((std::is_signed_v<element_type> && IApi::ArithmeticShiftRight<api_type>) || (std::is_unsigned_v<element_type> && IApi::ShiftRight<api_type>))
	{
		if constexpr (std::is_signed_v<element_type>)
			return PartialRegister{api_type::shift_right_arithmetic(value.native, count)};
		else
			return PartialRegister{api_type::shift_right(value.native, count)};
	}

	/**
	 * @brief Runtime-shifts the logical active byte payload toward higher indices.
	 * @param value Active byte payload to shift.
	 * @param count Runtime byte count; nonpositive values are identity and counts at least the active extent produce zero.
	 * @return Logical active-byte shift with discarded overflow and inactive zeros.
	 */
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		shift_bytes_left_slow(this PartialRegister value, int count) noexcept
		requires(register_width == 128 && IApi::ShiftBytesSlow<api_type> && IApi::BitwiseAnd<api_type>)
	{
		return PartialRegister{normalize_native(api_type::shift_bytes_left_slow(value.native, count))};
	}

	/**
	 * @brief Compile-time shifts the logical active byte payload toward higher indices.
	 * @tparam count Nonnegative byte count.
	 * @param value Active byte payload to shift.
	 * @return Logical active-byte shift with discarded overflow and inactive zeros.
	 */
	template <int count>
		requires(count >= 0 && IApi::ShiftBytesLeft<api_type, count> && IApi::BitwiseAnd<api_type>)
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shift_bytes_left(this PartialRegister value) noexcept
	{
		return PartialRegister{normalize_native(api_type::template shift_bytes_left<count>(value.native))};
	}

	/**
	 * @brief Runtime-shifts the logical active byte payload toward lower indices.
	 * @param value Active byte payload to shift.
	 * @param count Runtime byte count; nonpositive values are identity and counts at least the active extent produce zero.
	 * @return Logical active-byte shift with zero-filled high logical bytes.
	 */
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		shift_bytes_right_slow(this PartialRegister value, int count) noexcept
		requires(register_width == 128 && IApi::ShiftBytesSlow<api_type>)
	{
		return PartialRegister{api_type::shift_bytes_right_slow(value.native, count)};
	}

	/**
	 * @brief Compile-time shifts the logical active byte payload toward lower indices.
	 * @tparam count Nonnegative byte count.
	 * @param value Active byte payload to shift.
	 * @return Logical active-byte shift with zero-filled high logical bytes.
	 */
	template <int count>
		requires(count >= 0 && IApi::ShiftBytesRight<api_type, count>)
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shift_bytes_right(this PartialRegister value) noexcept
	{
		return PartialRegister{api_type::template shift_bytes_right<count>(value.native)};
	}

	/**
	 * @brief Runtime-shifts the logical active bit payload toward higher indices.
	 * @param value Active bit payload to shift.
	 * @param count Runtime bit count; nonpositive values are identity and counts at least the active extent produce zero.
	 * @return Logical active-bit shift with discarded overflow and inactive zeros.
	 */
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		shift_bits_left_slow(this PartialRegister value, int count) noexcept
		requires(register_width == 128 && IApi::ShiftBitsSlow<api_type> && IApi::BitwiseAnd<api_type>)
	{
		return PartialRegister{normalize_native(api_type::shift_bits_left_slow(value.native, count))};
	}

	/**
	 * @brief Runtime-shifts the logical active bit payload toward lower indices.
	 * @param value Active bit payload to shift.
	 * @param count Runtime bit count; nonpositive values are identity and counts at least the active extent produce zero.
	 * @return Logical active-bit shift with zero-filled high logical bits.
	 */
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		shift_bits_right_slow(this PartialRegister value, int count) noexcept
		requires(register_width == 128 && IApi::ShiftBitsSlow<api_type>)
	{
		return PartialRegister{api_type::shift_bits_right_slow(value.native, count)};
	}

	/**
	 * @brief Compile-time shifts the logical active bit payload toward higher indices.
	 * @tparam count Nonnegative bit count.
	 * @param value Active bit payload to shift.
	 * @return Logical active-bit shift with discarded overflow and inactive zeros.
	 */
	template <int count>
		requires(register_width == 128 && count >= 0 && IApi::ShiftBits<api_type, count> && IApi::BitwiseAnd<api_type>)
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shift_bits_left(this PartialRegister value) noexcept
	{
		return PartialRegister{normalize_native(api_type::template shift_bits_left<count>(value.native))};
	}

	/**
	 * @brief Compile-time shifts the logical active bit payload toward lower indices.
	 * @tparam count Nonnegative bit count.
	 * @param value Active bit payload to shift.
	 * @return Logical active-bit shift with zero-filled high logical bits.
	 */
	template <int count>
		requires(register_width == 128 && count >= 0 && IApi::ShiftBits<api_type, count>)
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shift_bits_right(this PartialRegister value) noexcept
	{
		return PartialRegister{api_type::template shift_bits_right<count>(value.native)};
	}

#pragma endregion

#pragma region Rearrangement and Conversion Operations

	/**
	 * @brief Extracts the meaningful low 128-bit half of a 256-bit partial register.
	 * @param value Source value in logical low-to-high lane order.
	 * @return Complete 128-bit Register containing the meaningful low half.
	 * @remarks Valid 256-bit PartialRegister geometries always reach the upper half, so the retained low half is complete.
	 */
	[[nodiscard]] constexpr auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) lower_half(this PartialRegister value) noexcept
		requires(register_width == 256 && IApi::LowerHalf<api_type>)
	{
		using result_t = partial_lower_half_result_t<element_type, lane_count>;
		return result_t{api_type::lower_half(value.native)};
	}

	/**
	 * @brief Interleaves each native 128-bit group's low source lanes and retains the low logical result prefix.
	 * @param lhs Supplies even-numbered lanes in each intrinsic result group.
	 * @param rhs Supplies odd-numbered lanes in each intrinsic result group.
	 * @return Same-shaped partial register containing the first `lane_count` intrinsic result lanes and an inactive zero suffix.
	 * @remarks Missing source lanes are the operands' invariant-preserving zeros.
	 */
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		unpack_low(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::UnpackLow<api_type> && IApi::BitwiseAnd<api_type>
	{
		return PartialRegister{normalize_native(api_type::unpack_lo(lhs.native, rhs.native))};
	}

	/**
	 * @brief Interleaves each native 128-bit group's high source lanes and retains the low logical result prefix.
	 * @param lhs Supplies even-numbered lanes in each intrinsic result group.
	 * @param rhs Supplies odd-numbered lanes in each intrinsic result group.
	 * @return Same-shaped partial register containing the first `lane_count` intrinsic result lanes and an inactive zero suffix.
	 * @remarks Missing source lanes are the operands' invariant-preserving zeros.
	 */
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		unpack_high(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::UnpackHigh<api_type>
	{
		return PartialRegister{api_type::unpack_hi(lhs.native, rhs.native)};
	}

	/**
	 * @brief Rearranges the logical active prefix with one compile-time source selector per result lane.
	 * @tparam indices Exactly `lane_count` source indices, each strictly less than `lane_count`.
	 * @param value Source partial register.
	 * @return Same-shaped partial register containing the selected active lanes and a zero inactive suffix.
	 * @remarks Padding cannot be requested explicitly; every logical result lane selects an active source lane.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == lane_count && ((indices < lane_count) && ...) &&
				 partial_shuffle_available<api_type, lane_count, indices...>(std::make_index_sequence<native_lane_count - lane_count>{}))
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(this PartialRegister value) noexcept
	{
		return PartialRegister{
			partial_shuffle_native<api_type, lane_count, indices...>(value.native, std::make_index_sequence<native_lane_count - lane_count>{})};
	}

	/**
	 * @brief Rearranges the logical active byte prefix with one compile-time source selector per result byte.
	 * @tparam indices Exactly `active_byte_count` source-byte indices, each strictly less than `active_byte_count`.
	 * @param value Source partial register.
	 * @return Same-shaped partial register containing the selected active bytes and a zero inactive suffix.
	 * @remarks Padding cannot be requested explicitly; every logical result byte selects an active source byte.
	 */
	template <std::size_t... indices>
		requires(sizeof...(indices) == active_byte_count && ((indices < active_byte_count) && ...) && IApi::BitCast<api_type, std::uint8_t> &&
				 IApi::BitCast<Api<register_width, std::uint8_t>, element_type> &&
				 partial_shuffle_available<Api<register_width, std::uint8_t>, active_byte_count, indices...>(
					 std::make_index_sequence<byte_count - active_byte_count>{}))
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle_bytes(this PartialRegister value) noexcept
	{
		using byte_api_type = Api<register_width, std::uint8_t>;
		const auto bytes = api_type::template bit_cast<std::uint8_t>(value.native);
		const auto shuffled =
			partial_shuffle_native<byte_api_type, active_byte_count, indices...>(bytes, std::make_index_sequence<byte_count - active_byte_count>{});
		return PartialRegister{byte_api_type::template bit_cast<element_type>(shuffled)};
	}

	/**
	 * @brief Applies Register's immediate-controlled low four-lane shuffle in each native 128-bit group.
	 * @tparam imm8 Immediate selector in the inclusive range `0..255`.
	 * @param value Source 16-bit partial register.
	 * @return Same-shaped projected intrinsic result with an inactive zero suffix.
	 * @remarks An active output may select an inactive source lane, in which case that logical output is zero.
	 */
	template <int imm8>
		requires IApi::ShuffleLow<api_type, imm8> && IApi::BitwiseAnd<api_type>
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle_low(this PartialRegister value) noexcept
	{
		return PartialRegister{normalize_native(api_type::template shuffle_lo<imm8>(value.native))};
	}

	/**
	 * @brief Applies Register's immediate-controlled high four-lane shuffle in each native 128-bit group.
	 * @tparam imm8 Immediate selector in the inclusive range `0..255`.
	 * @param value Source 16-bit partial register.
	 * @return Same-shaped projected intrinsic result with an inactive zero suffix.
	 * @remarks An active output may select an inactive source lane, in which case that logical output is zero.
	 */
	template <int imm8>
		requires IApi::ShuffleHigh<api_type, imm8> && IApi::BitwiseAnd<api_type>
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle_high(this PartialRegister value) noexcept
	{
		return PartialRegister{normalize_native(api_type::template shuffle_hi<imm8>(value.native))};
	}

	/**
	 * @brief Selects corresponding active lanes from two partial registers with Register's immediate-mask semantics.
	 * @tparam imm8 Immediate control in the inclusive range `0..255`; set applicable bits select `rhs`.
	 * @param lhs Source selected by cleared applicable control bits.
	 * @param rhs Source selected by set applicable control bits.
	 * @return Same-shaped blended result whose inactive suffix remains zero.
	 * @remarks A 256-bit 16-bit blend repeats the eight control bits in each 128-bit group.
	 */
	template <int imm8>
		requires IApi::Blend<api_type, imm8>
	[[nodiscard]] constexpr PartialRegister SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) blend(this PartialRegister lhs, PartialRegister rhs) noexcept
	{
		return PartialRegister{api_type::template blend<imm8>(lhs.native, rhs.native)};
	}

	/**
	 * @brief Reinterprets the active source bits as complete target lanes at the same native width.
	 * @tparam target_t Destination lane type whose size exactly divides `active_byte_count`.
	 * @param value Source partial register.
	 * @return Partial or complete result exposing exactly `active_byte_count / sizeof(target_t)` meaningful target lanes.
	 * @remarks Target types requiring a fractional logical lane are unavailable; no padding rule is applied.
	 */
	template <class target_t>
		requires RegisterAvailable<target_t, register_width> && (active_byte_count % sizeof(target_t) == 0) && IApi::BitCast<api_type, target_t>
	[[nodiscard]] constexpr auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bit_cast(this PartialRegister value) noexcept
	{
		using result_t = partial_bit_cast_result_t<element_type, target_t, register_width, lane_count>;
		return result_t{api_type::template bit_cast<target_t>(value.native)};
	}

	/**
	 * @brief Numerically converts each active source lane into one corresponding target lane.
	 * @tparam target_t Explicit numeric destination lane type supported by the source API.
	 * @param value Source partial register.
	 * @return Partial or complete result exposing exactly `lane_count` converted target lanes.
	 * @remarks Inactive positive-zero inputs convert to positive-zero target lanes and remain unobservable.
	 */
	template <class target_t>
		requires RegisterAvailable<target_t, register_width> && (lane_count <= Api<register_width, target_t>::element_count) &&
				 (lane_count == Api<register_width, target_t>::element_count || PartialRegisterAvailable<target_t, register_width, lane_count>) &&
				 IApi::Convert<api_type, target_t>
	[[nodiscard]] constexpr auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) convert(this PartialRegister value) noexcept
	{
		using result_t = partial_convert_result_t<element_type, target_t, register_width, lane_count>;
		return result_t{api_type::template convert<target_t>(value.native)};
	}

	/**
	 * @brief Widens the lowest active source lanes that fit in one target register.
	 * @tparam target_t Wider integral destination lane type supported by the source API.
	 * @tparam target_bits Destination native width, either 128 or 256 bits.
	 * @param value Source 128-bit partial register.
	 * @return Partial or complete target exposing exactly the number of consumed active source lanes.
	 * @remarks A target cell is unavailable when its meaningful prefix would leave an entirely inactive upper 128-bit half.
	 */
	template <class target_t, std::size_t target_bits>
		requires RegisterAvailable<target_t, target_bits> && IApi::Widen<api_type, Api<target_bits, target_t>> &&
				 (partial_widen_low_lane_count<element_type, target_t, register_width, target_bits, lane_count> == Api<target_bits, target_t>::element_count ||
				  PartialRegisterAvailable<target_t, target_bits,
										   partial_widen_low_lane_count<element_type, target_t, register_width, target_bits, lane_count>>)
	[[nodiscard]] constexpr auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) widen_low(this PartialRegister value) noexcept
	{
		using result_t = partial_widen_low_result_t<element_type, target_t, register_width, target_bits, lane_count>;
		return result_t{api_type::template widen<Api<target_bits, target_t>>(value.native)};
	}

#pragma endregion

#pragma region Comparison Operations

	/** @brief Compares active lanes for ordered equality and clears inactive predicate lanes. */
	[[nodiscard]] constexpr mask_type SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) compare_equal(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::CompareEqual<api_type> && IApi::BitwiseAnd<api_type>
	{
		return mask_type::from_native(api_type::compare_equal(lhs.native, rhs.native));
	}

	/** @brief Compares active lanes for ordered greater-than and clears inactive predicate lanes. */
	[[nodiscard]] constexpr mask_type SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) compare_greater(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::CompareGreater<api_type> && IApi::BitwiseAnd<api_type>
	{
		return mask_type::from_native(api_type::compare_greater(lhs.native, rhs.native));
	}

	/** @brief Compares active lanes for ordered greater-than-or-equal and clears inactive predicate lanes. */
	[[nodiscard]] constexpr mask_type SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten)
		compare_greater_equal(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::CompareGreaterEqual<api_type> && IApi::BitwiseAnd<api_type>
	{
		return mask_type::from_native(api_type::compare_greater_equal(lhs.native, rhs.native));
	}

	/** @brief Compares active lanes for ordered less-than and clears inactive predicate lanes. */
	[[nodiscard]] constexpr mask_type SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) compare_less(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::CompareLess<api_type> && IApi::BitwiseAnd<api_type>
	{
		return mask_type::from_native(api_type::compare_less(lhs.native, rhs.native));
	}

	/** @brief Compares active lanes for ordered less-than-or-equal and clears inactive predicate lanes. */
	[[nodiscard]] constexpr mask_type SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten)
		compare_less_equal(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::CompareLessEqual<api_type> && IApi::BitwiseAnd<api_type>
	{
		return mask_type::from_native(api_type::compare_less_equal(lhs.native, rhs.native));
	}

	/**
	 * @brief Tests whether every corresponding active lane compares equal.
	 * @return True only when all active lanes compare equal; inactive lanes are ignored.
	 * @remarks Floating NaNs compare unequal and signed zeros compare equal.
	 */
	[[nodiscard]] constexpr bool SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) operator==(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::CompareEqual<api_type> && IApi::BitwiseAnd<api_type> && IApi::MovemaskSlim<api_type>
	{
		return lhs.compare_equal(rhs).all();
	}

	/**
	 * @brief Tests whether at least one corresponding active lane fails ordered equality.
	 * @return Logical negation of active-prefix equality.
	 */
	[[nodiscard]] constexpr bool SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) operator!=(this PartialRegister lhs, PartialRegister rhs) noexcept
		requires IApi::CompareEqual<api_type> && IApi::BitwiseAnd<api_type> && IApi::MovemaskSlim<api_type>
	{
		return !lhs.compare_equal(rhs).all();
	}

#pragma endregion
};

} // namespace SimdLib

#include <SimdLib/PartialRegisterMask.h>
