#include <SimdLib/PartialRegister.h>

#include <cstddef>
#include <cstdint>
#include <utility>

#if defined(__clang__) || defined(__GNUC__)
#define SIMDLIB_PARTIAL_ABI_NOINLINE __attribute__((noinline, used))
#elif SIMDLIB_COMPILER_MSVC
#define SIMDLIB_PARTIAL_ABI_NOINLINE __declspec(noinline) __declspec(dllexport)
#else
#define SIMDLIB_PARTIAL_ABI_NOINLINE __attribute__((noinline))
#endif

using partial_type = SimdLib::PartialRegister<float, SIMDLIB_PARTIAL_ABI_WIDTH, SIMDLIB_PARTIAL_ABI_WIDTH == 256 ? 5 : 3>;
using integer_partial_type = SimdLib::PartialRegister<std::uint32_t, SIMDLIB_PARTIAL_ABI_WIDTH, SIMDLIB_PARTIAL_ABI_WIDTH == 256 ? 5 : 3>;
using native_type = typename partial_type::native_type;
using integer_native_type = typename integer_partial_type::native_type;
using mask_type = typename partial_type::mask_type;

/** @brief Non-inlined value parameter/return proof for one available specialization. */
template <class element_t, std::size_t active_count>
class simdlib_partial_abi_matrix_cell final
{
public:
	using value_type = SimdLib::PartialRegister<element_t, SIMDLIB_PARTIAL_ABI_WIDTH, active_count>;

