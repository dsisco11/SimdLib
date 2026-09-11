#pragma once

#if SIMDLIB_CODEGEN_USE_WRAPPER
#include <SimdLib/Register.h>
#else
#include <SimdLib/Api.h>
#endif

#include <cstddef>
#include <cstdint>
#include <span>

#if SIMDLIB_COMPILER_MSVC
#define SIMDLIB_CODEGEN_NOINLINE __declspec(noinline)
#else
#define SIMDLIB_CODEGEN_NOINLINE __attribute__((noinline))
#endif

namespace SimdLibCodegen
{

using api_type = SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, float>;
using native_type = typename api_type::vector_t;
#if SIMDLIB_CODEGEN_USE_WRAPPER
using register_type = SimdLib::Register<float, SIMDLIB_REGISTER_TEST_WIDTH>;
#endif
using uint_api_type = SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, std::uint32_t>;
using uint_native_type = typename uint_api_type::vector_t;
using byte_api_type = SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, std::uint8_t>;
#if SIMDLIB_CODEGEN_USE_WRAPPER
using uint_register_type = SimdLib::Register<std::uint32_t, SIMDLIB_REGISTER_TEST_WIDTH>;
#endif

#if SIMDLIB_CODEGEN_USE_WRAPPER
using value_type = register_type;
#else
using value_type = native_type;
#endif

/** @brief Converts the fixture value to its native vector representation. */
native_type SIMD_FLAGS(Out, RegisterOnly, ForceInline) unwrap(value_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return value.native;
#else
	return value;
#endif
}

/** @brief Converts a native vector to the fixture value representation. */
value_type SIMD_FLAGS(In, RegisterOnly, ForceInline) wrap(native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return value_type{value};
#else
	return value;
#endif
}

} // namespace SimdLibCodegen

using SimdLibCodegen::native_type;
using SimdLibCodegen::value_type;

/** @brief Opaque call boundary used to keep a register value live across a separately compiled call. */
SIMDLIB_CODEGEN_NOINLINE void SIMD_FLAGS(In) simdlib_codegen_opaque_sink(native_type value) noexcept;

/** @brief Forced-inline ternary expression fixture. */
SIMDLIB_CODEGEN_NOINLINE native_type SIMD_FLAGS(InOut, RegisterOnly) simdlib_codegen_ternary(native_type lhs, native_type rhs, native_type addend) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return ((SimdLibCodegen::register_type{lhs} * SimdLibCodegen::register_type{rhs}) + SimdLibCodegen::register_type{addend}).native;
#else
	return SimdLibCodegen::api_type::add(SimdLibCodegen::api_type::multiply(lhs, rhs), addend);
#endif
}

/** @brief Compare-and-combine mask fixture. */
SIMDLIB_CODEGEN_NOINLINE native_type SIMD_FLAGS(InOut, RegisterOnly) simdlib_codegen_mask_combine(native_type lhs, native_type rhs) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	const SimdLibCodegen::register_type left{lhs};
	const SimdLibCodegen::register_type right{rhs};
	return (left.compare_equal(right) | left.compare_greater(right)).native;
#else
	return SimdLibCodegen::api_type::bitwise_or(SimdLibCodegen::api_type::compare_equal(lhs, rhs), SimdLibCodegen::api_type::compare_greater(lhs, rhs));
#endif
}

/** @brief Compare-and-select mask fixture. */
SIMDLIB_CODEGEN_NOINLINE native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_mask_select(native_type lhs, native_type rhs, native_type when_true, native_type when_false) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type{lhs}
		.compare_greater(SimdLibCodegen::register_type{rhs})
		.select(SimdLibCodegen::register_type{when_true}, SimdLibCodegen::register_type{when_false})
		.native;
#else
	const native_type condition = SimdLibCodegen::api_type::compare_greater(lhs, rhs);
	return SimdLibCodegen::api_type::select(condition, when_true, when_false);
#endif
}

/** @brief Compact predicate-bit fixture. */
SIMDLIB_CODEGEN_NOINLINE std::uint32_t SIMD_FLAGS(In, RegisterOnly) simdlib_codegen_mask_bits(native_type lhs, native_type rhs) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type{lhs}.compare_equal(SimdLibCodegen::register_type{rhs}).bits();
#else
	return static_cast<std::uint32_t>(SimdLibCodegen::api_type::movemask_slim(SimdLibCodegen::api_type::compare_equal(lhs, rhs)));
#endif
}

