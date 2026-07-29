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

/**
 * @brief Emits every register-only common-operation result for one element type.
 * @param lhs First opaque native operand.
 * @param rhs Second opaque native operand.
 * @param third Third opaque native operand used by selection.
 * @param replacement Runtime lane replacement value.
 * @param count Runtime shift count.
 * @param vectors Opaque vector-result destination.
 * @param scalars Opaque scalar-result destination.
 */
template <class element_t>
SIMDLIB_FORCE_INLINE void VECTORCALL evaluate(native_t<element_t> lhs, native_t<element_t> rhs, native_t<element_t> third, element_t replacement, int count,
											  native_t<element_t> *vectors, typename api_t<element_t>::mask_t *scalars) noexcept
{
	using api_type [[maybe_unused]] = api_t<element_t>;
	using register_type [[maybe_unused]] = register_t<element_t>;
	using mask_bits_t = typename api_type::mask_t;
	std::size_t vector_index = 0;
	std::size_t scalar_index = 0;
#if SIMDLIB_CODEGEN_USE_WRAPPER
	const register_type left{lhs};
	const register_type right{rhs};
	const register_type other{third};
	vectors[vector_index++] = register_type::zero().native;
	vectors[vector_index++] = register_type::broadcast(replacement).native;
	if constexpr (SimdLib::IRegister::Add<register_type>)
		vectors[vector_index++] = (left + right).native;
	if constexpr (SimdLib::IRegister::Subtract<register_type>)
		vectors[vector_index++] = (left - right).native;
	if constexpr (SimdLib::IRegister::Multiply<register_type>)
		vectors[vector_index++] = (left * right).native;
	if constexpr (SimdLib::IRegister::Divide<register_type>)
		vectors[vector_index++] = (left / right).native;
	if constexpr (SimdLib::IRegister::Modulus<register_type>)
		vectors[vector_index++] = (left % right).native;
	if constexpr (SimdLib::IRegister::Negate<register_type>)
		vectors[vector_index++] = (-left).native;
	vectors[vector_index++] = (left & right).native;
	vectors[vector_index++] = (left | right).native;
	vectors[vector_index++] = (left ^ right).native;
	vectors[vector_index++] = (~left).native;
	vectors[vector_index++] = left.andnot(right).native;
	const auto equal = left.compare_equal(right);
	const auto greater = left.compare_greater(right);
	const auto greater_equal = left.compare_greater_equal(right);
	const auto less = left.compare_less(right);
	const auto less_equal = left.compare_less_equal(right);
	vectors[vector_index++] = equal.native;
	vectors[vector_index++] = greater.native;
	vectors[vector_index++] = greater_equal.native;
	vectors[vector_index++] = less.native;
	vectors[vector_index++] = less_equal.native;
	vectors[vector_index++] = ((equal & greater) | (equal ^ ~greater)).native;
	vectors[vector_index++] = greater.select(left, other).native;
	scalars[scalar_index++] = left.movemask();
	scalars[scalar_index++] = left.lane_sign_bits();
	scalars[scalar_index++] = equal.bits();
	scalars[scalar_index++] = static_cast<mask_bits_t>(equal.any());
	scalars[scalar_index++] = static_cast<mask_bits_t>(equal.all());
	scalars[scalar_index++] = static_cast<mask_bits_t>(equal.none());
	scalars[scalar_index++] = static_cast<mask_bits_t>(left == right);
	scalars[scalar_index++] = static_cast<mask_bits_t>(left != right);
	scalars[scalar_index++] = static_cast<mask_bits_t>(left.template lane<0>());
	vectors[vector_index++] = left.template with_lane<register_type::lane_count - 1>(replacement).native;
	if constexpr (SimdLib::IRegister::ShiftLeft<register_type>)
		vectors[vector_index++] = (left << count).native;
	if constexpr (SimdLib::IRegister::LogicalShiftRight<register_type>)
		vectors[vector_index++] = left.logical_shift_right(count).native;
	if constexpr (SimdLib::IRegister::ShiftRight<register_type>)
		vectors[vector_index++] = (left >> count).native;
#else
	vectors[vector_index++] = api_type::setzero();
	vectors[vector_index++] = api_type::set1(replacement);
	if constexpr (SimdLib::IRegister::Add<register_type>)
		vectors[vector_index++] = api_type::add(lhs, rhs);
	if constexpr (SimdLib::IRegister::Subtract<register_type>)
		vectors[vector_index++] = api_type::subtract(lhs, rhs);
	if constexpr (SimdLib::IRegister::Multiply<register_type>)
		vectors[vector_index++] = api_type::multiply(lhs, rhs);
	if constexpr (SimdLib::IRegister::Divide<register_type>)
		vectors[vector_index++] = api_type::divide(lhs, rhs);
	if constexpr (SimdLib::IRegister::Modulus<register_type>)
		vectors[vector_index++] = api_type::modulus(lhs, rhs);
	if constexpr (SimdLib::IRegister::Negate<register_type>)
		vectors[vector_index++] = api_type::negate(lhs);
	vectors[vector_index++] = api_type::bitwise_and(lhs, rhs);
	vectors[vector_index++] = api_type::bitwise_or(lhs, rhs);
	vectors[vector_index++] = api_type::bitwise_xor(lhs, rhs);
	vectors[vector_index++] = api_type::bitwise_not(lhs);
	vectors[vector_index++] = api_type::bitwise_andnot(lhs, rhs);
	const auto equal = api_type::compare_equal(lhs, rhs);
	const auto greater = api_type::compare_greater(lhs, rhs);
	const auto greater_equal = api_type::compare_greater_equal(lhs, rhs);
	const auto less = api_type::compare_less(lhs, rhs);
	const auto less_equal = api_type::compare_less_equal(lhs, rhs);
	vectors[vector_index++] = equal;
	vectors[vector_index++] = greater;
	vectors[vector_index++] = greater_equal;
	vectors[vector_index++] = less;
	vectors[vector_index++] = less_equal;
	vectors[vector_index++] = api_type::bitwise_or(api_type::bitwise_and(equal, greater), api_type::bitwise_xor(equal, api_type::bitwise_not(greater)));
	vectors[vector_index++] = api_type::select(greater, lhs, third);
	scalars[scalar_index++] = api_type::movemask(lhs);
	scalars[scalar_index++] = api_type::movemask_slim(lhs);
	const auto equal_bits = api_type::movemask_slim(equal);
	scalars[scalar_index++] = equal_bits;
	scalars[scalar_index++] = static_cast<mask_bits_t>(equal_bits != 0);
	scalars[scalar_index++] = static_cast<mask_bits_t>(equal_bits == all_lane_bits<element_t>());
	scalars[scalar_index++] = static_cast<mask_bits_t>(equal_bits == 0);
	scalars[scalar_index++] = static_cast<mask_bits_t>(equal_bits == all_lane_bits<element_t>());
	scalars[scalar_index++] = static_cast<mask_bits_t>(equal_bits != all_lane_bits<element_t>());
	scalars[scalar_index++] = static_cast<mask_bits_t>(api_type::template extract<0>(lhs));
	vectors[vector_index++] = api_type::template insert<register_type::lane_count - 1>(lhs, replacement);
	if constexpr (SimdLib::IRegister::ShiftLeft<register_type>)
		vectors[vector_index++] = api_type::shift_left(lhs, count);
	if constexpr (SimdLib::IRegister::LogicalShiftRight<register_type>)
		vectors[vector_index++] = api_type::shift_right(lhs, count);
	if constexpr (SimdLib::IRegister::ShiftRight<register_type>)
	{
		if constexpr (std::is_signed_v<element_t>)
			vectors[vector_index++] = api_type::shift_right_arithmetic(lhs, count);
		else
			vectors[vector_index++] = api_type::shift_right(lhs, count);
	}
#endif
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
 * @return Native result of the selected operation, or `lhs` when unavailable for the element type.
 */
#if SIMDLIB_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
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
	return lhs;
}
#if SIMDLIB_COMPILER_MSVC
#pragma warning(pop)
#endif

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

