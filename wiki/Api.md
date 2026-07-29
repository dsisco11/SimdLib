# Api

`Api<register_width, element_t>` is the low-level SIMD facade. Most applications should spell it through [`NativeApi`](NativeApi.md), which chooses the widest supported register width.

## Contents

- [Overview](#overview)
- [Example alias](#example-setup)
- [`absolute`](#absolute)
- [`add`](#add)
- [`add_horizontal`](#add-horizontal)
- [`add_saturated`](#add-saturated)
- [`add_subtract`](#add-subtract)
- [`avg`](#avg)
- [`bit_shift_left` and `bit_shift_left_slow`](#bit-shift-left)
- [`bit_shift_right` and `bit_shift_right_slow`](#bit-shift-right)
- [`bitwise_and`](#bitwise-and)
- [`bitwise_andnot`](#bitwise-andnot)
- [`bitwise_not`](#bitwise-not)
- [`bitwise_or`](#bitwise-or)
- [`bitwise_xor`](#bitwise-xor)
- [`blend`](#blend)
- [`byte_shift_left_slow`](#byte-shift-left-slow)
- [`byte_shift_right_slow`](#byte-shift-right-slow)
- [`cmp_eq`](#cmp-eq)
- [`cmp_eq_mask`](#cmp-eq-mask)
- [`cmp_ge`](#cmp-ge)
- [`cmp_gt`](#cmp-gt)
- [`cmp_le`](#cmp-le)
- [`cmp_lt`](#cmp-lt)
- [`compress`](#compress)
- [`construct`](#construct)
- [`convert`](#convert)
- [`convert_to_float`](#convert-to-float)
- [`convert_to_int`](#convert-to-int)
- [`divide`](#divide)
- [`dot_product`](#dot-product)
- [`expand`](#expand)
- [`extract` and `extract_slow`](#extract)
- [`hadd_saturated`](#hadd-saturated)
- [`hsubtract_saturated`](#hsubtract-saturated)
- [`insert` and `insert_slow`](#insert)
- [`load`](#load)
- [`load_aligned`](#load-aligned)
- [`load_partial`](#load-partial)
- [`load_unaligned`](#load-unaligned)
- [`load_unsafe`](#load-unsafe)
- [`lower_half`](#lower-half)
- [`magnitude`](#magnitude)
- [`max`](#max)
- [`max_position`](#max-position)
- [`min`](#min)
- [`min_position`](#min-position)
- [`modulus`](#modulus)
- [`movemask`](#movemask)
- [`movemask_slim`](#movemask-slim)
- [`multi_sum_absolute_byte_differences`](#multi-sum-absolute-byte-differences)
- [`multiply`](#multiply)
- [`multiply_add`](#multiply-add)
- [`multiply_add_adjacent`](#multiply-add-adjacent)
- [`multiply_add_unsigned_signed_bytes`](#multiply-add-unsigned-signed-bytes)
- [`negate`](#negate)
- [`normalize`](#normalize)
- [`set`](#set)
- [`set_partial`](#set-partial)
- [`set1`](#set1)
- [`setr`](#setr)
- [`setr_partial`](#setr-partial)
- [`setzero`](#setzero)
- [`shift_left`](#shift-left)
- [`shift_right`](#shift-right)
- [`shift_right_arithmetic`](#shift-right-arithmetic)
- [`shuffle` and `shuffle_slow`](#shuffle)
- [`shuffle_32` and `shuffle_32_slow`](#shuffle-32)
- [`shuffle_hi` and `shuffle_hi_slow`](#shuffle-hi)
- [`shuffle_lo` and `shuffle_lo_slow`](#shuffle-lo)
- [`sqrt`](#sqrt)
- [`store`](#store)
- [`store_aligned`](#store-aligned)
- [`store_unaligned`](#store-unaligned)
- [`subtract`](#subtract)
- [`subtract_horizontal`](#subtract-horizontal)
- [`subtract_saturated`](#subtract-saturated)
- [`sum_absolute_byte_differences`](#sum-absolute-byte-differences)
- [`to_array`](#to-array)
- [`transform`](#transform)
- [`transform_pack`](#transform-pack)
- [`unpack_hi`](#unpack-hi)
- [`unpack_lo`](#unpack-lo)
- [`widen`](#widen)
- [Related types and constants](#related-types-and-constants)

<a id="overview"></a>
## Overview

Include `<SimdLib/Api.h>`. Overloads with the same name are collected in one subsection; every public overload is listed below.

<a id="example-setup"></a>
## Example alias

```cpp
#include <SimdLib/SimdLib.h>

using ApiT = SimdLib::Api<128, float>;
```
<a id="absolute"></a>
## `absolute`

Computes the absolute value of each element in the register.

Signatures:

```cpp
static vector_t absolute(vector_t lhs)
```

Example:

```cpp
ApiT::absolute(
    ApiT::construct({-2.0F, 3.0F, 0.0F, 0.0F})); // => {2.0F, 3.0F, 0.0F, ...}
```

<a id="add"></a>
## `add`

Adds corresponding lanes.

Signatures:

```cpp
static vector_t add(vector_t lhs, vector_t rhs)
```

Example:

```cpp
ApiT::add(
    ApiT::construct({2.0F, 2.0F, 2.0F, 2.0F}),
    ApiT::construct({3.0F, 3.0F, 3.0F, 3.0F})); // => every lane is 5.0F
```

<a id="add-horizontal"></a>
## `add_horizontal`

Adds adjacent element pairs within each 128-bit lane of two registers.

Signatures:

```cpp
static vector_t add_horizontal(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using F32x4 = SimdLib::Api<128, float>;
F32x4::add_horizontal(
    F32x4::construct({1.0F, 2.0F, 3.0F, 4.0F}),
    F32x4::construct({5.0F, 6.0F, 7.0F, 8.0F})); // => {3.0F, 7.0F, 11.0F, 15.0F}
```

<a id="add-saturated"></a>
## `add_saturated`

Adds corresponding lanes with saturation where the specialization supports it.

Signatures:

```cpp
static auto add_saturated(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using U8 = SimdLib::Api<128, std::uint8_t>;
U8::add_saturated(U8::set1(250), U8::set1(10)); // => every lane is 255
```

<a id="add-subtract"></a>
## `add_subtract`

Alternates subtraction and addition across lanes for floating-point SIMD families.

Signatures:

```cpp
static auto add_subtract(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using F32x4 = SimdLib::Api<128, float>;
F32x4::add_subtract(
    F32x4::construct({10.0F, 10.0F, 10.0F, 10.0F}),
    F32x4::construct({1.0F, 2.0F, 3.0F, 4.0F})); // => {9.0F, 12.0F, 7.0F, 14.0F}
```

<a id="avg"></a>
## `avg`

Computes the average of corresponding lanes where the specialization supports it.

Signatures:

```cpp
static auto avg(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using U8 = SimdLib::Api<128, std::uint8_t>;
U8::avg(U8::set1(2U), U8::set1(6U)); // => every lane is 4U
```

<a id="bit-shift-left"></a>
## `bit_shift_left` and `bit_shift_left_slow`

Shifts the complete 128-bit register left as one unsigned bit string, carrying across element boundaries. The unsuffixed template form encodes a compile-time count. The `_slow` form accepts a runtime count; nonpositive counts return the input and counts of 128 or more return zero.

Signatures:

```cpp
template <int shift> static int_vector_t bit_shift_left(int_vector_t lhs)
static int_vector_t bit_shift_left_slow(int_vector_t lhs, int shift)
```

Examples:

```cpp
using U32x4 = SimdLib::Api<128, std::uint32_t>;
const auto value = U32x4::construct({3U, 3U, 3U, 3U});
U32x4::bit_shift_left<1>(value);       // => {6U, 6U, 6U, 6U}
U32x4::bit_shift_left_slow(value, 1); // same semantics with a runtime count
```

<a id="bit-shift-right"></a>
## `bit_shift_right` and `bit_shift_right_slow`

Shifts the complete 128-bit register right as one unsigned bit string, carrying across element boundaries. The unsuffixed template form encodes a compile-time count. The `_slow` form accepts a runtime count; nonpositive counts return the input and counts of 128 or more return zero.

Signatures:

```cpp
template <int shift> static int_vector_t bit_shift_right(int_vector_t lhs)
static int_vector_t bit_shift_right_slow(int_vector_t lhs, int shift)
```

Examples:

```cpp
using U32x4 = SimdLib::Api<128, std::uint32_t>;
const auto value = U32x4::construct({8U, 8U, 8U, 8U});
U32x4::bit_shift_right<1>(value);       // => {4U, 4U, 4U, 4U}
U32x4::bit_shift_right_slow(value, 1); // same semantics with a runtime count
```

<a id="bitwise-and"></a>
## `bitwise_and`

Computes a bitwise AND of two registers.

Signatures:

```cpp
static auto bitwise_and(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using U32 = SimdLib::Api<128, std::uint32_t>;
U32::bitwise_and(
    U32::construct({0b1100U, 0b1100U, 0b1100U, 0b1100U}),
    U32::construct({0b1010U, 0b1010U, 0b1010U, 0b1010U})); // => every lane is 0b1000U
```

<a id="bitwise-andnot"></a>
## `bitwise_andnot`

Computes a bitwise AND-NOT of two registers.

Signatures:

```cpp
static auto bitwise_andnot(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using U32 = SimdLib::Api<128, std::uint32_t>;
U32::bitwise_andnot(
    U32::construct({0b1100U, 0b1100U, 0b1100U, 0b1100U}),
    U32::construct({0b1010U, 0b1010U, 0b1010U, 0b1010U})); // => every lane is 0b0010U
```

<a id="bitwise-not"></a>
## `bitwise_not`

Computes a bitwise NOT of a register.

Signatures:

```cpp
static auto bitwise_not(vector_t lhs)
```

Example:

```cpp
using U32 = SimdLib::Api<128, std::uint32_t>;
U32::bitwise_not(U32::construct({0U, 0U, 0U, 0U})); // => every lane is 0xFFFFFFFFU
```

<a id="bitwise-or"></a>
## `bitwise_or`

Computes a bitwise OR of two registers.

Signatures:

```cpp
static auto bitwise_or(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using U32 = SimdLib::Api<128, std::uint32_t>;
U32::bitwise_or(
    U32::construct({0b1100U, 0b1100U, 0b1100U, 0b1100U}),
    U32::construct({0b1010U, 0b1010U, 0b1010U, 0b1010U})); // => every lane is 0b1110U
```

<a id="bitwise-xor"></a>
## `bitwise_xor`

Computes a bitwise XOR of two registers.

Signatures:

```cpp
static auto bitwise_xor(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using U32 = SimdLib::Api<128, std::uint32_t>;
U32::bitwise_xor(
    U32::construct({0b1100U, 0b1100U, 0b1100U, 0b1100U}),
    U32::construct({0b1010U, 0b1010U, 0b1010U, 0b1010U})); // => every lane is 0b0110U
```

<a id="blend"></a>
## `blend` and `blend_slow`

Selects corresponding lanes from two registers. `blend<imm8>` uses a compile-time immediate. An unsuffixed register-mask overload remains available where the instruction set provides a native runtime mask. `blend_slow` emulates immediate-mask semantics for a runtime scalar control.

Signatures:

```cpp
template <int imm8> static vector_t blend(vector_t lhs, vector_t rhs)
template <class... Args> static auto blend(Args &&...args)
template <class... Args> static auto blend_slow(Args &&...args)
```

Examples:

```cpp
using I32x4 = SimdLib::Api<128, std::int32_t>;
const auto lhs = I32x4::construct({10, 20, 30, 40});
const auto rhs = I32x4::construct({1, 2, 3, 4});
I32x4::blend<0b0101>(lhs, rhs);       // => {1, 20, 3, 40}
I32x4::blend_slow(lhs, rhs, 0b0101); // same semantics with a runtime control
```

<a id="byte-shift-left-slow"></a>
## `byte_shift_left_slow`

Shifts every byte in a 128-bit register toward higher byte indices. The `_slow` suffix identifies the runtime substitute for an immediate-controlled whole-register shift.

Signature:

```cpp
static int_vector_t byte_shift_left_slow(int_vector_t lhs, int shift)
```

Example:

```cpp
using U8x16 = SimdLib::Api<128, std::uint8_t>;
U8x16::byte_shift_left_slow(U8x16::set1(7U), 1); // => {0U, 7U, 7U, ..., 7U}
```

<a id="byte-shift-right-slow"></a>
## `byte_shift_right_slow`

Shifts every byte in a 128-bit register toward lower byte indices. The `_slow` suffix identifies the runtime substitute for an immediate-controlled whole-register shift.

Signature:

```cpp
static int_vector_t byte_shift_right_slow(int_vector_t lhs, int shift)
```

Example:

```cpp
using U8x16 = SimdLib::Api<128, std::uint8_t>;
U8x16::byte_shift_right_slow(U8x16::set1(7U), 1); // => {7U, 7U, ..., 7U, 0U}
```

<a id="cmp-eq"></a>
## `cmp_eq`

Computes an equality comparison mask for two registers.

Signatures:

```cpp
static mask_t cmp_eq(vector_t lhs, vector_t rhs)
```

Example:

```cpp
ApiT::cmp_eq(
    ApiT::construct({2.0F, 2.0F, 2.0F, 2.0F}),
    ApiT::construct(
        {2.0F, 2.0F, 2.0F, 2.0F})); // => every comparison lane has all bits set
```

<a id="cmp-eq-mask"></a>
## `cmp_eq_mask`

Computes a byte-granular equality comparison mask for two registers of this SIMD shape.

Signatures:

```cpp
static mask_t cmp_eq_mask(vector_t lhs, vector_t rhs)
```

Example:

```cpp
ApiT::cmp_eq_mask(
    ApiT::construct({2.0F, 2.0F, 2.0F, 2.0F}),
    ApiT::construct({2.0F, 2.0F, 2.0F, 2.0F})); // => one set mask bit for every lane
```

<a id="cmp-ge"></a>
## `cmp_ge`

Computes a greater-than-or-equal comparison mask for two registers.

Signatures:

```cpp
static mask_t cmp_ge(vector_t lhs, vector_t rhs)
```

Example:

```cpp
ApiT::cmp_ge(
    ApiT::construct({2.0F, 2.0F, 2.0F, 2.0F}),
    ApiT::construct(
        {2.0F, 2.0F, 2.0F, 2.0F})); // => every comparison lane has all bits set
```

<a id="cmp-gt"></a>
## `cmp_gt`

Computes a greater-than comparison mask for two registers.

Signatures:

```cpp
static mask_t cmp_gt(vector_t lhs, vector_t rhs)
```

Example:

```cpp
ApiT::cmp_gt(
    ApiT::construct({3.0F, 3.0F, 3.0F, 3.0F}),
    ApiT::construct(
        {2.0F, 2.0F, 2.0F, 2.0F})); // => every comparison lane has all bits set
```

<a id="cmp-le"></a>
## `cmp_le`

Computes a less-than-or-equal comparison mask for two registers.

Signatures:

```cpp
static mask_t cmp_le(vector_t lhs, vector_t rhs)
```

Example:

```cpp
ApiT::cmp_le(
    ApiT::construct({2.0F, 2.0F, 2.0F, 2.0F}),
    ApiT::construct(
        {2.0F, 2.0F, 2.0F, 2.0F})); // => every comparison lane has all bits set
```

<a id="cmp-lt"></a>
## `cmp_lt`

Computes a less-than comparison mask for two registers.

Signatures:

```cpp
static mask_t cmp_lt(vector_t lhs, vector_t rhs)
```

Example:

```cpp
ApiT::cmp_lt(
    ApiT::construct({2.0F, 2.0F, 2.0F, 2.0F}),
    ApiT::construct(
        {3.0F, 3.0F, 3.0F, 3.0F})); // => every comparison lane has all bits set
```

<a id="compress"></a>
## `compress`

Compresses two registers into a narrower-lane register where the specialization supports it.

Signatures:

```cpp
static auto compress(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using I16x8 = SimdLib::Api<128, std::int16_t>;
I16x8::compress(
    I16x8::set1(300),
    I16x8::set1(-300)); // => eight 127 lanes followed by eight -128 lanes
```

<a id="construct"></a>
## `construct`

Constructs a SIMD register from a fixed array.

Signatures:

```cpp
static vector_t construct(const std::array<element_t, element_count> &data)
```

Example:

```cpp
ApiT::construct({1.0F, 2.0F, 0.0F, 0.0F}); // => {1.0F, 2.0F, 0.0F, 0.0F}
```

<a id="convert"></a>
## `convert`

Converts between 32-bit integer and floating-point register representations.

Signatures:

```cpp
static auto convert(vector_t vector)
```

Example:

```cpp
ApiT::convert(ApiT::construct({3.6F, 3.6F, 3.6F, 3.6F})); // => every integer lane is 4
```

<a id="convert-to-float"></a>
## `convert_to_float`

Converts 32-bit integer lanes into floating-point lanes.

Signatures:

```cpp
static float_vector_t convert_to_float(int_vector_t vector)
```

Example:

```cpp
using I32 = SimdLib::Api<128, std::int32_t>;
I32::convert_to_float(
    I32::construct(
        {16777217, 16777217, 16777217,
         16777217})); // => every floating-point lane is 16777216.0F
```

<a id="convert-to-int"></a>
## `convert_to_int`

Converts 32-bit floating-point lanes into integer lanes.

Signatures:

```cpp
static int_vector_t convert_to_int(float_vector_t vector)
```

Example:

```cpp
ApiT::convert_to_int(
    ApiT::construct({3.6F, 3.6F, 3.6F, 3.6F})); // => every integer lane is 4
```

<a id="divide"></a>
## `divide`

Divides corresponding lanes.

Signatures:

```cpp
static vector_t divide(vector_t lhs, vector_t rhs)
```

Example:

```cpp
ApiT::divide(
    ApiT::construct({8.0F, 8.0F, 8.0F, 8.0F}),
    ApiT::construct({2.0F, 2.0F, 2.0F, 2.0F})); // => every lane is 4.0F
```

<a id="dot-product"></a>
## `dot_product`

Computes a dot product using a compile-time immediate mask where the specialization supports it.

Signatures:

```cpp
template <int imm8> static auto dot_product(vector_t lhs, vector_t rhs)
```

Example:

```cpp
ApiT::dot_product<0xFF>(
    ApiT::construct({1.0F, 2.0F, 0.0F, 0.0F}),
    ApiT::construct({3.0F, 4.0F, 0.0F, 0.0F})); // => selected lanes contain 11.0F
```

<a id="expand"></a>
## `expand`

Reserved expansion entry point. No current backend provides an end-user-callable overload.

Signatures:

```cpp
static auto expand(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using I8x16 = SimdLib::Api<128, std::int8_t>;
// I8x16::expand(...) // => no output; this reserved entry point has no callable backend
```

<a id="extract"></a>
## `extract` and `extract_slow`

Extracts one logical lane. The unsuffixed template form uses a compile-time lane index. `extract_slow` accepts a runtime-selected lane index.

Signatures:

```cpp
template <int index> static auto extract(vector_t lhs)
template <class selector_t> static auto extract_slow(vector_t lhs, selector_t rhs)
```

Examples:

```cpp
using I32x4 = SimdLib::Api<128, std::int32_t>;
const auto value = I32x4::construct({7, 8, 9, 10});
I32x4::extract<0>(value);    // => 7
I32x4::extract_slow(value, 2); // => 9 with a runtime lane index
```

<a id="hadd-saturated"></a>
## `hadd_saturated`

Adds adjacent pairs with saturation where the specialization supports it.

Signatures:

```cpp
static auto hadd_saturated(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using I16 = SimdLib::Api<128, std::int16_t>;
I16::hadd_saturated(
    I16::set1(20000),
    I16::set1(10000)); // => {32767, 32767, 32767, 32767, 20000, 20000, 20000, 20000}
```

<a id="hsubtract-saturated"></a>
## `hsubtract_saturated`

Subtracts adjacent pairs with saturation where the specialization supports it.

Signatures:

```cpp
static auto hsubtract_saturated(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using I16 = SimdLib::Api<128, std::int16_t>;
I16::hsubtract_saturated(
    I16::setr_partial(30000, -10000, -30000, 10000),
    I16::setr_partial(
        20000, -20000, 10000, -10000)); // => {32767, -32768, 0, 0, 32767, 20000, 0, 0}
```

<a id="insert"></a>
## `insert` and `insert_slow`

Replaces one logical lane. The unsuffixed template form uses a compile-time lane index. `insert_slow` accepts a runtime-selected lane index.

Signatures:

```cpp
template <std::size_t index> static vector_t insert(vector_t lhs, element_t rhs)
static vector_t insert_slow(vector_t lhs, element_t rhs, int index)
```

Examples:

```cpp
using I32x4 = SimdLib::Api<128, std::int32_t>;
const auto zero = I32x4::setzero();
I32x4::insert<0>(zero, 9);       // => {9, 0, 0, 0}
I32x4::insert_slow(zero, 9, 2); // => {0, 0, 9, 0} with a runtime lane index
```

<a id="load"></a>
## `load`

Loads element data into a SIMD register.

Signatures:

```cpp
static vector_t load(std::span<const element_t, element_count> data)
```

Example:

```cpp
alignas(ApiT::byte_count) const std::array<float, ApiT::element_count> input{
    1.0F, 2.0F};
ApiT::load(input); // => low lanes are {1.0F, 2.0F}; remaining lanes are zero
```

<a id="load-aligned"></a>
## `load_aligned`

Loads a full register from storage aligned to the register byte width.

Signatures:

```cpp
static vector_t load_aligned(std::span<const element_t, element_count> data)
```

Example:

```cpp
alignas(ApiT::byte_count) const std::array<float, ApiT::element_count> input{
    1.0F, 2.0F};
ApiT::load_aligned(input); // => low lanes are {1.0F, 2.0F}; remaining lanes are zero
```

<a id="load-partial"></a>
## `load_partial`

Loads a logical prefix of elements into a SIMD register and zero-fills the remaining lanes.

Signatures:

```cpp
template <std::size_t active_count> static vector_t load_partial(std::span<const element_t> data)
```

Example:

```cpp
const std::array input{1.0F, 2.0F};
ApiT::load_partial<2>(std::span<const float>{
    input}); // => low lanes are {1.0F, 2.0F}; remaining lanes are zero
```

<a id="load-unaligned"></a>
## `load_unaligned`

Explicit spelling for an unaligned full-register load.

Signatures:

```cpp
static vector_t load_unaligned(std::span<const element_t, element_count> data)
```

Example:

```cpp
alignas(ApiT::byte_count) const std::array<float, ApiT::element_count> input{
    1.0F, 2.0F};
ApiT::load_unaligned(input); // => low lanes are {1.0F, 2.0F}; remaining lanes are zero
```

<a id="load-unsafe"></a>
## `load_unsafe`

Loads element data into a SIMD register without enforcing a fixed extent.

Signatures:

```cpp
static vector_t load_unsafe(std::span<const element_t> data)
```

Example:

```cpp
alignas(ApiT::byte_count) const std::array<float, ApiT::element_count> input{
    1.0F, 2.0F};
ApiT::load_unsafe(input); // => low lanes are {1.0F, 2.0F}; remaining lanes are zero
```

<a id="lower-half"></a>
## `lower_half`

Returns the low 128-bit half of a 256-bit register when the specialization supports it.

Signatures:

```cpp
static vector_t lower_half(vector_t lhs)
```

Example:

```cpp
using F32x8 = SimdLib::Api<256, float>;
F32x8::lower_half(F32x8::set1(2.0F)); // => {2.0F, 2.0F, 2.0F, 2.0F}
```

<a id="magnitude"></a>
## `magnitude`

Computes the vector magnitude per 128-bit lane.

Signatures:

```cpp
static vector_t magnitude(vector_t lhs)
```

Example:

```cpp
ApiT::magnitude(ApiT::construct({3.0F, 4.0F, 0.0F, 0.0F})); // => every lane is 5.0F
```

<a id="max"></a>
## `max`

Returns the larger value in each lane.

Signatures:

```cpp
static vector_t max(vector_t lhs, vector_t rhs)
```

Example:

```cpp
ApiT::max(
    ApiT::construct({2.0F, 8.0F, 4.0F, 9.0F}),
    ApiT::construct({5.0F, 3.0F, 7.0F, 1.0F})); // => {5.0F, 8.0F, 7.0F, 9.0F}
```

<a id="max-position"></a>
## `max_position`

Returns the first index of the maximum value in the register.

Signatures:

```cpp
static std::size_t max_position(vector_t lhs)
```

Example:

```cpp
using U16x8 = SimdLib::Api<128, std::uint16_t>;
const auto values = U16x8::insert<3>(U16x8::set1(4), 9);
U16x8::max_position(values); // => 3
```

<a id="min"></a>
## `min`

Returns the smaller value in each lane.

Signatures:

```cpp
static vector_t min(vector_t lhs, vector_t rhs)
```

Example:

```cpp
ApiT::min(
    ApiT::construct({2.0F, 8.0F, 4.0F, 9.0F}),
    ApiT::construct({5.0F, 3.0F, 7.0F, 1.0F})); // => {2.0F, 3.0F, 4.0F, 1.0F}
```

<a id="min-position"></a>
## `min_position`

Returns the first index of the minimum value in the register.

Signatures:

```cpp
static std::size_t min_position(vector_t lhs)
```

Example:

```cpp
using U16x8 = SimdLib::Api<128, std::uint16_t>;
const auto values = U16x8::insert<3>(U16x8::set1(4), 1);
U16x8::min_position(values); // => 3
```

<a id="modulus"></a>
## `modulus`

Computes the remainder of each lhs element divided by the corresponding rhs element.

Signatures:

```cpp
static vector_t modulus(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using U32 = SimdLib::Api<128, std::uint32_t>;
U32::modulus(
    U32::construct({7U, 7U, 7U, 7U}),
    U32::construct({3U, 3U, 3U, 3U})); // => every lane is 1U
```

<a id="movemask"></a>
## `movemask`

Returns a mask composed from the most significant bit of each byte in the register.

Signatures:

```cpp
static mask_t movemask(vector_t lhs)
```

Example:

```cpp
ApiT::movemask(
    ApiT::construct(
        {-0.0F, -0.0F, -0.0F, -0.0F})); // => one set sign bit for every lane
```

<a id="movemask-slim"></a>
## `movemask_slim`

Returns a mask composed from the most significant bit of each element in the register.

Signatures:

```cpp
static mask_t movemask_slim(vector_t lhs)
```

Example:

```cpp
ApiT::movemask_slim(
    ApiT::construct({-0.0F, -0.0F, -0.0F, -0.0F})); // => one set bit for every lane
```

<a id="multi-sum-absolute-byte-differences"></a>
## `multi_sum_absolute_byte_differences`

Computes byte-window absolute-difference sums selected by an immediate control mask.

Signatures:

```cpp
template <int imm8> static auto multi_sum_absolute_byte_differences(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using U8 = SimdLib::Api<128, std::uint8_t>;
U8::multi_sum_absolute_byte_differences<0>(
    U8::set1(9U), U8::set1(4U)); // => every selected 16-bit result lane is 20
```

<a id="multiply"></a>
## `multiply`

Multiplies corresponding lanes.

Signatures:

```cpp
static vector_t multiply(vector_t lhs, vector_t rhs)
```

Example:

```cpp
ApiT::multiply(
    ApiT::construct({3.0F, 3.0F, 3.0F, 3.0F}),
    ApiT::construct({4.0F, 4.0F, 4.0F, 4.0F})); // => every lane is 12.0F
```

<a id="multiply-add"></a>
## `multiply_add`

Computes a fused multiply-add where the implementation supports it, or a multiply followed by add otherwise.

Signatures:

```cpp
static auto multiply_add(vector_t lhs, vector_t rhs, vector_t addend)
```

Example:

```cpp
ApiT::multiply_add(
    ApiT::construct({2.0F, 2.0F, 2.0F, 2.0F}),
    ApiT::construct({3.0F, 3.0F, 3.0F, 3.0F}),
    ApiT::construct({4.0F, 4.0F, 4.0F, 4.0F})); // => every lane is 10.0F
```

<a id="multiply-add-adjacent"></a>
## `multiply_add_adjacent`

Multiplies adjacent element pairs and accumulates them into promoted result lanes.

Signatures:

```cpp
static auto multiply_add_adjacent(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using I16 = SimdLib::Api<128, std::int16_t>;
I16::multiply_add_adjacent(
    I16::set1(2), I16::set1(3)); // => every 32-bit result lane is 12
```

<a id="multiply-add-unsigned-signed-bytes"></a>
## `multiply_add_unsigned_signed_bytes`

Multiplies raw register bytes as unsigned and signed pairs and accumulates them into signed 16-bit lanes.

Signatures:

```cpp
static auto multiply_add_unsigned_signed_bytes(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using U8 = SimdLib::Api<128, std::uint8_t>;
U8::multiply_add_unsigned_signed_bytes(
    U8::set1(2U), U8::set1(3U)); // => every signed 16-bit result lane is 12
```

<a id="negate"></a>
## `negate`

Negates each element in the register.

Signatures:

```cpp
static vector_t negate(vector_t lhs)
```

Example:

```cpp
ApiT::negate(ApiT::construct({2.0F, -3.0F, 0.0F, 0.0F})); // => {-2.0F, 3.0F, 0.0F, ...}
```

<a id="normalize"></a>
## `normalize`

Normalizes floating-point lanes using the vector length computed per 128-bit lane.

Signatures:

```cpp
static vector_t normalize(vector_t lhs)
```

Example:

```cpp
ApiT::normalize(
    ApiT::construct({3.0F, 4.0F, 0.0F, 0.0F})); // => {0.6F, 0.8F, 0.0F, ...}
```

<a id="set"></a>
## `set`

Constructs a register from lane values in native argument order.

Signatures:

```cpp
template <class... Args> static auto set(Args &&...args)
```

Example:

```cpp
using F32x4 = SimdLib::Api<128, float>;
F32x4::set(
    4.0F,
    3.0F,
    2.0F,
    1.0F); // => lanes follow native set order: {1.0F, 2.0F, 3.0F, 4.0F}
```

<a id="set-partial"></a>
## `set_partial`

Constructs a register from a partial native-order lane list and zero-fills the remaining lanes.

Signatures:

```cpp
template <class... Args> static auto set_partial(Args &&...args)
```

Example:

```cpp
ApiT::set_partial(2.0F, 1.0F); // => {0.0F, 0.0F, 1.0F, 2.0F} in native set order
```

<a id="set1"></a>
## `set1`

Broadcasts one scalar value to every lane in the register.

Signatures:

```cpp
static vector_t set1(element_t value)
```

Example:

```cpp
ApiT::set1(2.5F); // => every lane is 2.5F
```

<a id="setr"></a>
## `setr`

Constructs a register from lane values in forward lane order.

Signatures:

```cpp
template <class... Args> static auto setr(Args &&...args)
```

Example:

```cpp
using F32x4 = SimdLib::Api<128, float>;
F32x4::setr(1.0F, 2.0F, 3.0F, 4.0F); // => lanes are {1.0F, 2.0F, 3.0F, 4.0F}
```

<a id="setr-partial"></a>
## `setr_partial`

Constructs a register from a partial forward-order lane list and zero-fills the remaining lanes.

Signatures:

```cpp
template <class... Args> static auto setr_partial(Args &&...args)
```

Example:

```cpp
ApiT::setr_partial(
    1.0F, 2.0F); // => low lanes are {1.0F, 2.0F}; remaining lanes are zero
```

<a id="setzero"></a>
## `setzero`

Returns a zero-initialized SIMD register.

Signatures:

```cpp
static vector_t setzero()
```

Example:

```cpp
ApiT::setzero(); // => every lane is 0.0F
```

<a id="shift-left"></a>
## `shift_left`

Shifts each integer lane left by the specified amount.

Signatures:

```cpp
static int_vector_t shift_left(int_vector_t lhs, int shift)
```

Example:

```cpp
using I32 = SimdLib::Api<128, std::int32_t>;
I32::shift_left(I32::construct({3, 3, 3, 3}), 1); // => every lane is 6
```

<a id="shift-right"></a>
## `shift_right`

Shifts each integer lane right by the specified amount.

Signatures:

```cpp
static int_vector_t shift_right(int_vector_t lhs, int shift)
```

Example:

```cpp
using I32 = SimdLib::Api<128, std::int32_t>;
I32::shift_right(I32::construct({8, 8, 8, 8}), 1); // => every lane is 4
```

<a id="shift-right-arithmetic"></a>
## `shift_right_arithmetic`

Arithmetic-shifts each integer lane right by the specified amount.

Signatures:

```cpp
static int_vector_t shift_right_arithmetic(int_vector_t lhs, int shift)
```

Example:

```cpp
using I32 = SimdLib::Api<128, std::int32_t>;
I32::shift_right_arithmetic(I32::construct({-8, -8, -8, -8}), 1); // => every lane is -4
```

<a id="shuffle"></a>
## `shuffle` and `shuffle_slow`

The compile-time logical overload constructs each output lane from the source lane named by the selector at the same output position. It requires exactly one selector per lane, permits repeated selectors, and rejects selectors outside the complete source register. At 256 bits, any selector may cross the 128-bit boundary. Floating-point lanes are moved by object representation, preserving NaN payloads and signed zero.

An unsuffixed register-selector overload remains available for byte shuffles backed by a native runtime selector register. `shuffle_slow` provides immediate-mask floating shuffle semantics for a runtime scalar control.

Signatures:

```cpp
template <std::size_t... indices> static vector_t shuffle(vector_t lhs)
template <class... Args> static auto shuffle(Args &&...args)
template <class... Args> static auto shuffle_slow(Args &&...args)
```

Examples:

```cpp
using U16x8 = SimdLib::Api<128, std::uint16_t>;
const auto words = U16x8::construct({0, 1, 2, 3, 4, 5, 6, 7});
U16x8::shuffle<7, 6, 5, 4, 3, 2, 1, 0>(words); // => {7, 6, 5, 4, 3, 2, 1, 0}

using I32x8 = SimdLib::Api<256, std::int32_t>;
const auto integers = I32x8::construct({0, 1, 2, 3, 4, 5, 6, 7});
I32x8::shuffle<4, 5, 6, 7, 0, 1, 2, 3>(integers); // => exchanges the 128-bit halves

using F64x4 = SimdLib::Api<256, double>;
const auto doubles = F64x4::construct({1.0, 2.0, 3.0, 4.0});
F64x4::shuffle<3, 3, 0, 0>(doubles); // => {4.0, 4.0, 1.0, 1.0}

using U8x16 = SimdLib::Api<128, std::uint8_t>;
U8x16::shuffle(
    U8x16::set1(7U),
    U8x16::set1(0x80U)); // native selector-register shuffle: high bits clear output bytes

using F32x4 = SimdLib::Api<128, float>;
F32x4::shuffle_slow(F32x4::set1(1.0F), F32x4::set1(2.0F), 0b1110'0100);
```

<a id="shuffle-32"></a>
## `shuffle_32` and `shuffle_32_slow`

Shuffles 32-bit lanes within each 128-bit group. The unsuffixed template uses an immediate control byte; `_slow` accepts a runtime scalar control.

Signatures:

```cpp
template <int imm8> static int_vector_t shuffle_32(int_vector_t lhs)
static int_vector_t shuffle_32_slow(int_vector_t lhs, std::uint32_t imm8)
```

Example:

```cpp
using U32x4 = SimdLib::Api<128, std::uint32_t>;
const auto values = U32x4::construct({0U, 1U, 2U, 3U});
U32x4::shuffle_32<0b00'01'10'11>(values);       // => {3U, 2U, 1U, 0U}
U32x4::shuffle_32_slow(values, 0b00'01'10'11); // same semantics with a runtime control
```

<a id="shuffle-hi"></a>
## `shuffle_hi` and `shuffle_hi_slow`

Shuffles the high four 16-bit lanes in each 128-bit group. The unsuffixed template uses an immediate control byte; `_slow` accepts a runtime scalar control.

Signatures:

```cpp
template <int imm8> static auto shuffle_hi(vector_t lhs)
template <class... Args> static auto shuffle_hi_slow(Args &&...args)
```

Example:

```cpp
using I16x8 = SimdLib::Api<128, std::int16_t>;
const auto high = I16x8::byte_shift_left_slow(I16x8::setr_partial(1, 2, 3, 4), 8);
I16x8::shuffle_hi<0b0001'1011>(high);       // => {0, 0, 0, 0, 4, 3, 2, 1}
I16x8::shuffle_hi_slow(high, 0b0001'1011); // same semantics with a runtime control
```

<a id="shuffle-lo"></a>
## `shuffle_lo` and `shuffle_lo_slow`

Shuffles the low four 16-bit lanes in each 128-bit group. The unsuffixed template uses an immediate control byte; `_slow` accepts a runtime scalar control.

Signatures:

```cpp
template <int imm8> static auto shuffle_lo(vector_t lhs)
template <class... Args> static auto shuffle_lo_slow(Args &&...args)
```

Example:

```cpp
using I16x8 = SimdLib::Api<128, std::int16_t>;
const auto value = I16x8::setr_partial(1, 2, 3, 4);
I16x8::shuffle_lo<0b0001'1011>(value);       // => {4, 3, 2, 1, 0, 0, 0, 0}
I16x8::shuffle_lo_slow(value, 0b0001'1011); // same semantics with a runtime control
```

<a id="sqrt"></a>
## `sqrt`

Computes the square root of each element in the register.

Signatures:

```cpp
static auto sqrt(vector_t lhs)
```

Example:

```cpp
ApiT::sqrt(ApiT::construct({4.0F, 9.0F, 0.0F, 0.0F})); // => {2.0F, 3.0F, 0.0F, ...}
```

<a id="store"></a>
## `store`

Stores a SIMD register into an element span.

Signatures:

```cpp
static void store(vector_t vector, std::span<element_t, element_count> data)
static void store(vector_t vector, std::span<std::byte> data)
```

Example:

```cpp
alignas(ApiT::byte_count) std::array<float, ApiT::element_count> result{};
ApiT::store(
    ApiT::construct({1.0F, 2.0F, 0.0F, 0.0F}),
    result); // => result begins {1.0F, 2.0F, 0.0F, ...}
```

<a id="store-aligned"></a>
## `store_aligned`

Stores a full register to storage aligned to the register byte width.

Signatures:

```cpp
static void store_aligned(vector_t vector, std::span<element_t, element_count> data)
```

Example:

```cpp
alignas(ApiT::byte_count) std::array<float, ApiT::element_count> result{};
ApiT::store_aligned(
    ApiT::construct({1.0F, 2.0F, 0.0F, 0.0F}),
    result); // => result begins {1.0F, 2.0F, 0.0F, ...}
```

<a id="store-unaligned"></a>
## `store_unaligned`

Explicit spelling for an unaligned full-register store.

Signatures:

```cpp
static void store_unaligned(vector_t vector, std::span<element_t, element_count> data)
```

Example:

```cpp
alignas(ApiT::byte_count) std::array<float, ApiT::element_count> result{};
ApiT::store_unaligned(
    ApiT::construct({1.0F, 2.0F, 0.0F, 0.0F}),
    result); // => result begins {1.0F, 2.0F, 0.0F, ...}
```

<a id="subtract"></a>
## `subtract`

Subtracts corresponding lanes.

Signatures:

```cpp
static vector_t subtract(vector_t lhs, vector_t rhs)
```

Example:

```cpp
ApiT::subtract(
    ApiT::construct({7.0F, 7.0F, 7.0F, 7.0F}),
    ApiT::construct({2.0F, 2.0F, 2.0F, 2.0F})); // => every lane is 5.0F
```

<a id="subtract-horizontal"></a>
## `subtract_horizontal`

Subtracts adjacent element pairs within each 128-bit lane of two registers.

Signatures:

```cpp
static vector_t subtract_horizontal(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using F32x4 = SimdLib::Api<128, float>;
F32x4::subtract_horizontal(
    F32x4::construct({3.0F, 1.0F, 7.0F, 2.0F}),
    F32x4::construct({9.0F, 4.0F, 8.0F, 2.0F})); // => {2.0F, 5.0F, 5.0F, 6.0F}
```

<a id="subtract-saturated"></a>
## `subtract_saturated`

Subtracts corresponding lanes with saturation where the specialization supports it.

Signatures:

```cpp
static auto subtract_saturated(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using U8 = SimdLib::Api<128, std::uint8_t>;
U8::subtract_saturated(U8::set1(5), U8::set1(10)); // => every lane is 0
```

<a id="sum-absolute-byte-differences"></a>
## `sum_absolute_byte_differences`

Computes byte-wise absolute differences and accumulates them into 64-bit result lanes.

Signatures:

```cpp
static auto sum_absolute_byte_differences(vector_t lhs, vector_t rhs)
```

Example:

```cpp
using U8 = SimdLib::Api<128, std::uint8_t>;
U8::sum_absolute_byte_differences(
    U8::set1(9U), U8::set1(4U)); // => both 64-bit result lanes are 40
```

<a id="to-array"></a>
## `to_array`

Converts a SIMD register into a fixed array of elements.

Signatures:

```cpp
static std::array<element_t, element_count> to_array(vector_t vector)
```

Example:

```cpp
ApiT::to_array(ApiT::construct({1.0F, 2.0F, 0.0F, 0.0F})); // => {1.0F, 2.0F, 0.0F, ...}
```

<a id="transform"></a>
## `transform`

Applies a unary SIMD transform to an element span in place.

Signatures:

```cpp
template <std::invocable<vector_t> Func> static void transform(std::span<element_t> data, Func &&func)
template <std::invocable<vector_t> Func> static void transform(std::span<const element_t> lhs, std::span<element_t> write, Func &&func)
template <std::invocable<vector_t, vector_t> Func> static void transform(std::span<const element_t> lhs, std::span<const element_t> rhs, std::span<element_t> write, Func &&func)
```

Example:

```cpp
std::array<float, 3> result{};
ApiT::transform(std::array{1.0F, 2.0F, 3.0F}, result, [](auto lanes) {
  return ApiT::add(lanes, ApiT::construct({10.0F, 10.0F, 10.0F, 10.0F}));
}); // => result is {11.0F, 12.0F, 13.0F}
```

<a id="transform-pack"></a>
## `transform_pack`

Applies a SIMD transform whose fixed-width lane results are packed contiguously into integer storage.

Signatures:

```cpp
template <std::size_t result_bit_width, std::size_t count, std::invocable<vector_t> Func> static void transform_pack(std::span<const element_t, count> read, std::span<packed_element_t<result_bit_width>, packed_element_count<result_bit_width, count>> write, Func &&func)
```

Example:

```cpp
std::array<std::uint8_t, 1> result{};
ApiT::transform_pack<1>(
    std::span<const float, 4>{std::array{1.0F, -2.0F, 3.0F, -4.0F}},
    std::span<std::uint8_t, 1>{result},
    [](auto lanes) {
      return ApiT::movemask_slim(lanes);
    }); // => result[0] is 0b0000'1010
```

<a id="unpack-hi"></a>
## `unpack_hi`

Unpacks the high lanes of two registers.

Signatures:

```cpp
static auto unpack_hi(vector_t lhs, vector_t rhs)
```

Example:

```cpp
ApiT::unpack_hi(
    ApiT::construct({1.0F, 2.0F, 3.0F, 4.0F}),
    ApiT::construct({5.0F, 6.0F, 7.0F, 8.0F})); // => {3.0F, 7.0F, 4.0F, 8.0F}
```

<a id="unpack-lo"></a>
## `unpack_lo`

Unpacks the low lanes of two registers.

Signatures:

```cpp
static auto unpack_lo(vector_t lhs, vector_t rhs)
```

Example:

```cpp
ApiT::unpack_lo(
    ApiT::construct({1.0F, 2.0F, 3.0F, 4.0F}),
    ApiT::construct({5.0F, 6.0F, 7.0F, 8.0F})); // => {1.0F, 5.0F, 2.0F, 6.0F}
```

<a id="widen"></a>
## `widen`

Widens this SIMD register into the specified destination SIMD shape.

Signatures:

```cpp
template <class target_simd> static typename target_simd::vector_t widen(vector_t lhs)
```

Example:

```cpp
using I16x8 = SimdLib::Api<128, std::int16_t>;
using I32x8 = SimdLib::Api<256, std::int32_t>;
I16x8::widen<I32x8>(
    I16x8::set1(-30000)); // => eight 32-bit lanes, each containing -30000
```

<a id="related-types-and-constants"></a>
## Related types and constants

`vector_t`, `int_vector_t`, `mask_t`, `element_type`, `register_width`, `element_count`, and `byte_count` describe the selected specialization. `using_int`, `using_float`, and related Boolean constants describe its element category.