/** @brief Any-lane predicate reduction fixture. */
SIMDLIB_CODEGEN_NOINLINE bool SIMD_FLAGS(In, RegisterOnly) simdlib_codegen_mask_any(native_type lhs, native_type rhs) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type{lhs}.compare_equal(SimdLibCodegen::register_type{rhs}).any();
#else
	const auto predicate = SimdLibCodegen::api_type::compare_equal(lhs, rhs);
	const auto bits = SimdLibCodegen::api_type::template bit_cast<std::uint8_t>(predicate);
	return !SimdLibCodegen::byte_api_type::testz(bits, bits);
#endif
}

/** @brief All-lane predicate reduction fixture. */
SIMDLIB_CODEGEN_NOINLINE bool SIMD_FLAGS(In, RegisterOnly) simdlib_codegen_mask_all(native_type lhs, native_type rhs) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type{lhs}.compare_equal(SimdLibCodegen::register_type{rhs}).all();
#else
	constexpr std::uint32_t all_bits = (std::uint32_t{1} << SimdLibCodegen::api_type::element_count) - 1;
	return static_cast<std::uint32_t>(SimdLibCodegen::api_type::movemask_slim(SimdLibCodegen::api_type::compare_equal(lhs, rhs))) == all_bits;
#endif
}

/** @brief Native-result fixture. */
SIMDLIB_CODEGEN_NOINLINE native_type SIMD_FLAGS(InOut, RegisterOnly) simdlib_codegen_native(native_type value) noexcept
{
	return SimdLibCodegen::unwrap(SimdLibCodegen::wrap(value));
}

/** @brief Broadcast-reuse fixture. */
SIMDLIB_CODEGEN_NOINLINE native_type SIMD_FLAGS(Out, RegisterOnly) simdlib_codegen_broadcast_reuse(float value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	const auto broadcast = SimdLibCodegen::register_type::broadcast(value);
	return (broadcast + broadcast).native;
#else
	const auto broadcast = SimdLibCodegen::api_type::set1(value);
	return SimdLibCodegen::api_type::add(broadcast, broadcast);
#endif
}

/** @brief Highest-lane observation fixture. */
SIMDLIB_CODEGEN_NOINLINE float SIMD_FLAGS(In, RegisterOnly) simdlib_codegen_lane_last(native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type{value}.template lane<SimdLibCodegen::register_type::lane_count - 1>();
#else
	return SimdLibCodegen::api_type::template extract<static_cast<int>(SimdLibCodegen::api_type::element_count - 1)>(value);
#endif
}

/** @brief Full-register load, operation, and store fixture. */
SIMDLIB_CODEGEN_NOINLINE void simdlib_codegen_load_operate_store(const float *source, float *destination) noexcept
{
	constexpr auto count = SimdLibCodegen::api_type::element_count;
#if SIMDLIB_CODEGEN_USE_WRAPPER
	const auto value = SimdLibCodegen::register_type::load(std::span<const float, count>{source, count});
	(value + value).store(std::span<float, count>{destination, count});
#else
	const auto value = SimdLibCodegen::api_type::load(std::span<const float, count>{source, count});
	SimdLibCodegen::api_type::store(SimdLibCodegen::api_type::add(value, value), std::span<float, count>{destination, count});
#endif
}

/** @brief Aligned full-register load/store fixture. */
SIMDLIB_CODEGEN_NOINLINE void simdlib_codegen_aligned_transfer(const float *source, float *destination) noexcept
{
	constexpr auto count = SimdLibCodegen::api_type::element_count;
#if SIMDLIB_CODEGEN_USE_WRAPPER
	SimdLibCodegen::register_type::load_aligned(std::span<const float, count>{source, count}).store_aligned(std::span<float, count>{destination, count});
#else
	SimdLibCodegen::api_type::store_aligned(SimdLibCodegen::api_type::load_aligned(std::span<const float, count>{source, count}),
											std::span<float, count>{destination, count});
#endif
}

/** @brief Exact-byte load/store fixture. */
SIMDLIB_CODEGEN_NOINLINE void simdlib_codegen_byte_transfer(const std::byte *source, std::byte *destination) noexcept
{
	constexpr auto count = SimdLibCodegen::api_type::byte_count;
#if SIMDLIB_CODEGEN_USE_WRAPPER
	SimdLibCodegen::register_type::load_bytes(std::span<const std::byte, count>{source, count}).store_bytes(std::span<std::byte, count>{destination, count});
#else
	SimdLibCodegen::api_type::store(SimdLibCodegen::api_type::load(std::span<const std::byte, count>{source, count}),
									std::span<std::byte, count>{destination, count});
#endif
}

