#pragma once

#include <SimdLib/Register.h>

#include <array>
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
using backend_type = SimdLib::Detail::SimdMappings<SIMDLIB_REGISTER_TEST_WIDTH, float>;
using native_type = typename api_type::vector_t;
using register_type = SimdLib::Register<float, SIMDLIB_REGISTER_TEST_WIDTH>;
using mask_type = SimdLib::RegisterMask<float, SIMDLIB_REGISTER_TEST_WIDTH>;

#if SIMDLIB_CODEGEN_USE_WRAPPER
using value_type = register_type;
using predicate_type = mask_type;
#else
using value_type = native_type;
using predicate_type = native_type;
#endif

/** @brief Converts the fixture value to its native vector representation. */
SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY native_type VECTORCALL unwrap(value_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return value.native();
#else
	return value;
#endif
}

/** @brief Converts a native vector to the fixture value representation. */
SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY value_type VECTORCALL wrap(native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return value_type(value);
#else
	return value;
#endif
}

/** @brief Converts a native predicate vector to the fixture predicate representation. */
SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY predicate_type VECTORCALL zero_predicate() noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return predicate_type{};
#else
	return api_type::setzero();
#endif
}

/** @brief Stores a native register to potentially unaligned storage. */
SIMDLIB_FORCE_INLINE void VECTORCALL
	store_native(native_type value, float *destination) noexcept
{
#if SIMDLIB_REGISTER_TEST_WIDTH == 128
	_mm_storeu_ps(destination, value);
#else
	_mm256_storeu_ps(destination, value);
#endif
}

} // namespace SimdLibCodegen

using SimdLibCodegen::native_type;
using SimdLibCodegen::predicate_type;
using SimdLibCodegen::value_type;

/** @brief Opaque call boundary used to keep a register value live across a separately compiled call. */
SIMDLIB_CODEGEN_NOINLINE void VECTORCALL
	simdlib_codegen_opaque_sink(native_type value) noexcept;

