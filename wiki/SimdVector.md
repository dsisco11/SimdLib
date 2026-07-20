# SimdVector

`SimdVector<element_t, element_count>` is the friendly, position-vector-like class. It stores a small fixed number of numeric elements and offers arithmetic operators, vector math, comparisons, and named coordinates.

## Contents

- [Overview](#overview)
- [Example setup](#example-setup)
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
- [`sizeof`](#sizeof)
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
## Example setup

```cpp
#include <SimdLib/SimdLib.h>

using Vector3 = SimdLib::SimdVector<float, 3>;
Vector3 position{1.0F, 2.0F, 3.0F};
Vector3 other{4.0F, 5.0F, 6.0F};
Vector3 minimum{-10.0F};
Vector3 maximum{10.0F};
Vector3 scale{2.0F};
```

<a id="destructor-simdvector"></a>
## `~SimdVector`

Destroys the SIMD vector.

Signatures:

```cpp
~SimdVector() = default;
```

Example:

```cpp
// Destruction is automatic when the vector leaves scope.
```

<a id="abs"></a>
## `abs`

Returns a SIMD register containing the absolute value of each element.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE vector_t VECTORCALL abs() const noexcept requires requires(vector_t value)
```

Example:

```cpp
const auto result = position.abs();
```

<a id="add-horizontal"></a>
## `add_horizontal`

Adds adjacent lane pairs within each 128-bit lane.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE auto VECTORCALL add_horizontal(vector_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
```

Example:

```cpp
const auto result = position.add_horizontal(other);
```

<a id="add-horizontal-saturated"></a>
## `add_horizontal_saturated`

Adds adjacent lane pairs with saturation where the specialization supports it.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE auto VECTORCALL add_horizontal_saturated(vector_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
```

Example:

```cpp
const auto result = position.add_horizontal_saturated(other);
```

<a id="add-saturated"></a>
## `add_saturated`

Adds the two vectors together and clamps integer overflow to the underlying type's maximum value.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE vector_t VECTORCALL add_saturated(vector_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
SIMDLIB_FORCE_INLINE vector_t VECTORCALL add_saturated(element_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
```

Example:

```cpp
const auto result = position.add_saturated(other);
```

<a id="add-subtract"></a>
## `add_subtract`

Alternates subtraction and addition across lanes for floating-point SIMD families.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE auto VECTORCALL add_subtract(vector_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
```

Example:

```cpp
const auto result = position.add_subtract(other);
```

<a id="all-equal"></a>
## `all_equal`

Returns true if all elements equal the corresponding element in the other vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL all_equal(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position.all_equal(other);
```

<a id="all-greater"></a>
## `all_greater`

Returns true if all elements are greater than the corresponding element in the other vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL all_greater(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position.all_greater(other);
```

<a id="all-greater-equal"></a>
## `all_greater_equal`

Returns true if all elements are greater than or equal to the corresponding element in the other vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL all_greater_equal(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position.all_greater_equal(other);
```

<a id="all-less"></a>
## `all_less`

Returns true if all elements are less than the corresponding element in the other vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL all_less(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position.all_less(other);
```

<a id="all-less-equal"></a>
## `all_less_equal`

Returns true if all elements are less than or equal to the corresponding element in the other vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL all_less_equal(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position.all_less_equal(other);
```

<a id="any-equal"></a>
## `any_equal`

Returns true if any element equals the corresponding element in the other vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL any_equal(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position.any_equal(other);
```

<a id="any-greater"></a>
## `any_greater`

Returns true if any element is greater than the corresponding element in the other vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL any_greater(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position.any_greater(other);
```

<a id="any-greater-equal"></a>
## `any_greater_equal`

Returns true if any element is greater than or equal to the corresponding element in the other vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL any_greater_equal(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position.any_greater_equal(other);
```

<a id="any-less"></a>
## `any_less`

Returns true if any element is less than the corresponding element in the other vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL any_less(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position.any_less(other);
```

<a id="any-less-equal"></a>
## `any_less_equal`

Returns true if any element is less than or equal to the corresponding element in the other vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL any_less_equal(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position.any_less_equal(other);
```

<a id="area"></a>
## `area`

Computes the multiplicative inclusive extent between this vector and a minimum bound.

Signatures:

```cpp
template <class target_element_t = area_element_t> SIMDLIB_FORCE_INLINE auto VECTORCALL area(vector_t minInclusive) const noexcept requires(std::is_integral_v<element_t> && std::is_integral_v<target_element_t> && sizeof(target_element_t) >= sizeof(element_t))
SIMDLIB_FORCE_INLINE area_element_t VECTORCALL area() const noexcept requires std::is_integral_v<element_t>
```

Example:

```cpp
const auto result = position.area();
```

<a id="avg"></a>
## `avg`

Computes the average of corresponding lanes where the specialization supports it.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE auto VECTORCALL avg(vector_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
```

Example:

```cpp
const auto result = position.avg(other);
```

<a id="clamp"></a>
## `clamp`

Clamps each element between the corresponding minimum and maximum elements.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE vector_t VECTORCALL clamp(vector_t minValue, vector_t maxValue) const noexcept requires requires(vector_t value)
SIMDLIB_FORCE_INLINE vector_t VECTORCALL clamp(element_t minValue, element_t maxValue) const noexcept requires requires(vector_t value)
```

Example:

```cpp
const auto result = position.clamp(other, minimum);
```

<a id="dot-product"></a>
## `dot_product`

Computes the scalar dot product over the vector's declared dimension count.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE element_t VECTORCALL dot_product(vector_t rhs) const noexcept requires(std::is_floating_point_v<element_t> && requires(vector_t lhsValue, vector_t rhsValue)
```

Example:

```cpp
const auto result = position.dot_product(other);
```

<a id="getregister"></a>
## `getRegister`

Returns the underlying SIMD register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE vector_t &VECTORCALL getRegister() noexcept
SIMDLIB_FORCE_INLINE vector_t VECTORCALL getRegister() const noexcept
```

Example:

```cpp
const auto result = position.getRegister();
```

<a id="getspan"></a>
## `getSpan`

Returns a span over the SIMD vector's elements.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE std::span<element_t, simd::element_count> getSpan() noexcept
SIMDLIB_FORCE_INLINE std::span<const element_t, simd::element_count> getSpan() const noexcept
```

Example:

```cpp
const auto result = position.getSpan();
```

<a id="gettuple"></a>
## `getTuple`

Returns a tuple containing the span view used by tuple-like integrations.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr auto getTuple() const noexcept
```

Example:

```cpp
const auto result = position.getTuple();
```

<a id="magnitude"></a>
## `magnitude`

Computes the per-128-bit-lane magnitude when the underlying Simd specialization supports it.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE auto VECTORCALL magnitude() const noexcept requires requires(vector_t value)
```

Example:

```cpp
const auto result = position.magnitude();
```

<a id="max"></a>
## `max`

Returns a SIMD register containing the per-element maxima.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE vector_t VECTORCALL max(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position.max(other);
```

<a id="max-position"></a>
## `max_position`

Returns the first index of the maximum value in the vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE std::size_t VECTORCALL max_position() const noexcept requires requires(vector_t value)
```

Example:

```cpp
const auto result = position.max_position();
```

<a id="min"></a>
## `min`

Returns a SIMD register containing the per-element minima.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE vector_t VECTORCALL min(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position.min(other);
```

<a id="min-position"></a>
## `min_position`

Returns the first index of the minimum value in the vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE std::size_t VECTORCALL min_position() const noexcept requires requires(vector_t value)
```

Example:

```cpp
const auto result = position.min_position();
```

<a id="multi-sum-absolute-byte-differences"></a>
## `multi_sum_absolute_byte_differences`

Computes byte-window absolute-difference sums selected by a compile-time immediate mask.

Signatures:

```cpp
template <int imm8> SIMDLIB_FORCE_INLINE auto VECTORCALL multi_sum_absolute_byte_differences(vector_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
```

Example:

```cpp
const auto result = position.multi_sum_absolute_byte_differences(other);
```

<a id="multiply-add"></a>
## `multiply_add`

Computes a fused multiply-add where the specialization supports it.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE auto VECTORCALL multiply_add(vector_t rhs, vector_t addend) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue, vector_t addValue)
```

Example:

```cpp
const auto result = position.multiply_add(other, minimum);
```

<a id="multiply-add-adjacent"></a>
## `multiply_add_adjacent`

Multiplies adjacent lane pairs and accumulates them into promoted result lanes.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE auto VECTORCALL multiply_add_adjacent(vector_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
```

Example:

```cpp
const auto result = position.multiply_add_adjacent(other);
```

<a id="multiply-add-unsigned-signed-bytes"></a>
## `multiply_add_unsigned_signed_bytes`

Multiplies raw register bytes as unsigned and signed pairs and accumulates them into signed 16-bit lanes.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE auto VECTORCALL multiply_add_unsigned_signed_bytes(vector_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
```

Example:

```cpp
const auto result = position.multiply_add_unsigned_signed_bytes(other);
```

<a id="multiply-saturated"></a>
## `multiply_saturated`

Multiplies the two vectors and clamps integer overflow to the underlying type's maximum value.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE vector_t VECTORCALL multiply_saturated(vector_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
SIMDLIB_FORCE_INLINE vector_t VECTORCALL multiply_saturated(element_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
```

Example:

```cpp
const auto result = position.multiply_saturated(other);
```

<a id="normalize"></a>
## `normalize`

Normalizes floating-point lanes using the Simd API's lane-local length semantics.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE auto VECTORCALL normalize() const noexcept requires requires(vector_t value)
```

Example:

```cpp
const auto result = position.normalize();
```

<a id="operator-std-array-element-t-simd-element-countconversion"></a>
## `operator std::array<element_t, simd::element_count>`

Converts the wrapped SIMD register to a fixed array.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr explicit operator std::array<element_t, simd::element_count>() const noexcept
```

Example:

```cpp
const auto converted = static_cast<Destination>(position);
```

<a id="operator-std-span-const-element-t-simd-element-countconversion"></a>
## `operator std::span<const element_t, simd::element_count>`

Returns a readonly span view over the underlying register storage.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE operator std::span<const element_t, simd::element_count>() const noexcept
```

Example:

```cpp
const auto converted = static_cast<Destination>(position);
```

<a id="operator-std-span-element-t-simd-element-countconversion"></a>
## `operator std::span<element_t, simd::element_count>`

Returns a mutable span view over the underlying register storage.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE operator std::span<element_t, simd::element_count>() noexcept
```

Example:

```cpp
const auto converted = static_cast<Destination>(position);
```

<a id="operator-vector-t"></a>
## `operator vector_t`

Implicitly converts this wrapper to the underlying SIMD register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE VECTORCALL operator vector_t() const noexcept
```

Example:

```cpp
const auto converted = static_cast<Destination>(position);
```

<a id="operator-minus"></a>
## `operator-`

Subtracts another register lane-wise from this vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator-(vector_t rhs) const noexcept
SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator-(element_t rhs) const noexcept
SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator-() const noexcept
```

Example:

```cpp
const auto result = position - other;
```

<a id="operator-minus-assign"></a>
## `operator-=`

Subtracts another register lane-wise from this vector in place.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator-=(vector_t rhs) noexcept
SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator-=(element_t rhs) noexcept
```

Example:

```cpp
position -= other;
```

<a id="operator-subscript"></a>
## `operator[]`

Returns the element at the requested lane index.

Signatures:

```cpp
constexpr inline element_t operator[](const std::size_t index) const noexcept
constexpr inline element_t &operator[](const std::size_t index) noexcept
```

Example:

```cpp
const float x = position[0];
```

<a id="operator-multiply"></a>
## `operator*`

Multiplies this vector by another register lane-wise.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator*(vector_t rhs) const noexcept
SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator*(element_t rhs) const noexcept
```

Example:

```cpp
const auto result = position * other;
```

<a id="operator-multiply-assign"></a>
## `operator*=`

Multiplies this vector by another register lane-wise in place.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator*=(vector_t rhs) noexcept
SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator*=(element_t rhs) noexcept
```

Example:

```cpp
position *= other;
```

<a id="operator-divide"></a>
## `operator/`

Divides this vector by another register lane-wise.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator/(vector_t rhs) const noexcept
SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator/(element_t rhs) const noexcept
```

Example:

```cpp
const auto result = position / other;
```

<a id="operator-divide-assign"></a>
## `operator/=`

Divides this vector by another register lane-wise in place.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator/=(vector_t rhs) noexcept
SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator/=(element_t rhs) noexcept
```

Example:

```cpp
position /= other;
```

<a id="operator-and"></a>
## `operator&`

Computes a lane-wise bitwise AND with another register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE SimdVector VECTORCALL operator&(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position & other;
```

<a id="operator-and-assign"></a>
## `operator&=`

Applies a lane-wise bitwise AND with another register in place.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator&=(vector_t rhs) noexcept
```

Example:

```cpp
position &= other;
```

<a id="operator-modulus"></a>
## `operator%`

Computes the lane-wise remainder with another register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator%(vector_t rhs) const noexcept
SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator%(element_t rhs) const noexcept
```

Example:

```cpp
const auto result = position % other;
```

<a id="operator-modulus-assign"></a>
## `operator%=`

Computes the lane-wise remainder with another register in place.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator%=(vector_t rhs) noexcept
SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator%=(element_t rhs) noexcept
```

Example:

```cpp
position %= other;
```

<a id="operator-xor"></a>
## `operator^`

Computes a lane-wise bitwise XOR with another register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE SimdVector VECTORCALL operator^(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position ^ other;
```

<a id="operator-xor-assign"></a>
## `operator^=`

Applies a lane-wise bitwise XOR with another register in place.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator^=(vector_t rhs) noexcept
```

Example:

```cpp
position ^= other;
```

<a id="operator-plus"></a>
## `operator+`

Adds another register lane-wise to this vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator+(vector_t rhs) const noexcept
SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator+(element_t rhs) const noexcept
```

Example:

```cpp
const auto result = position + other;
```

<a id="operator-plus-assign"></a>
## `operator+=`

Adds another register lane-wise into this vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator+=(vector_t rhs) noexcept
SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator+=(element_t rhs) noexcept
```

Example:

```cpp
position += other;
```

<a id="operator-less"></a>
## `operator<`

Returns true if all elements are less than the corresponding element in the other vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL operator<(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position < other;
```

<a id="operator-shift-left"></a>
## `operator<<`

Shifts each integer lane left by the specified amount.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr SimdVector VECTORCALL operator<<(int shift) const noexcept
```

Example:

```cpp
const auto result = position << other;
```

<a id="operator-shift-left-assign"></a>
## `operator<<=`

Shifts each integer lane left in place.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr SimdVector &VECTORCALL operator<<=(int shift) noexcept
```

Example:

```cpp
position <<= other;
```

<a id="operator-less-equal"></a>
## `operator<=`

Returns true if all elements are less than or equal to the corresponding element in the other vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL operator<=(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position <= other;
```

<a id="operator-assign"></a>
## `operator=`

Replaces this SIMD vector with a copy of another SIMD vector.

Signatures:

```cpp
SimdVector &operator=(const SimdVector &other) = default;
SimdVector &operator=(SimdVector &&other) = default;
```

Example:

```cpp
position = other;
```

<a id="operator-equal"></a>
## `operator==`

Returns true if all elements equal the corresponding element in the other vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL operator==(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position == other;
```

<a id="operator-greater"></a>
## `operator>`

Returns true if all elements are greater than the corresponding element in the other vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL operator>(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position > other;
```

<a id="operator-greater-equal"></a>
## `operator>=`

Returns true if all elements are greater than or equal to the corresponding element in the other vector.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL operator>=(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position >= other;
```

<a id="operator-shift-right"></a>
## `operator>>`

Shifts each integer lane right by the specified amount.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr SimdVector VECTORCALL operator>>(int shift) const noexcept
```

Example:

```cpp
const auto result = position >> other;
```

<a id="operator-shift-right-assign"></a>
## `operator>>=`

Shifts each integer lane right in place.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr SimdVector &VECTORCALL operator>>=(int shift) noexcept
```

Example:

```cpp
position >>= other;
```

<a id="operator-or"></a>
## `operator|`

Computes a lane-wise bitwise OR with another register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE SimdVector VECTORCALL operator|(vector_t rhs) const noexcept
```

Example:

```cpp
const auto result = position | other;
```

<a id="operator-or-assign"></a>
## `operator|=`

Applies a lane-wise bitwise OR with another register in place.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator|=(vector_t rhs) noexcept
```

Example:

```cpp
position |= other;
```

<a id="operator-not"></a>
## `operator~`

Inverts every bit in the underlying register.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE SimdVector VECTORCALL operator~() const noexcept
```

Example:

```cpp
const auto result = ~position;
```

<a id="sign"></a>
## `sign`

Returns the sign of each element as -1, 0, or 1, or 0 and 1 for unsigned types.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE vector_t VECTORCALL sign() const noexcept requires requires(vector_t value)
```

Example:

```cpp
const auto result = position.sign();
```

<a id="simdvector"></a>
## `SimdVector`

Copies another SIMD vector.

Signatures:

```cpp
SimdVector(const SimdVector &other) = default;
SimdVector(SimdVector &&other) = default;
SIMDLIB_FORCE_INLINE constexpr SimdVector() noexcept
SIMDLIB_FORCE_INLINE constexpr SimdVector(vector_t data) noexcept
SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(element_t v) noexcept
SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(std::span<element_t, simd::element_count> data) noexcept
SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(std::span<const element_t, simd::element_count> data) noexcept
SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(std::span<element_t, element_count> data) noexcept requires(element_count != simd::element_count)
SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(std::span<const element_t, element_count> data) noexcept requires(element_count != simd::element_count)
SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(const std::array<element_t, simd::element_count> &data) noexcept
SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(const std::array<element_t, element_count> &data) noexcept requires(element_count != simd::element_count)
template <std::convertible_to<element_t>... Args> requires(sizeof...(Args) == element_count) SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(Args &&...args) noexcept
```

Example:

```cpp
Vector3 position{1.0F, 2.0F, 3.0F};
```

<a id="size"></a>
## `size`

Computes the inclusive per-lane extent between this vector and a minimum bound.

Signatures:

```cpp
template <class target_element_t = area_element_t> SIMDLIB_FORCE_INLINE auto VECTORCALL size(vector_t minInclusive) const noexcept requires(std::is_integral_v<element_t> && std::is_integral_v<target_element_t> && sizeof(target_element_t) >= sizeof(element_t))
```

Example:

```cpp
const auto result = position.size();
```

<a id="sizeof"></a>
## `sizeof`

Constructs a new SIMD vector by widening another SIMD vector with the same logical element count.

Signatures:

```cpp
template <class source_t> requires(std::is_integral_v<source_t> && std::is_integral_v<element_t> && sizeof(source_t) < sizeof(element_t)) SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(const SimdVector<source_t, element_count> &other) noexcept
```

Example:

```cpp
const auto result = position.sizeof(other);
```

<a id="sqrt"></a>
## `sqrt`

Computes the square root of each element.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE auto VECTORCALL sqrt() const noexcept requires requires(vector_t value)
```

Example:

```cpp
const auto result = position.sqrt();
```

<a id="subtract-horizontal"></a>
## `subtract_horizontal`

Subtracts adjacent lane pairs within each 128-bit lane.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE auto VECTORCALL subtract_horizontal(vector_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
```

Example:

```cpp
const auto result = position.subtract_horizontal(other);
```

<a id="subtract-horizontal-saturated"></a>
## `subtract_horizontal_saturated`

Subtracts adjacent lane pairs with saturation where the specialization supports it.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE auto VECTORCALL subtract_horizontal_saturated(vector_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
```

Example:

```cpp
const auto result = position.subtract_horizontal_saturated(other);
```

<a id="subtract-saturated"></a>
## `subtract_saturated`

Subtracts the two vectors and clamps integer overflow to the underlying type's maximum value.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE vector_t VECTORCALL subtract_saturated(vector_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
SIMDLIB_FORCE_INLINE vector_t VECTORCALL subtract_saturated(element_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
```

Example:

```cpp
const auto result = position.subtract_saturated(other);
```

<a id="sum-absolute-byte-differences"></a>
## `sum_absolute_byte_differences`

Computes byte-wise absolute differences and accumulates them into 64-bit result lanes.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE auto VECTORCALL sum_absolute_byte_differences(vector_t rhs) const noexcept requires requires(vector_t lhsValue, vector_t rhsValue)
```

Example:

```cpp
const auto result = position.sum_absolute_byte_differences(other);
```

<a id="toarray"></a>
## `toArray`

Converts the SIMD vector to an array of elements.

Signatures:

```cpp
SIMDLIB_FORCE_INLINE constexpr std::array<element_t, simd::element_count> toArray() const noexcept
```

Example:

```cpp
const auto result = position.toArray();
```

<a id="w"></a>
## `w`

Returns a mutable reference to the fourth element.

Signatures:

```cpp
constexpr inline element_t &w() noexcept requires(element_count > 3)
constexpr inline element_t w() const noexcept requires(element_count > 3)
```

Example:

```cpp
const auto result = position.w();
```

<a id="x"></a>
## `x`

Returns a mutable reference to the first element.

Signatures:

```cpp
constexpr inline element_t &x() noexcept requires(element_count > 0)
constexpr inline element_t x() const noexcept requires(element_count > 0)
```

Example:

```cpp
const auto result = position.x();
```

<a id="y"></a>
## `y`

Returns a mutable reference to the second element.

Signatures:

```cpp
constexpr inline element_t &y() noexcept requires(element_count > 1)
constexpr inline element_t y() const noexcept requires(element_count > 1)
```

Example:

```cpp
const auto result = position.y();
```

<a id="z"></a>
## `z`

Returns a mutable reference to the third element.

Signatures:

```cpp
constexpr inline element_t &z() noexcept requires(element_count > 2)
constexpr inline element_t z() const noexcept requires(element_count > 2)
```

Example:

```cpp
const auto result = position.z();
```

<a id="related-types-and-constants"></a>
## Related types and constants

The header provides `VectorInt8`, `VectorUInt8`, `VectorInt16`, `VectorUInt16`, `VectorInt32`, `VectorUInt32`, `VectorInt64`, and `VectorUInt64`, plus register-sized aliases such as `uint8x16`, `uint32x8`, `int16x8`, and `int64x4`. Use `SimdVector<T, N>` directly for position-like dimensions such as two, three, or four.