/** @brief Copy/move special-member fixture. */
SIMDLIB_CODEGEN_NOINLINE native_type SIMD_FLAGS(InOut, RegisterOnly) simdlib_codegen_special_members(native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	SimdLibCodegen::register_type first{value};
	const SimdLibCodegen::register_type second{first};
	first = second;
	return first.native;
#else
	native_type first = value;
	const native_type second = first;
	first = second;
	return first;
#endif
}

/** @brief Mutating-reference fixture. */
SIMDLIB_CODEGEN_NOINLINE void SIMD_FLAGS(In) simdlib_codegen_mutate(native_type &lhs, native_type rhs) noexcept
{
	value_type wrapped_lhs = SimdLibCodegen::wrap(lhs);
	const value_type wrapped_rhs = SimdLibCodegen::wrap(rhs);
#if SIMDLIB_CODEGEN_USE_WRAPPER
	wrapped_lhs = wrapped_lhs + wrapped_rhs;
#else
	wrapped_lhs = SimdLibCodegen::api_type::add(wrapped_lhs, wrapped_rhs);
#endif
	lhs = SimdLibCodegen::unwrap(wrapped_lhs);
}

/** @brief Controlled register-pressure fixture. */
SIMDLIB_CODEGEN_NOINLINE native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_pressure(native_type a, native_type b, native_type c, native_type d, native_type e, native_type f, native_type g, native_type h) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	const SimdLibCodegen::register_type ab = SimdLibCodegen::register_type{a} + SimdLibCodegen::register_type{b};
	const SimdLibCodegen::register_type cd = SimdLibCodegen::register_type{c} + SimdLibCodegen::register_type{d};
	const SimdLibCodegen::register_type ef = SimdLibCodegen::register_type{e} + SimdLibCodegen::register_type{f};
	const SimdLibCodegen::register_type gh = SimdLibCodegen::register_type{g} + SimdLibCodegen::register_type{h};
	return ((ab + cd) + (ef + gh)).native;
#else
	const native_type ab = SimdLibCodegen::api_type::add(a, b);
	const native_type cd = SimdLibCodegen::api_type::add(c, d);
	const native_type ef = SimdLibCodegen::api_type::add(e, f);
	const native_type gh = SimdLibCodegen::api_type::add(g, h);
	return SimdLibCodegen::unwrap(
		SimdLibCodegen::wrap(SimdLibCodegen::api_type::add(SimdLibCodegen::api_type::add(ab, cd), SimdLibCodegen::api_type::add(ef, gh))));
#endif
}

/** @brief Chained bitwise-expression fixture including the public andnot polarity. */
SIMDLIB_CODEGEN_NOINLINE native_type SIMD_FLAGS(InOut, RegisterOnly) simdlib_codegen_basic_bitwise(native_type lhs, native_type rhs) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	const SimdLibCodegen::register_type left{lhs};
	const SimdLibCodegen::register_type right{rhs};
	return ((left & right) | (left ^ ~right)).andnot(right).native;
#else
	const native_type combined = SimdLibCodegen::api_type::bitwise_or(SimdLibCodegen::api_type::bitwise_and(lhs, rhs),
																	  SimdLibCodegen::api_type::bitwise_xor(lhs, SimdLibCodegen::api_type::bitwise_not(rhs)));
	return SimdLibCodegen::api_type::bitwise_andnot(combined, rhs);
#endif
}

/** @brief Local reassignment expression fixture. */
SIMDLIB_CODEGEN_NOINLINE native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_reassignment_arithmetic(native_type lhs, native_type rhs, native_type multiplier) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	SimdLibCodegen::register_type result{lhs};
	result = result + SimdLibCodegen::register_type{rhs};
	result = result * SimdLibCodegen::register_type{multiplier};
	return result.native;
#else
	return SimdLibCodegen::api_type::multiply(SimdLibCodegen::api_type::add(lhs, rhs), multiplier);
#endif
}

/** @brief Explicit scalar-broadcast arithmetic-chain fixture. */
SIMDLIB_CODEGEN_NOINLINE native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_basic_broadcast_chain(native_type value, float scale, float offset) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return ((SimdLibCodegen::register_type{value} * SimdLibCodegen::register_type::broadcast(scale)) + SimdLibCodegen::register_type::broadcast(offset)).native;
#else
	return SimdLibCodegen::api_type::add(SimdLibCodegen::api_type::multiply(value, SimdLibCodegen::api_type::set1(scale)),
										 SimdLibCodegen::api_type::set1(offset));
