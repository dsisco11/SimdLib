#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace SimdLib::IImpl
{

/** @brief Identifies a backend mapping that exposes a native vector type. */
template <class implementation_t>
concept Mapping = requires { typename implementation_t::vector_t; };

/** @brief Reports whether a backend can create an all-zero register. */
template <class implementation_t>
concept SetZero = Mapping<implementation_t> && requires { implementation_t::setzero(); };

/** @brief Reports whether a backend can broadcast one scalar value. */
template <class implementation_t, class scalar_t>
concept SetOne = Mapping<implementation_t> && requires(scalar_t value) { implementation_t::set1(value); };

/** @brief Reports whether a backend accepts a native-order lane list. */
template <class implementation_t, class... argument_t>
concept Set = Mapping<implementation_t> && requires(argument_t &&...values) { implementation_t::set(std::forward<argument_t>(values)...); };

/** @brief Reports whether a backend accepts a logical-order lane list. */
template <class implementation_t, class... argument_t>
concept SetReverse = Mapping<implementation_t> && requires(argument_t &&...values) { implementation_t::setr(std::forward<argument_t>(values)...); };

/** @brief Reports whether a backend exposes lane-wise addition. */
template <class implementation_t>
concept Add = Mapping<implementation_t> &&
			  requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::add(lhs, rhs); };

/** @brief Reports whether a backend exposes lane-wise subtraction. */
template <class implementation_t>
concept Subtract = Mapping<implementation_t> &&
				   requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::subtract(lhs, rhs); };

/** @brief Reports whether a backend exposes lane-wise multiplication. */
template <class implementation_t>
concept Multiply = Mapping<implementation_t> &&
				   requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::multiply(lhs, rhs); };

/** @brief Reports whether a backend exposes lane-wise division. */
template <class implementation_t>
concept Divide = Mapping<implementation_t> &&
				 requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::divide(lhs, rhs); };

/** @brief Reports whether a backend exposes lane-wise remainder. */
template <class implementation_t>
concept Modulus = Mapping<implementation_t> &&
				  requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::modulus(lhs, rhs); };

/** @brief Reports whether a backend exposes arithmetic negation. */
template <class implementation_t>
concept Negate = Mapping<implementation_t> && requires(typename implementation_t::vector_t value) { implementation_t::negate(value); };

/** @brief Reports whether a backend exposes lane-wise minimum. */
template <class implementation_t>
concept Min = Mapping<implementation_t> &&
			  requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::min(lhs, rhs); };

/** @brief Reports whether a backend exposes lane-wise maximum. */
template <class implementation_t>
concept Max = Mapping<implementation_t> &&
			  requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::max(lhs, rhs); };

/** @brief Reports whether a backend exposes lane-wise absolute value. */
template <class implementation_t>
concept Absolute = Mapping<implementation_t> && requires(typename implementation_t::vector_t value) { implementation_t::absolute(value); };

/** @brief Reports whether a backend exposes lane-wise square root. */
template <class implementation_t>
concept Sqrt = Mapping<implementation_t> && requires(typename implementation_t::vector_t value) { implementation_t::sqrt(value); };

/** @brief Reports whether a backend exposes a register magnitude operation. */
template <class implementation_t>
concept Magnitude = Mapping<implementation_t> && requires(typename implementation_t::vector_t value) { implementation_t::magnitude(value); };

/** @brief Reports whether a backend exposes checked integer magnitude. */
template <class implementation_t>
concept MagnitudeChecked = Mapping<implementation_t> && requires(typename implementation_t::vector_t value) { implementation_t::magnitude_checked(value); };

/** @brief Reports whether a backend exposes lane-wise average. */
template <class implementation_t>
concept Average = Mapping<implementation_t> &&
				  requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::avg(lhs, rhs); };

/** @brief Reports whether a backend exposes fused or emulated multiply-add. */
template <class implementation_t>
concept MultiplyAdd = Mapping<implementation_t> && requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs,
															typename implementation_t::vector_t addend) { implementation_t::multiply_add(lhs, rhs, addend); };

/** @brief Reports whether backend primitives required by normalization are available. */
template <class implementation_t>
concept Normalize = Magnitude<implementation_t> && Divide<implementation_t>;

