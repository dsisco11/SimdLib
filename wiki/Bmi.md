# Bmi

`SimdLib::Bmi` contains constexpr-friendly bit-manipulation helpers. The generic forms work across integral widths, while optimized target paths are selected when available.

## Contents

- [Overview](#overview)
- [Example include](#example-setup)
- [`abs`](#abs)
- [`andn`](#andn)
- [`bextr`](#bextr)
- [`blse`](#blse)
- [`blsi`](#blsi)
- [`blsioff`](#blsioff)
- [`blsmsk`](#blsmsk)
- [`blsr`](#blsr)
- [`bmse`](#bmse)
- [`bmsi`](#bmsi)
- [`bmsmsk`](#bmsmsk)
- [`bmsr`](#bmsr)
- [`boolmask`](#boolmask)
- [`bzhi`](#bzhi)
- [`bzlo`](#bzlo)
- [`clear_bits_higher_than`](#clear-bits-higher-than)
- [`clear_bits_lower_than`](#clear-bits-lower-than)
- [`clear_leading_ones`](#clear-leading-ones)
- [`clear_lowest_set_bits`](#clear-lowest-set-bits)
- [`clear_trailing_ones`](#clear-trailing-ones)
- [`consume_bit_sequence_left`](#consume-bit-sequence-left)
- [`consume_bit_sequence_right`](#consume-bit-sequence-right)
- [`extract_bits_higher_than`](#extract-bits-higher-than)
- [`extract_bits_lower_than`](#extract-bits-lower-than)
- [`flip_trailing_zeros`](#flip-trailing-zeros)
- [`flipr_unset`](#flipr-unset)
- [`left_collapse_trailing_bits`](#left-collapse-trailing-bits)
- [`mask_bits_lower_than_lsb`](#mask-bits-lower-than-lsb)
- [`mask_bits_lower_than_lsb_or_all_ones`](#mask-bits-lower-than-lsb-or-all-ones)
- [`mask_leading_ones`](#mask-leading-ones)
- [`mask_leading_zeros`](#mask-leading-zeros)
- [`mask_trailing_ones`](#mask-trailing-ones)
- [`mask_trailing_zeros`](#mask-trailing-zeros)
- [`mask_trailing_zeros_or_zero`](#mask-trailing-zeros-or-zero)
- [`maskl_trailing_one`](#maskl-trailing-one)
- [`maskr_unset`](#maskr-unset)
- [`max`](#max)
- [`min`](#min)
- [`mulx`](#mulx)
- [`PartialSumBLSI`](#partialsumblsi)
- [`PartialSumBLSMSK`](#partialsumblsmsk)
- [`pdep_u32`](#pdep-u32)
- [`pdep_u64`](#pdep-u64)
- [`pdepl_u32`](#pdepl-u32)
- [`pdepl_u64`](#pdepl-u64)
- [`pext_u32`](#pext-u32)
- [`pext_u64`](#pext-u64)
- [`pp_and`](#pp-and)
- [`pp_andn`](#pp-andn)
- [`pp_andni`](#pp-andni)
- [`pp_lsor`](#pp-lsor)
- [`pp_or`](#pp-or)
- [`pp_xor`](#pp-xor)
- [`ps_and`](#ps-and)
- [`ps_andn`](#ps-andn)
- [`ps_andni`](#ps-andni)
- [`ps_or`](#ps-or)
- [`ps_xor`](#ps-xor)
- [`select`](#select)
- [Related types and constants](#related-types-and-constants)

<a id="overview"></a>
## Overview

Include `<SimdLib/Bmi.h>`. Overloads with the same name are collected in one subsection; every public overload is listed below.

<a id="example-setup"></a>
## Example include

```cpp
#include <SimdLib/SimdLib.h>
```
<a id="abs"></a>
## `abs`

Branchless find absolute value of the input.

Signatures:

```cpp
template <std::integral int_t> int_t abs(int_t lhs)
```

Example:

```cpp
SimdLib::Bmi::abs(-7); // => 7
```

<a id="andn"></a>
## `andn`

Compute the bitwise NOT of LHS and then AND with RHS.

Signatures:

```cpp
template <integer_like int_t> int_t andn(int_t lhs, int_t rhs)
```

Example:

```cpp
SimdLib::Bmi::andn(0b1100U, 0b1010U); // => 0b0010U
```

<a id="bextr"></a>
## `bextr`

Extract contiguous bits from source integer, and return them shifted to the LSB side of the output. Extract the number of bits specified by len, starting at the bit specified by start.

Signatures:

```cpp
template <std::integral int_t> int_t bextr(int_t source, std::uint8_t len, std::uint8_t start)
template <std::integral int_t, std::size_t len> int_t bextr(int_t source, std::uint8_t start)
template <std::integral int_t, std::size_t start, std::size_t len> int_t bextr(int_t source)
```

Example:

```cpp
SimdLib::Bmi::bextr(0b1101'0110U, 3, 2); // => 0b101U
```

<a id="blse"></a>
## `blse`

Extract and reset the lowest set bit in source.

Signatures:

```cpp
template <integer_like int_t> int_t blse(int_t source, int_t &out_lsb)
template <integer_like int_t> std::tuple<int_t, int_t> blse(int_t source)
```

Example:

```cpp
SimdLib::Bmi::blse(0b10100U); // => {remaining 0b10000U, extracted 0b00100U}
```

<a id="blsi"></a>
## `blsi`

Extract the lowest set bit from source integer and set the corresponding bit in dst. All other bits in dst are zeroed, and all bits are zeroed if no bits are set in source.

Signatures:

```cpp
template <integer_like int_t> int_t blsi(int_t source)
```

Example:

```cpp
SimdLib::Bmi::blsi(0b10100U); // => 0b00100U
```

<a id="blsioff"></a>
## `blsioff`

Extracts the first source bit at or above an inclusive one-hot boundary.

Signatures:

```cpp
template <integer_like int_t> int_t blsioff(int_t source, int_t starting_bit)
```

Example:

```cpp
SimdLib::Bmi::blsioff(0b10100, 0b01000); // => 0b10000
```

<a id="blsmsk"></a>
## `blsmsk`

Set all the lower bits of dst up to and including the lowest set bit in source.

Signatures:

```cpp
template <integer_like int_t> int_t blsmsk(int_t source)
```

Example:

```cpp
SimdLib::Bmi::blsmsk(0b10100U); // => 0b00111U
```

<a id="blsr"></a>
## `blsr`

Copy all bits from source to dst, and reset (set to 0) the bit in dst that corresponds to the lowest set bit in source.

Signatures:

```cpp
template <integer_like int_t> int_t blsr(int_t source)
```

Example:

```cpp
SimdLib::Bmi::blsr(0b10100U); // => 0b10000U
```

<a id="bmse"></a>
## `bmse`

Extract and reset the highest set bit in source.

Signatures:

```cpp
template <integer_like int_t> int_t bmse(int_t value, int_t &out_msb)
template <integer_like int_t> std::tuple<int_t, int_t> bmse(int_t value)
```

Example:

```cpp
SimdLib::Bmi::bmse(0b10100U); // => {remaining 0b00100U, extracted 0b10000U}
```

<a id="bmsi"></a>
## `bmsi`

Extract the highest set bit from source integer and set the corresponding bit in dst. All other bits in dst are zeroed, and all bits are zeroed if no bits are set in source.

Signatures:

```cpp
template <integer_like int_t> int_t bmsi(int_t value)
```

Example:

```cpp
SimdLib::Bmi::bmsi(0b10100U); // => 0b10000U
```

<a id="bmsmsk"></a>
## `bmsmsk`

Set all the lower bits of dst up to and including the highest set bit in source.

Signatures:

```cpp
template <std::integral int_t> int_t bmsmsk(int_t source)
```

Example:

```cpp
SimdLib::Bmi::bmsmsk(0b10100U); // => 0b11111U
```

<a id="bmsr"></a>
## `bmsr`

Copy all bits from source to dst, and reset (set to 0) the bit in dst that corresponds to the highest set bit in source.

Signatures:

```cpp
template <integer_like int_t> int_t bmsr(int_t value)
template <integer_like int_t> int_t bmsr(int_t value, int &out_msb_index)
```

Example:

```cpp
SimdLib::Bmi::bmsr(0b10100U); // => 0b00100U
```

<a id="boolmask"></a>
## `boolmask`

Turns a boolean value into an integer-width bitmask of all ones or zeros (0 for false, all 1s for true).

Signatures:

```cpp
template <integer_like int_t> int_t boolmask(bool state)
```

Example:

```cpp
SimdLib::Bmi::boolmask<std::uint32_t>(true); // => 0xFFFFFFFFU
```

<a id="bzhi"></a>
## `bzhi`

Copy all bits from source integer, and reset (set to 0) the high bits in output starting at index.

Signatures:

```cpp
template <integer_like int_t> int_t bzhi(int_t source, unsigned index)
```

Example:

```cpp
SimdLib::Bmi::bzhi(0b1111'0110U, 4U); // => 0b0000'0110U
```

<a id="bzlo"></a>
## `bzlo`

Clears every source bit whose bit index is less than index.

Signatures:

```cpp
template <integer_like int_t> int_t bzlo(int_t source, unsigned index)
```

Example:

```cpp
SimdLib::Bmi::bzlo(0b11111, 3); // => 0b11000
```

<a id="clear-bits-higher-than"></a>
## `clear_bits_higher_than`

Clears all bits higher than (not including) the given target-bit from the source integer.

Signatures:

```cpp
template <integer_like int_t> int_t clear_bits_higher_than(int_t value, int_t target_bit)
```

Example:

```cpp
SimdLib::Bmi::clear_bits_higher_than(0b10111, 0b100); // => 0b00111
```

<a id="clear-bits-lower-than"></a>
## `clear_bits_lower_than`

Clears all bits lower than (not including) the given target-bit from the source integer.

Signatures:

```cpp
template <integer_like int_t> int_t clear_bits_lower_than(int_t value, int_t target_bit)
```

Example:

```cpp
SimdLib::Bmi::clear_bits_lower_than(0b10111, 0b100); // => 0b10100
```

<a id="clear-leading-ones"></a>
## `clear_leading_ones`

Clears all most significant, rightmost (high-bits) leading set bits.

Signatures:

```cpp
template <integer_like int_t> int_t clear_leading_ones(int_t value)
```

Example:

```cpp
SimdLib::Bmi::clear_leading_ones<std::uint8_t>(0b1101'0101U); // => 0b0001'0101U
```

<a id="clear-lowest-set-bits"></a>
## `clear_lowest_set_bits`

Copy all bits from the source integer, and reset (set to 0) the leftmost (low-bits) string of contiguous set bits.

Signatures:

```cpp
template <integer_like int_t> int_t clear_lowest_set_bits(int_t value)
template <integer_like int_t> int_t clear_lowest_set_bits(int_t value, int_t &out_consumed)
```

Example:

```cpp
SimdLib::Bmi::clear_lowest_set_bits(0U); // => 0U
```

<a id="clear-trailing-ones"></a>
## `clear_trailing_ones`

Clears all least significant, leftmost (low-bits) trailing set bits.

Signatures:

```cpp
template <integer_like int_t> int_t clear_trailing_ones(int_t value)
```

Example:

```cpp
SimdLib::Bmi::clear_trailing_ones(0U); // => 0U
```

<a id="consume-bit-sequence-left"></a>
## `consume_bit_sequence_left`

Extracts and returns the rightmost (high-bits) string of contiguous set bits, said bits are also reset (set to 0) within the source integer.

Signatures:

```cpp
template <integer_like int_t> std::tuple<int_t, int_t> consume_bit_sequence_left(int_t value)
```

Example:

```cpp
SimdLib::Bmi::consume_bit_sequence_left(
    0b0110'111U); // => {remaining 0b0000'111U, extracted 0b0110'000U}
```

<a id="consume-bit-sequence-right"></a>
## `consume_bit_sequence_right`

Extracts and returns the leftmost (low-bits) string of contiguous set bits, said bits are also reset (set to 0) within the source integer.

Signatures:

```cpp
template <integer_like int_t> std::tuple<int_t, int_t> consume_bit_sequence_right(int_t value)
```

Example:

```cpp
SimdLib::Bmi::consume_bit_sequence_right(
    0b1011U); // => {remaining 0b1000U, extracted 0b0011U}
```

<a id="extract-bits-higher-than"></a>
## `extract_bits_higher_than`

Extracts all bits higher than (not including) the given target-bit from the source integer.

Signatures:

```cpp
template <integer_like int_t> int_t extract_bits_higher_than(int_t value, int_t target_bit)
```

Example:

```cpp
SimdLib::Bmi::extract_bits_higher_than(0b10111, 0b001); // => 0b10110
```

<a id="extract-bits-lower-than"></a>
## `extract_bits_lower_than`

Extracts all bits lower than (not including) the given target-bit from the source integer.

Signatures:

```cpp
template <integer_like int_t> int_t extract_bits_lower_than(int_t value, int_t target_bit)
```

Example:

```cpp
SimdLib::Bmi::extract_bits_lower_than(0b10111, 0b100); // => 0b00011
```

<a id="flip-trailing-zeros"></a>
## `flip_trailing_zeros`

Sets all least significant, leftmost (low-bits) trailing unset bits.

Signatures:

```cpp
template <integer_like int_t> int_t flip_trailing_zeros(int_t value)
```

Example:

```cpp
SimdLib::Bmi::flip_trailing_zeros(0U); // => 0U
```

<a id="flipr-unset"></a>
## `flipr_unset`

Sets the least significant, leftmost (low-bits) unset bit.

Signatures:

```cpp
template <integer_like int_t> int_t flipr_unset(int_t value)
```

Example:

```cpp
SimdLib::Bmi::flipr_unset(0U); // => 0U
```

<a id="left-collapse-trailing-bits"></a>
## `left_collapse_trailing_bits`

Copy all bits from the source integer, and reset (set to 0) the trailing bits up-to but excluding the rightmost (high-bits) trailing set bit.

Signatures:

```cpp
template <integer_like int_t> int_t left_collapse_trailing_bits(int_t value)
```

Example:

```cpp
SimdLib::Bmi::left_collapse_trailing_bits(0U); // => 0U
```

<a id="mask-bits-lower-than-lsb"></a>
## `mask_bits_lower_than_lsb`

Returns a mask of all bits strictly lower than the least-significant set bit (LSB). For value==0, returns 0.

Signatures:

```cpp
template <integer_like int_t> int_t mask_bits_lower_than_lsb(int_t value)
```

Example:

```cpp
SimdLib::Bmi::mask_bits_lower_than_lsb(0U); // => 0U
```

<a id="mask-bits-lower-than-lsb-or-all-ones"></a>
## `mask_bits_lower_than_lsb_or_all_ones`

Returns a mask of all bits strictly lower than the least-significant set bit (LSB). For value==0, returns all-ones (useful as a "no constraint" mask).

Signatures:

```cpp
template <integer_like int_t> int_t mask_bits_lower_than_lsb_or_all_ones(int_t value)
```

Example:

```cpp
SimdLib::Bmi::mask_bits_lower_than_lsb_or_all_ones(0U); // => 0U
```

<a id="mask-leading-ones"></a>
## `mask_leading_ones`

Returns a mask over the leading ones in the source integer.

Signatures:

```cpp
template <integer_like int_t> int_t mask_leading_ones(int_t value)
```

Example:

```cpp
SimdLib::Bmi::mask_leading_ones<std::uint8_t>(0b1110'1011U); // => 0b1110'0000U
```

<a id="mask-leading-zeros"></a>
## `mask_leading_zeros`

Returns a mask over the leading zeros in the source integer.

Signatures:

```cpp
template <integer_like int_t> int_t mask_leading_zeros(int_t value)
```

Example:

```cpp
SimdLib::Bmi::mask_leading_zeros<std::uint8_t>(0b0001'0101U); // => 0b1110'0000U
```

<a id="mask-trailing-ones"></a>
## `mask_trailing_ones`

Returns a mask over the trailing 1-bits in the source integer, producing 0 if none.

Signatures:

```cpp
template <integer_like int_t> int_t mask_trailing_ones(int_t value)
```

Example:

```cpp
SimdLib::Bmi::mask_trailing_ones(0U); // => 0U
```

<a id="mask-trailing-zeros"></a>
## `mask_trailing_zeros`

Returns a mask over the trailing 0-bits in the source integer.

Signatures:

```cpp
template <integer_like int_t> int_t mask_trailing_zeros(int_t value)
```

Example:

```cpp
SimdLib::Bmi::mask_trailing_zeros(0U); // => 0U
```

<a id="mask-trailing-zeros-or-zero"></a>
## `mask_trailing_zeros_or_zero`

Returns a mask over the trailing 0-bits in the source integer. For value==0, returns 0 ("safe" variant; avoids the wraparound/all-ones behavior).

Signatures:

```cpp
template <integer_like int_t> int_t mask_trailing_zeros_or_zero(int_t value)
```

Example:

```cpp
SimdLib::Bmi::mask_trailing_zeros_or_zero(0U); // => 0U
```

<a id="maskl-trailing-one"></a>
## `maskl_trailing_one`

Returns a single 1-bit at the position of the rightmost (high-bits) trailing 1-bit, producing 0 if none.

Signatures:

```cpp
template <integer_like int_t> int_t maskl_trailing_one(int_t value)
```

Example:

```cpp
SimdLib::Bmi::maskl_trailing_one(0U); // => 0U
```

<a id="maskr-unset"></a>
## `maskr_unset`

Returns a single 1-bit at the position of the leftmost (low-bits) 0-bit, producing 0 if none.

Signatures:

```cpp
template <integer_like int_t> int_t maskr_unset(int_t value)
```

Example:

```cpp
SimdLib::Bmi::maskr_unset(0U); // => 0U
```

<a id="max"></a>
## `max`

Branchless find maximum of two values.

Signatures:

```cpp
template <std::integral int_t> int_t max(int_t lhs, int_t rhs)
```

Example:

```cpp
SimdLib::Bmi::max(4, 9); // => 9
```

<a id="min"></a>
## `min`

Branchless find minimum of two values.

Signatures:

```cpp
template <std::integral int_t> int_t min(int_t lhs, int_t rhs)
```

Example:

```cpp
SimdLib::Bmi::min(4, 9); // => 4
```

<a id="mulx"></a>
## `mulx`

Multiplies the unsigned object-representation values of two integers.

Signatures:

```cpp
template <std::integral int_t> int_t mulx(int_t lhs, int_t rhs, int_t &hi)
```

Example:

```cpp
std::uint8_t high = 0;
SimdLib::Bmi::mulx<std::uint8_t>(0xFF, 0x02, high); // => 0xFE and high is 0x01
```

<a id="partialsumblsi"></a>
## `PartialSumBLSI`

Builds the partial sums used by the parallel-suffix isolated-bit helpers.

Signatures:

```cpp
template <integer_like int_t> int_t PartialSumBLSI(int_t value)
```

Example:

```cpp
SimdLib::Bmi::PartialSumBLSI<std::uint32_t>(0b1011U); // => 24U
```

<a id="partialsumblsmsk"></a>
## `PartialSumBLSMSK`

Builds the partial sums used by the parallel-prefix bit-mask helpers.

Signatures:

```cpp
template <integer_like int_t> int_t PartialSumBLSMSK(int_t value)
```

Example:

```cpp
SimdLib::Bmi::PartialSumBLSMSK<std::uint32_t>(0b1011U); // => 37U
```

<a id="pdep-u32"></a>
## `pdep_u32`

Note: This is a wrapper for the '_pdep_xxx' intrinsic providing compile-time emulation.

Signatures:

```cpp
std::uint32_t pdep_u32(std::uint32_t source, std::uint32_t mask)
```

Example:

```cpp
SimdLib::Bmi::pdep_u32(0b101U, 0b0101'0100U); // => 0b0100'0100U
```

<a id="pdep-u64"></a>
## `pdep_u64`

Note: This is a wrapper for the '_pdep_xxx' intrinsic providing compile-time emulation.

Signatures:

```cpp
std::uint64_t pdep_u64(std::uint64_t source, std::uint64_t mask)
```

Example:

```cpp
SimdLib::Bmi::pdep_u64(0b101ULL, 0b0101'0100ULL); // => 0b0100'0100ULL
```

<a id="pdepl-u32"></a>
## `pdepl_u32`

This is a "pdep, but from right (high-bits) to left (low-bits)" aka "expand left"

Signatures:

```cpp
std::uint32_t pdepl_u32(std::uint32_t source, std::uint32_t mask)
```

Example:

```cpp
SimdLib::Bmi::pdepl_u32(0xA000'0000U, 0b0101'0100U); // => 0b0100'0100U
```

<a id="pdepl-u64"></a>
## `pdepl_u64`

This is a "pdep, but from right (high-bits) to left (low-bits)" aka "expand left"

Signatures:

```cpp
std::uint64_t pdepl_u64(std::uint64_t source, std::uint64_t mask)
```

Example:

```cpp
SimdLib::Bmi::pdepl_u64(0xA000'0000'0000'0000ULL, 0b0101'0100ULL); // => 0b0100'0100ULL
```

<a id="pext-u32"></a>
## `pext_u32`

Note: This is a wrapper for the '_pext_xxx' intrinsic, providing compile-time emulation.

Signatures:

```cpp
std::uint32_t pext_u32(std::uint32_t source, std::uint32_t mask)
```

Example:

```cpp
SimdLib::Bmi::pext_u32(0b0100'0100U, 0b0101'0100U); // => 0b101U
```

<a id="pext-u64"></a>
## `pext_u64`

Note: This is a wrapper for the '_pext_xxx' intrinsic, providing compile-time emulation.

Signatures:

```cpp
std::uint64_t pext_u64(std::uint64_t source, std::uint64_t mask)
```

Example:

```cpp
SimdLib::Bmi::pext_u64(0b0100'0100ULL, 0b0101'0100ULL); // => 0b101ULL
```

<a id="pp-and"></a>
## `pp_and`

Computes a distance-1 parallel-prefix AND stage by ANDing each bit with its adjacent bit to the right (high-bits).

Signatures:

```cpp
template <integer_like int_t> int_t pp_and(int_t value)
```

Example:

```cpp
SimdLib::Bmi::pp_and(0U); // => 0U
```

<a id="pp-andn"></a>
## `pp_andn`

Computes a distance-1 parallel-prefix AND-NOT stage, retaining set bits whose adjacent bit to the right (high-bits) is clear.

Signatures:

```cpp
template <integer_like int_t> int_t pp_andn(int_t value)
```

Example:

```cpp
SimdLib::Bmi::pp_andn(0U); // => 0U
```

<a id="pp-andni"></a>
## `pp_andni`

Computes an inverse distance-1 parallel-prefix AND-NOT stage, marking clear bits whose adjacent bit to the right (high-bits) is set.

Signatures:

```cpp
template <integer_like int_t> int_t pp_andni(int_t value)
```

Example:

```cpp
SimdLib::Bmi::pp_andni(0U); // => 0U
```

<a id="pp-lsor"></a>
## `pp_lsor`

Computes the parallel-prefix-least-significant-OR of the given value, which is the result of clearing all bits to the right (high-bits) of the lsb and then or'ing each bit with all bits to the left (low-bits).

Signatures:

```cpp
template <integer_like int_t> int_t pp_lsor(int_t value)
```

Example:

```cpp
SimdLib::Bmi::pp_lsor(SimdLib::uint128_t{0b10100}); // => 0b00111
```

<a id="pp-or"></a>
## `pp_or`

Computes the parallel-prefix OR of the given value, which is the result of or'ing each bit with all bits to the left (low-bits).

Signatures:

```cpp
template <integer_like int_t> int_t pp_or(int_t value)
```

Example:

```cpp
SimdLib::Bmi::pp_or(SimdLib::uint128_t{0b10100}); // => 0b11111
```

<a id="pp-xor"></a>
## `pp_xor`

Computes a distance-1 parallel-prefix XOR stage by XORing each bit with its adjacent bit to the right (high-bits).

Signatures:

```cpp
template <integer_like int_t> int_t pp_xor(int_t value)
```

Example:

```cpp
SimdLib::Bmi::pp_xor(0U); // => 0U
```

<a id="ps-and"></a>
## `ps_and`

Computes a distance-1 parallel-suffix AND stage by ANDing each bit with its adjacent bit to the left (low-bits).

Signatures:

```cpp
template <integer_like int_t> int_t ps_and(int_t value)
```

Example:

```cpp
SimdLib::Bmi::ps_and(0U); // => 0U
```

<a id="ps-andn"></a>
## `ps_andn`

Computes a distance-1 parallel-suffix AND-NOT stage, retaining set bits whose adjacent bit to the left (low-bits) is clear.

Signatures:

```cpp
template <integer_like int_t> int_t ps_andn(int_t value)
```

Example:

```cpp
SimdLib::Bmi::ps_andn(0U); // => 0U
```

<a id="ps-andni"></a>
## `ps_andni`

Computes an inverse distance-1 parallel-suffix AND-NOT stage, marking clear bits whose adjacent bit to the left (low-bits) is set.

Signatures:

```cpp
template <integer_like int_t> int_t ps_andni(int_t value)
```

Example:

```cpp
SimdLib::Bmi::ps_andni(0U); // => 0U
```

<a id="ps-or"></a>
## `ps_or`

Computes the parallel-suffix OR of the given value, which is the result of or'ing each bit with all bits to the right (high-bits).

Signatures:

```cpp
template <integer_like int_t> int_t ps_or(int_t value)
```

Example:

```cpp
SimdLib::Bmi::ps_or<std::uint8_t>(0b0001'0100U); // => 0b1111'1100U
```

<a id="ps-xor"></a>
## `ps_xor`

Computes a distance-1 parallel-suffix XOR stage by XORing each bit with its adjacent bit to the left (low-bits).

Signatures:

```cpp
template <integer_like int_t> int_t ps_xor(int_t value)
```

Example:

```cpp
SimdLib::Bmi::ps_xor(0U); // => 0U
```

<a id="select"></a>
## `select`

Branchless selection between two values based on a switch bit.

Signatures:

```cpp
template <integer_like int_t> int_t select(int_t lhs, int_t rhs, bool selectionBit)
```

Example:

```cpp
SimdLib::Bmi::select(4U, 9U, true); // => 9U
```

<a id="related-types-and-constants"></a>
## Related types and constants

`Bmi::integer_like<T>` accepts numeric-limits-aware integer types other than `bool`. It permits the generic bit helpers to support both standard integers and compatible extended integer types.
