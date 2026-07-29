#pragma once

#include <SimdLib/Register.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>
#include <utility>

#if SIMDLIB_COMPILER_MSVC
#define SIMDLIB_TYPE_MATRIX_NOINLINE __declspec(noinline)
#else
#define SIMDLIB_TYPE_MATRIX_NOINLINE __attribute__((noinline))
#endif

namespace SimdLibTypeMatrixCodegen
{

/** @brief Api specialization for one common-operation fixture element type. */
template <class element_t> using api_t = SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, element_t>;

/** @brief Native vector for one common-operation fixture element type. */
template <class element_t> using native_t = typename api_t<element_t>::vector_t;

/** @brief Register wrapper for one common-operation fixture element type. */
template <class element_t> using register_t = SimdLib::Register<element_t, SIMDLIB_REGISTER_TEST_WIDTH>;

/** @brief Register-mask wrapper for one common-operation fixture element type. */
template <class element_t> using register_mask_t = SimdLib::RegisterMask<element_t, SIMDLIB_REGISTER_TEST_WIDTH>;

/** @brief Complete fixed-size lane array for one common-operation fixture element type. */
template <class element_t> using array_t = std::array<element_t, register_t<element_t>::lane_count>;

/** @brief Returns the compact all-lane predicate for one fixture element type. */
template <class element_t> [[nodiscard]] consteval typename api_t<element_t>::mask_t all_lane_bits() noexcept
{
	using mask_t = typename api_t<element_t>::mask_t;
	if constexpr (register_t<element_t>::lane_count == std::numeric_limits<mask_t>::digits)
		return std::numeric_limits<mask_t>::max();
	else
		return static_cast<mask_t>((mask_t{1} << register_t<element_t>::lane_count) - 1);
}

/** @brief Identifies one isolated native-result operation in the type matrix. */
enum class vector_operation
{
	zero,
	broadcast,
	add,
	subtract,
	multiply,
	divide,
	modulus,
	negate,
	bitwise_and,
	bitwise_or,
	bitwise_xor,
	bitwise_not,
	bitwise_andnot,
	compare_equal,
	compare_greater,
	compare_greater_equal,
	compare_less,
	compare_less_equal,
	mask_and,
	mask_or,
	mask_xor,
	mask_not,
	select,
	insert_last,
	shift_left,
	logical_shift_right,
	shift_right,
};

/**
 * @brief Emits one isolated native-result operation for exact wrapper/raw comparison.
 * @tparam operation Operation selected at compile time.
 * @param lhs First native operand.
 * @param rhs Second native operand.
 * @param third Third native operand.
 * @param scalar Scalar operand for broadcasts and insertion.
 * @param count Runtime shift count.
 * @return Native result of the selected operation.
 */
template <vector_operation operation, class element_t>
[[nodiscard]] SIMDLIB_FORCE_INLINE native_t<element_t> VECTORCALL vector_result(native_t<element_t> lhs, native_t<element_t> rhs, native_t<element_t> third,
																				element_t scalar, int count) noexcept
{
	using api_type [[maybe_unused]] = api_t<element_t>;
	using register_type = register_t<element_t>;
#if SIMDLIB_CODEGEN_USE_WRAPPER
	const register_type left{lhs};
	const register_type right{rhs};
	const register_type other{third};
	if constexpr (operation == vector_operation::zero)
		return register_type::zero().native;
	else if constexpr (operation == vector_operation::broadcast)
		return register_type::broadcast(scalar).native;
	else if constexpr (operation == vector_operation::add && SimdLib::IRegister::Add<register_type>)
		return (left + right).native;
	else if constexpr (operation == vector_operation::subtract && SimdLib::IRegister::Subtract<register_type>)
		return (left - right).native;
	else if constexpr (operation == vector_operation::multiply && SimdLib::IRegister::Multiply<register_type>)
		return (left * right).native;
	else if constexpr (operation == vector_operation::divide && SimdLib::IRegister::Divide<register_type>)
		return (left / right).native;
	else if constexpr (operation == vector_operation::modulus && SimdLib::IRegister::Modulus<register_type>)
		return (left % right).native;
	else if constexpr (operation == vector_operation::negate && SimdLib::IRegister::Negate<register_type>)
		return (-left).native;
	else if constexpr (operation == vector_operation::bitwise_and)
		return (left & right).native;
	else if constexpr (operation == vector_operation::bitwise_or)
		return (left | right).native;
	else if constexpr (operation == vector_operation::bitwise_xor)
		return (left ^ right).native;
	else if constexpr (operation == vector_operation::bitwise_not)
		return (~left).native;
	else if constexpr (operation == vector_operation::bitwise_andnot)
		return left.andnot(right).native;
	else if constexpr (operation == vector_operation::compare_equal)
		return left.compare_equal(right).native;
	else if constexpr (operation == vector_operation::compare_greater)
		return left.compare_greater(right).native;
	else if constexpr (operation == vector_operation::compare_greater_equal)
		return left.compare_greater_equal(right).native;
	else if constexpr (operation == vector_operation::compare_less)
		return left.compare_less(right).native;
	else if constexpr (operation == vector_operation::compare_less_equal)
		return left.compare_less_equal(right).native;
	else if constexpr (operation == vector_operation::mask_and)
		return (register_mask_t<element_t>{lhs} & register_mask_t<element_t>{rhs}).native;
	else if constexpr (operation == vector_operation::mask_or)
		return (register_mask_t<element_t>{lhs} | register_mask_t<element_t>{rhs}).native;
	else if constexpr (operation == vector_operation::mask_xor)
		return (register_mask_t<element_t>{lhs} ^ register_mask_t<element_t>{rhs}).native;
	else if constexpr (operation == vector_operation::mask_not)
		return (~register_mask_t<element_t>{lhs}).native;
	else if constexpr (operation == vector_operation::select)
		return register_mask_t<element_t>{lhs}.select(right, other).native;
	else if constexpr (operation == vector_operation::insert_last)
		return left.template with_lane<register_type::lane_count - 1>(scalar).native;
	else if constexpr (operation == vector_operation::shift_left && SimdLib::IRegister::ShiftLeft<register_type>)
		return (left << count).native;
	else if constexpr (operation == vector_operation::logical_shift_right && SimdLib::IRegister::LogicalShiftRight<register_type>)
		return left.logical_shift_right(count).native;
	else if constexpr (operation == vector_operation::shift_right && SimdLib::IRegister::ShiftRight<register_type>)
		return (left >> count).native;
#else
	if constexpr (operation == vector_operation::zero)
		return api_type::setzero();
	else if constexpr (operation == vector_operation::broadcast)
		return api_type::set1(scalar);
	else if constexpr (operation == vector_operation::add && SimdLib::IRegister::Add<register_type>)
		return api_type::add(lhs, rhs);
	else if constexpr (operation == vector_operation::subtract && SimdLib::IRegister::Subtract<register_type>)
		return api_type::subtract(lhs, rhs);
	else if constexpr (operation == vector_operation::multiply && SimdLib::IRegister::Multiply<register_type>)
		return api_type::multiply(lhs, rhs);
	else if constexpr (operation == vector_operation::divide && SimdLib::IRegister::Divide<register_type>)
		return api_type::divide(lhs, rhs);
	else if constexpr (operation == vector_operation::modulus && SimdLib::IRegister::Modulus<register_type>)
		return api_type::modulus(lhs, rhs);
	else if constexpr (operation == vector_operation::negate && SimdLib::IRegister::Negate<register_type>)
		return api_type::negate(lhs);
	else if constexpr (operation == vector_operation::bitwise_and || operation == vector_operation::mask_and)
		return api_type::bitwise_and(lhs, rhs);
	else if constexpr (operation == vector_operation::bitwise_or || operation == vector_operation::mask_or)
		return api_type::bitwise_or(lhs, rhs);
	else if constexpr (operation == vector_operation::bitwise_xor || operation == vector_operation::mask_xor)
		return api_type::bitwise_xor(lhs, rhs);
	else if constexpr (operation == vector_operation::bitwise_not || operation == vector_operation::mask_not)
		return api_type::bitwise_not(lhs);
	else if constexpr (operation == vector_operation::bitwise_andnot)
		return api_type::bitwise_andnot(lhs, rhs);
	else if constexpr (operation == vector_operation::compare_equal)
		return api_type::compare_equal(lhs, rhs);
	else if constexpr (operation == vector_operation::compare_greater)
		return api_type::compare_greater(lhs, rhs);
	else if constexpr (operation == vector_operation::compare_greater_equal)
		return api_type::compare_greater_equal(lhs, rhs);
	else if constexpr (operation == vector_operation::compare_less)
		return api_type::compare_less(lhs, rhs);
	else if constexpr (operation == vector_operation::compare_less_equal)
		return api_type::compare_less_equal(lhs, rhs);
	else if constexpr (operation == vector_operation::select)
		return api_type::select(lhs, rhs, third);
	else if constexpr (operation == vector_operation::insert_last)
		return api_type::template insert<register_type::lane_count - 1>(lhs, scalar);
	else if constexpr (operation == vector_operation::shift_left && SimdLib::IRegister::ShiftLeft<register_type>)
		return api_type::shift_left(lhs, count);
	else if constexpr (operation == vector_operation::logical_shift_right && SimdLib::IRegister::LogicalShiftRight<register_type>)
		return api_type::shift_right(lhs, count);
	else if constexpr (operation == vector_operation::shift_right && SimdLib::IRegister::ShiftRight<register_type>)
	{
		if constexpr (std::is_signed_v<element_t>)
			return api_type::shift_right_arithmetic(lhs, count);
		else
			return api_type::shift_right(lhs, count);
	}
#endif
	else
	{
		static_assert(SimdLib::Detail::dependent_false_v<element_t>, "The selected Register operation is unavailable for this element type.");
	}
}

/** @brief Identifies one isolated scalar-result operation in the type matrix. */
enum class scalar_operation
{
	movemask,
	lane_sign_bits,
	mask_bits,
	mask_any,
	mask_all,
	mask_none,
	equal,
	not_equal,
	extract_first,
};

/**
 * @brief Emits one isolated scalar-result operation for exact wrapper/raw comparison.
 * @tparam operation Operation selected at compile time.
 * @param lhs First native operand.
 * @param rhs Second native operand.
 * @return Compact scalar result of the selected operation.
 */
template <scalar_operation operation, class element_t>
[[nodiscard]] SIMDLIB_FORCE_INLINE typename api_t<element_t>::mask_t VECTORCALL scalar_result(native_t<element_t> lhs, native_t<element_t> rhs) noexcept
{
	using api_type = api_t<element_t>;
	using register_type [[maybe_unused]] = register_t<element_t>;
	using mask_bits_t = typename api_type::mask_t;
#if SIMDLIB_CODEGEN_USE_WRAPPER
	const register_type left{lhs};
	const register_type right{rhs};
	const register_mask_t<element_t> mask{lhs};
	if constexpr (operation == scalar_operation::movemask)
		return left.movemask();
	else if constexpr (operation == scalar_operation::lane_sign_bits)
		return left.lane_sign_bits();
	else if constexpr (operation == scalar_operation::mask_bits)
		return mask.bits();
	else if constexpr (operation == scalar_operation::mask_any)
		return static_cast<mask_bits_t>(mask.any());
	else if constexpr (operation == scalar_operation::mask_all)
		return static_cast<mask_bits_t>(mask.all());
	else if constexpr (operation == scalar_operation::mask_none)
		return static_cast<mask_bits_t>(mask.none());
	else if constexpr (operation == scalar_operation::equal)
		return static_cast<mask_bits_t>(left == right);
	else if constexpr (operation == scalar_operation::not_equal)
		return static_cast<mask_bits_t>(left != right);
	else
		return static_cast<mask_bits_t>(left.template lane<0>());
#else
	if constexpr (operation == scalar_operation::movemask)
		return api_type::movemask(lhs);
	else if constexpr (operation == scalar_operation::lane_sign_bits || operation == scalar_operation::mask_bits)
		return api_type::movemask_slim(lhs);
	else if constexpr (operation == scalar_operation::mask_any)
		return static_cast<mask_bits_t>(api_type::movemask_slim(lhs) != 0);
	else if constexpr (operation == scalar_operation::mask_all)
		return static_cast<mask_bits_t>(api_type::movemask_slim(lhs) == all_lane_bits<element_t>());
	else if constexpr (operation == scalar_operation::mask_none)
		return static_cast<mask_bits_t>(api_type::movemask_slim(lhs) == 0);
	else if constexpr (operation == scalar_operation::equal)
		return static_cast<mask_bits_t>(api_type::movemask_slim(api_type::compare_equal(lhs, rhs)) == all_lane_bits<element_t>());
	else if constexpr (operation == scalar_operation::not_equal)
		return static_cast<mask_bits_t>(api_type::movemask_slim(api_type::compare_equal(lhs, rhs)) != all_lane_bits<element_t>());
	else
		return static_cast<mask_bits_t>(api_type::template extract<0>(lhs));
#endif
}

/** @brief Returns a register constructed from a fixed array. */
template <class element_t> [[nodiscard]] SIMDLIB_FORCE_INLINE native_t<element_t> VECTORCALL construct_array(const array_t<element_t> &source) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return register_t<element_t>::from_array(source).native;
#else
	return api_t<element_t>::construct(source);
#endif
}

