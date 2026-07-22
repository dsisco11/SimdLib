#pragma once

#include <SimdLib/Register.h>

#include <cstdint>

#if SIMDLIB_COMPILER_MSVC
#define SIMDLIB_CODEGEN_NOINLINE __declspec(noinline)
#else
#define SIMDLIB_CODEGEN_NOINLINE __attribute__((noinline))
#endif

namespace SimdLibCodegen
{

using api_type = SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, float>;
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
SIMDLIB_FORCE_INLINE SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS native_type VECTORCALL unwrap(value_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return value.native();
#else
	return value;
#endif
}

/** @brief Converts a native vector to the fixture value representation. */
SIMDLIB_FORCE_INLINE SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS value_type VECTORCALL wrap(native_type value) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return value_type(value);
#else
	return value;
#endif
}

/** @brief Converts a native predicate vector to the fixture predicate representation. */
SIMDLIB_FORCE_INLINE SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS predicate_type VECTORCALL zero_predicate() noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return predicate_type{};
#else
	return api_type::setzero();
#endif
}

/** @brief Stores a native register to potentially unaligned storage. */
SIMDLIB_FORCE_INLINE SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS void VECTORCALL
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
SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS SIMDLIB_CODEGEN_NOINLINE void VECTORCALL
	simdlib_codegen_opaque_sink(native_type value) noexcept;

/** @brief Forced-inline unary expression fixture. */
SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
	simdlib_codegen_unary(native_type value) noexcept
{
	const value_type wrapped = SimdLibCodegen::wrap(value);
	return SimdLibCodegen::unwrap(
		SimdLibCodegen::wrap(SimdLibCodegen::api_type::bitwise_not(SimdLibCodegen::unwrap(wrapped))));
}

/** @brief Forced-inline binary expression fixture. */
SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
	simdlib_codegen_binary(native_type lhs, native_type rhs) noexcept
{
	const value_type wrapped_lhs = SimdLibCodegen::wrap(lhs);
	const value_type wrapped_rhs = SimdLibCodegen::wrap(rhs);
	return SimdLibCodegen::unwrap(SimdLibCodegen::wrap(
		SimdLibCodegen::api_type::add(SimdLibCodegen::unwrap(wrapped_lhs), SimdLibCodegen::unwrap(wrapped_rhs))));
}

/** @brief Forced-inline ternary expression fixture. */
SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL simdlib_codegen_ternary(
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
SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS SIMDLIB_CODEGEN_NOINLINE std::uint32_t VECTORCALL
	simdlib_codegen_scalar(native_type value) noexcept
{
	return SimdLibCodegen::api_type::movemask(SimdLibCodegen::unwrap(SimdLibCodegen::wrap(value)));
}

/** @brief Register-shaped mask-result fixture. */
SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
	simdlib_codegen_mask(native_type lhs, native_type rhs) noexcept
{
	(void)lhs;
	(void)rhs;
	const predicate_type predicate = SimdLibCodegen::zero_predicate();
	(void)predicate;
	return SimdLibCodegen::api_type::setzero();
}

/** @brief Native-result fixture. */
SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
	simdlib_codegen_native(native_type value) noexcept
{
	return SimdLibCodegen::unwrap(SimdLibCodegen::wrap(value));
}

/** @brief Store fixture. */
SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS SIMDLIB_CODEGEN_NOINLINE void VECTORCALL simdlib_codegen_store(
	native_type value,
	float *destination) noexcept
{
	SimdLibCodegen::store_native(SimdLibCodegen::unwrap(SimdLibCodegen::wrap(value)), destination);
}

/** @brief Mutating-reference fixture. */
SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS SIMDLIB_CODEGEN_NOINLINE void VECTORCALL simdlib_codegen_mutate(
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
SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL simdlib_codegen_pressure(
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
SIMDLIB_DETAIL_MSVC_SAFE_BUFFERS SIMDLIB_CODEGEN_NOINLINE native_type VECTORCALL
	simdlib_codegen_opaque(native_type value) noexcept
{
	const value_type wrapped = SimdLibCodegen::wrap(value);
	simdlib_codegen_opaque_sink(SimdLibCodegen::unwrap(wrapped));
	return SimdLibCodegen::unwrap(wrapped);
}

#undef SIMDLIB_CODEGEN_NOINLINE