	/** @brief Passes the specialization unchanged across a value boundary. */
	SIMDLIB_PARTIAL_ABI_NOINLINE static value_type SIMD_FLAGS(InOut, RegisterOnly) pass(value_type value) noexcept
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

/**
 * @brief Verifies native size and alignment for every available active count of one lane type.
 * @tparam element_t Lane type under qualification.
 * @tparam indices Zero-based candidates mapped to active counts starting at one.
 * @return True when every available partial specialization is exactly one native register.
 */
template <class element_t, std::size_t... indices>
[[nodiscard]] consteval bool simdlib_partial_abi_layouts(std::index_sequence<indices...>) noexcept
{
	return ([]<std::size_t active_count>() consteval {
		if constexpr (SimdLib::PartialRegisterAvailable<element_t, SIMDLIB_PARTIAL_ABI_WIDTH, active_count>)
		{
			using candidate = SimdLib::PartialRegister<element_t, SIMDLIB_PARTIAL_ABI_WIDTH, active_count>;
			using candidate_native = typename candidate::native_type;
			return sizeof(candidate) == sizeof(candidate_native) && alignof(candidate) == alignof(candidate_native);
		}
		else
		{
			return true;
		}
	}.template operator()<indices + 1>() && ...);
}

/** @brief Returns one partial value across a non-inlined value boundary. */
SIMDLIB_PARTIAL_ABI_NOINLINE partial_type SIMD_FLAGS(InOut, RegisterOnly) simdlib_partial_abi_return(partial_type lhs, partial_type rhs) noexcept
{
	return lhs + rhs;
}

/** @brief Passes one partial value across a non-inlined value boundary. */
SIMDLIB_PARTIAL_ABI_NOINLINE native_type SIMD_FLAGS(InOut, RegisterOnly) simdlib_partial_abi_pass(partial_type value) noexcept
{
	return value.native;
}

/** @brief Reassigns one partial value through a non-inlined reference boundary. */
SIMDLIB_PARTIAL_ABI_NOINLINE auto SIMD_FLAGS(In) simdlib_partial_abi_reassign(partial_type &lhs, partial_type rhs) noexcept -> partial_type &
{
	lhs = lhs + rhs;
	return lhs;
}

/** @brief Returns a partial predicate across a non-inlined boundary. */
SIMDLIB_PARTIAL_ABI_NOINLINE mask_type SIMD_FLAGS(In, RegisterOnly) simdlib_partial_abi_mask(partial_type lhs, partial_type rhs) noexcept
{
	return lhs.compare_greater(rhs);
}

/** @brief Returns an active-lane scalar observation across a non-inlined boundary. */
SIMDLIB_PARTIAL_ABI_NOINLINE std::uint32_t SIMD_FLAGS(In, RegisterOnly) simdlib_partial_abi_scalar(integer_partial_type value) noexcept
{
	return value.movemask();
}

/** @brief Returns a type-changing bit-cast result across a non-inlined boundary. */
SIMDLIB_PARTIAL_ABI_NOINLINE integer_native_type SIMD_FLAGS(InOut, RegisterOnly) simdlib_partial_abi_type_change(partial_type value) noexcept
{
	return value.template bit_cast<std::uint32_t>().native;
}

/** @brief Calls a non-inlined partial-value boundary to expose opaque-call handling. */
SIMDLIB_PARTIAL_ABI_NOINLINE partial_type SIMD_FLAGS(InOut, RegisterOnly) simdlib_partial_abi_opaque(partial_type lhs, partial_type rhs) noexcept
{
	return simdlib_partial_abi_return(lhs, rhs) - rhs;
}

/** @brief Keeps several partial values live across arithmetic to expose register pressure. */
SIMDLIB_PARTIAL_ABI_NOINLINE partial_type SIMD_FLAGS(InOut, RegisterOnly) simdlib_partial_abi_pressure(
	partial_type a, partial_type b, partial_type c, partial_type d) noexcept
{
	return (a + b) * (c - d);
}

static_assert(sizeof(partial_type) == sizeof(native_type));
static_assert(alignof(partial_type) == alignof(native_type));
static_assert(sizeof(integer_partial_type) == sizeof(integer_native_type));
static_assert(alignof(integer_partial_type) == alignof(integer_native_type));
static_assert(simdlib_partial_abi_layouts<std::int8_t>(
	std::make_index_sequence<SimdLib::Api<SIMDLIB_PARTIAL_ABI_WIDTH, std::int8_t>::element_count>{}));
static_assert(simdlib_partial_abi_layouts<std::uint8_t>(
	std::make_index_sequence<SimdLib::Api<SIMDLIB_PARTIAL_ABI_WIDTH, std::uint8_t>::element_count>{}));
static_assert(simdlib_partial_abi_layouts<std::int16_t>(
	std::make_index_sequence<SimdLib::Api<SIMDLIB_PARTIAL_ABI_WIDTH, std::int16_t>::element_count>{}));
static_assert(simdlib_partial_abi_layouts<std::uint16_t>(
	std::make_index_sequence<SimdLib::Api<SIMDLIB_PARTIAL_ABI_WIDTH, std::uint16_t>::element_count>{}));
static_assert(simdlib_partial_abi_layouts<std::int32_t>(
	std::make_index_sequence<SimdLib::Api<SIMDLIB_PARTIAL_ABI_WIDTH, std::int32_t>::element_count>{}));
static_assert(simdlib_partial_abi_layouts<std::uint32_t>(
	std::make_index_sequence<SimdLib::Api<SIMDLIB_PARTIAL_ABI_WIDTH, std::uint32_t>::element_count>{}));
static_assert(simdlib_partial_abi_layouts<std::int64_t>(
	std::make_index_sequence<SimdLib::Api<SIMDLIB_PARTIAL_ABI_WIDTH, std::int64_t>::element_count>{}));
static_assert(simdlib_partial_abi_layouts<std::uint64_t>(
	std::make_index_sequence<SimdLib::Api<SIMDLIB_PARTIAL_ABI_WIDTH, std::uint64_t>::element_count>{}));
static_assert(simdlib_partial_abi_layouts<float>(
	std::make_index_sequence<SimdLib::Api<SIMDLIB_PARTIAL_ABI_WIDTH, float>::element_count>{}));
static_assert(simdlib_partial_abi_layouts<double>(
	std::make_index_sequence<SimdLib::Api<SIMDLIB_PARTIAL_ABI_WIDTH, double>::element_count>{}));

#undef SIMDLIB_PARTIAL_ABI_NOINLINE