/** @brief Returns a register loaded from an unaligned fixed-size span. */
template <class element_t> [[nodiscard]] SIMDLIB_FORCE_INLINE native_t<element_t> VECTORCALL load(const element_t *source) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return register_t<element_t>::load(std::span<const element_t, register_t<element_t>::lane_count>{source, register_t<element_t>::lane_count}).native;
#else
	return api_t<element_t>::load(std::span<const element_t, register_t<element_t>::lane_count>{source, register_t<element_t>::lane_count});
#endif
}

/** @brief Returns a register loaded from an aligned fixed-size span. */
template <class element_t> [[nodiscard]] SIMDLIB_FORCE_INLINE native_t<element_t> VECTORCALL load_aligned(const element_t *source) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return register_t<element_t>::load_aligned(std::span<const element_t, register_t<element_t>::lane_count>{source, register_t<element_t>::lane_count}).native;
#else
	return api_t<element_t>::load_aligned(std::span<const element_t, register_t<element_t>::lane_count>{source, register_t<element_t>::lane_count});
#endif
}

/** @brief Returns a register loaded from a fixed-size byte span. */
template <class element_t> [[nodiscard]] SIMDLIB_FORCE_INLINE native_t<element_t> VECTORCALL load_bytes(const std::byte *source) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return register_t<element_t>::load_bytes(std::span<const std::byte, register_t<element_t>::byte_count>{source, register_t<element_t>::byte_count}).native;
#else
	return api_t<element_t>::load(std::span<const std::byte, register_t<element_t>::byte_count>{source, register_t<element_t>::byte_count});
