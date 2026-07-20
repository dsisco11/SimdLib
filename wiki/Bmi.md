# Bmi

`SimdLib::Bmi` contains constexpr-friendly bit-manipulation helpers. The generic forms work across integral widths, while optimized target paths are selected when available.

## Contents

- [Overview](#overview)
- [Example setup](#example-setup)
- [`_pdep_emulator`](#pdep-emulator)
- [`_pext_emulator`](#pext-emulator)
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
## Example setup

```cpp
#include <SimdLib/Bmi.h>
#include <cstdint>

std::uint32_t value = 0b1011'0100U;
std::uint32_t other = 0b0011'1110U;
```

<a id="pdep-emulator"></a>
## `_pdep_emulator`

Performs a software-emulated parallel bit deposit for the width of int_t. [eg: _pdep_emulator(0b101, 0b01010100) => 0b01000100]

Signatures:

```cpp
template <std::integral int_t> SIMDLIB_FORCE_INLINE constexpr static int_t _pdep_emulator(int_t source, int_t mask) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::_pdep_emulator(value, other);
```

<a id="pext-emulator"></a>
## `_pext_emulator`

Performs a constexpr software emulation of parallel bit extraction.

Signatures:

```cpp
template <std::integral int_t> static constexpr int_t _pext_emulator(int_t source, int_t mask) noexcept;
```

Example:

```cpp
const auto result = SimdLib::Bmi::_pext_emulator(value, other);
```

<a id="abs"></a>
## `abs`

Branchless find absolute value of the input.

Signatures:

```cpp
template <std::integral int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t abs(const int_t lhs) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::abs(value);
```

<a id="andn"></a>
## `andn`

Compute the bitwise NOT of LHS and then AND with RHS.

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t andn(const int_t lhs, const int_t rhs) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::andn(value, other);
```

<a id="bextr"></a>
## `bextr`

Extract contiguous bits from source integer, and return them shifted to the LSB side of the output. Extract the number of bits specified by len, starting at the bit specified by start.

Signatures:

```cpp
template <std::integral int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr int_t bextr(const int_t source, const std::uint8_t len, const std::uint8_t start) noexcept
template <std::integral int_t, std::size_t len> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr int_t bextr(const int_t source, const std::uint8_t start) noexcept
template <std::integral int_t, std::size_t start, std::size_t len> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr int_t bextr(const int_t source) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::bextr(value, other, 4);
```

<a id="blse"></a>
## `blse`

Extract and reset the lowest set bit in source.

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t blse(const int_t source, int_t &out_lsb) noexcept
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static std::tuple<int_t, int_t> blse(const int_t source) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::blse(value, other);
```

<a id="blsi"></a>
## `blsi`

Extract the lowest set bit from source integer and set the corresponding bit in dst. All other bits in dst are zeroed, and all bits are zeroed if no bits are set in source.

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t blsi(const int_t source) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::blsi(value);
```

<a id="blsioff"></a>
## `blsioff`

Extracts the first source bit at or above an inclusive one-hot boundary. [eg: blsioff(0b10100, 0b01000) => 0b10000]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t blsioff(const int_t source, const int_t starting_bit) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::blsioff(value, other);
```

<a id="blsmsk"></a>
## `blsmsk`

Set all the lower bits of dst up to and including the lowest set bit in source.

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t blsmsk(const int_t source) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::blsmsk(value);
```

<a id="blsr"></a>
## `blsr`

Copy all bits from source to dst, and reset (set to 0) the bit in dst that corresponds to the lowest set bit in source.

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t blsr(const int_t source) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::blsr(value);
```

<a id="bmse"></a>
## `bmse`

Extract and reset the highest set bit in source.

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t bmse(const int_t value, int_t &out_msb) noexcept
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static std::tuple<int_t, int_t> bmse(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::bmse(value, other);
```

<a id="bmsi"></a>
## `bmsi`

Extract the highest set bit from source integer and set the corresponding bit in dst. All other bits in dst are zeroed, and all bits are zeroed if no bits are set in source.

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t bmsi(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::bmsi(value);
```

<a id="bmsmsk"></a>
## `bmsmsk`

Set all the lower bits of dst up to and including the highest set bit in source.

Signatures:

```cpp
template <std::integral int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t bmsmsk(const int_t source) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::bmsmsk(value);
```

<a id="bmsr"></a>
## `bmsr`

Copy all bits from source to dst, and reset (set to 0) the bit in dst that corresponds to the highest set bit in source.

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t bmsr(const int_t value) noexcept
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t bmsr(const int_t value, int &out_msb_index) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::bmsr(value);
```

<a id="boolmask"></a>
## `boolmask`

Turns a boolean value into an integer-width bitmask of all ones or zeros (0 for false, all 1s for true).

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t boolmask(const bool state) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::boolmask(value);
```

<a id="bzhi"></a>
## `bzhi`

Copy all bits from source integer, and reset (set to 0) the high bits in output starting at index.

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t bzhi(const int_t source, unsigned index) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::bzhi(value, other);
```

<a id="bzlo"></a>
## `bzlo`

Clears every source bit whose bit index is less than index. [eg: bzlo(0b11111, 3) => 0b11000]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t bzlo(const int_t source, unsigned index) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::bzlo(value, other);
```

<a id="clear-bits-higher-than"></a>
## `clear_bits_higher_than`

Clears all bits higher than (not including) the given target-bit from the source integer. [eg: (10111, 100) => 00111]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t clear_bits_higher_than(const int_t value, const int_t target_bit) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::clear_bits_higher_than(value, other);
```

<a id="clear-bits-lower-than"></a>
## `clear_bits_lower_than`

Clears all bits lower than (not including) the given target-bit from the source integer. [eg: (10111, 100) => 10100]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t clear_bits_lower_than(const int_t value, const int_t target_bit) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::clear_bits_lower_than(value, other);
```

<a id="clear-leading-ones"></a>
## `clear_leading_ones`

Clears all most significant, rightmost (high-bits) leading set bits. [eg: 110101 => 000101]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t clear_leading_ones(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::clear_leading_ones(value);
```

<a id="clear-lowest-set-bits"></a>
## `clear_lowest_set_bits`

Copy all bits from the source integer, and reset (set to 0) the leftmost (low-bits) string of contiguous set bits. [eg: 1011 => 1000]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t clear_lowest_set_bits(const int_t value) noexcept
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t clear_lowest_set_bits(const int_t value, int_t &out_consumed) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::clear_lowest_set_bits(value);
```

<a id="clear-trailing-ones"></a>
## `clear_trailing_ones`

Clears all least significant, leftmost (low-bits) trailing set bits. [eg: 1011 => 1000]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t clear_trailing_ones(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::clear_trailing_ones(value);
```

<a id="consume-bit-sequence-left"></a>
## `consume_bit_sequence_left`

Extracts and returns the rightmost (high-bits) string of contiguous set bits, said bits are also reset (set to 0) within the source integer. [eg: 0110111 => 0110000]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::tuple<int_t, int_t> consume_bit_sequence_left(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::consume_bit_sequence_left(value);
```

<a id="consume-bit-sequence-right"></a>
## `consume_bit_sequence_right`

Extracts and returns the leftmost (low-bits) string of contiguous set bits, said bits are also reset (set to 0) within the source integer. [eg: 1011 => 0011]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::tuple<int_t, int_t> consume_bit_sequence_right(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::consume_bit_sequence_right(value);
```

<a id="extract-bits-higher-than"></a>
## `extract_bits_higher_than`

Extracts all bits higher than (not including) the given target-bit from the source integer. [eg: (10111, 001) => 10110]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t extract_bits_higher_than(const int_t value, const int_t target_bit) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::extract_bits_higher_than(value, other);
```

<a id="extract-bits-lower-than"></a>
## `extract_bits_lower_than`

Extracts all bits lower than (not including) the given target-bit from the source integer. [eg: (10111, 100) => 00011]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t extract_bits_lower_than(const int_t value, const int_t target_bit) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::extract_bits_lower_than(value, other);
```

<a id="flip-trailing-zeros"></a>
## `flip_trailing_zeros`

Sets all least significant, leftmost (low-bits) trailing unset bits. [eg: 10100 => 10111]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t flip_trailing_zeros(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::flip_trailing_zeros(value);
```

<a id="flipr-unset"></a>
## `flipr_unset`

Sets the least significant, leftmost (low-bits) unset bit. [eg: 01011 => 01111]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t flipr_unset(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::flipr_unset(value);
```

<a id="left-collapse-trailing-bits"></a>
## `left_collapse_trailing_bits`

Copy all bits from the source integer, and reset (set to 0) the trailing bits up-to but excluding the rightmost (high-bits) trailing set bit. [eg: 10111 => 10100]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t left_collapse_trailing_bits(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::left_collapse_trailing_bits(value);
```

<a id="mask-bits-lower-than-lsb"></a>
## `mask_bits_lower_than_lsb`

Returns a mask of all bits strictly lower than the least-significant set bit (LSB). For value==0, returns 0. [eg: 101000 => 000111]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mask_bits_lower_than_lsb(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::mask_bits_lower_than_lsb(value);
```

<a id="mask-bits-lower-than-lsb-or-all-ones"></a>
## `mask_bits_lower_than_lsb_or_all_ones`

Returns a mask of all bits strictly lower than the least-significant set bit (LSB). For value==0, returns all-ones (useful as a "no constraint" mask). [eg: 101000 => 000111]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mask_bits_lower_than_lsb_or_all_ones(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::mask_bits_lower_than_lsb_or_all_ones(value);
```

<a id="mask-leading-ones"></a>
## `mask_leading_ones`

Returns a mask over the leading ones in the source integer. [eg: 111011 => 111000]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mask_leading_ones(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::mask_leading_ones(value);
```

<a id="mask-leading-zeros"></a>
## `mask_leading_zeros`

Returns a mask over the leading zeros in the source integer. [eg: 000101 => 111000]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mask_leading_zeros(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::mask_leading_zeros(value);
```

<a id="mask-trailing-ones"></a>
## `mask_trailing_ones`

Returns a mask over the trailing 1-bits in the source integer, producing 0 if none. [eg: 10111 => 00111]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mask_trailing_ones(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::mask_trailing_ones(value);
```

<a id="mask-trailing-zeros"></a>
## `mask_trailing_zeros`

Returns a mask over the trailing 0-bits in the source integer. [eg: 10100 => 011]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mask_trailing_zeros(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::mask_trailing_zeros(value);
```

<a id="mask-trailing-zeros-or-zero"></a>
## `mask_trailing_zeros_or_zero`

Returns a mask over the trailing 0-bits in the source integer. For value==0, returns 0 ("safe" variant; avoids the wraparound/all-ones behavior). [eg: 10100 => 00011]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mask_trailing_zeros_or_zero(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::mask_trailing_zeros_or_zero(value);
```

<a id="maskl-trailing-one"></a>
## `maskl_trailing_one`

Returns a single 1-bit at the position of the rightmost (high-bits) trailing 1-bit, producing 0 if none. [eg: 010111 => 00100]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t maskl_trailing_one(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::maskl_trailing_one(value);
```

<a id="maskr-unset"></a>
## `maskr_unset`

Returns a single 1-bit at the position of the leftmost (low-bits) 0-bit, producing 0 if none. [eg: 01011 => 00100]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t maskr_unset(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::maskr_unset(value);
```

<a id="max"></a>
## `max`

Branchless find maximum of two values.

Signatures:

```cpp
template <std::integral int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t max(const int_t lhs, const int_t rhs) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::max(value, other);
```

<a id="min"></a>
## `min`

Branchless find minimum of two values.

Signatures:

```cpp
template <std::integral int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t min(const int_t lhs, const int_t rhs) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::min(value, other);
```

<a id="mulx"></a>
## `mulx`

Multiplies the unsigned object-representation values of two integers. [eg: mulx(0xFF, 0x02, hi) => 0xFE, hi = 0x01]

Signatures:

```cpp
template <std::integral int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t mulx(const int_t lhs, const int_t rhs, int_t &hi) noexcept requires(!std::same_as<std::remove_cv_t<int_t>, bool>)
```

Example:

```cpp
const auto result = SimdLib::Bmi::mulx(value, other, 4);
```

<a id="partialsumblsi"></a>
## `PartialSumBLSI`

Builds the partial sums used by the parallel-suffix isolated-bit helpers.

Signatures:

```cpp
template <integer_like int_t> static constexpr int_t PartialSumBLSI(int_t value) noexcept;
```

Example:

```cpp
const auto result = SimdLib::Bmi::PartialSumBLSI(value);
```

<a id="partialsumblsmsk"></a>
## `PartialSumBLSMSK`

Builds the partial sums used by the parallel-prefix bit-mask helpers.

Signatures:

```cpp
template <integer_like int_t> static constexpr int_t PartialSumBLSMSK(int_t value) noexcept;
```

Example:

```cpp
const auto result = SimdLib::Bmi::PartialSumBLSMSK(value);
```

<a id="pdep-u32"></a>
## `pdep_u32`

Note: This is a wrapper for the '_pdep_xxx' intrinsic providing compile-time emulation.

Signatures:

```cpp
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::uint32_t pdep_u32(std::uint32_t source, std::uint32_t mask) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::pdep_u32(value, other);
```

<a id="pdep-u64"></a>
## `pdep_u64`

Note: This is a wrapper for the '_pdep_xxx' intrinsic providing compile-time emulation.

Signatures:

```cpp
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::uint64_t pdep_u64(std::uint64_t source, std::uint64_t mask) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::pdep_u64(value, other);
```

<a id="pdepl-u32"></a>
## `pdepl_u32`

This is a "pdep, but from right (high-bits) to left (low-bits)" aka "expand left"

Signatures:

```cpp
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::uint32_t pdepl_u32(std::uint32_t source, std::uint32_t mask) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::pdepl_u32(value, other);
```

<a id="pdepl-u64"></a>
## `pdepl_u64`

This is a "pdep, but from right (high-bits) to left (low-bits)" aka "expand left"

Signatures:

```cpp
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::uint64_t pdepl_u64(std::uint64_t source, std::uint64_t mask) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::pdepl_u64(value, other);
```

<a id="pext-u32"></a>
## `pext_u32`

Note: This is a wrapper for the '_pext_xxx' intrinsic, providing compile-time emulation.

Signatures:

```cpp
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::uint32_t pext_u32(std::uint32_t source, std::uint32_t mask) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::pext_u32(value, other);
```

<a id="pext-u64"></a>
## `pext_u64`

Note: This is a wrapper for the '_pext_xxx' intrinsic, providing compile-time emulation.

Signatures:

```cpp
[[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static std::uint64_t pext_u64(std::uint64_t source, std::uint64_t mask) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::pext_u64(value, other);
```

<a id="pp-and"></a>
## `pp_and`

Computes a distance-1 parallel-prefix AND stage by ANDing each bit with its adjacent bit to the right (high-bits). [eg: pp_and(0b01101110) => 0b00100110]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t pp_and(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::pp_and(value);
```

<a id="pp-andn"></a>
## `pp_andn`

Computes a distance-1 parallel-prefix AND-NOT stage, retaining set bits whose adjacent bit to the right (high-bits) is clear. [eg: pp_andn(0b01110) => 0b01000]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t pp_andn(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::pp_andn(value);
```

<a id="pp-andni"></a>
## `pp_andni`

Computes an inverse distance-1 parallel-prefix AND-NOT stage, marking clear bits whose adjacent bit to the right (high-bits) is set. [eg: pp_andni(0b01110) => 0b00001]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t pp_andni(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::pp_andni(value);
```

<a id="pp-lsor"></a>
## `pp_lsor`

Computes the parallel-prefix-least-significant-OR of the given value, which is the result of clearing all bits to the right (high-bits) of the lsb and then or'ing each bit with all bits to the left (low-bits). [eg: 10100 => 00111 ]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t pp_lsor(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::pp_lsor(value);
```

<a id="pp-or"></a>
## `pp_or`

Computes the parallel-prefix OR of the given value, which is the result of or'ing each bit with all bits to the left (low-bits). [eg: 10100 => 11111 ]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t pp_or(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::pp_or(value);
```

<a id="pp-xor"></a>
## `pp_xor`

Computes a distance-1 parallel-prefix XOR stage by XORing each bit with its adjacent bit to the right (high-bits). [eg: pp_xor(0b01110) => 0b01001]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t pp_xor(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::pp_xor(value);
```

<a id="ps-and"></a>
## `ps_and`

Computes a distance-1 parallel-suffix AND stage by ANDing each bit with its adjacent bit to the left (low-bits). [eg: ps_and(0b01101110) => 0b01001100]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t ps_and(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::ps_and(value);
```

<a id="ps-andn"></a>
## `ps_andn`

Computes a distance-1 parallel-suffix AND-NOT stage, retaining set bits whose adjacent bit to the left (low-bits) is clear. [eg: ps_andn(0b01110) => 0b00010]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t ps_andn(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::ps_andn(value);
```

<a id="ps-andni"></a>
## `ps_andni`

Computes an inverse distance-1 parallel-suffix AND-NOT stage, marking clear bits whose adjacent bit to the left (low-bits) is set. [eg: ps_andni(0b01110) => 0b10000]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t ps_andni(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::ps_andni(value);
```

<a id="ps-or"></a>
## `ps_or`

Computes the parallel-suffix OR of the given value, which is the result of or'ing each bit with all bits to the right (high-bits). [eg: 010100 => 1...100 ]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t ps_or(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::ps_or(value);
```

<a id="ps-xor"></a>
## `ps_xor`

Computes a distance-1 parallel-suffix XOR stage by XORing each bit with its adjacent bit to the left (low-bits). [eg: ps_xor(0b01110) => 0b10010]

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] SIMDLIB_FORCE_INLINE constexpr static int_t ps_xor(const int_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::ps_xor(value);
```

<a id="select"></a>
## `select`

Branchless selection between two values based on a switch bit.

Signatures:

```cpp
template <integer_like int_t> [[nodiscard]] [[msvc::flatten]] SIMDLIB_FORCE_INLINE constexpr static int_t select(const int_t lhs, const int_t rhs, const bool selectionBit) noexcept
```

Example:

```cpp
const auto result = SimdLib::Bmi::select(value, other, 4);
```

<a id="related-types-and-constants"></a>
## Related types and constants

`Bmi::integer_like<T>` accepts numeric-limits-aware integer types other than `bool`. It permits the generic bit helpers to support both standard integers and compatible extended integer types.
