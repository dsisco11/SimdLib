#pragma once
#include <SimdLib/Detail/Implementations.h>
#include <SimdLib/IApi.h>
#include <SimdLib/IImpl.h>
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
#if SIMDLIB_COMPILER_MSVC && SIMDLIB_TARGET_X86
#include <intrin.h>
#endif
#include <limits>
#include <span>
#include <type_traits>
#include <utility>

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

#pragma region Data Transfer
	/** @brief Loads element data into a SIMD register.
	 *  @param data Source elements matching the full register width.
	 *  @return Register populated with the provided elements.
	 */
	static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load(std::span<const element_t, element_count> data) noexcept
	{
		return impl::load_unaligned(data.data());
	}

	/**
	 * @brief Loads one complete register bit pattern from an exact byte span.
	 * @param data Source containing exactly one register of bytes.
	 * @return Register containing the source object representation.
	 */
	static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load(std::span<const std::byte, byte_count> data) noexcept
	{
		return impl::load_bytes(data.data());
	}

	/** @brief Loads a full register from storage aligned to the register byte width. */
	static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load_aligned(std::span<const element_t, element_count> data) noexcept
	{
		SIMDLIB_PRECONDITION(reinterpret_cast<std::uintptr_t>(data.data()) % byte_count == 0, "Aligned SIMD load requires register-width alignment");
		return impl::load(data.data());
	}

	/** @brief Explicit spelling for an unaligned full-register load. */
	static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load_unaligned(std::span<const element_t, element_count> data) noexcept
	{
		return impl::load_unaligned(data.data());
	}

	/** @brief Loads a logical prefix of elements into a SIMD register and zero-fills the remaining lanes.
	 *  @tparam active_count Number of leading elements to load.
	 *  @param data Source span whose first `active_count` values are loaded.
	 *  @return Register containing the requested active values followed by zero-filled inactive lanes.
	 */
	template <std::size_t active_count>
	constexpr static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load_partial(std::span<const element_t> data) noexcept
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
		else if constexpr (active_count * sizeof(element_t) == byte_count_half)
		{
			if (std::is_constant_evaluated())
			{
				return [&data]<std::size_t... Indices>(std::index_sequence<Indices...>) constexpr noexcept -> vector_t
				{ return setr_partial(static_cast<element_t>(data[Indices])...); }(std::make_index_sequence<active_count>{});
			}
			else if constexpr (using_int)
			{
				return impl::load_half(data.data());
			}
			else
			{
				using byte_api = Api<register_width, std::uint8_t>;
				const auto bytes = byte_api::template load_partial<byte_count_half>(
					std::span<const std::uint8_t>{reinterpret_cast<const std::uint8_t *>(data.data()), byte_count_half});
				return byte_api::template bit_cast<element_t>(bytes);
			}
		}
		else
		{
			return [&data]<std::size_t... Indices>(std::index_sequence<Indices...>) constexpr noexcept -> vector_t
			{ return setr_partial(static_cast<element_t>(data[Indices])...); }(std::make_index_sequence<active_count>{});
		}
	}

	/** @brief Loads a logical prefix from register-aligned storage and zero-fills the remaining lanes.
	 *  @tparam active_count Number of leading elements to load.
	 *  @param data Register-aligned source containing at least `active_count` elements.
	 *  @return Register containing the requested active values followed by zero-filled inactive lanes.
	 *  @pre `data.data()` is aligned to `byte_count` bytes.
	 */
	template <std::size_t active_count>
	constexpr static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load_partial_aligned(std::span<const element_t> data) noexcept
		requires(active_count <= element_count)
	{
		if (!std::is_constant_evaluated())
		{
			SIMDLIB_PRECONDITION(reinterpret_cast<std::uintptr_t>(data.data()) % byte_count == 0,
								 "Aligned partial SIMD load requires register-width alignment");
		}
		if constexpr (active_count == element_count)
			return impl::load(data.data());
		else if constexpr (active_count * sizeof(element_t) == byte_count_half)
		{
			if (std::is_constant_evaluated())
				return load_partial<active_count>(data);
			else if constexpr (using_int)
				return impl::load_half_aligned(data.data());
			else
			{
				using byte_api = Api<register_width, std::uint8_t>;
				const auto bytes = byte_api::template load_partial_aligned<byte_count_half>(
					std::span<const std::uint8_t>{reinterpret_cast<const std::uint8_t *>(data.data()), byte_count_half});
				return byte_api::template bit_cast<element_t>(bytes);
			}
		}
		else
			return load_partial<active_count>(data);
	}

	/** @brief Loads a logical prefix of one register's byte representation and zero-fills the remaining bytes.
	 *  @tparam active_byte_count Number of leading bytes to load.
	 *  @param data Source containing at least `active_byte_count` bytes.
	 *  @return Register containing the requested byte prefix followed by zero-filled bytes.
	 */
	template <std::size_t active_byte_count>
	static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load_bytes_partial(std::span<const std::byte> data) noexcept
		requires(active_byte_count <= byte_count)
	{
		SIMDLIB_PRECONDITION(data.size() >= active_byte_count, "Data span must contain at least the requested active byte count");
		using byte_api = Api<register_width, std::uint8_t>;
		const auto bytes =
			byte_api::template load_partial<active_byte_count>(std::span<const std::uint8_t>{reinterpret_cast<const std::uint8_t *>(data.data()), data.size()});
		return byte_api::template bit_cast<element_t>(bytes);
	}

	/** @brief Loads element data into a SIMD register without enforcing a fixed extent.
	 *  @param data Source span whose leading elements are read into the register.
	 *  @return Register populated from the provided span.
	 */
	static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) load_unsafe(std::span<const element_t> data) noexcept
	{
		return impl::load_unaligned(data.data());
	}

	/** @brief Stores a SIMD register into an element span.
	 *  @param vector Register value to store.
	 *  @param data Destination span that receives all register elements.
	 *  @return None.
	 */
	static void SIMD_FLAGS(In, ForceInline, Flatten) store(vector_t vector, std::span<element_t, element_count> data) noexcept
	{
		impl::store_unaligned(vector, data.data());
	}

	/**
	 * @brief Stores one complete register bit pattern to an exact byte span.
	 * @param vector Register value to store.
	 * @param data Destination containing exactly one register of bytes.
	 */
	static void SIMD_FLAGS(In, ForceInline, Flatten) store(vector_t vector, std::span<std::byte, byte_count> data) noexcept
	{
		impl::store_unaligned(vector, data.data());
	}

	/** @brief Stores a full register to storage aligned to the register byte width. */
	static void SIMD_FLAGS(In, ForceInline, Flatten) store_aligned(vector_t vector, std::span<element_t, element_count> data) noexcept
	{
		SIMDLIB_PRECONDITION(reinterpret_cast<std::uintptr_t>(data.data()) % byte_count == 0, "Aligned SIMD store requires register-width alignment");
		impl::store(vector, data.data());
	}

	/** @brief Explicit spelling for an unaligned full-register store. */
	static void SIMD_FLAGS(In, ForceInline, Flatten) store_unaligned(vector_t vector, std::span<element_t, element_count> data) noexcept
	{
		impl::store_unaligned(vector, data.data());
	}

	/** @brief Stores a logical prefix of a SIMD register without writing the inactive suffix.
	 *  @tparam active_count Number of leading elements to store.
	 *  @param vector Register value to store.
	 *  @param data Destination containing at least `active_count` elements.
	 */
	template <std::size_t active_count>
	constexpr static void SIMD_FLAGS(In, ForceInline, Flatten) store_partial(vector_t vector, std::span<element_t> data) noexcept
		requires(active_count <= element_count)
	{
		if (!std::is_constant_evaluated())
		{
			SIMDLIB_PRECONDITION(data.size() >= active_count, "Data span must contain at least the requested active element count");
		}
		if constexpr (active_count == element_count)
		{
			impl::store_unaligned(vector, data.data());
		}
		else if (std::is_constant_evaluated())
		{
			const auto lanes = to_array_constexpr(vector);
			for (std::size_t index = 0; index < active_count; ++index)
				data[index] = lanes[index];
		}
		else
		{
			if constexpr (active_count * sizeof(element_t) == byte_count_half && using_int)
				impl::store_half(vector, data.data());
			else if constexpr (active_count * sizeof(element_t) == byte_count_half)
			{
				using byte_api = Api<register_width, std::uint8_t>;
				const auto bytes = Api::template bit_cast<std::uint8_t>(vector);
				byte_api::template store_partial<byte_count_half>(bytes,
																  std::span<std::uint8_t>{reinterpret_cast<std::uint8_t *>(data.data()), byte_count_half});
			}
			else
			{
				[vector, data]<std::size_t... Indices>(std::index_sequence<Indices...>) noexcept
				{ ((data[Indices] = static_cast<element_t>(extract<static_cast<int>(Indices)>(vector))), ...); }(std::make_index_sequence<active_count>{});
			}
		}
	}

	/** @brief Stores a logical prefix to register-aligned storage without writing the inactive suffix.
	 *  @tparam active_count Number of leading elements to store.
	 *  @param vector Register value to store.
	 *  @param data Register-aligned destination containing at least `active_count` elements.
	 *  @pre `data.data()` is aligned to `byte_count` bytes.
	 */
	template <std::size_t active_count>
	constexpr static void SIMD_FLAGS(In, ForceInline, Flatten) store_partial_aligned(vector_t vector, std::span<element_t> data) noexcept
		requires(active_count <= element_count)
	{
		if (!std::is_constant_evaluated())
		{
			SIMDLIB_PRECONDITION(reinterpret_cast<std::uintptr_t>(data.data()) % byte_count == 0,
								 "Aligned partial SIMD store requires register-width alignment");
		}
		if constexpr (active_count == element_count)
			impl::store(vector, data.data());
		else if (std::is_constant_evaluated())
			store_partial<active_count>(vector, data);
		else if constexpr (active_count * sizeof(element_t) == byte_count_half && using_int)
			impl::store_half_aligned(vector, data.data());
		else if constexpr (active_count * sizeof(element_t) == byte_count_half)
		{
			using byte_api = Api<register_width, std::uint8_t>;
			const auto bytes = Api::template bit_cast<std::uint8_t>(vector);
			byte_api::template store_partial_aligned<byte_count_half>(bytes,
																	  std::span<std::uint8_t>{reinterpret_cast<std::uint8_t *>(data.data()), byte_count_half});
		}
		else
			store_partial<active_count>(vector, data);
	}

	/** @brief Stores a logical prefix of one register's byte representation without writing the inactive suffix.
	 *  @tparam active_byte_count Number of leading bytes to store.
	 *  @param vector Register value to store.
	 *  @param data Destination containing at least `active_byte_count` bytes.
	 */
	template <std::size_t active_byte_count>
	static void SIMD_FLAGS(In, ForceInline, Flatten) store_bytes_partial(vector_t vector, std::span<std::byte> data) noexcept
		requires(active_byte_count <= byte_count)
	{
		SIMDLIB_PRECONDITION(data.size() >= active_byte_count, "Data span must contain at least the requested active byte count");
		using byte_api = Api<register_width, std::uint8_t>;
		const auto bytes = Api::template bit_cast<std::uint8_t>(vector);
		byte_api::template store_partial<active_byte_count>(bytes, std::span<std::uint8_t>{reinterpret_cast<std::uint8_t *>(data.data()), data.size()});
	}

	/** @brief Stores a SIMD register into a raw byte span.
	 *  @param vector Register value to store.
	 *  @param data Destination byte span with capacity for the full register payload.
	 *  @return None.
	 */
	static void SIMD_FLAGS(In, ForceInline, Flatten) store(vector_t vector, std::span<std::byte> data) noexcept
	{
		SIMDLIB_PRECONDITION(data.size() >= byte_count, "Data byte span must be at least the byte size of the register");
		impl::store_unaligned(vector, data.data());
	}

	/** @brief Constructs a SIMD register from a fixed array.
	 *  @param data Source array containing one full register worth of elements.
	 *  @return Register populated with the provided array contents.
	 */
	constexpr static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) construct(const std::array<element_t, element_count> &data) noexcept
	{
		return impl::construct(data);
	}

	/** @brief Converts a SIMD register into a fixed array of elements.
	 *  @param vector Register value to unpack.
	 *  @return Array containing the register elements in lane order.
	 */
	constexpr static std::array<element_t, element_count> SIMD_FLAGS(In, ForceInline, Flatten) to_array(const vector_t vector) noexcept
	{
		if (std::is_constant_evaluated())
			return to_array_constexpr(vector);
		std::array<element_t, element_count> result{};
		impl::store_unaligned(vector, result.data());
		return result;
	}

	/** @brief Converts a logical register prefix into an exactly sized fixed array.
	 *  @tparam active_count Number of leading lanes to return.
	 *  @param vector Register value to observe.
	 *  @return Array containing exactly the requested leading lanes.
	 */
	template <std::size_t active_count>
	[[nodiscard]] constexpr static std::array<element_t, active_count> SIMD_FLAGS(In, ForceInline, Flatten) to_array_partial(const vector_t vector) noexcept
		requires(active_count <= element_count)
	{
		if (std::is_constant_evaluated())
		{
			const auto lanes = to_array_constexpr(vector);
			return [&lanes]<std::size_t... Indices>(std::index_sequence<Indices...>) constexpr noexcept
			{ return std::array<element_t, active_count>{lanes[Indices]...}; }(std::make_index_sequence<active_count>{});
		}
		else
		{
			std::array<element_t, active_count> result{};
			store_partial<active_count>(vector, std::span<element_t>{result});
			return result;
		}
	}