#endif
}

/** @brief Stores a native register through the unaligned fixed-size span API. */
template <class element_t> SIMDLIB_FORCE_INLINE void VECTORCALL store(native_t<element_t> value, element_t *destination) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	register_t<element_t>{value}.store(std::span<element_t, register_t<element_t>::lane_count>{destination, register_t<element_t>::lane_count});
#else
	api_t<element_t>::store(value, std::span<element_t, register_t<element_t>::lane_count>{destination, register_t<element_t>::lane_count});
#endif
}

/** @brief Stores a native register through the aligned fixed-size span API. */
template <class element_t> SIMDLIB_FORCE_INLINE void VECTORCALL store_aligned(native_t<element_t> value, element_t *destination) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	register_t<element_t>{value}.store_aligned(std::span<element_t, register_t<element_t>::lane_count>{destination, register_t<element_t>::lane_count});
#else
	api_t<element_t>::store_aligned(value, std::span<element_t, register_t<element_t>::lane_count>{destination, register_t<element_t>::lane_count});
#endif
}

/** @brief Stores a native register through the fixed-size byte-span API. */
template <class element_t> SIMDLIB_FORCE_INLINE void VECTORCALL store_bytes(native_t<element_t> value, std::byte *destination) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	register_t<element_t>{value}.store_bytes(std::span<std::byte, register_t<element_t>::byte_count>{destination, register_t<element_t>::byte_count});
#else
	api_t<element_t>::store(value, std::span<std::byte, register_t<element_t>::byte_count>{destination, register_t<element_t>::byte_count});