/** @brief Reports whether a backend exposes adjacent horizontal addition. */
template <class implementation_t>
concept HorizontalAdd = Mapping<implementation_t> && requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) {
	implementation_t::add_horizontal(lhs, rhs);
};

/** @brief Reports whether a backend exposes adjacent horizontal subtraction. */
template <class implementation_t>
concept HorizontalSubtract = Mapping<implementation_t> && requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) {
	implementation_t::subtract_horizontal(lhs, rhs);
};

/** @brief Reports whether a backend exposes adjacent multiply-add. */
template <class implementation_t>
concept MultiplyAddAdjacent = Mapping<implementation_t> && requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) {
	implementation_t::multiply_add_adjacent(lhs, rhs);
};

/** @brief Reports whether a backend exposes unsigned-byte by signed-byte multiply-add. */
template <class implementation_t>
concept ByteMultiplyAdd = Mapping<implementation_t> && requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) {
	implementation_t::multiply_add_unsigned_signed_bytes(lhs, rhs);
};

/** @brief Reports whether a backend exposes byte sum-of-absolute-differences. */
template <class implementation_t>
concept Sad = Mapping<implementation_t> && requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) {
	implementation_t::sum_absolute_byte_differences(lhs, rhs);
};

/** @brief Reports whether a backend exposes immediate-controlled multi-SAD. */
template <class implementation_t, int immediate>
concept MultiSad = Mapping<implementation_t> && requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) {
	implementation_t::template multi_sum_absolute_byte_differences<immediate>(lhs, rhs);
};

/** @brief Reports whether a backend exposes the primitives used to locate an extremum. */
template <class implementation_t>
concept Position = Mapping<implementation_t> && requires(typename implementation_t::vector_t value) {
	implementation_t::min_position(value);
	implementation_t::template extract<1>(value);
};

/** @brief Reports whether a backend exposes saturating addition. */
template <class implementation_t>
concept AddSaturated = Mapping<implementation_t> && requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) {
	implementation_t::add_saturated(lhs, rhs);
};

/** @brief Reports whether a backend exposes saturating subtraction. */
template <class implementation_t>
concept SubtractSaturated = Mapping<implementation_t> && requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) {
	implementation_t::subtract_saturated(lhs, rhs);
};

/** @brief Reports whether a backend exposes saturating horizontal addition. */
template <class implementation_t>
concept HorizontalAddSaturated = Mapping<implementation_t> && requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) {
	implementation_t::hadd_saturated(lhs, rhs);
};

/** @brief Reports whether a backend exposes saturating horizontal subtraction. */
template <class implementation_t>
concept HorizontalSubtractSaturated = Mapping<implementation_t> && requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) {
	implementation_t::hsubtract_saturated(lhs, rhs);
};

/** @brief Reports whether a backend exposes alternating add-subtract. */
template <class implementation_t>
concept AddSubtract = Mapping<implementation_t> &&
					  requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::add_subtract(lhs, rhs); };

/** @brief Reports whether a backend exposes an immediate-controlled dot product. */
template <class implementation_t, int immediate>
concept DotProduct = Mapping<implementation_t> && requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) {
	implementation_t::template dot_product<immediate>(lhs, rhs);
};

/** @brief Reports whether a backend exposes bitwise AND. */
template <class implementation_t>
concept BitwiseAnd = Mapping<implementation_t> &&
					 requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::bitwise_and(lhs, rhs); };

/** @brief Reports whether a backend exposes bitwise OR. */
template <class implementation_t>
concept BitwiseOr = Mapping<implementation_t> &&
					requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::bitwise_or(lhs, rhs); };

/** @brief Reports whether a backend exposes bitwise XOR. */
template <class implementation_t>
concept BitwiseXor = Mapping<implementation_t> &&
					 requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::bitwise_xor(lhs, rhs); };

/** @brief Reports whether a backend exposes bitwise AND-NOT. */
template <class implementation_t>
concept BitwiseAndNot = Mapping<implementation_t> && requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) {
	implementation_t::bitwise_andnot(lhs, rhs);
};

