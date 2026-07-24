#include <SimdLib/Register.h>

#include <cstdint>

#if defined(__clang__) || defined(__GNUC__)
#define SIMDLIB_ABI_NOINLINE __attribute__((noinline, used))
#elif SIMDLIB_COMPILER_MSVC
#define SIMDLIB_ABI_NOINLINE __declspec(noinline) __declspec(dllexport)
#else
#define SIMDLIB_ABI_NOINLINE __attribute__((noinline))
#endif

using api_type = SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, float>;
using native_type = typename api_type::vector_t;
using register_type = SimdLib::Register<float, SIMDLIB_REGISTER_TEST_WIDTH>;
using mask_type = typename register_type::mask_type;

/** @brief Test-only one-vector predicate used to mirror RegisterMask call boundaries. */
class AbiMask final
{
  public:
	/** @brief Owns the native predicate value represented by this aggregate mirror. */
	[[maybe_unused]] native_type m_data = api_type::setzero();
};

/** @brief Test-only one-vector value used to validate explicit-object call boundaries. */
class AbiRegister final
{
  public:
	/** @brief Owns the native register value represented by this aggregate mirror. */
	native_type m_data = api_type::setzero();

	/** @brief Mirrors a unary explicit-object member boundary. */
	SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE AbiRegister VECTORCALL simdlib_abi_unary(this AbiRegister value) noexcept
	{
		return AbiRegister{api_type::bitwise_not(value.m_data)};
	}

	/** @brief Mirrors a binary explicit-object member boundary. */
	SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE AbiRegister VECTORCALL simdlib_abi_binary(this AbiRegister lhs, AbiRegister rhs) noexcept
	{
		return AbiRegister{api_type::add(lhs.m_data, rhs.m_data)};
	}

	/** @brief Mirrors a ternary explicit-object member boundary. */
	SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE AbiRegister VECTORCALL simdlib_abi_ternary(this AbiRegister lhs, AbiRegister rhs, AbiRegister addend) noexcept
	{
		return AbiRegister{api_type::add(api_type::multiply(lhs.m_data, rhs.m_data), addend.m_data)};
	}

	/** @brief Mirrors a scalar-result explicit-object member boundary. */
	SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE std::uint32_t VECTORCALL simdlib_abi_scalar(this AbiRegister value) noexcept
	{
		return api_type::movemask(value.m_data);
	}

	/** @brief Mirrors a register-shaped mask-result explicit-object member boundary. */
	SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE AbiMask VECTORCALL simdlib_abi_mask(this AbiRegister value) noexcept
	{
		(void)value;
		return AbiMask{api_type::setzero()};
	}

	/** @brief Mirrors a native-result explicit-object member boundary. */
	SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE native_type VECTORCALL simdlib_abi_native(this AbiRegister value) noexcept
	{
		return value.m_data;
	}

	/** @brief Mirrors a store explicit-object member boundary. */
	SIMDLIB_ABI_NOINLINE void VECTORCALL simdlib_abi_store(this AbiRegister value, float *destination) noexcept
	{
		api_type::store(value.m_data, std::span<float, api_type::element_count>(destination, api_type::element_count));
	}

	/** @brief Mirrors a mutating-reference explicit-object member boundary. */
	SIMDLIB_ABI_NOINLINE AbiRegister &VECTORCALL simdlib_abi_mutate(this AbiRegister &lhs, AbiRegister rhs) noexcept
	{
		lhs.m_data = api_type::add(lhs.m_data, rhs.m_data);
		return lhs;
	}
};

/** @brief Returns a real Register across a separately compiled consumer boundary. */
SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE register_type VECTORCALL simdlib_consumer_abi_register_return(register_type lhs, register_type rhs) noexcept
{
	return register_type{api_type::add(lhs.native, rhs.native)};
}

/** @brief Passes a real Register across a separately compiled consumer boundary. */
SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE native_type VECTORCALL simdlib_consumer_abi_register_pass(register_type value) noexcept
{
	return value.native;
}

/** @brief Returns a real RegisterMask across a separately compiled ABI boundary. */
SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE mask_type VECTORCALL simdlib_consumer_abi_mask_return(register_type lhs, register_type rhs) noexcept
{
	return lhs.compare_equal(rhs);
}

/** @brief Passes a real RegisterMask across a separately compiled ABI boundary. */
SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE native_type VECTORCALL simdlib_consumer_abi_mask_pass(mask_type value) noexcept
{
	return value.native;
}

#undef SIMDLIB_ABI_NOINLINE
