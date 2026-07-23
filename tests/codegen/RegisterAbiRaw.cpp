#include <SimdLib/Register.h>

#include <cstdint>

#if SIMDLIB_COMPILER_MSVC
#define SIMDLIB_ABI_NOINLINE __declspec(noinline)
#else
#define SIMDLIB_ABI_NOINLINE __attribute__((noinline))
#endif

using api_type = SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, float>;
using native_type = typename api_type::vector_t;
using backend_type = SimdLib::Detail::SimdMappings<SIMDLIB_REGISTER_TEST_WIDTH, float>;

/** @brief Returns a raw predicate across a separately compiled ABI boundary. */
SIMDLIB_ABI_NOINLINE native_type VECTORCALL simdlib_abi_mask_return(native_type lhs, native_type rhs) noexcept
{
	return backend_type::cmpeq(lhs, rhs);
}

/** @brief Passes a raw predicate across a separately compiled ABI boundary. */
SIMDLIB_ABI_NOINLINE native_type VECTORCALL simdlib_abi_mask_pass(native_type value) noexcept
{
	return value;
}

/** @brief Raw unary ABI mirror. */
SIMDLIB_ABI_NOINLINE native_type VECTORCALL simdlib_abi_unary(native_type value) noexcept
{
	return api_type::bitwise_not(value);
}

/** @brief Raw binary ABI mirror. */
SIMDLIB_ABI_NOINLINE native_type VECTORCALL simdlib_abi_binary(native_type lhs, native_type rhs) noexcept
{
	return api_type::add(lhs, rhs);
}

/** @brief Raw ternary ABI mirror. */
SIMDLIB_ABI_NOINLINE native_type VECTORCALL simdlib_abi_ternary(
	native_type lhs,
	native_type rhs,
	native_type addend) noexcept
{
	return api_type::add(api_type::multiply(lhs, rhs), addend);
}

/** @brief Raw scalar-result ABI mirror. */
SIMDLIB_ABI_NOINLINE std::uint32_t VECTORCALL simdlib_abi_scalar(native_type value) noexcept
{
	return api_type::movemask(value);
}

/** @brief Raw register-shaped mask-result ABI mirror. */
SIMDLIB_ABI_NOINLINE native_type VECTORCALL simdlib_abi_mask(native_type value) noexcept
{
	(void)value;
	return api_type::setzero();
}

/** @brief Raw native-result ABI mirror. */
SIMDLIB_ABI_NOINLINE native_type VECTORCALL simdlib_abi_native(native_type value) noexcept
{
	return value;
}

/** @brief Raw store ABI mirror. */
SIMDLIB_ABI_NOINLINE void VECTORCALL simdlib_abi_store(native_type value, float *destination) noexcept
{
	api_type::store(value, std::span<float, api_type::element_count>(destination, api_type::element_count));
}

/** @brief Raw mutating-reference ABI mirror. */
SIMDLIB_ABI_NOINLINE native_type &VECTORCALL simdlib_abi_mutate(native_type &lhs, native_type rhs) noexcept
{
	lhs = api_type::add(lhs, rhs);
	return lhs;
}

#undef SIMDLIB_ABI_NOINLINE