#pragma endregion

#pragma region Arithmetic Operations

	/** @brief Returns a zero-initialized SIMD register.
	 *  @return Register with every lane initialized to zero.
	 */
	constexpr static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) setzero() noexcept
		requires IImpl::SetZero<impl>
	{
		return impl::setzero();
	}

	/** @brief Broadcasts one scalar value to every lane in the register.
	 *  @param value Scalar value to broadcast.
	 *  @return Register with every lane initialized to `value`.
	 */
	constexpr static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) set1(const element_t value) noexcept
		requires IImpl::SetOne<impl, element_t>
	{
		return impl::set1(value);
	}

	/**
	 * @brief Broadcasts one scalar value into a logical lane prefix and zero-fills the remaining lanes.
	 * @tparam active_count Number of leading lanes initialized to `value`.
	 * @param value Scalar value to broadcast.
	 * @return Register containing `value` in the active prefix and all-bits-zero inactive lanes.
	 */
	template <std::size_t active_count>
	constexpr static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) broadcast_partial(const element_t value) noexcept
		requires(active_count <= element_count)
	{
		return broadcast_partial_native(value, std::make_index_sequence<active_count>{});
	}

  protected:
	/**
	 * @brief Expands one scalar into the requested active-prefix argument count.
	 * @tparam indices Active lane positions used to repeat the scalar argument.
	 * @param value Scalar value repeated across the active prefix.
	 * @return Register containing the repeated active prefix and a zero-filled suffix.
	 */
	template <std::size_t... indices>
	constexpr static vector_t SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten)
		broadcast_partial_native(const element_t value, std::index_sequence<indices...>) noexcept
	{
		return setr_partial(((void)indices, value)...);
	}

  public:
	/** @brief Constructs a register from lane values in native argument order.
	 *  @tparam Args Argument pack matching the register lane count.
	 *  @param args Lane values in native intrinsic order.
	 *  @return Register containing the provided lane values.
	 */
	template <class... Args>
	constexpr static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) set(Args &&...args) noexcept
		requires IImpl::Set<impl, Args...>
	{
		return impl::set(std::forward<Args>(args)...);
	}

	/** @brief Constructs a register from a partial native-order lane list and zero-fills the remaining lanes.
	 *  @tparam Args Argument pack containing up to one full register worth of lanes.
	 *  @param args Lane values in native intrinsic argument order.
	 *  @return Register containing the provided lanes with any remaining lanes initialized to zero.
	 */
	template <class... Args>
	constexpr static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) set_partial(Args &&...args) noexcept
		requires(sizeof...(Args) <= element_count)
	{
		return []<std::size_t... ZeroIndices>(std::index_sequence<ZeroIndices...>, Args &&...values) constexpr noexcept
			requires IImpl::Set<impl, Args..., decltype(((void)ZeroIndices, element_t{}))...>
		{ return impl::set(std::forward<Args>(values)..., ((void)ZeroIndices, element_t{})...); }(std::make_index_sequence<element_count - sizeof...(Args)>{},
																								  std::forward<Args>(args)...);
	}

	/** @brief Constructs a register from lane values in forward lane order.
	 *  @tparam Args Argument pack matching the register lane count.
	 *  @param args Lane values in logical low-to-high order.
	 *  @return Register containing the provided lane values.
	 */
	template <class... Args>
	constexpr static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) setr(Args &&...args) noexcept
		requires IImpl::SetReverse<impl, Args...>
	{
		return impl::setr(std::forward<Args>(args)...);
	}

	/** @brief Constructs a register from a partial forward-order lane list and zero-fills the remaining lanes.
	 *  @tparam Args Argument pack containing up to one full register worth of lanes.
	 *  @param args Lane values in low-to-high logical lane order.
	 *  @return Register containing the provided lanes with any remaining lanes initialized to zero.
	 */
	template <class... Args>
	constexpr static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) setr_partial(Args &&...args) noexcept
		requires(sizeof...(Args) <= element_count)
	{
		return []<std::size_t... ZeroIndices>(std::index_sequence<ZeroIndices...>, Args &&...values) constexpr noexcept
			requires IImpl::SetReverse<impl, Args..., decltype(((void)ZeroIndices, element_t{}))...>
		{ return impl::setr(std::forward<Args>(values)..., ((void)ZeroIndices, element_t{})...); }(std::make_index_sequence<element_count - sizeof...(Args)>{},
																								   std::forward<Args>(args)...);
	}

	/** @brief Computes a fused multiply-add where the implementation supports it, or a multiply followed by add otherwise.
	 *  @param lhs Left-hand multiplicand register.
	 *  @param rhs Right-hand multiplicand register.
	 *  @param addend Register added to the product.
	 *  @return Register containing the multiply-add result.
	 */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add(const vector_t lhs, const vector_t rhs, const vector_t addend) noexcept
		requires IImpl::MultiplyAdd<impl>
	{
		return impl::multiply_add(lhs, rhs, addend);
	}

	/** @brief Widens this SIMD register into the specified destination SIMD shape.
	 *  @tparam target_simd Destination SIMD wrapper type carrying the target register width and element type.
	 *  @param lhs Source register to widen.
	 *  @return Destination register widened according to the source element signedness.
	 */
	template <class target_simd>
		requires IApi::WidenTarget<target_simd> && using_int && std::is_integral_v<typename target_simd::element_type> &&
				 (std::is_signed_v<element_t> == std::is_signed_v<typename target_simd::element_type>) &&
				 (sizeof(element_t) < sizeof(typename target_simd::element_type)) && (register_width == 128) &&
				 (target_simd::register_width == 128 || target_simd::register_width == 256) &&
				 ApiAvailable<target_simd::register_width, typename target_simd::element_type> && IImpl::Widen<impl, target_simd>
	constexpr static typename target_simd::vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) widen(const vector_t lhs) noexcept
	{
		if (std::is_constant_evaluated())
			return widen_constexpr<target_simd>(lhs);
		return impl::template widen<target_simd>(lhs);
	}

	/** @brief Computes the remainder of each lhs element divided by the corresponding rhs element.
	 *  @param lhs Dividend register.
	 *  @param rhs Divisor register.
	 *  @return Register containing per-lane remainder results.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) modulus(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::Modulus<impl>
	{
		return impl::modulus(lhs, rhs);
	}

	/** @brief Negates each element in the register.
	 *  @param lhs Input register.
	 *  @return Register containing the negated element values.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) negate(const vector_t lhs) noexcept
		requires IImpl::Negate<impl>
	{
		return impl::negate(lhs);
	}

	/** @brief Computes the absolute value of each element in the register.
	 *  @param lhs Input register.
	 *  @return Register containing per-lane absolute values.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) absolute(const vector_t lhs) noexcept
		requires IImpl::Absolute<impl>
	{
		return impl::absolute(lhs);
	}

	/** @brief Computes the square root of each element in the register.
	 *  @param lhs Input register.
	 *  @return Register containing per-lane square roots.
	 */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sqrt(const vector_t lhs) noexcept
		requires IImpl::Sqrt<impl>
	{
		return impl::sqrt(lhs);
	}

	/** @brief Computes the vector magnitude independently for each 128-bit group.
	 *  @param lhs Input register. Integer inputs require a magnitude representable by `element_t`.
	 *  @return Floating magnitudes broadcast within each group, or unchecked integer magnitudes in each group-leading lane.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude(const vector_t lhs) noexcept
		requires IImpl::Magnitude<impl>
	{
		return impl::magnitude(lhs);
	}

	/** @brief Computes saturated integer magnitudes with canonical overflow masks.
	 *  @param lhs Input integer register.
	 *  @return Each 128-bit group stores its magnitude in lane zero and a zero/all-ones overflow mask in lane one.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) magnitude_checked(const vector_t lhs) noexcept
		requires(using_int && IImpl::MagnitudeChecked<impl>)
	{
		return impl::magnitude_checked(lhs);
	}

	/** @brief Normalizes floating-point lanes using the vector length computed per 128-bit lane.
	 *  @param lhs Input floating-point register.
	 *  @return Register containing the normalized per-lane values.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) normalize(const vector_t lhs) noexcept
		requires(std::is_floating_point_v<element_t> && IImpl::Normalize<impl>)
	{
		return divide(lhs, magnitude(lhs));
	}

	/** @brief Computes the average of corresponding lanes where the specialization supports it.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing per-lane averages.
	 */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) avg(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::Average<impl>
	{
		return impl::avg(lhs, rhs);
	}

	/** @brief Adds adjacent element pairs within each 128-bit lane of two registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing pairwise horizontal sums.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_horizontal(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::HorizontalAdd<impl>
	{
		return impl::add_horizontal(lhs, rhs);
	}

	/** @brief Subtracts adjacent element pairs within each 128-bit lane of two registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing pairwise horizontal differences.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_horizontal(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::HorizontalSubtract<impl>
	{
		return impl::subtract_horizontal(lhs, rhs);
	}

	/** @brief Multiplies adjacent element pairs and accumulates them into promoted result lanes.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register whose lane type follows the promoted integer mapping rather than `vector_t`.
	 */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_adjacent(const vector_t lhs, const vector_t rhs) noexcept
		requires(using_int && IImpl::MultiplyAddAdjacent<impl>)
	{
		return impl::multiply_add_adjacent(lhs, rhs);
	}

	/** @brief Multiplies raw register bytes as unsigned and signed pairs and accumulates them into signed 16-bit lanes.
	 *  @param lhs Left-hand input register whose bytes are interpreted as unsigned.
	 *  @param rhs Right-hand input register whose bytes are interpreted as signed.
	 *  @return Register containing signed 16-bit accumulation results derived from the raw register bytes.
	 */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multiply_add_unsigned_signed_bytes(const vector_t lhs, const vector_t rhs) noexcept
		requires(using_int && IImpl::ByteMultiplyAdd<impl>)
	{
		return impl::multiply_add_unsigned_signed_bytes(lhs, rhs);
	}

	/** @brief Computes byte-wise absolute differences and accumulates them into 64-bit result lanes.
	 *  @param lhs Left-hand input register interpreted byte-wise.
	 *  @param rhs Right-hand input register interpreted byte-wise.
	 *  @return Register containing 64-bit absolute-difference accumulations derived from the raw register bytes.
	 */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) sum_absolute_byte_differences(const vector_t lhs, const vector_t rhs) noexcept
		requires(using_int && IImpl::Sad<impl>)
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
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) multi_sum_absolute_byte_differences(const vector_t lhs, const vector_t rhs) noexcept
		requires(using_int && IImpl::MultiSad<impl, imm8>)
	{
		return impl::template multi_sum_absolute_byte_differences<imm8>(lhs, rhs);
	}

	/** @brief Returns the first index of the minimum value in the register.
	 *  @param lhs Input register.
	 *  @return Zero-based index of the first minimum element across the full SIMD register.
	 */
	constexpr static std::size_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) min_position(const vector_t lhs) noexcept
		requires(using_int && IImpl::Position<impl>)
	{
		if (std::is_constant_evaluated())
			return min_position_constexpr(lhs);

		return static_cast<std::size_t>(impl::template extract<1>(impl::min_position(lhs)));
	}

	/** @brief Returns the first index of the maximum value in the register.
	 *  @param lhs Input register.
	 *  @return Zero-based index of the first maximum element across the full SIMD register.
	 */
	constexpr static std::size_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) max_position(const vector_t lhs) noexcept
		requires(using_int && IImpl::Position<impl>)
	{
		if (std::is_constant_evaluated())
			return max_position_constexpr(lhs);

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
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_saturated(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::AddSaturated<impl>
	{
		return impl::add_saturated(lhs, rhs);
	}

	/** @brief Subtracts corresponding lanes with saturation where the specialization supports it.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing saturated differences.
	 */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) subtract_saturated(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::SubtractSaturated<impl>
	{
		return impl::subtract_saturated(lhs, rhs);
	}

	/** @brief Adds adjacent pairs with saturation where the specialization supports it.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing saturated horizontal sums.
	 */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) hadd_saturated(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::HorizontalAddSaturated<impl>
	{
		return impl::hadd_saturated(lhs, rhs);
	}

	/** @brief Subtracts adjacent pairs with saturation where the specialization supports it.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing saturated horizontal differences.
	 */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) hsubtract_saturated(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::HorizontalSubtractSaturated<impl>
	{
		return impl::hsubtract_saturated(lhs, rhs);
	}

	/** @brief Alternates subtraction and addition across lanes for floating-point SIMD families.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing alternating subtract/add results.
	 */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) add_subtract(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::AddSubtract<impl>
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
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) dot_product(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::DotProduct<impl, imm8>
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
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_and(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::BitwiseAnd<impl>
	{
		if (std::is_constant_evaluated())
			return bitwise_and_constexpr(lhs, rhs);
		else
			return impl::bitwise_and(lhs, rhs);
	}

	/** @brief Computes a bitwise OR of two registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the bitwise OR result.
	 */
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_or(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::BitwiseOr<impl>
	{
		if (std::is_constant_evaluated())
			return bitwise_or_constexpr(lhs, rhs);
		else
			return impl::bitwise_or(lhs, rhs);
	}

	/** @brief Computes a bitwise XOR of two registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the bitwise XOR result.
	 */
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_xor(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::BitwiseXor<impl>
	{
		if (std::is_constant_evaluated())
			return bitwise_xor_constexpr(lhs, rhs);
		else
			return impl::bitwise_xor(lhs, rhs);
	}

	/** @brief Computes a bitwise AND-NOT of two registers.
	 *  @param lhs Left-hand input register whose bits are inverted before the AND.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the bitwise AND-NOT result.
	 */
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_andnot(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::BitwiseAndNot<impl>
	{
		if (std::is_constant_evaluated())
			return bitwise_andnot_constexpr(lhs, rhs);
		else
			return impl::bitwise_andnot(lhs, rhs);
	}

	/** @brief Computes a bitwise NOT of a register.
	 *  @param lhs Input register.
	 *  @return Register containing the bitwise NOT result.
	 */
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) bitwise_not(const vector_t lhs) noexcept
		requires IImpl::BitwiseNot<impl>
	{
		if (std::is_constant_evaluated())
			return bitwise_not_constexpr(lhs);
		else
			return impl::bitwise_not(lhs);
	}

#pragma endregion

#pragma region Selection Operations

	/** @brief Selects lanes from two registers using a canonical native predicate.
	 *  @param condition Canonical predicate register containing all-zero or all-one lanes.
	 *  @param when_true Register selected where the corresponding predicate lane is true.
	 *  @param when_false Register selected where the corresponding predicate lane is false.
	 *  @return Register containing the selected lanes without reducing the predicate.
	 */
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		select(const vector_t condition, const vector_t when_true, const vector_t when_false) noexcept
		requires IImpl::Select<impl>
	{
		if (std::is_constant_evaluated())
			return select_constexpr(condition, when_true, when_false);
		else
			return impl::select(condition, when_true, when_false);
	}

#pragma endregion

#pragma region Comparison Operations

#pragma region Mask Reductions

	/** @brief Returns a mask composed from the most significant bit of each byte in the register.
	 *  @param lhs Input register.
	 *  @return Byte-granular movemask for the register contents.
	 */
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) movemask(const vector_t lhs) noexcept
	{
		if (std::is_constant_evaluated())
			return movemask_constexpr(lhs);
		else
		{
			return impl::movemask(lhs);
		}
	}

	/** @brief Returns a mask composed from the most significant bit of each element in the register.
	 *  @param lhs Input register.
	 *  @return Element-granular movemask for the register contents.
	 */
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) movemask_slim(const vector_t lhs) noexcept
	{
		if (std::is_constant_evaluated())
			return movemask_slim_constexpr(lhs);
		else
		{
			return impl::movemask_slim(lhs);
		}
	}

#pragma endregion

#pragma region Native Predicate Comparisons

	/** @brief Compares corresponding lanes for ordered equality without reducing the result.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Native predicate register containing an all-one true lane or an all-zero false lane.
	 */
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) compare_equal(const vector_t lhs, const vector_t rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return compare_equal_constexpr(lhs, rhs);
		else
			return impl::cmpeq(lhs, rhs);
	}

	/** @brief Compares corresponding lanes for greater-than ordering without reducing the result.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Native predicate register containing an all-one true lane or an all-zero false lane.
	 */
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) compare_greater(const vector_t lhs, const vector_t rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return compare_greater_constexpr(lhs, rhs);
		else
			return impl::cmpgt(lhs, rhs);
	}

	/** @brief Compares corresponding lanes for greater-than-or-equal ordering without reducing the result.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Native predicate register containing an all-one true lane or an all-zero false lane.
	 */
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) compare_greater_equal(const vector_t lhs, const vector_t rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return compare_greater_equal_constexpr(lhs, rhs);
		else
			return bitwise_or(compare_equal(lhs, rhs), compare_greater(lhs, rhs));
	}

	/** @brief Compares corresponding lanes for less-than ordering without reducing the result.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Native predicate register containing an all-one true lane or an all-zero false lane.
	 */
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) compare_less(const vector_t lhs, const vector_t rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return compare_less_constexpr(lhs, rhs);
		else
			return impl::cmpgt(rhs, lhs);
	}

	/** @brief Compares corresponding lanes for less-than-or-equal ordering without reducing the result.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Native predicate register containing an all-one true lane or an all-zero false lane.
	 */
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) compare_less_equal(const vector_t lhs, const vector_t rhs) noexcept
	{
		if (std::is_constant_evaluated())
			return compare_less_equal_constexpr(lhs, rhs);
		else
			return bitwise_or(compare_equal(lhs, rhs), compare_less(lhs, rhs));
	}