#endif
}

/** @brief Stores a native register through the fixed-array observation API. */
template <class element_t> SIMDLIB_FORCE_INLINE void VECTORCALL observe_array(native_t<element_t> value, array_t<element_t> &destination) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	destination = register_t<element_t>{value}.to_array();
#else
	destination = api_t<element_t>::to_array(value);
#endif
}

/** @brief Expands a complete array through the lane-list construction overload. */
template <class element_t, std::size_t... indices>
SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY native_t<element_t> VECTORCALL from_lanes(const array_t<element_t> &source, std::index_sequence<indices...>) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return register_t<element_t>::from_lanes(source[indices]...).native;
#else
	return api_t<element_t>::setr(source[indices]...);
#endif
}

} // namespace SimdLibTypeMatrixCodegen

#define SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, operation)                                                                                      \
	/** @brief Compares one isolated native-result operation with its raw Api expression. */                                                                   \
	SIMDLIB_TYPE_MATRIX_NOINLINE SimdLibTypeMatrixCodegen::native_t<element_type> VECTORCALL simdlib_type_matrix_##operation##_##token(                        \
		SimdLibTypeMatrixCodegen::native_t<element_type> lhs, SimdLibTypeMatrixCodegen::native_t<element_type> rhs,                                            \
		SimdLibTypeMatrixCodegen::native_t<element_type> third, element_type scalar, int count) noexcept                                                       \
	{                                                                                                                                                          \
		return SimdLibTypeMatrixCodegen::vector_result<SimdLibTypeMatrixCodegen::vector_operation::operation, element_type>(lhs, rhs, third, scalar, count);   \
	}