/**
 * @brief Extracts one runtime-selected lane through the public Api or its direct implementation reference.
 * @tparam element_t Scalar lane type.
 * @param lhs Source register.
 * @param index Runtime-selected lane index.
 * @return Selected scalar lane.
 */
template <class element_t>
[[nodiscard]] SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY element_t VECTORCALL runtime_extract(native_t<element_t> lhs, const int index) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return api_t<element_t>::extract(lhs, index);
#else
#if SIMDLIB_REGISTER_TEST_WIDTH == 128
	return SimdLib::Detail::SimdImpl128<element_t>::extract(lhs, index);
#else
	return SimdLib::Detail::SimdImpl256<element_t>::extract(lhs, index);
#endif
#endif
}

#if SIMDLIB_REGISTER_TEST_WIDTH == 128
/**
 * @brief Replaces one runtime-selected lane through the public Api or its direct 128-bit implementation reference.
 * @tparam element_t Scalar lane type.
 * @param lhs Source register.
 * @param rhs Replacement scalar lane.
 * @param index Runtime-selected lane index.
 * @return Register with the selected lane replaced.
 */
template <class element_t>
[[nodiscard]] SIMDLIB_FORCE_INLINE SIMDLIB_REGISTER_ONLY native_t<element_t> VECTORCALL runtime_insert(native_t<element_t> lhs, const element_t rhs,
																									   const int index) noexcept
{
#if SIMDLIB_CODEGEN_USE_WRAPPER
	return api_t<element_t>::insert(lhs, rhs, index);
#else
	return SimdLib::Detail::SimdImpl128<element_t>::insert(lhs, rhs, index);
#endif
}
#endif

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