#pragma endregion

#pragma region Byte Comparison Masks

	/** @brief Reduces an equality comparison to a byte-granular scalar mask.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with one set bit for every all-one byte produced by the comparison.
	 */
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) cmp_eq_mask(const vector_t lhs, const vector_t rhs) noexcept
	{
		return movemask(compare_equal(lhs, rhs));
	}

	/** @brief Reduces a greater-than comparison to a byte-granular scalar mask.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with one set bit for every all-one byte produced by the comparison.
	 */
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) cmp_gt_mask(const vector_t lhs, const vector_t rhs) noexcept
	{
		return movemask(compare_greater(lhs, rhs));
	}

	/** @brief Reduces a greater-than-or-equal comparison to a byte-granular scalar mask.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with one set bit for every all-one byte produced by the comparison.
	 */
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) cmp_ge_mask(const vector_t lhs, const vector_t rhs) noexcept
	{
		return movemask(compare_greater_equal(lhs, rhs));
	}

	/** @brief Reduces a less-than comparison to a byte-granular scalar mask.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with one set bit for every all-one byte produced by the comparison.
	 */
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) cmp_lt_mask(const vector_t lhs, const vector_t rhs) noexcept
	{
		return movemask(compare_less(lhs, rhs));
	}

	/** @brief Reduces a less-than-or-equal comparison to a byte-granular scalar mask.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with one set bit for every all-one byte produced by the comparison.
	 */
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) cmp_le_mask(const vector_t lhs, const vector_t rhs) noexcept
	{
		return movemask(compare_less_equal(lhs, rhs));
	}