#define SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, operation)                                                                                      \
	/** @brief Compares one isolated scalar-result operation with its raw Api expression. */                                                                   \
	SIMDLIB_TYPE_MATRIX_NOINLINE typename SimdLibTypeMatrixCodegen::api_t<element_type>::mask_t VECTORCALL simdlib_type_matrix_##operation##_##token(          \
		SimdLibTypeMatrixCodegen::native_t<element_type> lhs, SimdLibTypeMatrixCodegen::native_t<element_type> rhs) noexcept                                   \
	{                                                                                                                                                          \
		return SimdLibTypeMatrixCodegen::scalar_result<SimdLibTypeMatrixCodegen::scalar_operation::operation, element_type>(lhs, rhs);                         \
	}

#define SIMDLIB_DEFINE_TYPE_MATRIX_COMMON_FIXTURES(token, element_type)                                                                                        \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, zero)                                                                                               \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, broadcast)                                                                                          \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, add)                                                                                                \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, subtract)                                                                                           \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, multiply)                                                                                           \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, divide)                                                                                             \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, negate)                                                                                             \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, bitwise_and)                                                                                        \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, bitwise_or)                                                                                         \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, bitwise_xor)                                                                                        \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, bitwise_not)                                                                                        \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, bitwise_andnot)                                                                                     \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, compare_equal)                                                                                      \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, compare_greater)                                                                                    \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, compare_greater_equal)                                                                              \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, compare_less)                                                                                       \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, compare_less_equal)                                                                                 \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, mask_and)                                                                                           \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, mask_or)                                                                                            \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, mask_xor)                                                                                           \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, mask_not)                                                                                           \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, select)                                                                                             \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, insert_last)                                                                                        \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, movemask)                                                                                           \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, lane_sign_bits)                                                                                     \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, mask_bits)                                                                                          \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, mask_any)                                                                                           \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, mask_all)                                                                                           \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, mask_none)                                                                                          \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, equal)                                                                                              \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, not_equal)                                                                                          \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, extract_first)                                                                                      \
	/** @brief Compares fixed-array construction for one element type. */                                                                                      \
	SIMDLIB_TYPE_MATRIX_NOINLINE SimdLibTypeMatrixCodegen::native_t<element_type> VECTORCALL simdlib_type_matrix_construct_array_##token(                      \
		const SimdLibTypeMatrixCodegen::array_t<element_type> &source) noexcept                                                                                \
	{                                                                                                                                                          \
		return SimdLibTypeMatrixCodegen::construct_array<element_type>(source);                                                                                \
	}                                                                                                                                                          \
	/** @brief Compares lane-list construction for one element type. */                                                                                        \
	SIMDLIB_TYPE_MATRIX_NOINLINE SimdLibTypeMatrixCodegen::native_t<element_type> VECTORCALL simdlib_type_matrix_construct_lanes_##token(                      \
		const SimdLibTypeMatrixCodegen::array_t<element_type> &source) noexcept                                                                                \
	{                                                                                                                                                          \
		return SimdLibTypeMatrixCodegen::from_lanes<element_type>(source,                                                                                      \
																  std::make_index_sequence<SimdLibTypeMatrixCodegen::register_t<element_type>::lane_count>{}); \
	}                                                                                                                                                          \
	/** @brief Compares unaligned loading for one element type. */                                                                                             \
	SIMDLIB_TYPE_MATRIX_NOINLINE SimdLibTypeMatrixCodegen::native_t<element_type> VECTORCALL simdlib_type_matrix_load_##token(                                 \
		const element_type *source) noexcept                                                                                                                   \
	{                                                                                                                                                          \
		return SimdLibTypeMatrixCodegen::load<element_type>(source);                                                                                           \
	}                                                                                                                                                          \
	/** @brief Compares aligned loading for one element type. */                                                                                               \
	SIMDLIB_TYPE_MATRIX_NOINLINE SimdLibTypeMatrixCodegen::native_t<element_type> VECTORCALL simdlib_type_matrix_load_aligned_##token(                         \
		const element_type *source) noexcept                                                                                                                   \
	{                                                                                                                                                          \
		return SimdLibTypeMatrixCodegen::load_aligned<element_type>(source);                                                                                   \
	}                                                                                                                                                          \
	/** @brief Compares byte-span loading for one element type. */                                                                                             \
	SIMDLIB_TYPE_MATRIX_NOINLINE SimdLibTypeMatrixCodegen::native_t<element_type> VECTORCALL simdlib_type_matrix_load_bytes_##token(                           \
		const std::byte *source) noexcept                                                                                                                      \
	{                                                                                                                                                          \
		return SimdLibTypeMatrixCodegen::load_bytes<element_type>(source);                                                                                     \
	}                                                                                                                                                          \
	/** @brief Compares unaligned storage for one element type. */                                                                                             \
	SIMDLIB_TYPE_MATRIX_NOINLINE void VECTORCALL simdlib_type_matrix_store_##token(SimdLibTypeMatrixCodegen::native_t<element_type> value,                     \
																				   element_type *destination) noexcept                                         \
	{                                                                                                                                                          \
		SimdLibTypeMatrixCodegen::store<element_type>(value, destination);                                                                                     \
	}                                                                                                                                                          \
	/** @brief Compares aligned storage for one element type. */                                                                                               \
	SIMDLIB_TYPE_MATRIX_NOINLINE void VECTORCALL simdlib_type_matrix_store_aligned_##token(SimdLibTypeMatrixCodegen::native_t<element_type> value,             \
																						   element_type *destination) noexcept                                 \
	{                                                                                                                                                          \
		SimdLibTypeMatrixCodegen::store_aligned<element_type>(value, destination);                                                                             \
	}                                                                                                                                                          \
	/** @brief Compares byte-span storage for one element type. */                                                                                             \
	SIMDLIB_TYPE_MATRIX_NOINLINE void VECTORCALL simdlib_type_matrix_store_bytes_##token(SimdLibTypeMatrixCodegen::native_t<element_type> value,               \
																						 std::byte *destination) noexcept                                      \
	{                                                                                                                                                          \
		SimdLibTypeMatrixCodegen::store_bytes<element_type>(value, destination);                                                                               \
	}                                                                                                                                                          \
	/** @brief Compares fixed-array observation for one element type. */                                                                                       \
	SIMDLIB_TYPE_MATRIX_NOINLINE void VECTORCALL simdlib_type_matrix_observe_array_##token(                                                                    \
		SimdLibTypeMatrixCodegen::native_t<element_type> value, SimdLibTypeMatrixCodegen::array_t<element_type> &destination) noexcept                         \
	{                                                                                                                                                          \
		SimdLibTypeMatrixCodegen::observe_array<element_type>(value, destination);                                                                             \
	}