/**
 * @brief Emits every fixed-width construction, observation, and transfer shape for one element type.
 * @param source Complete element source.
 * @param destination Complete element destination.
 * @param byte_source Complete raw-byte source.
 * @param byte_destination Complete raw-byte destination.
 * @param observed Fixed-array observation destination.
 * @param vectors Opaque native-result destination.
 */
template <class element_t>
SIMDLIB_FORCE_INLINE void VECTORCALL transfer(const array_t<element_t> &source_array, element_t *destination, const std::byte *byte_source,
											  std::byte *byte_destination, array_t<element_t> &observed, native_t<element_t> *vectors) noexcept
{
	using api_type [[maybe_unused]] = api_t<element_t>;
	using register_type [[maybe_unused]] = register_t<element_t>;
	const element_t *source = source_array.data();
#if SIMDLIB_CODEGEN_USE_WRAPPER
	const auto from_array = register_type::from_array(source_array);
	vectors[0] = from_array.native;
	vectors[1] = from_lanes<element_t>(source_array, std::make_index_sequence<register_type::lane_count>{});
	register_type::load(std::span<const element_t, register_type::lane_count>{source, register_type::lane_count})
		.store(std::span<element_t, register_type::lane_count>{destination, register_type::lane_count});
	register_type::load_aligned(std::span<const element_t, register_type::lane_count>{source, register_type::lane_count})
		.store_aligned(std::span<element_t, register_type::lane_count>{destination, register_type::lane_count});
	register_type::load_bytes(std::span<const std::byte, register_type::byte_count>{byte_source, register_type::byte_count})
		.store_bytes(std::span<std::byte, register_type::byte_count>{byte_destination, register_type::byte_count});
	observed = from_array.to_array();
#else
	const auto from_array = api_type::construct(source_array);
	vectors[0] = from_array;
	vectors[1] = from_lanes<element_t>(source_array, std::make_index_sequence<register_type::lane_count>{});
	api_type::store(api_type::load(std::span<const element_t, register_type::lane_count>{source, register_type::lane_count}),
					std::span<element_t, register_type::lane_count>{destination, register_type::lane_count});
	api_type::store_aligned(api_type::load_aligned(std::span<const element_t, register_type::lane_count>{source, register_type::lane_count}),
							std::span<element_t, register_type::lane_count>{destination, register_type::lane_count});
	api_type::store(api_type::load(std::span<const std::byte, register_type::byte_count>{byte_source, register_type::byte_count}),
					std::span<std::byte, register_type::byte_count>{byte_destination, register_type::byte_count});
	observed = api_type::to_array(from_array);
#endif
}

} // namespace SimdLibTypeMatrixCodegen

#define SIMDLIB_DEFINE_TYPE_MATRIX_FIXTURES(token, element_type)                                                                                               \
	/** @brief Compares every register-only common operation for one element type. */                                                                          \
	SIMDLIB_TYPE_MATRIX_NOINLINE void VECTORCALL simdlib_type_matrix_evaluate_##token(                                                                         \
		SimdLibTypeMatrixCodegen::native_t<element_type> lhs, SimdLibTypeMatrixCodegen::native_t<element_type> rhs,                                            \
		SimdLibTypeMatrixCodegen::native_t<element_type> third, element_type replacement, int count,                                                           \
		SimdLibTypeMatrixCodegen::native_t<element_type> *vectors, typename SimdLibTypeMatrixCodegen::api_t<element_type>::mask_t *scalars) noexcept           \
	{                                                                                                                                                          \
		SimdLibTypeMatrixCodegen::evaluate<element_type>(lhs, rhs, third, replacement, count, vectors, scalars);                                               \
	}                                                                                                                                                          \
	/** @brief Compares every fixed-width construction, observation, and transfer shape for one element type. */                                               \
	SIMDLIB_TYPE_MATRIX_NOINLINE void VECTORCALL simdlib_type_matrix_transfer_##token(                                                                         \
		const SimdLibTypeMatrixCodegen::array_t<element_type> &source, element_type *destination, const std::byte *byte_source, std::byte *byte_destination,   \
		SimdLibTypeMatrixCodegen::array_t<element_type> &observed, SimdLibTypeMatrixCodegen::native_t<element_type> *vectors) noexcept                         \
	{                                                                                                                                                          \
		SimdLibTypeMatrixCodegen::transfer<element_type>(source, destination, byte_source, byte_destination, observed, vectors);                               \
	}

