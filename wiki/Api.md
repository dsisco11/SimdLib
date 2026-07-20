# Api

`Api<register_width, element_t>` is the low-level SIMD facade. Most applications should spell it through [`NativeApi`](NativeApi.md), which chooses the widest supported register width.

## Contents

- [Overview](#overview)
- [Example setup](#example-setup)
- [`absolute`](#absolute)
- [`add`](#add)
- [`add_horizontal`](#add-horizontal)
- [`add_saturated`](#add-saturated)
- [`add_subtract`](#add-subtract)
- [`avg`](#avg)
- [`bit_shift_left`](#bit-shift-left)
- [`bit_shift_right`](#bit-shift-right)
- [`bitwise_and`](#bitwise-and)
- [`bitwise_andnot`](#bitwise-andnot)
- [`bitwise_not`](#bitwise-not)
- [`bitwise_or`](#bitwise-or)
- [`bitwise_xor`](#bitwise-xor)
- [`blend`](#blend)
- [`byte_shift_left`](#byte-shift-left)
- [`byte_shift_right`](#byte-shift-right)
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
- [`extract`](#extract)
- [`hadd_saturated`](#hadd-saturated)
- [`hsubtract_saturated`](#hsubtract-saturated)
- [`insert`](#insert)
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
- [`shuffle`](#shuffle)
- [`shuffle_hi`](#shuffle-hi)
- [`shuffle_lo`](#shuffle-lo)
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
## Example setup

```cpp
#include <SimdLib/SimdLib.h>
#include <array>

using ApiT = SimdLib::NativeApi<float>;
using Register = ApiT::vector_t;
std::array<float, ApiT::element_count> input{};
std::array<float, ApiT::element_count> output{};
const Register lhs = ApiT::load(input);
const Register rhs = ApiT::set1(2.0F);
const Register addend = ApiT::set1(1.0F);
const Register value = lhs;
const int selector = 0;
```

<a id="absolute"></a>
## `absolute`

Computes the absolute value of each element in the register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static vector_t VECTORCALL absolute(const vector_t lhs) noexcept requires requires(vector_t value)
```

Example:

```cpp
const auto result = ApiT::absolute(value);
```

<a id="add"></a>
## `add`

Adds corresponding lanes.

Signatures:

```cpp
static vector_t add(vector_t lhs, vector_t rhs) noexcept;
```

Example:

```cpp
const auto result = ApiT::add(lhs, rhs);
```

<a id="add-horizontal"></a>
## `add_horizontal`

Adds adjacent element pairs within each 128-bit lane of two registers.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static vector_t VECTORCALL add_horizontal(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::add_horizontal(lhs, rhs);
```

<a id="add-saturated"></a>
## `add_saturated`

Adds corresponding lanes with saturation where the specialization supports it.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL add_saturated(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::add_saturated(lhs, rhs);
```

<a id="add-subtract"></a>
## `add_subtract`

Alternates subtraction and addition across lanes for floating-point SIMD families.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL add_subtract(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::add_subtract(lhs, rhs);
```

<a id="avg"></a>
## `avg`

Computes the average of corresponding lanes where the specialization supports it.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL avg(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::avg(lhs, rhs);
```

<a id="bit-shift-left"></a>
## `bit_shift_left`

Shifts the complete 128-bit register left, carrying bits across lane boundaries. Unlike `shift_left`, this treats the register as one unsigned 128-bit bit string. A zero or negative runtime count returns the input; counts of 128 or more return zero.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL bit_shift_left(const int_vector_t lhs, const int shift) noexcept requires(using_int && register_width == 128)
template <int shift> SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL bit_shift_left(const int_vector_t lhs) noexcept requires(using_int && register_width == 128)
```

Example:

```cpp
const auto result = ApiT::bit_shift_left(value); // Add the compile-time selector/type required by the overload.
```

<a id="bit-shift-right"></a>
## `bit_shift_right`

Shifts the complete 128-bit register right, carrying bits across lane boundaries. Unlike `shift_right`, this treats the register as one unsigned 128-bit bit string. A zero or negative runtime count returns the input; counts of 128 or more return zero.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL bit_shift_right(const int_vector_t lhs, const int shift) noexcept requires(using_int && register_width == 128)
template <int shift> SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL bit_shift_right(const int_vector_t lhs) noexcept requires(using_int && register_width == 128)
```

Example:

```cpp
const auto result = ApiT::bit_shift_right(value); // Add the compile-time selector/type required by the overload.
```

<a id="bitwise-and"></a>
## `bitwise_and`

Computes a bitwise AND of two registers.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL bitwise_and(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::bitwise_and(lhs, rhs);
```

<a id="bitwise-andnot"></a>
## `bitwise_andnot`

Computes a bitwise AND-NOT of two registers.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL bitwise_andnot(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::bitwise_andnot(lhs, rhs);
```

<a id="bitwise-not"></a>
## `bitwise_not`

Computes a bitwise NOT of a register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL bitwise_not(const vector_t lhs) noexcept requires requires(vector_t value)
```

Example:

```cpp
const auto result = ApiT::bitwise_not(value);
```

<a id="bitwise-or"></a>
## `bitwise_or`

Computes a bitwise OR of two registers.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL bitwise_or(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::bitwise_or(lhs, rhs);
```

<a id="bitwise-xor"></a>
## `bitwise_xor`

Computes a bitwise XOR of two registers.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL bitwise_xor(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::bitwise_xor(lhs, rhs);
```

<a id="blend"></a>
## `blend`

Blends two registers according to the implementation-specific control form.

Signatures:

```cpp
template <class... Args> SIMDLIB_FORCE_INLINE static auto VECTORCALL blend(Args &&...args) noexcept requires requires(Args &&...values)
```

Example:

```cpp
const auto result = ApiT::blend(lhs);
```

<a id="byte-shift-left"></a>
## `byte_shift_left`

Shifts every byte in a 128-bit register toward higher byte indices.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL byte_shift_left(const int_vector_t lhs, const int shift) noexcept requires(using_int && register_width == 128)
```

Example:

```cpp
const auto result = ApiT::byte_shift_left(value); // Add the compile-time selector/type required by the overload.
```

<a id="byte-shift-right"></a>
## `byte_shift_right`

Shifts every byte in a 128-bit register toward lower byte indices.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL byte_shift_right(const int_vector_t lhs, const int shift) noexcept requires(using_int && register_width == 128)
```

Example:

```cpp
const auto result = ApiT::byte_shift_right(value); // Add the compile-time selector/type required by the overload.
```

<a id="cmp-eq"></a>
## `cmp_eq`

Computes an equality comparison mask for two registers.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL cmp_eq(const vector_t lhs, const vector_t rhs) noexcept
```

Example:

```cpp
const auto result = ApiT::cmp_eq(lhs, rhs);
```

<a id="cmp-eq-mask"></a>
## `cmp_eq_mask`

Computes a byte-granular equality comparison mask for two registers of this SIMD shape.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL cmp_eq_mask(const vector_t lhs, const vector_t rhs) noexcept
```

Example:

```cpp
const auto result = ApiT::cmp_eq_mask(lhs, rhs);
```

<a id="cmp-ge"></a>
## `cmp_ge`

Computes a greater-than-or-equal comparison mask for two registers.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL cmp_ge(const vector_t lhs, const vector_t rhs) noexcept
```

Example:

```cpp
const auto result = ApiT::cmp_ge(lhs, rhs);
```

<a id="cmp-gt"></a>
## `cmp_gt`

Computes a greater-than comparison mask for two registers.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL cmp_gt(const vector_t lhs, const vector_t rhs) noexcept
```

Example:

```cpp
const auto result = ApiT::cmp_gt(lhs, rhs);
```

<a id="cmp-le"></a>
## `cmp_le`

Computes a less-than-or-equal comparison mask for two registers.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL cmp_le(const vector_t lhs, const vector_t rhs) noexcept
```

Example:

```cpp
const auto result = ApiT::cmp_le(lhs, rhs);
```

<a id="cmp-lt"></a>
## `cmp_lt`

Computes a less-than comparison mask for two registers.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL cmp_lt(const vector_t lhs, const vector_t rhs) noexcept
```

Example:

```cpp
const auto result = ApiT::cmp_lt(lhs, rhs);
```

<a id="compress"></a>
## `compress`

Compresses two registers into a narrower-lane register where the specialization supports it.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL compress(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::compress(lhs, rhs);
```

<a id="construct"></a>
## `construct`

Constructs a SIMD register from a fixed array.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static vector_t VECTORCALL construct(const std::array<element_t, element_count> &data) noexcept
```

Example:

```cpp
const auto result = ApiT::construct(lhs);
```

<a id="convert"></a>
## `convert`

Converts between 32-bit integer and floating-point register representations.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL convert(vector_t vector) noexcept requires(element_width == 32)
```

Example:

```cpp
const auto result = ApiT::convert(value); // Add the compile-time selector/type required by the overload.
```

<a id="convert-to-float"></a>
## `convert_to_float`

Converts 32-bit integer lanes into floating-point lanes.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static float_vector_t VECTORCALL convert_to_float(int_vector_t vector) noexcept requires(element_width == 32)
```

Example:

```cpp
const auto result = ApiT::convert_to_float(value); // Add the compile-time selector/type required by the overload.
```

<a id="convert-to-int"></a>
## `convert_to_int`

Converts 32-bit floating-point lanes into integer lanes.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static int_vector_t VECTORCALL convert_to_int(float_vector_t vector) noexcept requires(element_width == 32)
```

Example:

```cpp
const auto result = ApiT::convert_to_int(value); // Add the compile-time selector/type required by the overload.
```

<a id="divide"></a>
## `divide`

Divides corresponding lanes.

Signatures:

```cpp
static vector_t divide(vector_t lhs, vector_t rhs) noexcept;
```

Example:

```cpp
const auto result = ApiT::divide(lhs, rhs);
```

<a id="dot-product"></a>
## `dot_product`

Computes a dot product using a compile-time immediate mask where the specialization supports it.

Signatures:

```cpp
template <int imm8> SIMDLIB_FORCE_INLINE static auto VECTORCALL dot_product(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::dot_product(lhs, rhs);
```

<a id="expand"></a>
## `expand`

Expands a register into a wider-lane register where the specialization supports it.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL expand(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::expand(lhs, rhs);
```

<a id="extract"></a>
## `extract`

Extracts a lane or subvalue from a register.

Signatures:

```cpp
template <int index> SIMDLIB_FORCE_INLINE static auto VECTORCALL extract(const vector_t lhs) noexcept requires requires(vector_t value)
template <class selector_t> SIMDLIB_FORCE_INLINE static auto VECTORCALL extract(const vector_t lhs, selector_t rhs) noexcept requires requires(vector_t left, selector_t selector)
```

Example:

```cpp
const auto result = ApiT::extract(value); // Add the compile-time selector/type required by the overload.
```

<a id="hadd-saturated"></a>
## `hadd_saturated`

Adds adjacent pairs with saturation where the specialization supports it.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL hadd_saturated(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::hadd_saturated(lhs, rhs);
```

<a id="hsubtract-saturated"></a>
## `hsubtract_saturated`

Subtracts adjacent pairs with saturation where the specialization supports it.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL hsubtract_saturated(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::hsubtract_saturated(lhs, rhs);
```

<a id="insert"></a>
## `insert`

Inserts a lane or subvalue into a register.

Signatures:

```cpp
template <class... Args> SIMDLIB_FORCE_INLINE static auto VECTORCALL insert(Args &&...args) noexcept requires requires(Args &&...values)
```

Example:

```cpp
const auto result = ApiT::insert(value); // Add the compile-time selector/type required by the overload.
```

<a id="load"></a>
## `load`

Loads element data into a SIMD register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static vector_t VECTORCALL load(std::span<const element_t, element_count> data) noexcept
```

Example:

```cpp
const auto value = ApiT::load(input);
```

<a id="load-aligned"></a>
## `load_aligned`

Loads a full register from storage aligned to the register byte width.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static vector_t VECTORCALL load_aligned(std::span<const element_t, element_count> data) noexcept
```

Example:

```cpp
const auto value = ApiT::load_aligned(input);
```

<a id="load-partial"></a>
## `load_partial`

Loads a logical prefix of elements into a SIMD register and zero-fills the remaining lanes.

Signatures:

```cpp
template <std::size_t active_count> SIMDLIB_FORCE_INLINE constexpr static vector_t VECTORCALL load_partial(std::span<const element_t> data) noexcept requires(active_count <= element_count)
```

Example:

```cpp
const auto value = ApiT::load_partial(input);
```

<a id="load-unaligned"></a>
## `load_unaligned`

Explicit spelling for an unaligned full-register load.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static vector_t VECTORCALL load_unaligned(std::span<const element_t, element_count> data) noexcept
```

Example:

```cpp
const auto value = ApiT::load_unaligned(input);
```

<a id="load-unsafe"></a>
## `load_unsafe`

Loads element data into a SIMD register without enforcing a fixed extent.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static vector_t VECTORCALL load_unsafe(std::span<const element_t> data) noexcept
```

Example:

```cpp
const auto value = ApiT::load_unsafe(input);
```

<a id="lower-half"></a>
## `lower_half`

Returns the low 128-bit half of a 256-bit register when the specialization supports it.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static typename SimdLib::Detail::SimdMappings<128, element_t>::vector_t VECTORCALL lower_half(const vector_t lhs) noexcept requires(register_width == 256 && requires(vector_t value)
```

Example:

```cpp
const auto result = ApiT::lower_half(value);
```

<a id="magnitude"></a>
## `magnitude`

Computes the vector magnitude per 128-bit lane.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static vector_t VECTORCALL magnitude(const vector_t lhs) noexcept requires((std::is_floating_point_v<element_t> && requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::magnitude(value);
```

<a id="max"></a>
## `max`

Returns the larger value in each lane.

Signatures:

```cpp
static vector_t max(vector_t lhs, vector_t rhs) noexcept;
```

Example:

```cpp
const auto result = ApiT::max(lhs, rhs);
```

<a id="max-position"></a>
## `max_position`

Returns the first index of the maximum value in the register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static std::size_t VECTORCALL max_position(const vector_t lhs) noexcept requires(using_int && requires(vector_t value)
```

Example:

```cpp
const auto result = ApiT::max_position(value);
```

<a id="min"></a>
## `min`

Returns the smaller value in each lane.

Signatures:

```cpp
static vector_t min(vector_t lhs, vector_t rhs) noexcept;
```

Example:

```cpp
const auto result = ApiT::min(lhs, rhs);
```

<a id="min-position"></a>
## `min_position`

Returns the first index of the minimum value in the register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static std::size_t VECTORCALL min_position(const vector_t lhs) noexcept requires(using_int && requires(vector_t value)
```

Example:

```cpp
const auto result = ApiT::min_position(value);
```

<a id="modulus"></a>
## `modulus`

Computes the remainder of each lhs element divided by the corresponding rhs element.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static vector_t VECTORCALL modulus(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::modulus(lhs, rhs);
```

<a id="movemask"></a>
## `movemask`

Returns a mask composed from the most significant bit of each byte in the register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL movemask(const vector_t lhs) noexcept
```

Example:

```cpp
const auto result = ApiT::movemask(value);
```

<a id="movemask-slim"></a>
## `movemask_slim`

Returns a mask composed from the most significant bit of each element in the register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static mask_t VECTORCALL movemask_slim(const vector_t lhs) noexcept
```

Example:

```cpp
const auto result = ApiT::movemask_slim(value);
```

<a id="multi-sum-absolute-byte-differences"></a>
## `multi_sum_absolute_byte_differences`

Computes byte-window absolute-difference sums selected by an immediate control mask.

Signatures:

```cpp
template <int imm8> SIMDLIB_FORCE_INLINE static auto VECTORCALL multi_sum_absolute_byte_differences(const vector_t lhs, const vector_t rhs) noexcept requires(using_int && requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::multi_sum_absolute_byte_differences(lhs, rhs);
```

<a id="multiply"></a>
## `multiply`

Multiplies corresponding lanes.

Signatures:

```cpp
static vector_t multiply(vector_t lhs, vector_t rhs) noexcept;
```

Example:

```cpp
const auto result = ApiT::multiply(lhs, rhs);
```

<a id="multiply-add"></a>
## `multiply_add`

Computes a fused multiply-add where the implementation supports it, or a multiply followed by add otherwise.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL multiply_add(const vector_t lhs, const vector_t rhs, const vector_t addend) noexcept requires requires(vector_t left, vector_t right, vector_t sum)
```

Example:

```cpp
const auto result = ApiT::multiply_add(lhs, rhs, addend);
```

<a id="multiply-add-adjacent"></a>
## `multiply_add_adjacent`

Multiplies adjacent element pairs and accumulates them into promoted result lanes.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL multiply_add_adjacent(const vector_t lhs, const vector_t rhs) noexcept requires(using_int && requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::multiply_add_adjacent(lhs, rhs);
```

<a id="multiply-add-unsigned-signed-bytes"></a>
## `multiply_add_unsigned_signed_bytes`

Multiplies raw register bytes as unsigned and signed pairs and accumulates them into signed 16-bit lanes.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL multiply_add_unsigned_signed_bytes(const vector_t lhs, const vector_t rhs) noexcept requires(using_int && requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::multiply_add_unsigned_signed_bytes(lhs, rhs);
```

<a id="negate"></a>
## `negate`

Negates each element in the register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static vector_t VECTORCALL negate(const vector_t lhs) noexcept requires requires(vector_t value)
```

Example:

```cpp
const auto result = ApiT::negate(value);
```

<a id="normalize"></a>
## `normalize`

Normalizes floating-point lanes using the vector length computed per 128-bit lane.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static vector_t VECTORCALL normalize(const vector_t lhs) noexcept requires(std::is_floating_point_v<element_t> && requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::normalize(value);
```

<a id="set"></a>
## `set`

Constructs a register from lane values in native argument order.

Signatures:

```cpp
template <class... Args> SIMDLIB_FORCE_INLINE constexpr static auto VECTORCALL set(Args &&...args) noexcept requires requires(Args &&...values)
```

Example:

```cpp
const auto value = ApiT::set(1.0F, 2.0F, 3.0F, 4.0F);
```

<a id="set-partial"></a>
## `set_partial`

Constructs a register from a partial native-order lane list and zero-fills the remaining lanes.

Signatures:

```cpp
template <class... Args> SIMDLIB_FORCE_INLINE constexpr static auto VECTORCALL set_partial(Args &&...args) noexcept requires(sizeof...(Args) <= element_count)
```

Example:

```cpp
const auto value = ApiT::set_partial(1.0F, 2.0F, 3.0F, 4.0F);
```

<a id="set1"></a>
## `set1`

Broadcasts one scalar value to every lane in the register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static vector_t VECTORCALL set1(const element_t value) noexcept requires requires(element_t scalar)
```

Example:

```cpp
const auto value = ApiT::set1(2.0F);
```

<a id="setr"></a>
## `setr`

Constructs a register from lane values in forward lane order.

Signatures:

```cpp
template <class... Args> SIMDLIB_FORCE_INLINE constexpr static auto VECTORCALL setr(Args &&...args) noexcept requires requires(Args &&...values)
```

Example:

```cpp
const auto value = ApiT::setr(1.0F, 2.0F, 3.0F, 4.0F);
```

<a id="setr-partial"></a>
## `setr_partial`

Constructs a register from a partial forward-order lane list and zero-fills the remaining lanes.

Signatures:

```cpp
template <class... Args> SIMDLIB_FORCE_INLINE constexpr static auto VECTORCALL setr_partial(Args &&...args) noexcept requires(sizeof...(Args) <= element_count)
```

Example:

```cpp
const auto value = ApiT::setr_partial(1.0F, 2.0F, 3.0F, 4.0F);
```

<a id="setzero"></a>
## `setzero`

Returns a zero-initialized SIMD register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static vector_t VECTORCALL setzero() noexcept requires requires
```

Example:

```cpp
const auto value = ApiT::setzero();
```

<a id="shift-left"></a>
## `shift_left`

Shifts each integer lane left by the specified amount.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL shift_left(const int_vector_t lhs, int shift) noexcept requires(using_int)
```

Example:

```cpp
const auto result = ApiT::shift_left(value); // Add the compile-time selector/type required by the overload.
```

<a id="shift-right"></a>
## `shift_right`

Shifts each integer lane right by the specified amount.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL shift_right(const int_vector_t lhs, int shift) noexcept requires(using_int)
```

Example:

```cpp
const auto result = ApiT::shift_right(value); // Add the compile-time selector/type required by the overload.
```

<a id="shift-right-arithmetic"></a>
## `shift_right_arithmetic`

Arithmetic-shifts each integer lane right by the specified amount.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static int_vector_t VECTORCALL shift_right_arithmetic(const int_vector_t lhs, int shift) noexcept requires(using_int)
```

Example:

```cpp
const auto result = ApiT::shift_right_arithmetic(value); // Add the compile-time selector/type required by the overload.
```

<a id="shuffle"></a>
## `shuffle`

Shuffles register contents according to the implementation-specific control form.

Signatures:

```cpp
template <std::size_t... indices> SIMDLIB_FORCE_INLINE static auto VECTORCALL shuffle(const int_vector_t lhs) noexcept requires requires(int_vector_t value)
template <class... Args> SIMDLIB_FORCE_INLINE static auto VECTORCALL shuffle(Args &&...args) noexcept requires requires(Args &&...values)
```

Example:

```cpp
const auto result = ApiT::shuffle(value); // Add the compile-time selector/type required by the overload.
```

<a id="shuffle-hi"></a>
## `shuffle_hi`

Shuffles the high half of a register where the specialization supports it.

Signatures:

```cpp
template <class... Args> SIMDLIB_FORCE_INLINE static auto VECTORCALL shuffle_hi(Args &&...args) noexcept requires requires(Args &&...values)
```

Example:

```cpp
const auto result = ApiT::shuffle_hi(value); // Add the compile-time selector/type required by the overload.
```

<a id="shuffle-lo"></a>
## `shuffle_lo`

Shuffles the low half of a register where the specialization supports it.

Signatures:

```cpp
template <class... Args> SIMDLIB_FORCE_INLINE static auto VECTORCALL shuffle_lo(Args &&...args) noexcept requires requires(Args &&...values)
```

Example:

```cpp
const auto result = ApiT::shuffle_lo(value); // Add the compile-time selector/type required by the overload.
```

<a id="sqrt"></a>
## `sqrt`

Computes the square root of each element in the register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL sqrt(const vector_t lhs) noexcept requires requires(vector_t value)
```

Example:

```cpp
const auto result = ApiT::sqrt(value);
```

<a id="store"></a>
## `store`

Stores a SIMD register into an element span.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static void VECTORCALL store(vector_t vector, std::span<element_t, element_count> data) noexcept
SIMDLIB_FORCE_INLINE static void VECTORCALL store(vector_t vector, std::span<std::byte> data) noexcept
```

Example:

```cpp
ApiT::store(value, output);
```

<a id="store-aligned"></a>
## `store_aligned`

Stores a full register to storage aligned to the register byte width.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static void VECTORCALL store_aligned(vector_t vector, std::span<element_t, element_count> data) noexcept
```

Example:

```cpp
ApiT::store_aligned(value, output);
```

<a id="store-unaligned"></a>
## `store_unaligned`

Explicit spelling for an unaligned full-register store.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static void VECTORCALL store_unaligned(vector_t vector, std::span<element_t, element_count> data) noexcept
```

Example:

```cpp
ApiT::store_unaligned(value, output);
```

<a id="subtract"></a>
## `subtract`

Subtracts corresponding lanes.

Signatures:

```cpp
static vector_t subtract(vector_t lhs, vector_t rhs) noexcept;
```

Example:

```cpp
const auto result = ApiT::subtract(lhs, rhs);
```

<a id="subtract-horizontal"></a>
## `subtract_horizontal`

Subtracts adjacent element pairs within each 128-bit lane of two registers.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static vector_t VECTORCALL subtract_horizontal(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::subtract_horizontal(lhs, rhs);
```

<a id="subtract-saturated"></a>
## `subtract_saturated`

Subtracts corresponding lanes with saturation where the specialization supports it.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL subtract_saturated(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::subtract_saturated(lhs, rhs);
```

<a id="sum-absolute-byte-differences"></a>
## `sum_absolute_byte_differences`

Computes byte-wise absolute differences and accumulates them into 64-bit result lanes.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL sum_absolute_byte_differences(const vector_t lhs, const vector_t rhs) noexcept requires(using_int && requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::sum_absolute_byte_differences(lhs, rhs);
```

<a id="to-array"></a>
## `to_array`

Converts a SIMD register into a fixed array of elements.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr static std::array<element_t, element_count> VECTORCALL to_array(const vector_t vector) noexcept
```

Example:

```cpp
const auto result = ApiT::to_array(value);
```

<a id="transform"></a>
## `transform`

Applies a unary SIMD transform to an element span in place.

Signatures:

```cpp
template <std::invocable<vector_t> Func> static inline void transform(std::span<element_t> data, Func &&func) noexcept
template <std::invocable<vector_t> Func> static inline void transform(std::span<const element_t> lhs, std::span<element_t> write, Func &&func) noexcept
template <std::invocable<vector_t, vector_t> Func> static inline void transform(std::span<const element_t> lhs, std::span<const element_t> rhs, std::span<element_t> write, Func &&func) noexcept
```

Example:

```cpp
ApiT::transform(input, output, [](auto chunk) { return ApiT::multiply_add(chunk, scale, offset); });
```

<a id="transform-pack"></a>
## `transform_pack`

Applies a SIMD transform whose fixed-width lane results are packed contiguously into integer storage.

Signatures:

```cpp
template <std::size_t result_bit_width, std::size_t count, std::invocable<vector_t> Func> SIMDLIB_FORCE_INLINE constexpr static void transform_pack( std::span<const element_t, count> read, std::span<packed_element_t<result_bit_width>, packed_element_count<result_bit_width, count>> write, Func &&func) noexcept requires(result_bit_width > 0 && result_bit_width <= 64)
```

Example:

```cpp
ApiT::transform_pack(input, output, [](auto chunk) { return ApiT::convert(chunk); });
```

<a id="unpack-hi"></a>
## `unpack_hi`

Unpacks the high lanes of two registers.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL unpack_hi(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::unpack_hi(lhs, rhs);
```

<a id="unpack-lo"></a>
## `unpack_lo`

Unpacks the low lanes of two registers.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE static auto VECTORCALL unpack_lo(const vector_t lhs, const vector_t rhs) noexcept requires requires(vector_t left, vector_t right)
```

Example:

```cpp
const auto result = ApiT::unpack_lo(lhs, rhs);
```

<a id="widen"></a>
## `widen`

Widens this SIMD register into the specified destination SIMD shape.

Signatures:

```cpp
template <class target_simd> SIMDLIB_FORCE_INLINE static typename target_simd::vector_t VECTORCALL widen(const vector_t lhs) noexcept
```

Example:

```cpp
const auto result = ApiT::widen(value); // Add the compile-time selector/type required by the overload.
```

<a id="related-types-and-constants"></a>
## Related types and constants

`vector_t`, `int_vector_t`, `mask_t`, `element_type`, `register_width`, `element_count`, and `byte_count` describe the selected specialization. `using_int`, `using_float`, and related Boolean constants describe its element category.