/** @brief Forced-inline unary expression fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
	simdlib_codegen_unary(native_type value) noexcept
{
	const value_type wrapped = SimdLibCodegen::wrap(value);
	return SimdLibCodegen::unwrap(
		SimdLibCodegen::wrap(SimdLibCodegen::api_type::bitwise_not(SimdLibCodegen::unwrap(wrapped))));
}

/** @brief Forced-inline binary expression fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
	simdlib_codegen_binary(native_type lhs, native_type rhs) noexcept
{
	const value_type wrapped_lhs = SimdLibCodegen::wrap(lhs);
	const value_type wrapped_rhs = SimdLibCodegen::wrap(rhs);
	return SimdLibCodegen::unwrap(SimdLibCodegen::wrap(
		SimdLibCodegen::api_type::add(SimdLibCodegen::unwrap(wrapped_lhs), SimdLibCodegen::unwrap(wrapped_rhs))));
}

/** @brief Forced-inline ternary expression fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL simdlib_codegen_ternary(
	native_type lhs,
	native_type rhs,
	native_type addend) noexcept
{
	const value_type wrapped_lhs = SimdLibCodegen::wrap(lhs);
	const value_type wrapped_rhs = SimdLibCodegen::wrap(rhs);
	const value_type wrapped_addend = SimdLibCodegen::wrap(addend);
	const native_type product = SimdLibCodegen::api_type::multiply(
		SimdLibCodegen::unwrap(wrapped_lhs), SimdLibCodegen::unwrap(wrapped_rhs));
	return SimdLibCodegen::unwrap(SimdLibCodegen::wrap(
		SimdLibCodegen::api_type::add(product, SimdLibCodegen::unwrap(wrapped_addend))));
}

/** @brief Scalar-result fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE std::uint32_t VECTORCALL
	simdlib_codegen_scalar(native_type value) noexcept
{
	return SimdLibCodegen::api_type::movemask(SimdLibCodegen::unwrap(SimdLibCodegen::wrap(value)));
}

/** @brief Register-shaped mask-result fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
simdlib_codegen_mask(native_type lhs, native_type rhs) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type(lhs).compare_equal(SimdLibCodegen::register_type(rhs)).native();
#else
	return SimdLibCodegen::backend_type::cmpeq(lhs, rhs);
#endif
}

/** @brief Compare-and-combine mask fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
	simdlib_codegen_mask_combine(native_type lhs, native_type rhs) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	const SimdLibCodegen::register_type left(lhs);
	const SimdLibCodegen::register_type right(rhs);
	return (left.compare_equal(right) | left.compare_greater(right)).native();
#else
	return SimdLibCodegen::api_type::bitwise_or(
		SimdLibCodegen::backend_type::cmpeq(lhs, rhs), SimdLibCodegen::backend_type::cmpgt(lhs, rhs));
#endif
}

/** @brief Compare-and-select mask fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL simdlib_codegen_mask_select(
	native_type lhs,
	native_type rhs,
	native_type when_true,
	native_type when_false) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type(lhs)
		.compare_greater(SimdLibCodegen::register_type(rhs))
		.select(SimdLibCodegen::register_type(when_true), SimdLibCodegen::register_type(when_false))
		.native();
#else
	const native_type condition = SimdLibCodegen::backend_type::cmpgt(lhs, rhs);
	return SimdLibCodegen::backend_type::select(condition, when_true, when_false);
#endif
}

/** @brief Compact predicate-bit fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE std::uint32_t VECTORCALL
	simdlib_codegen_mask_bits(native_type lhs, native_type rhs) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type(lhs).compare_equal(SimdLibCodegen::register_type(rhs)).bits();
#else
	return static_cast<std::uint32_t>(
		SimdLibCodegen::api_type::movemask_slim(SimdLibCodegen::backend_type::cmpeq(lhs, rhs)));
#endif
}

/** @brief Any-lane predicate reduction fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE bool VECTORCALL
	simdlib_codegen_mask_any(native_type lhs, native_type rhs) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type(lhs).compare_equal(SimdLibCodegen::register_type(rhs)).any();
#else
	return SimdLibCodegen::api_type::movemask_slim(SimdLibCodegen::backend_type::cmpeq(lhs, rhs)) != 0;
#endif
}

/** @brief All-lane predicate reduction fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE bool VECTORCALL
	simdlib_codegen_mask_all(native_type lhs, native_type rhs) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type(lhs).compare_equal(SimdLibCodegen::register_type(rhs)).all();
#else
	constexpr std::uint32_t all_bits =
		(std::uint32_t{1} << SimdLibCodegen::register_type::lane_count) - 1;
	return static_cast<std::uint32_t>(
		SimdLibCodegen::api_type::movemask_slim(SimdLibCodegen::backend_type::cmpeq(lhs, rhs))) == all_bits;
#endif
}

/** @brief Native predicate observation fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
	simdlib_codegen_mask_native(native_type lhs, native_type rhs) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type(lhs).compare_less(SimdLibCodegen::register_type(rhs)).native();
#else
	return SimdLibCodegen::backend_type::cmpgt(rhs, lhs);
#endif
}

/** @brief Native-result fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
simdlib_codegen_native(native_type value) noexcept
{
	return SimdLibCodegen::unwrap(SimdLibCodegen::wrap(value));
}

/** @brief Zero-construction fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
	simdlib_codegen_zero() noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type::zero().native();
#else
	return SimdLibCodegen::api_type::setzero();
#endif
}

/** @brief Broadcast-reuse fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
	simdlib_codegen_broadcast_reuse(float value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	const auto broadcast = SimdLibCodegen::register_type::broadcast(value);
	return SimdLibCodegen::api_type::add(broadcast.native(), broadcast.native());
#else
	const auto broadcast = SimdLibCodegen::api_type::set1(value);
	return SimdLibCodegen::api_type::add(broadcast, broadcast);
#endif
}

/** @brief Fixed-array construction fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL simdlib_codegen_from_array(
	const std::array<float, SimdLibCodegen::api_type::element_count> &source) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type::from_array(source).native();
#else
	return SimdLibCodegen::api_type::construct(source);
#endif
}

/** @brief Fixed-array observation fixture. */
SIMDLIB_CODEGEN_NOINLINE void VECTORCALL simdlib_codegen_to_array(
	native_type value,
	std::array<float, SimdLibCodegen::api_type::element_count> &destination) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	destination = SimdLibCodegen::register_type(value).to_array();
#else
	destination = SimdLibCodegen::api_type::to_array(value);
#endif
}

/** @brief Lowest-lane observation fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE float VECTORCALL
	simdlib_codegen_lane_first(native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type(value).template lane<0>();
#else
	return SimdLibCodegen::api_type::template extract<0>(value);
#endif
}

/** @brief Highest-lane observation fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE float VECTORCALL
	simdlib_codegen_lane_last(native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type(value)
		.template lane<SimdLibCodegen::register_type::lane_count - 1>();
#else
	return SimdLibCodegen::api_type::template extract<
		static_cast<int>(SimdLibCodegen::register_type::lane_count - 1)>(value);
#endif
}

/** @brief Highest-lane replacement fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
	simdlib_codegen_with_lane_last(native_type value, float replacement) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return SimdLibCodegen::register_type(value)
		.template with_lane<SimdLibCodegen::register_type::lane_count - 1>(replacement)
		.native();
#else
	return SimdLibCodegen::api_type::template insert<
		SimdLibCodegen::register_type::lane_count - 1>(value, replacement);
#endif
}

/** @brief Full-register load, operation, and store fixture. */
SIMDLIB_CODEGEN_NOINLINE void simdlib_codegen_load_operate_store(
	const float *source,
	float *destination) noexcept
{
	constexpr auto count = SimdLibCodegen::api_type::element_count;
#if SIMDLIB_CODEGEN_USE_WRAPPER
	const auto value = SimdLibCodegen::register_type::load(std::span<const float, count>{source, count});
	SimdLibCodegen::register_type(SimdLibCodegen::api_type::add(value.native(), value.native()))
		.store(std::span<float, count>{destination, count});
#else
	const auto value = SimdLibCodegen::api_type::load(std::span<const float, count>{source, count});
	SimdLibCodegen::api_type::store(SimdLibCodegen::api_type::add(value, value),
		std::span<float, count>{destination, count});
#endif
}

