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
	/** @brief Wraps a native predicate value. */
	SIMDLIB_REGISTER_ONLY explicit AbiMask(native_type value) noexcept : m_data(value) {}

  private:
	[[maybe_unused]] native_type m_data;
};

/** @brief Test-only one-vector value used to validate explicit-object call boundaries. */
class AbiRegister final
{
  public:
	/** @brief Wraps a native register value. */
	SIMDLIB_REGISTER_ONLY explicit AbiRegister(native_type value) noexcept : m_data(value) {}

	/** @brief Mirrors a unary explicit-object member boundary. */
	SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE AbiRegister VECTORCALL
		simdlib_abi_unary(this AbiRegister value) noexcept
	{
		return AbiRegister(api_type::bitwise_not(value.m_data));
	}

	/** @brief Mirrors a binary explicit-object member boundary. */
	SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE AbiRegister VECTORCALL simdlib_abi_binary(
		this AbiRegister lhs,
		AbiRegister rhs) noexcept
	{
		return AbiRegister(api_type::add(lhs.m_data, rhs.m_data));
	}

	/** @brief Mirrors a ternary explicit-object member boundary. */
	SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE AbiRegister VECTORCALL simdlib_abi_ternary(
		this AbiRegister lhs,
		AbiRegister rhs,
		AbiRegister addend) noexcept
	{
		return AbiRegister(api_type::add(api_type::multiply(lhs.m_data, rhs.m_data), addend.m_data));
	}

	/** @brief Mirrors a scalar-result explicit-object member boundary. */
	SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE std::uint32_t VECTORCALL
		simdlib_abi_scalar(this AbiRegister value) noexcept
	{
		return api_type::movemask(value.m_data);
	}

	/** @brief Mirrors a register-shaped mask-result explicit-object member boundary. */
	SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE AbiMask VECTORCALL
		simdlib_abi_mask(this AbiRegister value) noexcept
	{
		(void)value;
		return AbiMask(api_type::setzero());
	}

	/** @brief Mirrors a native-result explicit-object member boundary. */
	SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE native_type VECTORCALL
		simdlib_abi_native(this AbiRegister value) noexcept
	{
		return value.m_data;
	}

	/** @brief Mirrors a store explicit-object member boundary. */
	SIMDLIB_ABI_NOINLINE void VECTORCALL simdlib_abi_store(
		this AbiRegister value,
		float *destination) noexcept
	{
		api_type::store(value.m_data, std::span<float, api_type::element_count>(destination, api_type::element_count));
	}

	/** @brief Mirrors a mutating-reference explicit-object member boundary. */
	SIMDLIB_ABI_NOINLINE AbiRegister &VECTORCALL simdlib_abi_mutate(
		this AbiRegister &lhs,
		AbiRegister rhs) noexcept
	{
		lhs.m_data = api_type::add(lhs.m_data, rhs.m_data);
		return lhs;
	}

  private:
	native_type m_data;
};

/** @brief Returns a real RegisterMask across a separately compiled ABI boundary. */
SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE mask_type VECTORCALL
	simdlib_abi_mask_return(register_type lhs, register_type rhs) noexcept
{
	return lhs.compare_equal(rhs);
}

/** @brief Passes a real RegisterMask across a separately compiled ABI boundary. */
SIMDLIB_REGISTER_ONLY SIMDLIB_ABI_NOINLINE native_type VECTORCALL
	simdlib_abi_mask_pass(mask_type value) noexcept
{
	return value.native();
}

#undef SIMDLIB_ABI_NOINLINE