#endif
}

/** @brief Immediate per-lane unsigned left-shift fixture. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_basic_shift_left_immediate(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return (SimdLibCodegen::uint_register_type{value} << 3).native;
#else
	return SimdLibCodegen::uint_api_type::shift_left(value, 3);
#endif
}

#if SIMDLIB_REGISTER_TEST_WIDTH == 128
/** @brief Static complete-register bit-shift fixture. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut)
	simdlib_codegen_complete_shift_static(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bits_left<19>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bits_left<19>(value);
#endif
}

/** @brief Runtime complete-register bit-shift fixture. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut)
	simdlib_codegen_complete_shift_runtime(SimdLibCodegen::uint_native_type value, int count) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.shift_bits_right_slow(count).native;
#else
	return SimdLibCodegen::uint_api_type::shift_bits_right_slow(value, count);
#endif
}

/** @brief Runtime complete-register byte-shift fixture. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut)
	simdlib_codegen_complete_byte_shift(SimdLibCodegen::uint_native_type value, int count) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.shift_bytes_left_slow(count).native;
#else
	return SimdLibCodegen::uint_api_type::shift_bytes_left_slow(value, count);
#endif
}
#endif

/** @brief Immediate complete-register byte left shift by 0 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_left_0(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_left<0>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_left<0>(value);
#endif
}

/** @brief Immediate complete-register byte right shift by 0 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_right_0(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_right<0>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_right<0>(value);
#endif
}

/** @brief Immediate complete-register byte left shift by 1 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_left_1(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_left<1>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_left<1>(value);
#endif
}

/** @brief Immediate complete-register byte right shift by 1 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_right_1(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_right<1>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_right<1>(value);
#endif
}

/** @brief Immediate complete-register byte left shift by 7 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_left_7(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_left<7>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_left<7>(value);
#endif
}

/** @brief Immediate complete-register byte right shift by 7 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_right_7(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_right<7>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_right<7>(value);
#endif
}

/** @brief Immediate complete-register byte left shift by 15 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_left_15(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_left<15>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_left<15>(value);
#endif
}

/** @brief Immediate complete-register byte right shift by 15 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_right_15(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_right<15>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_right<15>(value);
#endif
}

/** @brief Immediate complete-register byte left shift by 16 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_left_16(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_left<16>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_left<16>(value);
#endif
}

/** @brief Immediate complete-register byte right shift by 16 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_right_16(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_right<16>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_right<16>(value);
#endif
}

/** @brief Immediate complete-register byte left shift by 17 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_left_17(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_left<17>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_left<17>(value);
#endif
}

/** @brief Immediate complete-register byte right shift by 17 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_right_17(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_right<17>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_right<17>(value);
#endif
}

#if SIMDLIB_REGISTER_TEST_WIDTH == 256
/** @brief Immediate complete-register byte left shift by 31 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_left_31(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_left<31>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_left<31>(value);
#endif
}

/** @brief Immediate complete-register byte right shift by 31 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_right_31(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_right<31>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_right<31>(value);
#endif
}

/** @brief Immediate complete-register byte left shift by 32 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_left_32(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_left<32>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_left<32>(value);
#endif
}

/** @brief Immediate complete-register byte right shift by 32 bytes. */
SIMDLIB_CODEGEN_NOINLINE SimdLibCodegen::uint_native_type SIMD_FLAGS(InOut, RegisterOnly)
	simdlib_codegen_shift_bytes_right_32(SimdLibCodegen::uint_native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::uint_register_type{value}.template shift_bytes_right<32>().native;
#else
	return SimdLibCodegen::uint_api_type::template shift_bytes_right<32>(value);
#endif
}

#endif

/** @brief Opaque-call fixture used to compare wrapper and raw spill behavior. */
SIMDLIB_CODEGEN_NOINLINE native_type SIMD_FLAGS(InOut) simdlib_codegen_opaque(native_type value) noexcept
{
	const value_type wrapped = SimdLibCodegen::wrap(value);
	simdlib_codegen_opaque_sink(SimdLibCodegen::unwrap(wrapped));
	return SimdLibCodegen::unwrap(wrapped);
}

#undef SIMDLIB_CODEGEN_NOINLINE