/** @brief Reports whether a backend exposes bitwise complement. */
template <class implementation_t>
concept BitwiseNot = Mapping<implementation_t> && requires(typename implementation_t::vector_t value) { implementation_t::bitwise_not(value); };

/** @brief Reports whether a backend exposes predicate-based selection. */
template <class implementation_t>
concept Select =
	Mapping<implementation_t> && requires(typename implementation_t::vector_t condition, typename implementation_t::vector_t when_true,
										  typename implementation_t::vector_t when_false) { implementation_t::select(condition, when_true, when_false); };

/** @brief Reports whether a backend exposes its legacy expand operation. */
template <class implementation_t>
concept Expand = Mapping<implementation_t> &&
				 requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::expand(lhs, rhs); };

/** @brief Reports whether a backend exposes its legacy compress operation. */
template <class implementation_t>
concept Compress = Mapping<implementation_t> &&
				   requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::compress(lhs, rhs); };

/** @brief Reports whether a backend can widen into the requested destination mapping. */
template <class implementation_t, class target_t>
concept Widen = Mapping<implementation_t> && requires(typename implementation_t::vector_t value) { implementation_t::template widen<target_t>(value); };

/** @brief Reports whether a backend exposes compile-time lane extraction. */
template <class implementation_t, std::size_t index>
concept IndexedExtract = Mapping<implementation_t> && requires(typename implementation_t::vector_t value) { implementation_t::template extract<index>(value); };

/** @brief Reports whether a backend exposes explicit slow-path runtime-selected extraction. */
template <class implementation_t, class selector_t>
concept ExtractSlow =
	Mapping<implementation_t> && requires(typename implementation_t::vector_t value, selector_t selector) { implementation_t::extract_slow(value, selector); };

/** @brief Reports whether a backend exposes extraction of its lower 128-bit half. */
template <class implementation_t>
concept LowerHalf = Mapping<implementation_t> && requires(typename implementation_t::vector_t value) { implementation_t::lower_half(value); };

/** @brief Reports whether a backend accepts explicit slow-path runtime insertion arguments. */
template <class implementation_t, class... argument_t>
concept InsertSlow = Mapping<implementation_t> && requires(argument_t &&...values) { implementation_t::insert_slow(std::forward<argument_t>(values)...); };

/** @brief Reports whether a backend exposes low-lane unpacking. */
template <class implementation_t>
concept UnpackLow = Mapping<implementation_t> &&
					requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::unpack_lo(lhs, rhs); };

/** @brief Reports whether a backend exposes high-lane unpacking. */
template <class implementation_t>
concept UnpackHigh = Mapping<implementation_t> &&
					 requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) { implementation_t::unpack_hi(lhs, rhs); };

/** @brief Reports whether a backend accepts a logical shuffle index sequence for its native vector type. */
template <class implementation_t, std::size_t... indices>
concept IndexedShuffle =
	Mapping<implementation_t> && requires(typename implementation_t::vector_t value) { implementation_t::template shuffle<indices...>(value); };

/** @brief Reports whether a backend accepts the supplied shuffle arguments. */
template <class implementation_t, class... argument_t>
concept Shuffle = Mapping<implementation_t> && requires(argument_t &&...values) { implementation_t::shuffle(std::forward<argument_t>(values)...); };
/** @brief Reports whether a backend accepts explicit slow-path scalar-controlled shuffle arguments. */
template <class implementation_t, class... argument_t>
concept ShuffleSlow = Mapping<implementation_t> && requires(argument_t &&...values) { implementation_t::shuffle_slow(std::forward<argument_t>(values)...); };

/** @brief Reports whether a backend accepts the supplied low-half shuffle arguments. */
template <class implementation_t, class... argument_t>
concept ShuffleLow = Mapping<implementation_t> && requires(argument_t &&...values) { implementation_t::shuffle_lo(std::forward<argument_t>(values)...); };
/** @brief Reports whether a backend accepts explicit slow-path low-half shuffle arguments. */
template <class implementation_t, class... argument_t>
concept ShuffleLowSlow =
	Mapping<implementation_t> && requires(argument_t &&...values) { implementation_t::shuffle_lo_slow(std::forward<argument_t>(values)...); };