#pragma endregion

#pragma region Slim Comparison Masks

	/** @brief Reduces an equality comparison to one scalar bit per logical lane.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with one set bit for every true predicate lane.
	 */
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) cmp_eq_slim(const vector_t lhs, const vector_t rhs) noexcept
	{
		return movemask_slim(compare_equal(lhs, rhs));
	}

	/** @brief Reduces a greater-than comparison to one scalar bit per logical lane.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with one set bit for every true predicate lane.
	 */
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) cmp_gt_slim(const vector_t lhs, const vector_t rhs) noexcept
	{
		return movemask_slim(compare_greater(lhs, rhs));
	}

	/** @brief Reduces a greater-than-or-equal comparison to one scalar bit per logical lane.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with one set bit for every true predicate lane.
	 */
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) cmp_ge_slim(const vector_t lhs, const vector_t rhs) noexcept
	{
		return movemask_slim(compare_greater_equal(lhs, rhs));
	}

	/** @brief Reduces a less-than comparison to one scalar bit per logical lane.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with one set bit for every true predicate lane.
	 */
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) cmp_lt_slim(const vector_t lhs, const vector_t rhs) noexcept
	{
		return movemask_slim(compare_less(lhs, rhs));
	}

	/** @brief Reduces a less-than-or-equal comparison to one scalar bit per logical lane.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Mask with one set bit for every true predicate lane.
	 */
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) cmp_le_slim(const vector_t lhs, const vector_t rhs) noexcept
	{
		return movemask_slim(compare_less_equal(lhs, rhs));
	}

#pragma endregion

#pragma region Deprecated Comparison Masks

	/** @brief Legacy byte-granular equality mask spelling.
	 *  @deprecated Use cmp_eq_mask() instead.
	 */
	[[deprecated("Use cmp_eq_mask() instead.")]]
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) cmp_eq(const vector_t lhs, const vector_t rhs) noexcept
	{
		return cmp_eq_mask(lhs, rhs);
	}

	/** @brief Legacy byte-granular greater-than mask spelling.
	 *  @deprecated Use cmp_gt_mask() instead.
	 */
	[[deprecated("Use cmp_gt_mask() instead.")]]
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) cmp_gt(const vector_t lhs, const vector_t rhs) noexcept
	{
		return cmp_gt_mask(lhs, rhs);
	}

	/** @brief Legacy byte-granular greater-than-or-equal mask spelling.
	 *  @deprecated Use cmp_ge_mask() instead.
	 */
	[[deprecated("Use cmp_ge_mask() instead.")]]
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) cmp_ge(const vector_t lhs, const vector_t rhs) noexcept
	{
		return cmp_ge_mask(lhs, rhs);
	}

	/** @brief Legacy byte-granular less-than mask spelling.
	 *  @deprecated Use cmp_lt_mask() instead.
	 */
	[[deprecated("Use cmp_lt_mask() instead.")]]
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) cmp_lt(const vector_t lhs, const vector_t rhs) noexcept
	{
		return cmp_lt_mask(lhs, rhs);
	}

	/** @brief Legacy byte-granular less-than-or-equal mask spelling.
	 *  @deprecated Use cmp_le_mask() instead.
	 */
	[[deprecated("Use cmp_le_mask() instead.")]]
	constexpr static mask_t SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) cmp_le(const vector_t lhs, const vector_t rhs) noexcept
	{
		return cmp_le_mask(lhs, rhs);
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
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) expand(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::Expand<impl>
	{
		return impl::expand(lhs, rhs);
	}

	/** @brief Compresses two registers into a narrower-lane register where the specialization supports it.
	 *  @param lhs Left-hand source register.
	 *  @param rhs Right-hand source register.
	 *  @return Compressed register value.
	 */
	static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) compress(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::Compress<impl>
	{
		return impl::compress(lhs, rhs);
	}

	/** @brief Extracts a lane or subvalue from a register.
	 *  @param lhs Source register.
	 *  @param rhs Extract selector.
	 *  @return Extracted value as defined by the specialization.
	 */
	template <int index>
	static auto SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) extract(const vector_t lhs) noexcept
		requires IImpl::IndexedExtract<impl, index>
	{
		static_assert(index >= 0 && static_cast<std::size_t>(index) < element_count, "Api::extract index out of range.");
		return impl::template extract<index>(lhs);
	}

	/** @brief Extracts a lane or subvalue from a register using a runtime selector.
	 *  @param lhs Source register.
	 *  @param rhs Extract selector.
	 *  @return Extracted value as defined by the specialization.
	 *  @note `_slow` marks runtime emulation of an immediate lane selector.
	 */
	template <class selector_t>
	constexpr static auto SIMD_FLAGS(InOut, ForceInline, Flatten) extract_slow(const vector_t lhs, selector_t rhs) noexcept
		requires IImpl::ExtractSlow<impl, selector_t>
	{
		if (std::is_constant_evaluated())
			return extract_constexpr(lhs, static_cast<int>(rhs));
		return impl::extract_slow(lhs, rhs);
	}

	/** @brief Returns the low 128-bit half of a 256-bit register when the specialization supports it.
	 *  @param lhs Source register.
	 *  @return Register containing the low 128-bit half in the corresponding 128-bit SIMD family.
	 */
	constexpr static typename SimdLib::Detail::SimdMappings<128, element_t>::vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
		lower_half(const vector_t lhs) noexcept
		requires(register_width == 256 && IImpl::LowerHalf<impl>)
	{
		if (std::is_constant_evaluated())
			return lower_half_constexpr(lhs);
		return impl::lower_half(lhs);
	}

	/** @brief Inserts a compile-time-selected scalar lane into a register.
	 *  @tparam index Compile-time logical lane index.
	 *  @param lhs Register whose unselected lanes are preserved.
	 *  @param rhs Scalar replacement value.
	 *  @return Register with lane `index` replaced.
	 */
	template <std::size_t index>
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) insert(const vector_t lhs, const element_t rhs) noexcept
		requires(index < element_count)
	{
		if (std::is_constant_evaluated())
			return impl::template insert_constexpr<static_cast<int>(index)>(lhs, rhs);
		else
			return impl::template insert<static_cast<int>(index)>(lhs, rhs);
	}

	/**
	 * @brief Replaces one runtime-selected scalar lane in a register.
	 * @param lhs Register whose unselected lanes are preserved.
	 * @param rhs Scalar replacement value.
	 * @param index Runtime-selected logical lane index.
	 * @return Register with the selected lane replaced.
	 * @note `_slow` marks runtime emulation of an immediate lane selector.
	 */
	constexpr static vector_t SIMD_FLAGS(InOut, ForceInline, Flatten) insert_slow(const vector_t lhs, const element_t rhs, const int index) noexcept
		requires IImpl::InsertSlow<impl, vector_t, element_t, int>
	{
		if (std::is_constant_evaluated())
			return insert_constexpr(lhs, rhs, index);
		return impl::insert_slow(lhs, rhs, index);
	}

	/** @brief Unpacks the low lanes of two registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the unpacked low-lane interleave.
	 */
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) unpack_lo(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::UnpackLow<impl>
	{
		if (std::is_constant_evaluated())
			return unpack_constexpr<false>(lhs, rhs);
		return impl::unpack_lo(lhs, rhs);
	}

	/** @brief Unpacks the high lanes of two registers.
	 *  @param lhs Left-hand input register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the unpacked high-lane interleave.
	 */
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) unpack_hi(const vector_t lhs, const vector_t rhs) noexcept
		requires IImpl::UnpackHigh<impl>
	{
		if (std::is_constant_evaluated())
			return unpack_constexpr<true>(lhs, rhs);
		return impl::unpack_hi(lhs, rhs);
	}

	/** @brief Rearranges logical lanes with one compile-time selector per result lane.
	 *  @tparam indices Exact selector sequence in logical result-lane order.
	 *  @param lhs Source register.
	 *  @return Register containing the selected lanes.
	 *  @note Every selector may name any logical lane in the complete source register.
	 */
	template <std::size_t... indices>
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle(const vector_t lhs) noexcept
		requires(Api::template logical_shuffle_indices_valid<indices...>() && IImpl::IndexedShuffle<impl, indices...>)
	{
		if (std::is_constant_evaluated())
			return shuffle_constexpr<indices...>(lhs);
		return impl::template shuffle<indices...>(lhs);
	}

	/** @brief Shuffles register contents according to the implementation-specific control form.
	 *  @tparam Args Argument pack matching the specialization shuffle signature.
	 *  @param args Arguments forwarded to the specialization shuffle operation.
	 *  @return Register containing the shuffled result.
	 */
	template <class... Args>
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) shuffle(Args &&...args) noexcept
		requires IImpl::Shuffle<impl, Args...>
	{
		return impl::shuffle(std::forward<Args>(args)...);
	}

	/**
	 * @brief Emulates an immediate-controlled shuffle from a runtime scalar control.
	 * @tparam Args Argument pack matching the implementation slow-path shuffle signature.
	 * @param args Arguments forwarded to the specialization slow-path shuffle operation.
	 * @return Register containing the shuffled result.
	 * @note `_slow` identifies a deliberate runtime substitute for an immediate-controlled operation and may require dispatch, branching, or a longer
	 * synthesized instruction sequence.
	 */
	template <class... Args>
	static auto SIMD_FLAGS(Out, ForceInline, Flatten) shuffle_slow(Args &&...args) noexcept
		requires IImpl::ShuffleSlow<impl, Args...>
	{
		return impl::shuffle_slow(std::forward<Args>(args)...);
	}
	/** @brief Shuffles the low four 16-bit lanes in each 128-bit group using an immediate control.
	 *  @tparam imm8 Immediate control in the inclusive range `0..255`; every two-bit field selects one lane.
	 *  @param lhs Source register.
	 *  @return Register with each low four-lane group shuffled and all high four-lane groups preserved.
	 */
	template <int imm8>
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle_lo(const vector_t lhs) noexcept
		requires(using_int && element_width == 16 && imm8 >= 0 && imm8 <= 255 && IImpl::IndexedShuffleLow<impl, imm8>)
	{
		if (std::is_constant_evaluated())
			return shuffle_half_constexpr<imm8, false>(lhs);
		return impl::template shuffle_lo<imm8>(lhs);
	}

	/** @brief Shuffles the low half of a register where the specialization supports it.
	 *  @tparam Args Argument pack matching the specialization
	 * shuffle-low signature.
	 *  @param args Arguments forwarded to the specialization shuffle-low operation.
	 *  @return Register containing the shuffled low-half result.
	 *  @note `_slow` marks runtime emulation of an immediate control byte and may require a longer synthesized sequence.
	 */
	template <class... Args>
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) shuffle_lo_slow(Args &&...args) noexcept
		requires IImpl::ShuffleLowSlow<impl, Args...>
	{
		return impl::shuffle_lo_slow(std::forward<Args>(args)...);
	}

	/** @brief Shuffles the high four 16-bit lanes in each 128-bit group using an immediate control.
	 *  @tparam imm8 Immediate control in the inclusive range `0..255`; every two-bit field selects one lane.
	 *  @param lhs Source register.
	 *  @return Register with each high four-lane group shuffled and all low four-lane groups preserved.
	 */
	template <int imm8>
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shuffle_hi(const vector_t lhs) noexcept
		requires(using_int && element_width == 16 && imm8 >= 0 && imm8 <= 255 && IImpl::IndexedShuffleHigh<impl, imm8>)
	{
		if (std::is_constant_evaluated())
			return shuffle_half_constexpr<imm8, true>(lhs);
		return impl::template shuffle_hi<imm8>(lhs);
	}

	/** @brief Shuffles the high half of a register where the specialization supports it.
	 *  @tparam Args Argument pack matching the specialization
	 * shuffle-high signature.
	 *  @param args Arguments forwarded to the specialization shuffle-high operation.
	 *  @return Register containing the shuffled high-half result.
	 *  @note `_slow` marks runtime emulation of an immediate control byte and may require a longer synthesized sequence.
	 */
	template <class... Args>
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) shuffle_hi_slow(Args &&...args) noexcept
		requires IImpl::ShuffleHighSlow<impl, Args...>
	{
		return impl::shuffle_hi_slow(std::forward<Args>(args)...);
	}

	/** @brief Selects corresponding lanes from two registers using an immediate bit mask.
	 *  @tparam imm8 Immediate control in the inclusive range `0..255`;
	 * set bits select `rhs`.
	 *  @param lhs Register selected by cleared applicable control bits.
	 *  @param rhs Register selected by set applicable
	 * control bits.
	 *  @return Register containing the intrinsic-defined immediate blend.
	 *  @note Bits unused by the selected intrinsic have no effect.
	 * A 256-bit 16-bit blend repeats the eight mask bits in each 128-bit group.
	 */
	template <int imm8>
	constexpr static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) blend(const vector_t lhs, const vector_t rhs) noexcept
		requires(imm8 >= 0 && imm8 <= 255 && IImpl::IndexedBlend<impl, imm8>)
	{
		return impl::template blend<imm8>(lhs, rhs);
	}

	/** @brief Blends two registers according to the implementation-specific control form.
	 *  @tparam Args Argument pack matching the specialization blend
	 * signature.
	 *  @param args Arguments forwarded to the specialization blend operation.
	 *  @return Register containing the blended result.
	 */
	template <class... Args>
	static auto SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) blend(Args &&...args) noexcept
		requires IImpl::Blend<impl, Args...>
	{
		return impl::blend(std::forward<Args>(args)...);
	}

	/**
	 * @brief Emulates an immediate-controlled blend from a runtime scalar control.
	 * @tparam Args Argument pack matching the implementation slow-path blend signature.
	 * @param args Arguments forwarded to the specialization slow-path blend operation.
	 * @return Register containing the blended result.
	 * @note `_slow` identifies a deliberate runtime substitute for an immediate-controlled operation and may require dispatch, branching, or a longer
	 * synthesized instruction sequence.
	 */
	template <class... Args>
	static auto SIMD_FLAGS(Out, ForceInline, Flatten) blend_slow(Args &&...args) noexcept
		requires IImpl::BlendSlow<impl, Args...>
	{
		return impl::blend_slow(std::forward<Args>(args)...);
	}
