# SimdVector

`SimdVector<element_t, element_count>` is the friendly, position-vector-like class. It stores a small fixed number of numeric elements and offers arithmetic operators, vector math, comparisons, and named coordinates.

## Contents

- [Overview](#overview)
- [Example alias](#example-setup)
- [`~SimdVector`](#destructor-simdvector)
- [`abs`](#abs)
- [`add_horizontal`](#add-horizontal)
- [`add_horizontal_saturated`](#add-horizontal-saturated)
- [`add_saturated`](#add-saturated)
- [`add_subtract`](#add-subtract)
- [`all_equal`](#all-equal)
- [`all_greater`](#all-greater)
- [`all_greater_equal`](#all-greater-equal)
- [`all_less`](#all-less)
- [`all_less_equal`](#all-less-equal)
- [`any_equal`](#any-equal)
- [`any_greater`](#any-greater)
- [`any_greater_equal`](#any-greater-equal)
- [`any_less`](#any-less)
- [`any_less_equal`](#any-less-equal)
- [`area`](#area)
- [`avg`](#avg)
- [`clamp`](#clamp)
- [`dot_product`](#dot-product)
- [`getRegister`](#getregister)
- [`getSpan`](#getspan)
- [`getTuple`](#gettuple)
- [`magnitude`](#magnitude)
- [`max`](#max)
- [`max_position`](#max-position)
- [`min`](#min)
- [`min_position`](#min-position)
- [`multi_sum_absolute_byte_differences`](#multi-sum-absolute-byte-differences)
- [`multiply_add`](#multiply-add)
- [`multiply_add_adjacent`](#multiply-add-adjacent)
- [`multiply_add_unsigned_signed_bytes`](#multiply-add-unsigned-signed-bytes)
- [`multiply_saturated`](#multiply-saturated)
- [`normalize`](#normalize)
- [`operator std::array<element_t, simd::element_count>`](#operator-std-array-element-t-simd-element-countconversion)
- [`operator std::span<const element_t, simd::element_count>`](#operator-std-span-const-element-t-simd-element-countconversion)
- [`operator std::span<element_t, simd::element_count>`](#operator-std-span-element-t-simd-element-countconversion)
- [`operator vector_t`](#operator-vector-t)
- [`operator-`](#operator-minus)
- [`operator-=`](#operator-minus-assign)
- [`operator[]`](#operator-subscript)
- [`operator*`](#operator-multiply)
- [`operator*=`](#operator-multiply-assign)
- [`operator/`](#operator-divide)
- [`operator/=`](#operator-divide-assign)
- [`operator&`](#operator-and)
- [`operator&=`](#operator-and-assign)
- [`operator%`](#operator-modulus)
- [`operator%=`](#operator-modulus-assign)
- [`operator^`](#operator-xor)
- [`operator^=`](#operator-xor-assign)
- [`operator+`](#operator-plus)
- [`operator+=`](#operator-plus-assign)
- [`operator<`](#operator-less)
- [`operator<<`](#operator-shift-left)
- [`operator<<=`](#operator-shift-left-assign)
- [`operator<=`](#operator-less-equal)
- [`operator=`](#operator-assign)
- [`operator==`](#operator-equal)
- [`operator>`](#operator-greater)
- [`operator>=`](#operator-greater-equal)
- [`operator>>`](#operator-shift-right)
- [`operator>>=`](#operator-shift-right-assign)
- [`operator|`](#operator-or)
- [`operator|=`](#operator-or-assign)
- [`operator~`](#operator-not)
- [`sign`](#sign)
- [`SimdVector`](#simdvector)
- [`size`](#size)
- [`SimdVector` widening constructor](#simdvector-widening)
- [`sqrt`](#sqrt)
- [`subtract_horizontal`](#subtract-horizontal)
- [`subtract_horizontal_saturated`](#subtract-horizontal-saturated)
- [`subtract_saturated`](#subtract-saturated)
- [`sum_absolute_byte_differences`](#sum-absolute-byte-differences)
- [`toArray`](#toarray)
- [`w`](#w)
- [`x`](#x)
- [`y`](#y)
- [`z`](#z)
- [Related types and constants](#related-types-and-constants)

<a id="overview"></a>
## Overview

Include `<SimdLib/SimdVector.h>`. Overloads with the same name are collected in one subsection; every public overload is listed below.

<a id="example-setup"></a>
## Example alias

```cpp
#include <SimdLib/SimdLib.h>

using Vector3 = SimdLib::SimdVector<float, 3>;
```
<a id="destructor-simdvector"></a>
## `~SimdVector`

Destroys the SIMD vector.

Signatures:

```cpp
~SimdVector() = default
```

Example:

```cpp
{
  const Vector3 temporary{1.0F, 2.0F, 3.0F};
} // => temporary is destroyed at the closing brace
```

<a id="abs"></a>
## `abs`

Returns a SIMD register containing the absolute value of each element.

Signatures:

```cpp
vector_t abs() const
```

Example:

```cpp
Vector3{-1.0F, 2.0F, -3.0F}.abs(); // => {1.0F, 2.0F, 3.0F}
```

<a id="add-horizontal"></a>
## `add_horizontal`

Adds adjacent lane pairs within each 128-bit lane.

Signatures:

```cpp
auto add_horizontal(vector_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F}.add_horizontal(
    Vector3{4.0F, 5.0F, 6.0F}); // => register lanes are {3.0F, 3.0F, 9.0F, 6.0F}
```

<a id="add-horizontal-saturated"></a>
## `add_horizontal_saturated`

Adds adjacent lane pairs with saturation where the specialization supports it.

Signatures:

```cpp
auto add_horizontal_saturated(vector_t rhs) const
```

Example:

```cpp
using I16x4 = SimdLib::SimdVector<std::int16_t, 4>;
I16x4{30000, 10000, 200, 300}.add_horizontal_saturated(
    I16x4{1, 2, 3, 4}); // => register lanes begin {32767, 500, 0, 0, ...}
```

<a id="add-saturated"></a>
## `add_saturated`

Adds the two vectors together and clamps integer overflow to the underlying type's maximum value.

Signatures:

```cpp
vector_t add_saturated(vector_t rhs) const
vector_t add_saturated(element_t rhs) const
```

Example:

```cpp
using U8x3 = SimdLib::SimdVector<std::uint8_t, 3>;
U8x3{250, 10, 20}.add_saturated(U8x3{10, 20, 30}); // => {255, 30, 50}
```

<a id="add-subtract"></a>
## `add_subtract`

Alternates subtraction and addition across lanes for floating-point SIMD families.

Signatures:

```cpp
auto add_subtract(vector_t rhs) const
```

Example:

```cpp
Vector3{10.0F, 10.0F, 10.0F}.add_subtract(
    Vector3{1.0F, 2.0F, 3.0F}); // => low lanes are {9.0F, 12.0F, 7.0F}
```

<a id="all-equal"></a>
## `all_equal`

Returns true if all elements equal the corresponding element in the other vector.

Signatures:

```cpp
bool all_equal(vector_t rhs) const
```

Example:

```cpp
Vector3{2.0F, 2.0F, 2.0F}.all_equal(Vector3{2.0F, 2.0F, 2.0F}); // => true
```

<a id="all-greater"></a>
## `all_greater`

Returns true if all elements are greater than the corresponding element in the other vector.

Signatures:

```cpp
bool all_greater(vector_t rhs) const
```

Example:

```cpp
Vector3{4.0F, 5.0F, 6.0F}.all_greater(Vector3{1.0F, 2.0F, 3.0F}); // => true
```

<a id="all-greater-equal"></a>
## `all_greater_equal`

Returns true if all elements are greater than or equal to the corresponding element in the other vector.

Signatures:

```cpp
bool all_greater_equal(vector_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F}.all_greater_equal(Vector3{1.0F, 1.0F, 3.0F}); // => true
```

<a id="all-less"></a>
## `all_less`

Returns true if all elements are less than the corresponding element in the other vector.

Signatures:

```cpp
bool all_less(vector_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F}.all_less(Vector3{4.0F, 5.0F, 6.0F}); // => true
```

<a id="all-less-equal"></a>
## `all_less_equal`

Returns true if all elements are less than or equal to the corresponding element in the other vector.

Signatures:

```cpp
bool all_less_equal(vector_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F}.all_less_equal(Vector3{1.0F, 3.0F, 3.0F}); // => true
```

<a id="any-equal"></a>
## `any_equal`

Returns true if any element equals the corresponding element in the other vector.

Signatures:

```cpp
bool any_equal(vector_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F}.any_equal(Vector3{9.0F, 2.0F, 8.0F}); // => true
```

<a id="any-greater"></a>
## `any_greater`

Returns true if any element is greater than the corresponding element in the other vector.

Signatures:

```cpp
bool any_greater(vector_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 5.0F, 2.0F}.any_greater(Vector3{3.0F, 4.0F, 6.0F}); // => true
```

<a id="any-greater-equal"></a>
## `any_greater_equal`

Returns true if any element is greater than or equal to the corresponding element in the other vector.

Signatures:

```cpp
bool any_greater_equal(vector_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F}.any_greater_equal(Vector3{4.0F, 2.0F, 5.0F}); // => true
```

<a id="any-less"></a>
## `any_less`

Returns true if any element is less than the corresponding element in the other vector.

Signatures:

```cpp
bool any_less(vector_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 5.0F, 6.0F}.any_less(Vector3{2.0F, 4.0F, 3.0F}); // => true
```

<a id="any-less-equal"></a>
## `any_less_equal`

Returns true if any element is less than or equal to the corresponding element in the other vector.

Signatures:

```cpp
bool any_less_equal(vector_t rhs) const
```

Example:

```cpp
Vector3{5.0F, 2.0F, 6.0F}.any_less_equal(Vector3{4.0F, 2.0F, 3.0F}); // => true
```

<a id="area"></a>
## `area`

Computes the multiplicative inclusive extent between this vector and a minimum bound.

Signatures:

```cpp
template <class target_element_t = area_element_t> auto area(vector_t minInclusive) const
area_element_t area() const
```

Example:

```cpp
using U16x3 = SimdLib::SimdVector<std::uint16_t, 3>;
U16x3{2U, 3U, 4U}.area(); // => 24U
```

<a id="avg"></a>
## `avg`

Computes the average of corresponding lanes where the specialization supports it.

Signatures:

```cpp
auto avg(vector_t rhs) const
```

Example:

```cpp
using U8x3 = SimdLib::SimdVector<std::uint8_t, 3>;
U8x3{2U, 4U, 6U}.avg(U8x3{4U, 6U, 8U}); // => {3U, 5U, 7U}
```

<a id="clamp"></a>
## `clamp`

Clamps each element between the corresponding minimum and maximum elements.

Signatures:

```cpp
vector_t clamp(vector_t minValue, vector_t maxValue) const
vector_t clamp(element_t minValue, element_t maxValue) const
```

Example:

```cpp
Vector3{-2.0F, 5.0F, 12.0F}.clamp(
    Vector3{0.0F}, Vector3{10.0F}); // => {0.0F, 5.0F, 10.0F}
```

<a id="dot-product"></a>
## `dot_product`

Computes the scalar dot product over the vector's declared dimension count.

Signatures:

```cpp
element_t dot_product(vector_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F}.dot_product(
    Vector3{4.0F, 5.0F, 6.0F}); // => 32.0F in every result lane
```

<a id="getregister"></a>
## `getRegister`

Returns the underlying SIMD register.

Signatures:

```cpp
vector_t &getRegister()
vector_t getRegister() const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F}
    .getRegister(); // => register lanes {1.0F, 2.0F, 3.0F, 0.0F, ...}
```

<a id="getspan"></a>
## `getSpan`

Returns a span over the SIMD vector's elements.

Signatures:

```cpp
std::span<element_t, simd::element_count> getSpan()
std::span<const element_t, simd::element_count> getSpan() const
```

Example:

```cpp
Vector3 input{1.0F, 2.0F, 3.0F};
input.getSpan(); // => span over {1.0F, 2.0F, 3.0F}
```

<a id="gettuple"></a>
## `getTuple`

Returns a tuple containing the span view used by tuple-like integrations.

Signatures:

```cpp
auto getTuple() const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F}.getTuple(); // => tuple {1.0F, 2.0F, 3.0F}
```

<a id="magnitude"></a>
## `magnitude`

Computes the per-128-bit-lane magnitude when the underlying Simd specialization supports it.

Signatures:

```cpp
auto magnitude() const
```

Example:

```cpp
Vector3{3.0F, 4.0F, 0.0F}.magnitude(); // => 5.0F
```

<a id="max"></a>
## `max`

Returns a SIMD register containing the per-element maxima.

Signatures:

```cpp
vector_t max(vector_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 5.0F, 3.0F}.max(Vector3{2.0F, 4.0F, 6.0F}); // => {2.0F, 5.0F, 6.0F}
```

<a id="max-position"></a>
## `max_position`

Returns the first index of the maximum value in the vector.

Signatures:

```cpp
std::size_t max_position() const
```

Example:

```cpp
using U16x3 = SimdLib::SimdVector<std::uint16_t, 3>;
U16x3{4U, 1U, 3U}.max_position(); // => 0
```

<a id="min"></a>
## `min`

Returns a SIMD register containing the per-element minima.

Signatures:

```cpp
vector_t min(vector_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 5.0F, 3.0F}.min(Vector3{2.0F, 4.0F, 6.0F}); // => {1.0F, 4.0F, 3.0F}
```

<a id="min-position"></a>
## `min_position`

Returns the first index of the minimum value in the vector.

Signatures:

```cpp
std::size_t min_position() const
```

Example:

```cpp
using U16x3 = SimdLib::SimdVector<std::uint16_t, 3>;
U16x3{4U, 1U, 3U}.min_position(); // => 1
```

<a id="multi-sum-absolute-byte-differences"></a>
## `multi_sum_absolute_byte_differences`

Computes byte-window absolute-difference sums selected by a compile-time immediate mask.

Signatures:

```cpp
template <int imm8> auto multi_sum_absolute_byte_differences(vector_t rhs) const
```

Example:

```cpp
using U8x16 = SimdLib::uint8x16;
U8x16{9}.multi_sum_absolute_byte_differences<0>(U8x16{
    4}); // => every selected 16-bit result lane is 20
```

<a id="multiply-add"></a>
## `multiply_add`

Computes a fused multiply-add where the specialization supports it.

Signatures:

```cpp
auto multiply_add(vector_t rhs, vector_t addend) const
```

Example:

```cpp
Vector3{2.0F, 3.0F, 4.0F}.multiply_add(
    Vector3{5.0F, 6.0F, 7.0F}, Vector3{1.0F}); // => {11.0F, 19.0F, 29.0F}
```

<a id="multiply-add-adjacent"></a>
## `multiply_add_adjacent`

Multiplies adjacent lane pairs and accumulates them into promoted result lanes.

Signatures:

```cpp
auto multiply_add_adjacent(vector_t rhs) const
```

Example:

```cpp
using I16x4 = SimdLib::SimdVector<std::int16_t, 4>;
I16x4{1, 2, 3, 4}.multiply_add_adjacent(
    I16x4{5, 6, 7, 8}); // => low promoted lanes are {17, 53}
```

<a id="multiply-add-unsigned-signed-bytes"></a>
## `multiply_add_unsigned_signed_bytes`

Multiplies raw register bytes as unsigned and signed pairs and accumulates them into signed 16-bit lanes.

Signatures:

```cpp
auto multiply_add_unsigned_signed_bytes(vector_t rhs) const
```

Example:

```cpp
using U8x16 = SimdLib::uint8x16;
U8x16{2}.multiply_add_unsigned_signed_bytes(
    U8x16{3}); // => every signed 16-bit result lane is 12
```

<a id="multiply-saturated"></a>
## `multiply_saturated`

Multiplies the two vectors and clamps integer overflow to the underlying type's maximum value.

Signatures:

```cpp
vector_t multiply_saturated(vector_t rhs) const
vector_t multiply_saturated(element_t rhs) const
```

Example:

```cpp
using U16x3 = SimdLib::SimdVector<std::uint16_t, 3>;
U16x3{40000U, 5U, 2U}.multiply_saturated(U16x3{2U, 6U, 4U}); // => {65535U, 30U, 8U}
```

<a id="normalize"></a>
## `normalize`

Normalizes floating-point lanes using the Simd API's lane-local length semantics.

Signatures:

```cpp
auto normalize() const
```

Example:

```cpp
Vector3{3.0F, 4.0F, 0.0F}.normalize(); // => {0.6F, 0.8F, 0.0F}
```

<a id="operator-std-array-element-t-simd-element-countconversion"></a>
## `operator std::array<element_t, simd::element_count>`

Converts the wrapped SIMD register to a fixed array.

Signatures:

```cpp
explicit operator std::array<element_t, simd::element_count>() const
```

Example:

```cpp
static_cast<std::array<float, Vector3::simd::element_count>>(Vector3{
    1.0F, 2.0F, 3.0F}); // => {1.0F, 2.0F, 3.0F, 0.0F, ...}
```

<a id="operator-std-span-const-element-t-simd-element-countconversion"></a>
## `operator std::span<const element_t, simd::element_count>`

Returns a readonly span view over the underlying register storage.

Signatures:

```cpp
operator std::span<const element_t, simd::element_count>() const
```

Example:

```cpp
const Vector3 input{1.0F, 2.0F, 3.0F};
static_cast<std::span<const float, Vector3::simd::element_count>>(
    input); // => read-only view beginning {1.0F, 2.0F, 3.0F}
```

<a id="operator-std-span-element-t-simd-element-countconversion"></a>
## `operator std::span<element_t, simd::element_count>`

Returns a mutable span view over the underlying register storage.

Signatures:

```cpp
operator std::span<element_t, simd::element_count>()
```

Example:

```cpp
Vector3 input{1.0F, 2.0F, 3.0F};
static_cast<std::span<float, Vector3::simd::element_count>>(
    input); // => mutable view beginning {1.0F, 2.0F, 3.0F}
```

<a id="operator-vector-t"></a>
## `operator vector_t`

Implicitly converts this wrapper to the underlying SIMD register.

Signatures:

```cpp
operator vector_t() const
```

Example:

```cpp
static_cast<Vector3::vector_t>(Vector3{
    1.0F, 2.0F, 3.0F}); // => register lanes {1.0F, 2.0F, 3.0F, 0.0F, ...}
```

<a id="operator-minus"></a>
## `operator-`

Subtracts another register lane-wise from this vector.

Signatures:

```cpp
vector_t operator-(vector_t rhs) const
vector_t operator-(element_t rhs) const
vector_t operator-() const
```

Example:

```cpp
Vector3{4.0F, 5.0F, 6.0F} - Vector3{1.0F, 2.0F, 3.0F}; // => {3.0F, 3.0F, 3.0F}
```

<a id="operator-minus-assign"></a>
## `operator-=`

Subtracts another register lane-wise from this vector in place.

Signatures:

```cpp
SimdVector &operator-=(vector_t rhs)
SimdVector &operator-=(element_t rhs)
```

Example:

```cpp
Vector3 result{7.0F, 8.0F, 9.0F};
result -= Vector3{1.0F, 2.0F, 3.0F}; // => {6.0F, 6.0F, 6.0F}
```

<a id="operator-subscript"></a>
## `operator[]`

Returns the element at the requested lane index.

Signatures:

```cpp
element_t operator[](std::size_t index) const
element_t &operator[](std::size_t index)
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F}[1]; // => 2.0F
```

<a id="operator-multiply"></a>
## `operator*`

Multiplies this vector by another register lane-wise.

Signatures:

```cpp
vector_t operator*(vector_t rhs) const
vector_t operator*(element_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F} * 2.0F; // => {2.0F, 4.0F, 6.0F}
```

<a id="operator-multiply-assign"></a>
## `operator*=`

Multiplies this vector by another register lane-wise in place.

Signatures:

```cpp
SimdVector &operator*=(vector_t rhs)
SimdVector &operator*=(element_t rhs)
```

Example:

```cpp
Vector3 result{1.0F, 2.0F, 3.0F};
result *= Vector3{4.0F, 5.0F, 6.0F}; // => {4.0F, 10.0F, 18.0F}
```

<a id="operator-divide"></a>
## `operator/`

Divides this vector by another register lane-wise.

Signatures:

```cpp
vector_t operator/(vector_t rhs) const
vector_t operator/(element_t rhs) const
```

Example:

```cpp
Vector3{2.0F, 4.0F, 6.0F} / 2.0F; // => {1.0F, 2.0F, 3.0F}
```

<a id="operator-divide-assign"></a>
## `operator/=`

Divides this vector by another register lane-wise in place.

Signatures:

```cpp
SimdVector &operator/=(vector_t rhs)
SimdVector &operator/=(element_t rhs)
```

Example:

```cpp
Vector3 result{8.0F, 12.0F, 18.0F};
result /= Vector3{2.0F, 3.0F, 6.0F}; // => {4.0F, 4.0F, 3.0F}
```

<a id="operator-and"></a>
## `operator&`

Computes a lane-wise bitwise AND with another register.

Signatures:

```cpp
SimdVector operator&(vector_t rhs) const
```

Example:

```cpp
using U32x3 = SimdLib::SimdVector<std::uint32_t, 3>;
U32x3{0b1100U, 0b1010U, 0b1111U} &
    U32x3{0b1010U, 0b0110U, 0b0101U}; // => {0b1000U, 0b0010U, 0b0101U}
```

<a id="operator-and-assign"></a>
## `operator&=`

Applies a lane-wise bitwise AND with another register in place.

Signatures:

```cpp
SimdVector &operator&=(vector_t rhs)
```

Example:

```cpp
using U32x3 = SimdLib::SimdVector<std::uint32_t, 3>;
U32x3 result{0b1100U, 0b1010U, 0b1111U};
result &= U32x3{0b1010U, 0b0110U, 0b0101U}; // => {0b1000U, 0b0010U, 0b0101U}
```

<a id="operator-modulus"></a>
## `operator%`

Computes the lane-wise remainder with another register.

Signatures:

```cpp
vector_t operator%(vector_t rhs) const
vector_t operator%(element_t rhs) const
```

Example:

```cpp
using I32x3 = SimdLib::SimdVector<std::int32_t, 3>;
I32x3{7, 8, 9} % 4; // => {3, 0, 1}
```

<a id="operator-modulus-assign"></a>
## `operator%=`

Computes the lane-wise remainder with another register in place.

Signatures:

```cpp
SimdVector &operator%=(vector_t rhs)
SimdVector &operator%=(element_t rhs)
```

Example:

```cpp
using I32x3 = SimdLib::SimdVector<std::int32_t, 3>;
I32x3 result{7, 8, 9};
result %= I32x3{4, 4, 4}; // => {3, 0, 1}
```

<a id="operator-xor"></a>
## `operator^`

Computes a lane-wise bitwise XOR with another register.

Signatures:

```cpp
SimdVector operator^(vector_t rhs) const
```

Example:

```cpp
using U32x3 = SimdLib::SimdVector<std::uint32_t, 3>;
U32x3{0b1100U, 0b1010U, 0b1111U} ^
    U32x3 { 0b1010U, 0b0110U, 0b0101U }; // => {0b0110U, 0b1100U, 0b1010U}
```

<a id="operator-xor-assign"></a>
## `operator^=`

Applies a lane-wise bitwise XOR with another register in place.

Signatures:

```cpp
SimdVector &operator^=(vector_t rhs)
```

Example:

```cpp
using U32x3 = SimdLib::SimdVector<std::uint32_t, 3>;
U32x3 result{0b1100U, 0b1010U, 0b1111U};
result ^= U32x3{0b1010U, 0b0110U, 0b0101U}; // => {0b0110U, 0b1100U, 0b1010U}
```

<a id="operator-plus"></a>
## `operator+`

Adds another register lane-wise to this vector.

Signatures:

```cpp
vector_t operator+(vector_t rhs) const
vector_t operator+(element_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F} + Vector3{4.0F, 5.0F, 6.0F}; // => {5.0F, 7.0F, 9.0F}
```

<a id="operator-plus-assign"></a>
## `operator+=`

Adds another register lane-wise into this vector.

Signatures:

```cpp
SimdVector &operator+=(vector_t rhs)
SimdVector &operator+=(element_t rhs)
```

Example:

```cpp
Vector3 result{1.0F, 2.0F, 3.0F};
result += Vector3{4.0F, 5.0F, 6.0F}; // => {5.0F, 7.0F, 9.0F}
```

<a id="operator-less"></a>
## `operator<`

Returns true if all elements are less than the corresponding element in the other vector.

Signatures:

```cpp
bool operator<(vector_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F} < Vector3{2.0F, 3.0F, 4.0F}; // => true
```

<a id="operator-shift-left"></a>
## `operator<<`

Shifts each integer lane left by the specified amount.

Signatures:

```cpp
SimdVector operator<<(int shift) const
```

Example:

```cpp
using U32x3 = SimdLib::SimdVector<std::uint32_t, 3>;
U32x3{1U, 2U, 3U} << 1; // => {2U, 4U, 6U}
```

<a id="operator-shift-left-assign"></a>
## `operator<<=`

Shifts each integer lane left in place.

Signatures:

```cpp
SimdVector &operator<<=(int shift)
```

Example:

```cpp
using U32x3 = SimdLib::SimdVector<std::uint32_t, 3>;
U32x3 result{1U, 2U, 3U};
result <<= 1; // => {2U, 4U, 6U}
```

<a id="operator-less-equal"></a>
## `operator<=`

Returns true if all elements are less than or equal to the corresponding element in the other vector.

Signatures:

```cpp
bool operator<=(vector_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F} <= Vector3{1.0F, 3.0F, 3.0F}; // => true
```

<a id="operator-assign"></a>
## `operator=`

Replaces this SIMD vector with a copy of another SIMD vector.

Signatures:

```cpp
SimdVector &operator=(const SimdVector &other) = default
SimdVector &operator=(SimdVector &&other) = default
```

Example:

```cpp
Vector3 result{};
result = Vector3{1.0F, 2.0F, 3.0F}; // => result is {1.0F, 2.0F, 3.0F}
```

<a id="operator-equal"></a>
## `operator==`

Returns true if all elements equal the corresponding element in the other vector.

Signatures:

```cpp
bool operator==(vector_t rhs) const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F} == Vector3{1.0F, 2.0F, 3.0F}; // => true
```

<a id="operator-greater"></a>
## `operator>`

Returns true if all elements are greater than the corresponding element in the other vector.

Signatures:

```cpp
bool operator>(vector_t rhs) const
```

Example:

```cpp
Vector3{4.0F, 5.0F, 6.0F} > Vector3{1.0F, 2.0F, 3.0F}; // => true
```

<a id="operator-greater-equal"></a>
## `operator>=`

Returns true if all elements are greater than or equal to the corresponding element in the other vector.

Signatures:

```cpp
bool operator>=(vector_t rhs) const
```

Example:

```cpp
Vector3{4.0F, 5.0F, 6.0F} >= Vector3{4.0F, 2.0F, 6.0F}; // => true
```

<a id="operator-shift-right"></a>
## `operator>>`

Shifts each integer lane right by the specified amount.

Signatures:

```cpp
SimdVector operator>>(int shift) const
```

Example:

```cpp
using U32x3 = SimdLib::SimdVector<std::uint32_t, 3>;
U32x3{2U, 4U, 6U} >> 1; // => {1U, 2U, 3U}
```

<a id="operator-shift-right-assign"></a>
## `operator>>=`

Shifts each integer lane right in place.

Signatures:

```cpp
SimdVector &operator>>=(int shift)
```

Example:

```cpp
using U32x3 = SimdLib::SimdVector<std::uint32_t, 3>;
U32x3 result{2U, 4U, 6U};
result >>= 1; // => {1U, 2U, 3U}
```

<a id="operator-or"></a>
## `operator|`

Computes a lane-wise bitwise OR with another register.

Signatures:

```cpp
SimdVector operator|(vector_t rhs) const
```

Example:

```cpp
using U32x3 = SimdLib::SimdVector<std::uint32_t, 3>;
U32x3{0b1100U, 0b1010U, 0b1111U} |
    U32x3{0b1010U, 0b0110U, 0b0101U}; // => {0b1110U, 0b1110U, 0b1111U}
```

<a id="operator-or-assign"></a>
## `operator|=`

Applies a lane-wise bitwise OR with another register in place.

Signatures:

```cpp
SimdVector &operator|=(vector_t rhs)
```

Example:

```cpp
using U32x3 = SimdLib::SimdVector<std::uint32_t, 3>;
U32x3 result{0b1100U, 0b1010U, 0b1111U};
result |= U32x3{0b1010U, 0b0110U, 0b0101U}; // => {0b1110U, 0b1110U, 0b1111U}
```

<a id="operator-not"></a>
## `operator~`

Inverts every bit in the underlying register.

Signatures:

```cpp
SimdVector operator~() const
```

Example:

```cpp
using U32x3 = SimdLib::SimdVector<std::uint32_t, 3>;
~U32x3{0U}; // => {0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU}
```

<a id="sign"></a>
## `sign`

Returns the sign of each element as -1, 0, or 1, or 0 and 1 for unsigned types.

Signatures:

```cpp
vector_t sign() const
```

Example:

```cpp
Vector3{-1.0F, 0.0F, 3.0F}.sign(); // => {-1.0F, 0.0F, 1.0F}
```

<a id="simdvector"></a>
## `SimdVector`

Copies another SIMD vector.

Signatures:

```cpp
SimdVector(const SimdVector &other) = default
SimdVector(SimdVector &&other) = default
SimdVector()
SimdVector(vector_t data)
explicit SimdVector(element_t v)
explicit SimdVector(std::span<element_t, simd::element_count> data)
explicit SimdVector(std::span<const element_t, simd::element_count> data)
explicit SimdVector(std::span<element_t, element_count> data)
explicit SimdVector(std::span<const element_t, element_count> data)
explicit SimdVector(const std::array<element_t, simd::element_count> &data)
explicit SimdVector(const std::array<element_t, element_count> &data)
template <std::convertible_to<element_t>... Args>
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F}; // => {1.0F, 2.0F, 3.0F}
```

<a id="size"></a>
## `size`

Computes the inclusive per-lane extent between this vector and a minimum bound.

Signatures:

```cpp
template <class target_element_t = area_element_t> auto size(vector_t minInclusive) const
```

Example:

```cpp
using U16x3 = SimdLib::SimdVector<std::uint16_t, 3>;
U16x3{5U, 7U, 9U}.size(U16x3{1U, 2U, 3U}.getRegister()); // => {5U, 6U, 7U}
```

<a id="simdvector-widening"></a>
## `SimdVector` widening constructor

Constructs a new SIMD vector by widening another SIMD vector with the same logical element count.

Signatures:

```cpp
template <class source_t>
explicit SimdVector(const SimdVector<source_t, element_count> &other)
```

Example:

```cpp
SimdLib::SimdVector<std::uint32_t, 3>{
    SimdLib::SimdVector<std::uint16_t, 3>{1U, 2U, 3U}}; // => {1U, 2U, 3U}
```

<a id="sqrt"></a>
## `sqrt`

Computes the square root of each element.

Signatures:

```cpp
auto sqrt() const
```

Example:

```cpp
Vector3{1.0F, 4.0F, 9.0F}.sqrt(); // => {1.0F, 2.0F, 3.0F}
```

<a id="subtract-horizontal"></a>
## `subtract_horizontal`

Subtracts adjacent lane pairs within each 128-bit lane.

Signatures:

```cpp
auto subtract_horizontal(vector_t rhs) const
```

Example:

```cpp
Vector3{5.0F, 2.0F, 9.0F}.subtract_horizontal(
    Vector3{8.0F, 3.0F, 6.0F}); // => register lanes are {3.0F, 9.0F, 5.0F, 6.0F}
```

<a id="subtract-horizontal-saturated"></a>
## `subtract_horizontal_saturated`

Subtracts adjacent lane pairs with saturation where the specialization supports it.

Signatures:

```cpp
auto subtract_horizontal_saturated(vector_t rhs) const
```

Example:

```cpp
using I16x4 = SimdLib::SimdVector<std::int16_t, 4>;
I16x4{30000, -10000, -30000, 10000}.subtract_horizontal_saturated(
    I16x4{1, 2, 3, 4}); // => register lanes begin {32767, -32768, 0, 0, ...}
```

<a id="subtract-saturated"></a>
## `subtract_saturated`

Subtracts the two vectors and clamps integer overflow to the underlying type's maximum value.

Signatures:

```cpp
vector_t subtract_saturated(vector_t rhs) const
vector_t subtract_saturated(element_t rhs) const
```

Example:

```cpp
using U8x3 = SimdLib::SimdVector<std::uint8_t, 3>;
U8x3{5, 20, 30}.subtract_saturated(U8x3{10, 7, 40}); // => {0, 13, 0}
```

<a id="sum-absolute-byte-differences"></a>
## `sum_absolute_byte_differences`

Computes byte-wise absolute differences and accumulates them into 64-bit result lanes.

Signatures:

```cpp
auto sum_absolute_byte_differences(vector_t rhs) const
```

Example:

```cpp
using U8x16 = SimdLib::uint8x16;
U8x16{9}.sum_absolute_byte_differences(U8x16{4}); // => both 64-bit result lanes are 40
```

<a id="toarray"></a>
## `toArray`

Converts the SIMD vector to an array of elements.

Signatures:

```cpp
std::array<element_t, simd::element_count> toArray() const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F}.toArray(); // => {1.0F, 2.0F, 3.0F, 0.0F, ...}
```

<a id="w"></a>
## `w`

Returns a mutable reference to the fourth element.

Signatures:

```cpp
element_t &w()
element_t w() const
```

Example:

```cpp
SimdLib::SimdVector<float, 4>{1.0F, 2.0F, 3.0F, 4.0F}.w(); // => 4.0F
```

<a id="x"></a>
## `x`

Returns a mutable reference to the first element.

Signatures:

```cpp
element_t &x()
element_t x() const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F}.x(); // => 1.0F
```

<a id="y"></a>
## `y`

Returns a mutable reference to the second element.

Signatures:

```cpp
element_t &y()
element_t y() const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F}.y(); // => 2.0F
```

<a id="z"></a>
## `z`

Returns a mutable reference to the third element.

Signatures:

```cpp
element_t &z()
element_t z() const
```

Example:

```cpp
Vector3{1.0F, 2.0F, 3.0F}.z(); // => 3.0F
```

<a id="related-types-and-constants"></a>
## Related types and constants

The header provides `VectorInt8`, `VectorUInt8`, `VectorInt16`, `VectorUInt16`, `VectorInt32`, `VectorUInt32`, `VectorInt64`, and `VectorUInt64`, plus register-sized aliases such as `uint8x16`, `uint32x8`, `int16x8`, and `int64x4`. Use `SimdVector<T, N>` directly for position-like dimensions such as two, three, or four.