/** @brief Reports whether a backend accepts the supplied high-half shuffle arguments. */
template <class implementation_t, class... argument_t>
concept ShuffleHigh = Mapping<implementation_t> && requires(argument_t &&...values) { implementation_t::shuffle_hi(std::forward<argument_t>(values)...); };
/** @brief Reports whether a backend accepts explicit slow-path high-half shuffle arguments. */
template <class implementation_t, class... argument_t>
concept ShuffleHighSlow =
	Mapping<implementation_t> && requires(argument_t &&...values) { implementation_t::shuffle_hi_slow(std::forward<argument_t>(values)...); };

/** @brief Reports whether a backend accepts the supplied blend arguments. */
template <class implementation_t, class... argument_t>
concept Blend = Mapping<implementation_t> && requires(argument_t &&...values) { implementation_t::blend(std::forward<argument_t>(values)...); };
/** @brief Reports whether a backend accepts explicit slow-path scalar-controlled blend arguments. */
template <class implementation_t, class... argument_t>
concept BlendSlow = Mapping<implementation_t> && requires(argument_t &&...values) { implementation_t::blend_slow(std::forward<argument_t>(values)...); };

/** @brief Reports whether a backend exposes explicit slow-path 32-bit immediate-mask shuffling. */
template <class implementation_t>
concept Shuffle32Slow =
	Mapping<implementation_t> && requires(typename implementation_t::int_vector_t value) { implementation_t::shuffle_32_slow(value, std::uint32_t{}); };

/** @brief Reports whether a backend exposes explicit slow-path complete-register byte shifts. */
template <class implementation_t>
concept ShiftBytesSlow = Mapping<implementation_t> && requires(typename implementation_t::int_vector_t value) {
	implementation_t::shift_bytes_left_slow(value, 1);
	implementation_t::shift_bytes_right_slow(value, 1);
};

/** @brief Reports whether a backend exposes an immediate complete-register byte left shift. */
template <class implementation_t, int count>
concept ShiftBytesLeft =
	Mapping<implementation_t> && requires(typename implementation_t::int_vector_t value) { implementation_t::template shift_bytes_left<count>(value); };

/** @brief Reports whether a backend exposes an immediate complete-register byte right shift. */
template <class implementation_t, int count>
concept ShiftBytesRight =
	Mapping<implementation_t> && requires(typename implementation_t::int_vector_t value) { implementation_t::template shift_bytes_right<count>(value); };

/** @brief Reports whether a backend exposes explicit slow-path complete-register bit shifts. */
template <class implementation_t>
concept ShiftBitsSlow = Mapping<implementation_t> && requires(typename implementation_t::int_vector_t value) {
	implementation_t::shift_bits_left_slow(value, 1);
	implementation_t::shift_bits_right_slow(value, 1);
};

/** @brief Reports whether a backend exposes compile-time complete-register bit shifts. */
template <class implementation_t, int count>
concept ShiftBits = Mapping<implementation_t> && requires(typename implementation_t::int_vector_t value) {
	implementation_t::template shift_bits_left<count>(value);
	implementation_t::template shift_bits_right<count>(value);
};

/** @brief Reports whether a backend exposes an immediate-controlled low-half shuffle. */
template <class implementation_t, int immediate>
concept IndexedShuffleLow =
	Mapping<implementation_t> && requires(typename implementation_t::vector_t value) { implementation_t::template shuffle_lo<immediate>(value); };

/** @brief Reports whether a backend exposes an immediate-controlled high-half shuffle. */
template <class implementation_t, int immediate>
concept IndexedShuffleHigh =
	Mapping<implementation_t> && requires(typename implementation_t::vector_t value) { implementation_t::template shuffle_hi<immediate>(value); };

/** @brief Reports whether a backend exposes an immediate-controlled blend. */
template <class implementation_t, int immediate>
concept IndexedBlend = Mapping<implementation_t> && requires(typename implementation_t::vector_t lhs, typename implementation_t::vector_t rhs) {
	implementation_t::template blend<immediate>(lhs, rhs);
};

} // namespace SimdLib::IImpl
