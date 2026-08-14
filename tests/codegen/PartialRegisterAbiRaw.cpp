#include <SimdLib/Api.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

#if SIMDLIB_COMPILER_MSVC
#define SIMDLIB_PARTIAL_ABI_NOINLINE __declspec(noinline)
#else
#define SIMDLIB_PARTIAL_ABI_NOINLINE __attribute__((noinline))
#endif

using api_type = SimdLib::Api<SIMDLIB_PARTIAL_ABI_WIDTH, float>;
using integer_api_type = SimdLib::Api<SIMDLIB_PARTIAL_ABI_WIDTH, std::uint32_t>;
using native_type = typename api_type::vector_t;
using integer_native_type = typename integer_api_type::vector_t;

/** @brief Native-vector value parameter/return mirror for one specialization cell. */
template <class element_t, std::size_t active_count>
class simdlib_partial_abi_matrix_cell final
{
public:
	using value_type = typename SimdLib::Api<SIMDLIB_PARTIAL_ABI_WIDTH, element_t>::vector_t;

	/** @brief Passes the native vector unchanged across a value boundary. */
	SIMDLIB_PARTIAL_ABI_NOINLINE static value_type SIMD_FLAGS(InOut) pass(value_type value) noexcept
	{
		return value;
	}
};

#define SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, count) template class simdlib_partial_abi_matrix_cell<type, count>
#if SIMDLIB_PARTIAL_ABI_WIDTH == 128
#define SIMDLIB_PARTIAL_ABI_COUNTS_8(type) \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 1); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 2); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 3); \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 4); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 5); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 6); \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 7); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 8); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 9); \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 10); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 11); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 12); \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 13); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 14); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 15)
#define SIMDLIB_PARTIAL_ABI_COUNTS_16(type) \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 1); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 2); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 3); \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 4); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 5); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 6); \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 7)
#define SIMDLIB_PARTIAL_ABI_COUNTS_32(type) \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 1); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 2); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 3)
#define SIMDLIB_PARTIAL_ABI_COUNTS_64(type) SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 1)
#else
#define SIMDLIB_PARTIAL_ABI_COUNTS_8(type) \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 17); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 18); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 19); \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 20); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 21); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 22); \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 23); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 24); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 25); \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 26); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 27); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 28); \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 29); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 30); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 31)
#define SIMDLIB_PARTIAL_ABI_COUNTS_16(type) \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 9); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 10); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 11); \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 12); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 13); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 14); \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 15)
#define SIMDLIB_PARTIAL_ABI_COUNTS_32(type) \
	SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 5); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 6); SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 7)
#define SIMDLIB_PARTIAL_ABI_COUNTS_64(type) SIMDLIB_PARTIAL_ABI_INSTANTIATE(type, 3)
#endif

SIMDLIB_PARTIAL_ABI_COUNTS_8(std::int8_t);
SIMDLIB_PARTIAL_ABI_COUNTS_8(std::uint8_t);
SIMDLIB_PARTIAL_ABI_COUNTS_16(std::int16_t);
SIMDLIB_PARTIAL_ABI_COUNTS_16(std::uint16_t);
SIMDLIB_PARTIAL_ABI_COUNTS_32(std::int32_t);
SIMDLIB_PARTIAL_ABI_COUNTS_32(std::uint32_t);
SIMDLIB_PARTIAL_ABI_COUNTS_32(float);
SIMDLIB_PARTIAL_ABI_COUNTS_64(std::int64_t);
SIMDLIB_PARTIAL_ABI_COUNTS_64(std::uint64_t);
SIMDLIB_PARTIAL_ABI_COUNTS_64(double);

#undef SIMDLIB_PARTIAL_ABI_COUNTS_64
#undef SIMDLIB_PARTIAL_ABI_COUNTS_32
#undef SIMDLIB_PARTIAL_ABI_COUNTS_16
#undef SIMDLIB_PARTIAL_ABI_COUNTS_8
#undef SIMDLIB_PARTIAL_ABI_INSTANTIATE

/** @brief Applies the PartialRegister inactive-suffix projection to a raw floating vector. */
[[nodiscard]] constexpr native_type normalize_partial(native_type value) noexcept
{
	constexpr std::size_t active_lanes = SIMDLIB_PARTIAL_ABI_WIDTH == 256 ? 5 : 3;
	constexpr auto filter = [] {
		std::array<float, api_type::element_count> lanes{};
		constexpr auto true_bits = std::array<std::byte, sizeof(float)>{std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff}};
		for (std::size_t lane = 0; lane < active_lanes; ++lane)
			lanes[lane] = std::bit_cast<float>(true_bits);
		return lanes;
	}();
	return api_type::bitwise_and(value, api_type::construct(filter));
}

/** @brief Raw native mirror for a partial value return. */
SIMDLIB_PARTIAL_ABI_NOINLINE native_type SIMD_FLAGS(InOut) simdlib_partial_abi_return(native_type lhs, native_type rhs) noexcept
{
	return api_type::add(lhs, rhs);
}

/** @brief Raw native mirror for a partial value parameter. */
SIMDLIB_PARTIAL_ABI_NOINLINE native_type SIMD_FLAGS(InOut) simdlib_partial_abi_pass(native_type value) noexcept
{
	return value;
}

/** @brief Raw native mirror for partial-value reassignment. */
SIMDLIB_PARTIAL_ABI_NOINLINE auto SIMD_FLAGS(In) simdlib_partial_abi_reassign(native_type &lhs, native_type rhs) noexcept -> native_type &
{
	lhs = api_type::add(lhs, rhs);
	return lhs;
}

/** @brief Raw native mirror for a partial predicate result. */
SIMDLIB_PARTIAL_ABI_NOINLINE native_type SIMD_FLAGS(InOut) simdlib_partial_abi_mask(native_type lhs, native_type rhs) noexcept
{
	return normalize_partial(api_type::compare_greater(lhs, rhs));
}

/** @brief Raw native mirror for an active-lane scalar observation. */
SIMDLIB_PARTIAL_ABI_NOINLINE std::uint32_t SIMD_FLAGS(In) simdlib_partial_abi_scalar(integer_native_type value) noexcept
{
	return integer_api_type::movemask(value);
}

/** @brief Raw native mirror for a same-width type-changing result. */
SIMDLIB_PARTIAL_ABI_NOINLINE integer_native_type SIMD_FLAGS(InOut) simdlib_partial_abi_type_change(native_type value) noexcept
{
	return std::bit_cast<integer_native_type>(value);
}

/** @brief Raw native mirror for opaque-call handling. */
SIMDLIB_PARTIAL_ABI_NOINLINE native_type SIMD_FLAGS(InOut) simdlib_partial_abi_opaque(native_type lhs, native_type rhs) noexcept
{
	return normalize_partial(api_type::subtract(simdlib_partial_abi_return(lhs, rhs), rhs));
}

/** @brief Raw native mirror for register-pressure handling. */
SIMDLIB_PARTIAL_ABI_NOINLINE native_type SIMD_FLAGS(InOut) simdlib_partial_abi_pressure(
	native_type a, native_type b, native_type c, native_type d) noexcept
{
	return api_type::multiply(api_type::add(a, b), normalize_partial(api_type::subtract(c, d)));
}

#undef SIMDLIB_PARTIAL_ABI_NOINLINE