#undef SIMDLIB_DEFINE_TYPE_MATRIX_FIXTURES

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

#define SIMDLIB_DEFINE_TYPE_MATRIX_RUNTIME_EXTRACT(token, element_type)                                                                                        \
	/** @brief Compares runtime-selected extraction with the direct width-specific implementation operation. */                                                \
	SIMDLIB_REGISTER_ONLY SIMDLIB_TYPE_MATRIX_NOINLINE element_type VECTORCALL simdlib_type_matrix_extract_runtime_##token(                                    \
		SimdLibTypeMatrixCodegen::native_t<element_type> lhs, const int index) noexcept                                                                        \
	{                                                                                                                                                          \
		return SimdLibTypeMatrixCodegen::runtime_extract<element_type>(lhs, index);                                                                            \
	}

#if SIMDLIB_REGISTER_TEST_WIDTH == 128
#define SIMDLIB_DEFINE_TYPE_MATRIX_RUNTIME_INSERT(token, element_type)                                                                                         \
	/** @brief Compares runtime-selected insertion with the direct 128-bit implementation operation. */                                                        \
	SIMDLIB_REGISTER_ONLY SIMDLIB_TYPE_MATRIX_NOINLINE SimdLibTypeMatrixCodegen::native_t<element_type> VECTORCALL simdlib_type_matrix_insert_runtime_##token( \
		SimdLibTypeMatrixCodegen::native_t<element_type> lhs, const element_type rhs, const int index) noexcept                                                \
	{                                                                                                                                                          \
		return SimdLibTypeMatrixCodegen::runtime_insert<element_type>(lhs, rhs, index);                                                                        \
	}
#else
#define SIMDLIB_DEFINE_TYPE_MATRIX_RUNTIME_INSERT(token, element_type)
#endif

#define SIMDLIB_DEFINE_TYPE_MATRIX_FIXTURES(token, element_type)                                                                                               \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, zero)                                                                                               \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, broadcast)                                                                                          \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, add)                                                                                                \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, subtract)                                                                                           \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, multiply)                                                                                           \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, divide)                                                                                             \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, modulus)                                                                                            \
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
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, shift_left)                                                                                         \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, logical_shift_right)                                                                                \
	SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR(token, element_type, shift_right)                                                                                        \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, movemask)                                                                                           \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, lane_sign_bits)                                                                                     \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, mask_bits)                                                                                          \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, mask_any)                                                                                           \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, mask_all)                                                                                           \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, mask_none)                                                                                          \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, equal)                                                                                              \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, not_equal)                                                                                          \
	SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR(token, element_type, extract_first)                                                                                      \
	SIMDLIB_DEFINE_TYPE_MATRIX_RUNTIME_EXTRACT(token, element_type)                                                                                            \
	SIMDLIB_DEFINE_TYPE_MATRIX_RUNTIME_INSERT(token, element_type)                                                                                             \
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

SIMDLIB_DEFINE_TYPE_MATRIX_FIXTURES(i8, std::int8_t)
SIMDLIB_DEFINE_TYPE_MATRIX_FIXTURES(u8, std::uint8_t)
SIMDLIB_DEFINE_TYPE_MATRIX_FIXTURES(i16, std::int16_t)
SIMDLIB_DEFINE_TYPE_MATRIX_FIXTURES(u16, std::uint16_t)
SIMDLIB_DEFINE_TYPE_MATRIX_FIXTURES(i32, std::int32_t)
SIMDLIB_DEFINE_TYPE_MATRIX_FIXTURES(u32, std::uint32_t)
SIMDLIB_DEFINE_TYPE_MATRIX_FIXTURES(i64, std::int64_t)
SIMDLIB_DEFINE_TYPE_MATRIX_FIXTURES(u64, std::uint64_t)
SIMDLIB_DEFINE_TYPE_MATRIX_FIXTURES(f32, float)
SIMDLIB_DEFINE_TYPE_MATRIX_FIXTURES(f64, double)

#undef SIMDLIB_DEFINE_TYPE_MATRIX_FIXTURES
#undef SIMDLIB_DEFINE_TYPE_MATRIX_RUNTIME_INSERT
#undef SIMDLIB_DEFINE_TYPE_MATRIX_RUNTIME_EXTRACT
#undef SIMDLIB_DEFINE_TYPE_MATRIX_SCALAR
#undef SIMDLIB_DEFINE_TYPE_MATRIX_VECTOR
#undef SIMDLIB_TYPE_MATRIX_NOINLINE