#define SIMDLIB_DEFINE_TYPE_MATRIX_INTEGER_FIXTURES(token, element_type)                                                                                       \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, modulus)                                                                                            \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, shift_left)                                                                                         \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, logical_shift_right)                                                                                \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, shift_right)
SIMDLIB_DEFINE_TYPE_MATRIX_COMMON_FIXTURES(i8, std::int8_t)
SIMDLIB_DEFINE_TYPE_MATRIX_COMMON_FIXTURES(u8, std::uint8_t)
SIMDLIB_DEFINE_TYPE_MATRIX_COMMON_FIXTURES(i16, std::int16_t)
SIMDLIB_DEFINE_TYPE_MATRIX_COMMON_FIXTURES(u16, std::uint16_t)
SIMDLIB_DEFINE_TYPE_MATRIX_COMMON_FIXTURES(i32, std::int32_t)
SIMDLIB_DEFINE_TYPE_MATRIX_COMMON_FIXTURES(u32, std::uint32_t)
SIMDLIB_DEFINE_TYPE_MATRIX_COMMON_FIXTURES(i64, std::int64_t)
SIMDLIB_DEFINE_TYPE_MATRIX_COMMON_FIXTURES(u64, std::uint64_t)
SIMDLIB_DEFINE_TYPE_MATRIX_COMMON_FIXTURES(f32, float)
SIMDLIB_DEFINE_TYPE_MATRIX_COMMON_FIXTURES(f64, double)