#pragma endregion

#pragma region Shifting Operations

	/** @brief Shifts each integer lane left by the specified amount.
	 *  @param lhs Input integer register.
	 *  @param shift Shift count applied to each lane.
	 *  @return Register containing per-lane left-shifted values.
	 */
	constexpr static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shift_left(const int_vector_t lhs, int shift) noexcept
		requires(using_int)
	{
		SIMDLIB_PRECONDITION(shift >= 0, "Per-lane left shifts require a nonnegative count");
		if (std::is_constant_evaluated())
			return shift_left_constexpr(lhs, shift);

		return impl::shift_left(lhs, shift);
	}

	/** @brief Shifts each integer lane right by the specified amount.
	 *  @param lhs Input integer register.
	 *  @param shift Shift count applied to each lane.
	 *  @return Register containing per-lane right-shifted values.
	 */
	constexpr static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shift_right(const int_vector_t lhs, int shift) noexcept
		requires(using_int)
	{
		SIMDLIB_PRECONDITION(shift >= 0, "Per-lane logical right shifts require a nonnegative count");
		if (std::is_constant_evaluated())
			return shift_right_constexpr(lhs, shift);

		return impl::shift_right(lhs, shift);
	}

	/** @brief Arithmetic-shifts each integer lane right by the specified amount.
	 *  @param lhs Input integer register.
	 *  @param shift Shift count applied to each lane.
	 *  @return Register containing per-lane arithmetic right-shifted values.
	 */
	constexpr static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shift_right_arithmetic(const int_vector_t lhs, int shift) noexcept
		requires(using_int)
	{
		SIMDLIB_PRECONDITION(shift >= 0, "Per-lane arithmetic right shifts require a nonnegative count");
		if (std::is_constant_evaluated())
			return shift_right_arithmetic_constexpr(lhs, shift);

		return impl::shift_right_arithmetic(lhs, shift);
	}

	/**
	 * @brief Shifts every byte in a 128-bit register toward higher byte indices.
	 *
	 * A zero or negative count returns the input unchanged. A count greater than
	 * or equal to the register byte width returns zero. [eg: shift_bytes_left_slow(
	 * {0x01, 0x02, ...}, 1) => {0x00, 0x01, 0x02, ...}]
	 *
	 * @param lhs The source register.
	 * @param shift The runtime byte count.
	 * @return The byte-shifted register.
	 * @note `_slow` marks runtime emulation of an immediate byte count.
	 */
	constexpr static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shift_bytes_left_slow(const int_vector_t lhs, const int shift) noexcept
		requires(using_int && register_width == 128)
	{
		if (std::is_constant_evaluated())
			return shift_bytes_left_constexpr(lhs, shift);
		return impl::shift_bytes_left_slow(lhs, shift);
	}

	/**
	 * @brief Shifts every byte in a complete integral register toward higher byte indices.
	 *
	 * The count is encoded as an immediate. Zero returns the input unchanged; counts
	 * at least as large as the register byte width return zero.
	 *
	 * @tparam count Nonnegative compile-time byte count.
	 * @param lhs The source register.
	 * @return The byte-shifted register with zero-filled low bytes.
	 */
	template <int count>
	constexpr static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shift_bytes_left(const int_vector_t lhs) noexcept
		requires(using_int && IImpl::ShiftBytesLeft<impl, count>)
	{
		static_assert(count >= 0, "Complete-register byte shifts require a nonnegative count.");
		if (std::is_constant_evaluated())
			return shift_bytes_left_constexpr(lhs, count);
		return impl::template shift_bytes_left<count>(lhs);
	}

	/**
	 * @brief Shifts every byte in a 128-bit register toward lower byte indices.
	 *
	 * A zero or negative count returns the input unchanged. A count greater than
	 * or equal to the register byte width returns zero. [eg: shift_bytes_right_slow(
	 * {0x01, 0x02, ...}, 1) => {0x02, ..., 0x00}]
	 *
	 * @param lhs The source register.
	 * @param shift The runtime byte count.
	 * @return The byte-shifted register.
	 * @note `_slow` marks runtime emulation of an immediate byte count.
	 */
	constexpr static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shift_bytes_right_slow(const int_vector_t lhs, const int shift) noexcept
		requires(using_int && register_width == 128)
	{
		if (std::is_constant_evaluated())
			return shift_bytes_right_constexpr(lhs, shift);
		return impl::shift_bytes_right_slow(lhs, shift);
	}

	/**
	 * @brief Shifts every byte in a complete integral register toward lower byte indices.
	 *
	 * The count is encoded as an immediate. Zero returns the input unchanged; counts
	 * at least as large as the register byte width return zero.
	 *
	 * @tparam count Nonnegative compile-time byte count.
	 * @param lhs The source register.
	 * @return The byte-shifted register with zero-filled high bytes.
	 */
	template <int count>
	constexpr static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shift_bytes_right(const int_vector_t lhs) noexcept
		requires(using_int && IImpl::ShiftBytesRight<impl, count>)
	{
		static_assert(count >= 0, "Complete-register byte shifts require a nonnegative count.");
		if (std::is_constant_evaluated())
			return shift_bytes_right_constexpr(lhs, count);
		return impl::template shift_bytes_right<count>(lhs);
	}

	/** @brief Shifts the complete 128-bit register left, carrying bits across lane boundaries.
	 * Unlike `shift_left`, this treats the register as one
	 * unsigned 128-bit bit string.
	 * A zero or negative runtime count returns the input; counts of 128 or more return zero.
	 * @note `_slow` marks the synthesized runtime-count substitute for immediate complete-register shifts.
	 */
	constexpr static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shift_bits_left_slow(const int_vector_t lhs, const int shift) noexcept
		requires(using_int && register_width == 128)
	{
		if (std::is_constant_evaluated())
			return shift_bits_left_constexpr(lhs, shift);
		return impl::shift_bits_left_slow(lhs, shift);
	}

	/** @brief Compile-time complete-register left shift. Counts of 128 or more return zero. */
	template <int shift>
	constexpr static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shift_bits_left(const int_vector_t lhs) noexcept
		requires(using_int && register_width == 128)
	{
		static_assert(shift >= 0, "Whole-register shifts require a non-negative count.");
		if (std::is_constant_evaluated())
			return shift_bits_left_constexpr(lhs, shift);
		return impl::template shift_bits_left<shift>(lhs);
	}

	/** @brief Shifts the complete 128-bit register right, carrying bits across lane boundaries.
	 * Unlike `shift_right`, this treats the register as one
	 * unsigned 128-bit bit string.
	 * A zero or negative runtime count returns the input; counts of 128 or more return zero.
	 * @note `_slow` marks the synthesized runtime-count substitute for immediate complete-register shifts.
	 */
	constexpr static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shift_bits_right_slow(const int_vector_t lhs, const int shift) noexcept
		requires(using_int && register_width == 128)
	{
		if (std::is_constant_evaluated())
			return shift_bits_right_constexpr(lhs, shift);
		return impl::shift_bits_right_slow(lhs, shift);
	}

	/** @brief Compile-time complete-register right shift. Counts of 128 or more return zero. */
	template <int shift>
	constexpr static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) shift_bits_right(const int_vector_t lhs) noexcept
		requires(using_int && register_width == 128)
	{
		static_assert(shift >= 0, "Whole-register shifts require a non-negative count.");
		if (std::is_constant_evaluated())
			return shift_bits_right_constexpr(lhs, shift);
		return impl::template shift_bits_right<shift>(lhs);
	}