/** @brief Aligned full-register load/store fixture. */
SIMDLIB_CODEGEN_NOINLINE void simdlib_codegen_aligned_transfer(
	const float *source,
	float *destination) noexcept
{
	constexpr auto count = SimdLibCodegen::api_type::element_count;
#if SIMDLIB_CODEGEN_USE_WRAPPER
	SimdLibCodegen::register_type::load_aligned(std::span<const float, count>{source, count})
		.store_aligned(std::span<float, count>{destination, count});
#else
	SimdLibCodegen::api_type::store_aligned(
		SimdLibCodegen::api_type::load_aligned(std::span<const float, count>{source, count}),
		std::span<float, count>{destination, count});
#endif
}

/** @brief Exact-byte load/store fixture. */
SIMDLIB_CODEGEN_NOINLINE void simdlib_codegen_byte_transfer(
	const std::byte *source,
	std::byte *destination) noexcept
{
	constexpr auto count = SimdLibCodegen::api_type::byte_count;
#if SIMDLIB_CODEGEN_USE_WRAPPER
	SimdLibCodegen::register_type::load_bytes(std::span<const std::byte, count>{source, count})
		.store_bytes(std::span<std::byte, count>{destination, count});
#else
	SimdLibCodegen::api_type::store(
		SimdLibCodegen::api_type::load(std::span<const std::byte, count>{source, count}),
		std::span<std::byte, count>{destination, count});
#endif
}

/** @brief Copy/move special-member fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
	simdlib_codegen_special_members(native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	SimdLibCodegen::register_type first(value);
	const SimdLibCodegen::register_type second(first);
	first = second;
	return first.native();
#else
	native_type first = value;
	const native_type second = first;
	first = second;
	return first;
#endif
}

/** @brief Store fixture. */
SIMDLIB_CODEGEN_NOINLINE void VECTORCALL simdlib_codegen_store(
	native_type value,
	float *destination) noexcept
{
	SimdLibCodegen::store_native(SimdLibCodegen::unwrap(SimdLibCodegen::wrap(value)), destination);
}

/** @brief Mutating-reference fixture. */
SIMDLIB_CODEGEN_NOINLINE void VECTORCALL simdlib_codegen_mutate(
	native_type &lhs,
	native_type rhs) noexcept
{
	value_type wrapped_lhs = SimdLibCodegen::wrap(lhs);
	const value_type wrapped_rhs = SimdLibCodegen::wrap(rhs);
	wrapped_lhs = SimdLibCodegen::wrap(SimdLibCodegen::api_type::add(
		SimdLibCodegen::unwrap(wrapped_lhs), SimdLibCodegen::unwrap(wrapped_rhs)));
	lhs = SimdLibCodegen::unwrap(wrapped_lhs);
}

/** @brief Controlled register-pressure fixture. */
SIMDLIB_REGISTER_ONLY SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL simdlib_codegen_pressure(
	native_type a,
	native_type b,
	native_type c,
	native_type d,
	native_type e,
	native_type f,
	native_type g,
	native_type h) noexcept
{
	const native_type ab = SimdLibCodegen::api_type::add(a, b);
	const native_type cd = SimdLibCodegen::api_type::add(c, d);
	const native_type ef = SimdLibCodegen::api_type::add(e, f);
	const native_type gh = SimdLibCodegen::api_type::add(g, h);
	return SimdLibCodegen::unwrap(SimdLibCodegen::wrap(SimdLibCodegen::api_type::add(
		SimdLibCodegen::api_type::add(ab, cd), SimdLibCodegen::api_type::add(ef, gh))));
}

/** @brief Opaque-call fixture used to compare wrapper and raw spill behavior. */
SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
	simdlib_codegen_opaque(native_type value) noexcept
{
	const value_type wrapped = SimdLibCodegen::wrap(value);
	simdlib_codegen_opaque_sink(SimdLibCodegen::unwrap(wrapped));
	return SimdLibCodegen::unwrap(wrapped);
}

#undef SIMDLIB_CODEGEN_NOINLINE