SIMDLIB_DEFINE_TYPE_MATRIX_INTEGER_FIXTURES(i8, std::int8_t)
SIMDLIB_DEFINE_TYPE_MATRIX_INTEGER_FIXTURES(u8, std::uint8_t)
SIMDLIB_DEFINE_TYPE_MATRIX_INTEGER_FIXTURES(i16, std::int16_t)
SIMDLIB_DEFINE_TYPE_MATRIX_INTEGER_FIXTURES(u16, std::uint16_t)
SIMDLIB_DEFINE_TYPE_MATRIX_INTEGER_FIXTURES(i32, std::int32_t)
SIMDLIB_DEFINE_TYPE_MATRIX_INTEGER_FIXTURES(u32, std::uint32_t)
SIMDLIB_DEFINE_TYPE_MATRIX_INTEGER_FIXTURES(i64, std::int64_t)
SIMDLIB_DEFINE_TYPE_MATRIX_INTEGER_FIXTURES(u64, std::uint64_t)
static_assert(!SimdLib::IRegister::Modulus<SimdLibTypeMatrixCodegen::register_t<float>>);
static_assert(!SimdLib::IRegister::Modulus<SimdLibTypeMatrixCodegen::register_t<double>>);
static_assert(!SimdLib::IRegister::ShiftLeft<SimdLibTypeMatrixCodegen::register_t<float>>);
static_assert(!SimdLib::IRegister::ShiftLeft<SimdLibTypeMatrixCodegen::register_t<double>>);
static_assert(!SimdLib::IRegister::LogicalShiftRight<SimdLibTypeMatrixCodegen::register_t<float>>);
static_assert(!SimdLib::IRegister::LogicalShiftRight<SimdLibTypeMatrixCodegen::register_t<double>>);
static_assert(!SimdLib::IRegister::ShiftRight<SimdLibTypeMatrixCodegen::register_t<float>>);
static_assert(!SimdLib::IRegister::ShiftRight<SimdLibTypeMatrixCodegen::register_t<double>>);

#undef SIMDLIB_DEFINE_TYPE_MATRIX_INTEGER_FIXTURES
#undef SIMDLIB_DEFINE_TYPE_MATRIX_COMMON_FIXTURES
#undef SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR
#undef SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR
#undef SIMDLIB_TYPE_MATRIX_NOINLINE