#pragma endregion

#pragma region Conversion Operations

	/** @brief Reinterprets every bit of a complete register as another supported lane type.
	 *  @tparam target_t Destination lane interpretation at the same register width.
	 *  @param vector Source register whose complete bit pattern is preserved.
	 *  @return Destination native register containing exactly the source bits.
	 */
	template <class target_t>
	constexpr static mapped_vector_t<target_t> SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) bit_cast(const vector_t vector) noexcept
		requires ApiAvailable<register_width, target_t>
	{
		if (std::is_constant_evaluated())
			return bit_cast_constexpr<target_t>(vector);
		return std::bit_cast<mapped_vector_t<target_t>>(vector);
	}

	/** @brief Converts 32-bit integer lanes into floating-point lanes.
	 *  @param vector Input integer register.
	 *  @return Floating-point register containing the converted lane values.
	 */
	constexpr static float_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) convert_to_float(int_vector_t vector) noexcept
		requires(element_width == 32 && using_int)
	{
		if (std::is_constant_evaluated())
			return convert_to_float_constexpr(vector);
		if constexpr (register_width == 128)
		{
			if constexpr (using_unsigned)
				return impl::convert_to_float(vector);
			else
				return _mm_cvtepi32_ps(vector);
		}
		else if constexpr (register_width == 256)
		{
			if constexpr (using_unsigned)
				return impl::convert_to_float(vector);
			else
				return _mm256_cvtepi32_ps(vector);
		}
	}

	/** @brief Converts 32-bit floating-point lanes into integer lanes.
	 *  @param vector Input floating-point register.
	 *  @return Integer register containing the converted lane values.
	 */
	constexpr static int_vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) convert_to_int(float_vector_t vector) noexcept
		requires(element_width == 32 && std::same_as<element_t, float>)
	{
		if (std::is_constant_evaluated())
			return convert_to_int_constexpr(vector);
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
	constexpr static auto SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) convert(vector_t vector) noexcept
		requires(element_width == 32)
	{
		if constexpr (std::is_floating_point_v<element_t>)
			return convert_to_int(vector);
		else
			return convert_to_float(vector);
	}

	/** @brief Numerically converts every source lane into one complete destination register.
	 *  @tparam target_t Explicit numeric destination lane type.
	 *  @param vector Source register.
	 *  @return Complete destination native register containing the converted lane values.
	 *  @note The initial conversion surface supports signed or unsigned 32-bit integers to `float`, and `float` to signed 32-bit integers.
	 */
	template <class target_t>
	constexpr static mapped_vector_t<target_t> SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) convert(const vector_t vector) noexcept
		requires((std::same_as<target_t, float> && (std::same_as<element_t, std::int32_t> || std::same_as<element_t, std::uint32_t>)) ||
				 (std::same_as<target_t, std::int32_t> && std::same_as<element_t, float>))
	{
		if constexpr (std::same_as<target_t, float>)
			return convert_to_float(vector);
		else
			return convert_to_int(vector);
	}

#pragma endregion

