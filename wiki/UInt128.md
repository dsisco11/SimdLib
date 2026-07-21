# uint128_t

`uint128_t` is SimdLib's portable unsigned 128-bit integer. It provides two-word construction, arithmetic, bitwise operations, shifts, comparisons, extraction, and SIMD-register conversion.

## Contents

- [Overview](#overview)
- [Example include](#example-setup)
- [`~uint128_t`](#destructor-uint128-t)
- [`abs_diff`](#abs-diff)
- [`bit_ceil`](#bit-ceil)
- [`bit_floor`](#bit-floor)
- [`bit_width`](#bit-width)
- [`countl_one`](#countl-one)
- [`countl_zero`](#countl-zero)
- [`countr_one`](#countr-one)
- [`countr_zero`](#countr-zero)
- [`create_mask`](#create-mask)
- [`extract`](#extract)
- [`from_register`](#from-register)
- [`getBlock`](#getblock)
- [`has_single_bit`](#has-single-bit)
- [`high`](#high)
- [`low`](#low)
- [`operator bool`](#operator-bool)
- [`operator T`](#operator-t)
- [`operator-`](#operator-minus)
- [`operator--`](#operator-decrement)
- [`operator-=`](#operator-minus-assign)
- [`operator""_u128`](#operator-literal-u128)
- [`operator&`](#operator-and)
- [`operator&=`](#operator-and-assign)
- [`operator^`](#operator-xor)
- [`operator^=`](#operator-xor-assign)
- [`operator+`](#operator-plus)
- [`operator++`](#operator-increment)
- [`operator+=`](#operator-plus-assign)
- [`operator<<`](#operator-shift-left)
- [`operator<<=`](#operator-shift-left-assign)
- [`operator<=>`](#operator-compare)
- [`operator=`](#operator-assign)
- [`operator==`](#operator-equal)
- [`operator>>`](#operator-shift-right)
- [`operator>>=`](#operator-shift-right-assign)
- [`operator|`](#operator-or)
- [`operator|=`](#operator-or-assign)
- [`operator~`](#operator-not)
- [`popcount`](#popcount)
- [`to_register`](#to-register)
- [`uint128_t`](#uint128-t)
- [Related types and constants](#related-types-and-constants)

<a id="overview"></a>
## Overview

Include `<SimdLib/UInt128.h>`. Overloads with the same name are collected in one subsection; every public overload is listed below.

<a id="example-setup"></a>
## Example include

```cpp
#include <SimdLib/UInt128.h>
```
<a id="destructor-uint128-t"></a>
## `~uint128_t`

Destroys the value.

Signatures:

```cpp
~uint128_t() = default
```

Example:

```cpp
{
  const SimdLib::uint128_t temporary{42};
} // => temporary is destroyed at the closing brace
```

<a id="abs-diff"></a>
## `abs_diff`

Computes the absolute difference between two unsigned 128-bit values.

Signatures:

```cpp
uint128_t abs_diff(const uint128_t& other) const
```

Example:

```cpp
SimdLib::uint128_t{10}.abs_diff(SimdLib::uint128_t{3}); // => 7
```

<a id="bit-ceil"></a>
## `bit_ceil`

Returns the smallest representable power of two not less than the value, or zero on overflow.

Signatures:

```cpp
uint128_t bit_ceil(uint128_t value)
```

Example:

```cpp
SimdLib::bit_ceil(SimdLib::uint128_t{9}); // => 16
```

<a id="bit-floor"></a>
## `bit_floor`

Returns the greatest power of two not greater than the value.

Signatures:

```cpp
uint128_t bit_floor(uint128_t value)
```

Example:

```cpp
SimdLib::bit_floor(SimdLib::uint128_t{9}); // => 8
```

<a id="bit-width"></a>
## `bit_width`

Returns the number of bits required to represent the value.

Signatures:

```cpp
int bit_width(uint128_t value)
```

Example:

```cpp
SimdLib::bit_width(SimdLib::uint128_t{9}); // => 4
```

<a id="countl-one"></a>
## `countl_one`

Returns the number of consecutive one bits from the most-significant side.

Signatures:

```cpp
int countl_one(uint128_t value)
```

Example:

```cpp
SimdLib::countl_one(SimdLib::uint128_t{0, 0xE000'0000'0000'0000ULL}); // => 3
```

<a id="countl-zero"></a>
## `countl_zero`

Returns the number of consecutive zero bits from the most-significant side.

Signatures:

```cpp
int countl_zero(uint128_t value)
```

Example:

```cpp
SimdLib::countl_zero(SimdLib::uint128_t{1}); // => 127
```

<a id="countr-one"></a>
## `countr_one`

Returns the number of consecutive one bits from the least-significant side.

Signatures:

```cpp
int countr_one(uint128_t value)
```

Example:

```cpp
SimdLib::countr_one(SimdLib::uint128_t{7}); // => 3
```

<a id="countr-zero"></a>
## `countr_zero`

Returns the number of consecutive zero bits from the least-significant side.

Signatures:

```cpp
int countr_zero(uint128_t value)
```

Example:

```cpp
SimdLib::countr_zero(SimdLib::uint128_t{8}); // => 3
```

<a id="create-mask"></a>
## `create_mask`

Creates a mask containing `bitCount` low one bits.

Signatures:

```cpp
static uint128_t create_mask(int bitCount)
```

Example:

```cpp
SimdLib::uint128_t::create_mask(4); // => 0b1111
```

<a id="extract"></a>
## `extract`

Extracts a contiguous bit range and shifts it to bit zero. Prefer `SimdLib::Bmi::bextr` in new code.

Signatures:

```cpp
uint128_t extract(std::uint8_t len, std::uint8_t start) const
template <std::size_t len> std::uint64_t extract(std::uint8_t start) const
template <std::size_t start, std::size_t len> uint128_t extract() const
```

Example:

```cpp
SimdLib::uint128_t{0xABCD}.extract(8, 4); // => 0xBC (deprecated; prefer Bmi::bextr)
```

<a id="from-register"></a>
## `from_register`

Constructs a value by extracting both words from a backend register through Api.

Signatures:

```cpp
template <class Dependency = void>
```

Example:

```cpp
SimdLib::uint128_t::from_register(
    SimdLib::Api<128, std::uint64_t>::construct({5, 7})); // => low word 5, high word 7
```

<a id="getblock"></a>
## `getBlock`

Returns the backing word at index zero (low) or one (high).

Signatures:

```cpp
std::uint64_t getBlock(int index) const
```

Example:

```cpp
SimdLib::uint128_t{5, 7}.getBlock(1); // => 7
```

<a id="has-single-bit"></a>
## `has_single_bit`

Returns true when exactly one bit is set.

Signatures:

```cpp
bool has_single_bit(uint128_t value)
```

Example:

```cpp
SimdLib::has_single_bit(SimdLib::uint128_t{8}); // => true
```

<a id="high"></a>
## `high`

Returns the high 64-bit word by value or mutable reference.

Signatures:

```cpp
std::uint64_t& high()
std::uint64_t high() const
```

Example:

```cpp
SimdLib::uint128_t{5, 7}.high(); // => 7
```

<a id="low"></a>
## `low`

Returns the low 64-bit word by value or mutable reference.

Signatures:

```cpp
std::uint64_t& low()
std::uint64_t low() const
```

Example:

```cpp
SimdLib::uint128_t{5, 7}.low(); // => 5
```

<a id="operator-bool"></a>
## `operator bool`

Returns true when any bit is set.

Signatures:

```cpp
explicit operator bool() const
```

Example:

```cpp
static_cast<bool>(SimdLib::uint128_t{1}); // => true
```

<a id="operator-t"></a>
## `operator T`

Explicitly converts the low word to a supported integral type.

Signatures:

```cpp
template <std::integral T> explicit operator T() const
```

Example:

```cpp
static_cast<std::uint64_t>(SimdLib::uint128_t{42}); // => 42
```

<a id="operator-minus"></a>
## `operator-`

Subtracts or negates with unsigned wraparound.

Signatures:

```cpp
uint128_t operator-(const uint128_t& rhs) const
uint128_t operator-() const
```

Example:

```cpp
SimdLib::uint128_t{45} - SimdLib::uint128_t{3}; // => 42
```

<a id="operator-decrement"></a>
## `operator--`

Decrements the value; prefix and postfix forms are available.

Signatures:

```cpp
uint128_t& operator--()
uint128_t operator--(int)
```

Example:

```cpp
SimdLib::uint128_t result{43};
--result; // => 42
```

<a id="operator-minus-assign"></a>
## `operator-=`

Subtracts another value in place.

Signatures:

```cpp
uint128_t& operator-=(const uint128_t& rhs)
```

Example:

```cpp
SimdLib::uint128_t result{45};
result -= SimdLib::uint128_t{3}; // => 42
```

<a id="operator-literal-u128"></a>
## `operator""_u128`

Constructs a 128-bit value from an unsigned long long literal.

Signatures:

```cpp
uint128_t operator""_u128(const unsigned long long value)
```

Example:

```cpp
using namespace SimdLib;
42_u128; // => 42
```

<a id="operator-and"></a>
## `operator&`

Computes bitwise AND.

Signatures:

```cpp
uint128_t operator&(const uint128_t& rhs) const
```

Example:

```cpp
SimdLib::uint128_t{0b1100} & SimdLib::uint128_t{0b1010}; // => 0b1000
```

<a id="operator-and-assign"></a>
## `operator&=`

Applies bitwise AND in place.

Signatures:

```cpp
uint128_t& operator&=(const uint128_t& rhs)
```

Example:

```cpp
SimdLib::uint128_t result{0b1100};
result &= SimdLib::uint128_t{0b1010}; // => 0b1000
```

<a id="operator-xor"></a>
## `operator^`

Computes bitwise exclusive OR.

Signatures:

```cpp
uint128_t operator^(const uint128_t& rhs) const
```

Example:

```cpp
SimdLib::uint128_t{0b1100} ^ SimdLib::uint128_t{0b1010}; // => 0b0110
```

<a id="operator-xor-assign"></a>
## `operator^=`

Applies bitwise exclusive OR in place.

Signatures:

```cpp
uint128_t& operator^=(const uint128_t& rhs)
```

Example:

```cpp
SimdLib::uint128_t result{0b1100};
result ^= SimdLib::uint128_t{0b1010}; // => 0b0110
```

<a id="operator-plus"></a>
## `operator+`

Adds two 128-bit values with unsigned wraparound.

Signatures:

```cpp
uint128_t operator+(const uint128_t& rhs) const
```

Example:

```cpp
SimdLib::uint128_t{40} + SimdLib::uint128_t{2}; // => 42
```

<a id="operator-increment"></a>
## `operator++`

Increments the value; prefix and postfix forms are available.

Signatures:

```cpp
uint128_t& operator++()
uint128_t operator++(int)
```

Example:

```cpp
SimdLib::uint128_t result{41};
++result; // => 42
```

<a id="operator-plus-assign"></a>
## `operator+=`

Adds another value in place.

Signatures:

```cpp
uint128_t& operator+=(const uint128_t& rhs)
```

Example:

```cpp
SimdLib::uint128_t result{40};
result += SimdLib::uint128_t{2}; // => 42
```

<a id="operator-shift-left"></a>
## `operator<<`

Whole-value left shift. Negative counts are treated as zero; counts of 128 or more produce zero.

Signatures:

```cpp
template <std::integral T> uint128_t operator<<(T count) const
template <std::integral T> uint128_t operator<<(T count) const
```

Example:

```cpp
SimdLib::uint128_t{3} << 2; // => 12
```

<a id="operator-shift-left-assign"></a>
## `operator<<=`

Shifts left in place.

Signatures:

```cpp
template <std::integral T> uint128_t& operator<<=(T count)
```

Example:

```cpp
SimdLib::uint128_t result{3};
result <<= 2; // => 12
```

<a id="operator-compare"></a>
## `operator<=>`

Provides strong ordering against another 128-bit value or a supported integral value.

Signatures:

```cpp
std::strong_ordering operator<=>(const uint128_t& rhs) const
template <std::integral T> std::strong_ordering operator<=>(T rhs) const
```

Example:

```cpp
SimdLib::uint128_t{1} <=> SimdLib::uint128_t{2}; // => std::strong_ordering::less
```

<a id="operator-assign"></a>
## `operator=`

Copies or moves another value into this object.

Signatures:

```cpp
uint128_t& operator=(const uint128_t&) = default
uint128_t& operator=(uint128_t&&) = default
```

Example:

```cpp
SimdLib::uint128_t result{};
result = SimdLib::uint128_t{42}; // => 42
```

<a id="operator-equal"></a>
## `operator==`

Tests equality against another 128-bit value or a supported integral value.

Signatures:

```cpp
bool operator==(const uint128_t& rhs) const
template <std::integral T> bool operator==(T rhs) const
```

Example:

```cpp
SimdLib::uint128_t{42} == SimdLib::uint128_t{42}; // => true
```

<a id="operator-shift-right"></a>
## `operator>>`

Returns the value shifted right; out-of-range counts produce zero.

Signatures:

```cpp
template <std::integral T> uint128_t operator>>(T count) const
```

Example:

```cpp
SimdLib::uint128_t{12} >> 2; // => 3
```

<a id="operator-shift-right-assign"></a>
## `operator>>=`

Shifts right in place.

Signatures:

```cpp
template <std::integral T> uint128_t& operator>>=(T count)
```

Example:

```cpp
SimdLib::uint128_t result{12};
result >>= 2; // => 3
```

<a id="operator-or"></a>
## `operator|`

Computes bitwise OR.

Signatures:

```cpp
uint128_t operator|(const uint128_t& rhs) const
```

Example:

```cpp
SimdLib::uint128_t{0b1100} | SimdLib::uint128_t{0b1010}; // => 0b1110
```

<a id="operator-or-assign"></a>
## `operator|=`

Applies bitwise OR in place.

Signatures:

```cpp
uint128_t& operator|=(const uint128_t& rhs)
```

Example:

```cpp
SimdLib::uint128_t result{0b1100};
result |= SimdLib::uint128_t{0b1010}; // => 0b1110
```

<a id="operator-not"></a>
## `operator~`

Complements all 128 bits.

Signatures:

```cpp
uint128_t operator~() const
```

Example:

```cpp
~SimdLib::uint128_t{}; // => all 128 bits are set
```

<a id="popcount"></a>
## `popcount`

Returns the number of one bits.

Signatures:

```cpp
int popcount(uint128_t value)
```

Example:

```cpp
SimdLib::popcount(SimdLib::uint128_t{0b1011}); // => 3
```

<a id="to-register"></a>
## `to_register`

Loads the stored words into a backend register through Api.

Signatures:

```cpp
template <class Dependency = void>
```

Example:

```cpp
SimdLib::uint128_t{5, 7}.to_register(); // => register words {5, 7}
```

<a id="uint128-t"></a>
## `uint128_t`

Constructs a value from low and high words, in that order.

Signatures:

```cpp
uint128_t(std::uint64_t lower, std::uint64_t upper) : m_data
uint128_t() = default
uint128_t(const uint128_t&) = default
uint128_t(uint128_t&&) = default
uint128_t(std::uint64_t lower, std::uint64_t upper)
template <std::integral T> uint128_t(T value)
uint128_t(bool value)
```

Example:

```cpp
SimdLib::uint128_t{42}; // => 42
```

<a id="related-types-and-constants"></a>
## Related types and constants

`block_t` is `std::uint64_t`; `block_count` is 2 and `block_width` is 64. Related free functions on this page provide bit counting, bit-width, power-of-two, and literal support. The [`Formatting`](Formatting.md) page covers `numeric_limits`, hashing, and formatting specializations.