#pragma region Transform

	/** @brief Applies a SIMD transform whose fixed-width lane results are packed contiguously into integer storage.
	 *  @tparam result_bit_width Number of logical result bits produced per source element.
	 *  @tparam count Number of source elements.
	 *  @tparam Func Callable that accepts `vector_t` and returns an unsigned integer containing packed lane results, with lane zero in the least-significant
	 * bits.
	 *  @param read Source elements to transform.
	 *  @param write Destination storage for the packed result bit stream.
	 *  @param func SIMD transformation that returns one packed result for each loaded register.
	 *  @return None.
	 */
	template <std::size_t result_bit_width, std::size_t count, std::invocable<vector_t> Func>
	constexpr static void SIMD_FLAGS(Neither, ForceInline, Flatten)
		transform_pack(std::span<const element_t, count> read,
					   std::span<packed_element_t<result_bit_width>, packed_element_count<result_bit_width, count>> write, Func &&func) noexcept
		requires(result_bit_width > 0 && result_bit_width <= 64)
	{
		using result_t = std::remove_cvref_t<std::invoke_result_t<Func, vector_t>>;
		using write_t = packed_element_t<result_bit_width>;
		// uintptr_t is the standard unsigned type that most closely represents the target's native integer register width.
		// Accumulating into it lets us write whole machine words instead of updating individual destination bytes.
		using native_word_t = std::uintptr_t;
		static_assert(std::unsigned_integral<result_t> && !std::same_as<result_t, bool>, "Packed SIMD transforms must return an unsigned integer");
		static_assert(element_count * result_bit_width <= 64, "A packed SIMD register result cannot exceed 64 bits");
		static_assert(element_count * result_bit_width <= std::numeric_limits<result_t>::digits,
					  "The packed transform result type must contain every result bit for one SIMD register");
		// Callback results place the first SIMD lane in the least-significant bits. Copying the accumulator directly to
		// sequential storage preserves that lane order only when the least-significant byte is stored first.
		static_assert(std::endian::native == std::endian::little, "Packed SIMD transforms require little-endian integer storage");

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
				const std::uint64_t consumed_mask =
					consumed_bit_count == 64 ? std::numeric_limits<std::uint64_t>::max() : (std::uint64_t{1} << consumed_bit_count) - 1;

				pending |= static_cast<native_word_t>((remaining & consumed_mask) << pending_bit_count);
				remaining = consumed_bit_count == 64 ? 0 : remaining >> consumed_bit_count;
				pending_bit_count += consumed_bit_count;
				result_bit_count -= consumed_bit_count;

				// memcpy permits a native-width store without imposing alignment or aliasing requirements on write_t.
				if constexpr (flushed_native_word_count != 0)
				{
					if (pending_bit_count == native_word_width)
					{
						std::memcpy(write_bytes.data() + write_byte_offset, &pending, sizeof(pending));
						write_byte_offset += sizeof(pending);
						pending = 0;
						pending_bit_count = 0;
					}
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
	template <std::invocable<vector_t> Func> static void SIMD_FLAGS(Neither, Flatten) transform(std::span<element_t> data, Func &&func) noexcept
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
	template <std::invocable<vector_t> Func>
	static void SIMD_FLAGS(Neither, Flatten) transform(std::span<const element_t> lhs, std::span<element_t> write, Func &&func) noexcept
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
	static void SIMD_FLAGS(Neither, Flatten)
		transform(std::span<const element_t> lhs, std::span<const element_t> rhs, std::span<element_t> write, Func &&func) noexcept
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
	/** @brief Validates a logical lane-shuffle selector sequence at overload resolution.
	 *  @tparam indices Logical source-lane indices for every output lane.
	 *  @return `true` when the selector count is exact and every selector names a lane in the complete source register.
	 */
	template <std::size_t... indices> [[nodiscard]] consteval static bool logical_shuffle_indices_valid() noexcept
	{
		return sizeof...(indices) == element_count && ((indices < element_count) && ...);
	}

	/** @brief Extracts the low 128-bit lanes during constant evaluation. */
	[[nodiscard]] constexpr static typename SimdLib::Detail::SimdMappings<128, element_t>::vector_t lower_half_constexpr(const vector_t value) noexcept
	{
		using target_api = Api<128, element_t>;
		const auto source = to_array(value);
		std::array<element_t, target_api::element_count> result{};
		for (std::size_t lane = 0; lane < result.size(); ++lane)
			result[lane] = source[lane];
		return target_api::construct(result);
	}

	/** @brief Interleaves low or high lane halves within each 128-bit group during constant evaluation.
	 *  @tparam high Selects the high source half when
	 * `true`, otherwise the low source half.
	 */
	template <bool high> [[nodiscard]] constexpr static vector_t unpack_constexpr(const vector_t lhs, const vector_t rhs) noexcept
	{
		const auto left = to_array(lhs);
		const auto right = to_array(rhs);
		std::array<element_t, element_count> result{};
		constexpr std::size_t lanes_per_group = 128 / element_width;
		constexpr std::size_t lanes_per_half = lanes_per_group / 2;
		for (std::size_t group = 0; group < element_count; group += lanes_per_group)
		{
			constexpr std::size_t source_half_offset = high ? lanes_per_half : 0;
			for (std::size_t lane = 0; lane < lanes_per_half; ++lane)
			{
				result[group + lane * 2] = left[group + source_half_offset + lane];
				result[group + lane * 2 + 1] = right[group + source_half_offset + lane];
			}
		}
		return construct(result);
	}

	/** @brief Applies a validated logical lane shuffle during constant evaluation. */
	template <std::size_t... indices> [[nodiscard]] constexpr static vector_t shuffle_constexpr(const vector_t value) noexcept
	{
		const auto source = to_array(value);
		constexpr std::array<std::size_t, element_count> selectors{indices...};
		std::array<element_t, element_count> result{};
		for (std::size_t lane = 0; lane < element_count; ++lane)
			result[lane] = source[selectors[lane]];
		return construct(result);
	}

	/** @brief Applies an immediate 16-bit half shuffle during constant evaluation.
	 *  @tparam imm8 Immediate selector fields.
	 *  @tparam high Selects
	 * the high four-lane half in each 128-bit group.
	 */
	template <int imm8, bool high> [[nodiscard]] constexpr static vector_t shuffle_half_constexpr(const vector_t value) noexcept
	{
		const auto source = to_array(value);
		auto result = source;
		constexpr std::size_t lanes_per_group = 8;
		constexpr std::size_t half_offset = high ? 4 : 0;
		for (std::size_t group = 0; group < element_count; group += lanes_per_group)
		{
			for (std::size_t lane = 0; lane < 4; ++lane)
			{
				const std::size_t selected = static_cast<unsigned int>(imm8) >> (lane * 2) & 0x3u;
				result[group + half_offset + lane] = source[group + half_offset + selected];
			}
		}
		return construct(result);
	}

	/** @brief Reinterprets a complete register bit pattern during constant evaluation. */
	template <class target_t> [[nodiscard]] constexpr static mapped_vector_t<target_t> bit_cast_constexpr(const vector_t value) noexcept
	{
		using target_api = Api<register_width, target_t>;
		const auto target_lanes = std::bit_cast<std::array<target_t, target_api::element_count>>(to_array(value));
		return target_api::construct(target_lanes);
	}

	/** @brief Widens only the source prefix required to fill one target register during constant evaluation. */
	template <class target_simd> [[nodiscard]] constexpr static typename target_simd::vector_t widen_constexpr(const vector_t value) noexcept
	{
		using target_element_t = typename target_simd::element_type;
		const auto source = to_array(value);
		std::array<target_element_t, target_simd::element_count> result{};
		for (std::size_t lane = 0; lane < result.size(); ++lane)
			result[lane] = static_cast<target_element_t>(source[lane]);
		return target_simd::construct(result);
	}

	/** @brief Converts signed or unsigned 32-bit integer lanes to float during constant evaluation. */
	[[nodiscard]] constexpr static float_vector_t convert_to_float_constexpr(const int_vector_t value) noexcept
	{
		using target_api = Api<register_width, float>;
		const auto source = to_array(value);
		std::array<float, target_api::element_count> result{};
		for (std::size_t lane = 0; lane < result.size(); ++lane)
			result[lane] = static_cast<float>(source[lane]);
		return target_api::construct(result);
	}

	/** @brief Converts one float with default-MXCSR round-to-nearest-even semantics. */
	[[nodiscard]] constexpr static std::int32_t convert_float_lane_to_int(const float value) noexcept
	{
		constexpr float minimum = -2147483648.0F;
		constexpr float upper_exclusive = 2147483648.0F;
		if (!(value >= minimum && value < upper_exclusive))
			return std::numeric_limits<std::int32_t>::min();

		std::int32_t rounded = static_cast<std::int32_t>(value);
		const float fraction = value - static_cast<float>(rounded);
		if (fraction > 0.5F || (fraction == 0.5F && rounded % 2 != 0))
			++rounded;
		else if (fraction < -0.5F || (fraction == -0.5F && rounded % 2 != 0))
			--rounded;
		return rounded;
	}

	/** @brief Converts float lanes to signed 32-bit integers during constant evaluation. */
	[[nodiscard]] constexpr static int_vector_t convert_to_int_constexpr(const float_vector_t value) noexcept
	{
		using source_api = Api<register_width, float>;
		using target_api = Api<register_width, std::int32_t>;
		const auto source = source_api::to_array(value);
		std::array<std::int32_t, target_api::element_count> result{};
		for (std::size_t lane = 0; lane < result.size(); ++lane)
			result[lane] = convert_float_lane_to_int(source[lane]);
		return target_api::construct(result);
	}

	/** @brief Applies bitwise AND during constant evaluation.
	 *  @param lhs Left-hand input register represented in constant evaluation.
	 *  @param rhs Right-hand input register represented in constant evaluation.
	 *  @return Register containing the bitwise intersection.
	 */
	constexpr static vector_t bitwise_and_constexpr(const vector_t lhs, const vector_t rhs) noexcept
	{
		const auto left = to_array(lhs);
		const auto right = to_array(rhs);
		std::array<element_t, element_count> result{};
		using unsigned_element_t = select_unsigned_integer_t<sizeof(element_t) * 8>;
		for (std::size_t lane = 0; lane < element_count; ++lane)
		{
			const auto left_bits = std::bit_cast<unsigned_element_t>(left[lane]);
			const auto right_bits = std::bit_cast<unsigned_element_t>(right[lane]);
			result[lane] = std::bit_cast<element_t>(static_cast<unsigned_element_t>(left_bits & right_bits));
		}
		return construct(result);
	}

	/** @brief Applies bitwise OR during constant evaluation.
	 *  @param lhs Left-hand input register represented in constant evaluation.
	 *  @param rhs Right-hand input register represented in constant evaluation.
	 *  @return Register containing the bitwise union.
	 */
	constexpr static vector_t bitwise_or_constexpr(const vector_t lhs, const vector_t rhs) noexcept
	{
		const auto left = to_array(lhs);
		const auto right = to_array(rhs);
		std::array<element_t, element_count> result{};
		using unsigned_element_t = select_unsigned_integer_t<sizeof(element_t) * 8>;
		for (std::size_t lane = 0; lane < element_count; ++lane)
		{
			const auto left_bits = std::bit_cast<unsigned_element_t>(left[lane]);
			const auto right_bits = std::bit_cast<unsigned_element_t>(right[lane]);
			result[lane] = std::bit_cast<element_t>(static_cast<unsigned_element_t>(left_bits | right_bits));
		}
		return construct(result);
	}

	/** @brief Applies bitwise XOR during constant evaluation.
	 *  @param lhs Left-hand input register represented in constant evaluation.
	 *  @param rhs Right-hand input register represented in constant evaluation.
	 *  @return Register containing the bitwise exclusive union.
	 */
	constexpr static vector_t bitwise_xor_constexpr(const vector_t lhs, const vector_t rhs) noexcept
	{
		const auto left = to_array(lhs);
		const auto right = to_array(rhs);
		std::array<element_t, element_count> result{};
		using unsigned_element_t = select_unsigned_integer_t<sizeof(element_t) * 8>;
		for (std::size_t lane = 0; lane < element_count; ++lane)
		{
			const auto left_bits = std::bit_cast<unsigned_element_t>(left[lane]);
			const auto right_bits = std::bit_cast<unsigned_element_t>(right[lane]);
			result[lane] = std::bit_cast<element_t>(static_cast<unsigned_element_t>(left_bits ^ right_bits));
		}
		return construct(result);
	}

	/** @brief Applies bitwise AND-NOT during constant evaluation.
	 *  @param lhs Left-hand input register inverted before intersection.
	 *  @param rhs Right-hand input register represented in constant evaluation.
	 *  @return Register containing the intersection of inverted lhs and rhs.
	 */
	constexpr static vector_t bitwise_andnot_constexpr(const vector_t lhs, const vector_t rhs) noexcept
	{
		const auto left = to_array(lhs);
		const auto right = to_array(rhs);
		std::array<element_t, element_count> result{};
		using unsigned_element_t = select_unsigned_integer_t<sizeof(element_t) * 8>;
		for (std::size_t lane = 0; lane < element_count; ++lane)
		{
			const auto left_bits = std::bit_cast<unsigned_element_t>(left[lane]);
			const auto right_bits = std::bit_cast<unsigned_element_t>(right[lane]);
			result[lane] = std::bit_cast<element_t>(static_cast<unsigned_element_t>(~left_bits & right_bits));
		}
		return construct(result);
	}

	/** @brief Applies bitwise inversion during constant evaluation.
	 *  @param value Input register represented in constant evaluation.
	 *  @return Register containing the bitwise inverse.
	 */
	constexpr static vector_t bitwise_not_constexpr(const vector_t value) noexcept
	{
		const auto lanes = to_array(value);
		std::array<element_t, element_count> result{};
		using unsigned_element_t = select_unsigned_integer_t<sizeof(element_t) * 8>;
		for (std::size_t lane = 0; lane < element_count; ++lane)
		{
			const auto bits = std::bit_cast<unsigned_element_t>(lanes[lane]);
			result[lane] = std::bit_cast<element_t>(static_cast<unsigned_element_t>(~bits));
		}
		return construct(result);
	}

	/** @brief Selects lanes using a canonical predicate during constant evaluation.
	 *  @param condition Canonical predicate register containing all-zero or all-one lanes.
	 *  @param when_true Register selected where the corresponding predicate lane is true.
	 *  @param when_false Register selected where the corresponding predicate lane is false.
	 *  @return Register containing the selected lanes.
	 */
	constexpr static vector_t select_constexpr(const vector_t condition, const vector_t when_true, const vector_t when_false) noexcept
	{
		return bitwise_or(bitwise_and(condition, when_true), bitwise_andnot(condition, when_false));
	}

	/** @brief Converts a register to lane storage during constant evaluation.
	 *  @param vector Register represented in constant evaluation.
	 *  @return Array containing the register elements in lane order.
	 */
	constexpr static std::array<element_t, element_count> to_array_constexpr(const vector_t vector) noexcept
	{
		std::array<element_t, element_count> result{};
		for (std::size_t index = 0; index < element_count; ++index)
			result[index] = extract_constexpr(vector, static_cast<int>(index));
		return result;
	}

	/**
	 * @brief Extracts one lane through the portable constant-evaluation representation.
	 * @param lhs Source register represented during constant evaluation.
	 * @param index Selected lane index.
	 * @return Selected scalar lane.
	 */
	constexpr static element_t extract_constexpr(const vector_t lhs, const int index) noexcept
	{
		return Detail::register_get_constexpr<element_t>(lhs, static_cast<std::size_t>(index));
	}

	/**
	 * @brief Replaces one lane through the portable constant-evaluation representation.
	 * @param lhs Source register represented during constant evaluation.
	 * @param rhs Replacement scalar lane.
	 * @param index Selected lane index.
	 * @return Register with the selected lane replaced.
	 */
	constexpr static vector_t insert_constexpr(const vector_t lhs, const element_t rhs, const int index) noexcept
	{
		return Detail::register_insert_constexpr<element_t>(lhs, rhs, static_cast<std::size_t>(index));
	}

	/** @brief Computes the byte-granular movemask during constant evaluation.
	 *  @param lhs Input register represented in constant evaluation.
	 *  @return Byte-granular movemask for the register contents.
	 */
	constexpr static mask_t movemask_constexpr(const vector_t lhs) noexcept
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

	/** @brief Finds the first minimum lane during constant evaluation.
	 *  @param lhs Input register represented in constant evaluation.
	 *  @return Zero-based index of the first minimum element.
	 */
	constexpr static std::size_t min_position_constexpr(const vector_t lhs) noexcept
	{
		const auto values = to_array(lhs);
		return static_cast<std::size_t>(std::min_element(values.begin(), values.end()) - values.begin());
	}

	/** @brief Finds the first maximum lane during constant evaluation.
	 *  @param lhs Input register represented in constant evaluation.
	 *  @return Zero-based index of the first maximum element.
	 */
	constexpr static std::size_t max_position_constexpr(const vector_t lhs) noexcept
	{
		const auto values = to_array(lhs);
		return static_cast<std::size_t>(std::max_element(values.begin(), values.end()) - values.begin());
	}

	/** @brief Computes the element-granular movemask during constant evaluation.
	 *  @param lhs Input register represented in constant evaluation.
	 *  @return Element-granular movemask for the register contents.
	 */
	constexpr static mask_t movemask_slim_constexpr(const vector_t lhs) noexcept
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

	/** @brief Returns the canonical all-one predicate value for one lane. */
	[[nodiscard]] constexpr static element_t comparison_true_lane() noexcept
	{
		using unsigned_element_t = select_unsigned_integer_t<sizeof(element_t) * 8>;
		return std::bit_cast<element_t>(std::numeric_limits<unsigned_element_t>::max());
	}

	/** @brief Compares lanes for equality during constant evaluation. */
	[[nodiscard]] constexpr static vector_t compare_equal_constexpr(const vector_t lhs, const vector_t rhs) noexcept
	{
		const auto left = to_array(lhs);
		const auto right = to_array(rhs);
		std::array<element_t, element_count> result{};
		for (std::size_t index = 0; index < element_count; ++index)
			result[index] = left[index] == right[index] ? comparison_true_lane() : element_t{};
		return construct(result);
	}

	/** @brief Compares lanes for greater-than ordering during constant evaluation. */
	[[nodiscard]] constexpr static vector_t compare_greater_constexpr(const vector_t lhs, const vector_t rhs) noexcept
	{
		const auto left = to_array(lhs);
		const auto right = to_array(rhs);
		std::array<element_t, element_count> result{};
		for (std::size_t index = 0; index < element_count; ++index)
			result[index] = left[index] > right[index] ? comparison_true_lane() : element_t{};
		return construct(result);
	}

	/** @brief Compares lanes for greater-than-or-equal ordering during constant evaluation. */
	[[nodiscard]] constexpr static vector_t compare_greater_equal_constexpr(const vector_t lhs, const vector_t rhs) noexcept
	{
		const auto left = to_array(lhs);
		const auto right = to_array(rhs);
		std::array<element_t, element_count> result{};
		for (std::size_t index = 0; index < element_count; ++index)
			result[index] = left[index] >= right[index] ? comparison_true_lane() : element_t{};
		return construct(result);
	}

	/** @brief Compares lanes for less-than ordering during constant evaluation. */
	[[nodiscard]] constexpr static vector_t compare_less_constexpr(const vector_t lhs, const vector_t rhs) noexcept
	{
		const auto left = to_array(lhs);
		const auto right = to_array(rhs);
		std::array<element_t, element_count> result{};
		for (std::size_t index = 0; index < element_count; ++index)
			result[index] = left[index] < right[index] ? comparison_true_lane() : element_t{};
		return construct(result);
	}

	/** @brief Compares lanes for less-than-or-equal ordering during constant evaluation. */
	[[nodiscard]] constexpr static vector_t compare_less_equal_constexpr(const vector_t lhs, const vector_t rhs) noexcept
	{
		const auto left = to_array(lhs);
		const auto right = to_array(rhs);
		std::array<element_t, element_count> result{};
		for (std::size_t index = 0; index < element_count; ++index)
			result[index] = left[index] <= right[index] ? comparison_true_lane() : element_t{};
		return construct(result);
	}

	/** @brief Left-shifts integer lanes during constant evaluation.
	 *  @param lhs Input integer register represented in constant evaluation.
	 *  @param shift Shift count applied to each lane.
	 *  @return Register containing shifted lanes.
	 */
	constexpr static int_vector_t shift_left_constexpr(const int_vector_t lhs, const int shift) noexcept
	{
		if (shift >= static_cast<int>(element_width))
			return impl::setzero();
		std::array<element_t, element_count> results{};
		for (std::size_t index = 0; index < element_count; ++index)
			results[index] = static_cast<element_t>(extract_constexpr(lhs, static_cast<int>(index)) << shift);
		return impl::construct(results);
	}

	/** @brief Logically right-shifts integer lanes during constant evaluation.
	 *  @param lhs Input integer register represented in constant evaluation.
	 *  @param shift Shift count applied to each lane.
	 *  @return Register containing shifted lanes.
	 */
	constexpr static int_vector_t shift_right_constexpr(const int_vector_t lhs, const int shift) noexcept
	{
		if (shift >= static_cast<int>(element_width))
			return impl::setzero();
		std::array<element_t, element_count> results{};
		for (std::size_t index = 0; index < element_count; ++index)
		{
			results[index] = static_cast<element_t>(static_cast<std::make_unsigned_t<element_t>>(extract_constexpr(lhs, static_cast<int>(index))) >> shift);
		}
		return impl::construct(results);
	}

	/** @brief Arithmetically right-shifts integer lanes during constant evaluation.
	 *  @param lhs Input integer register represented in constant evaluation.
	 *  @param shift Shift count applied to each lane.
	 *  @return Register containing shifted lanes.
	 */
	constexpr static int_vector_t shift_right_arithmetic_constexpr(const int_vector_t lhs, int shift) noexcept
	{
		if (shift >= static_cast<int>(element_width))
			shift = static_cast<int>(element_width) - 1;
		std::array<element_t, element_count> results{};
		for (std::size_t index = 0; index < element_count; ++index)
			results[index] = static_cast<element_t>(extract_constexpr(lhs, static_cast<int>(index)) >> shift);
		return impl::construct(results);
	}

	/** @brief Shifts a complete register toward higher byte indices during constant evaluation.
	 *  @param lhs Input integer register represented in constant evaluation.
	 *  @param shift Runtime-compatible byte count.
	 *  @return Byte-shifted register.
	 */
	constexpr static int_vector_t shift_bytes_left_constexpr(const int_vector_t lhs, const int shift) noexcept
	{
		if (shift <= 0)
			return lhs;
		if (shift >= static_cast<int>(byte_count))
			return impl::setzero();
		const auto sourceBytes = std::bit_cast<std::array<std::uint8_t, byte_count>>(to_array(lhs));
		std::array<std::uint8_t, byte_count> resultBytes{};
		for (std::size_t index = static_cast<std::size_t>(shift); index < byte_count; ++index)
			resultBytes[index] = sourceBytes[index - static_cast<std::size_t>(shift)];
		return construct(std::bit_cast<std::array<element_t, element_count>>(resultBytes));
	}

	/** @brief Shifts a complete register toward lower byte indices during constant evaluation.
	 *  @param lhs Input integer register represented in constant evaluation.
	 *  @param shift Runtime-compatible byte count.
	 *  @return Byte-shifted register.
	 */
	constexpr static int_vector_t shift_bytes_right_constexpr(const int_vector_t lhs, const int shift) noexcept
	{
		if (shift <= 0)
			return lhs;
		if (shift >= static_cast<int>(byte_count))
			return impl::setzero();
		const auto sourceBytes = std::bit_cast<std::array<std::uint8_t, byte_count>>(to_array(lhs));
		std::array<std::uint8_t, byte_count> resultBytes{};
		for (std::size_t index = 0; index + static_cast<std::size_t>(shift) < byte_count; ++index)
			resultBytes[index] = sourceBytes[index + static_cast<std::size_t>(shift)];
		return construct(std::bit_cast<std::array<element_t, element_count>>(resultBytes));
	}

	/**
	 * @brief Shifts a complete 128-bit register left during constant evaluation.
	 * @param lhs Input integer register represented in constant evaluation.
	 * @param shift Runtime-compatible bit count.
	 * @return Shifted register with zero-filled low bits.
	 */
	constexpr static int_vector_t shift_bits_left_constexpr(const int_vector_t lhs, const int shift) noexcept
	{
		if (shift <= 0)
			return lhs;
		if (shift >= 128)
			return impl::setzero();

		const auto source = std::bit_cast<std::array<std::uint64_t, 2>>(to_array(lhs));
		std::array<std::uint64_t, 2> result{};
		if (shift < 64)
		{
			result = {source[0] << shift, (source[1] << shift) | (source[0] >> (64 - shift))};
		}
		else if (shift == 64)
		{
			result = {0, source[0]};
		}
		else
		{
			result = {0, source[0] << (shift - 64)};
		}
		return construct(std::bit_cast<std::array<element_t, element_count>>(result));
	}

	/**
	 * @brief Shifts a complete 128-bit register right during constant evaluation.
	 * @param lhs Input integer register represented in constant evaluation.
	 * @param shift Runtime-compatible bit count.
	 * @return Shifted register with zero-filled high bits.
	 */
	constexpr static int_vector_t shift_bits_right_constexpr(const int_vector_t lhs, const int shift) noexcept
	{
		if (shift <= 0)
			return lhs;
		if (shift >= 128)
			return impl::setzero();

		const auto source = std::bit_cast<std::array<std::uint64_t, 2>>(to_array(lhs));
		std::array<std::uint64_t, 2> result{};
		if (shift < 64)
		{
			result = {(source[0] >> shift) | (source[1] << (64 - shift)), source[1] >> shift};
		}
		else if (shift == 64)
		{
			result = {source[1], 0};
		}
		else
		{
			result = {source[1] >> (shift - 64), 0};
		}
		return construct(std::bit_cast<std::array<element_t, element_count>>(result));
	}

	/** @brief Re-encodes integer lanes so a minimum-position backend yields the first maximum index.
	 *  @param lhs Input integer register.
	 *  @return Transformed register whose first minimum corresponds to the original first maximum.
	 */
	static vector_t SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) TransformForMaxPosition(const vector_t lhs) noexcept
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

/**
 * @brief Selects the widest available SIMD API for an element type.
 * @tparam element_t Element type stored in each SIMD lane.
 *
 * Resolves to `Api<256, element_t>` when the compile target enables the
 * 256-bit facade, and otherwise resolves to `Api<128, element_t>`. The alias
 * is available only when at least the 128-bit facade supports `element_t`.
 */
template <class element_t>
	requires ApiAvailable<128, element_t>
using NativeApi = Api<(is_api_available_v<256, element_t> ? 256 : 128), element_t>;

} // namespace SimdLib
