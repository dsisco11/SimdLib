# uint128_t

`uint128_t` is SimdLib's portable unsigned 128-bit integer. It provides two-word construction, arithmetic, bitwise operations, shifts, comparisons, extraction, and SIMD-register conversion.

## Contents

- [Overview](#overview)
- [Example setup](#example-setup)
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
## Example setup

```cpp
#include <SimdLib/UInt128.h>

SimdLib::uint128_t value{42};
SimdLib::uint128_t other{7};
```

<a id="destructor-uint128-t"></a>
## `~uint128_t`

Destroys the value.

Signatures:

```cpp
constexpr ~uint128_t() = default;
```

Example:

```cpp
const auto result = value.~uint128_t();
```

<a id="abs-diff"></a>
## `abs_diff`

Computes the absolute difference between two unsigned 128-bit values.

Signatures:

```cpp
[[nodiscard]] constexpr uint128_t abs_diff(const uint128_t& other) const noexcept
```

Example:

```cpp
const auto result = value.abs_diff(other);
```

<a id="bit-ceil"></a>
## `bit_ceil`

Returns the smallest representable power of two not less than the value, or zero on overflow.

Signatures:

```cpp
[[nodiscard]] constexpr uint128_t bit_ceil(const uint128_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::bit_ceil(value);
```

<a id="bit-floor"></a>
## `bit_floor`

Returns the greatest power of two not greater than the value.

Signatures:

```cpp
[[nodiscard]] constexpr uint128_t bit_floor(const uint128_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::bit_floor(value);
```

<a id="bit-width"></a>
## `bit_width`

Returns the number of bits required to represent the value.

Signatures:

```cpp
[[nodiscard]] constexpr int bit_width(const uint128_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::bit_width(value);
```

<a id="countl-one"></a>
## `countl_one`

Returns the number of consecutive one bits from the most-significant side.

Signatures:

```cpp
[[nodiscard]] constexpr int countl_one(const uint128_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::countl_one(value);
```

<a id="countl-zero"></a>
## `countl_zero`

Returns the number of consecutive zero bits from the most-significant side.

Signatures:

```cpp
[[nodiscard]] constexpr int countl_zero(const uint128_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::countl_zero(value);
```

<a id="countr-one"></a>
## `countr_one`

Returns the number of consecutive one bits from the least-significant side.

Signatures:

```cpp
[[nodiscard]] constexpr int countr_one(const uint128_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::countr_one(value);
```

<a id="countr-zero"></a>
## `countr_zero`

Returns the number of consecutive zero bits from the least-significant side.

Signatures:

```cpp
[[nodiscard]] constexpr int countr_zero(const uint128_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::countr_zero(value);
```

<a id="create-mask"></a>
## `create_mask`

Creates a mask containing `bitCount` low one bits.

Signatures:

```cpp
[[nodiscard]] static constexpr uint128_t create_mask(const int bitCount) noexcept
```

Example:

```cpp
const auto result = SimdLib::uint128_t::create_mask(12);
```

<a id="extract"></a>
## `extract`

Extracts a contiguous bit range and shifts it to bit zero. Prefer `SimdLib::Bmi::bextr` in new code.

Signatures:

```cpp
constexpr uint128_t extract(std::uint8_t len, std::uint8_t start) const noexcept;
template <std::size_t len> constexpr std::uint64_t extract(std::uint8_t start) const noexcept;
template <std::size_t start, std::size_t len> constexpr uint128_t extract() const noexcept;
```

Example:

```cpp
const auto result = value.extract(8, 16); // Deprecated: prefer Bmi::bextr.
```

<a id="from-register"></a>
## `from_register`

Constructs a value by extracting both words from a backend register through Api.

Signatures:

```cpp
template <class Dependency = void> requires(simd_available<Dependency>) [[nodiscard]] static uint128_t from_register(const typename simd<Dependency>::vector_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::uint128_t::from_register(registerValue);
```

<a id="getblock"></a>
## `getBlock`

Returns the backing word at index zero (low) or one (high).

Signatures:

```cpp
[[nodiscard]] constexpr std::uint64_t getBlock(const int index) const noexcept
```

Example:

```cpp
const std::uint64_t upper = value.getBlock(1);
```

<a id="has-single-bit"></a>
## `has_single_bit`

Returns true when exactly one bit is set.

Signatures:

```cpp
[[nodiscard]] constexpr bool has_single_bit(const uint128_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::has_single_bit(value);
```

<a id="high"></a>
## `high`

Returns the high 64-bit word by value or mutable reference.

Signatures:

```cpp
constexpr std::uint64_t& high() noexcept;
constexpr std::uint64_t high() const noexcept;
```

Example:

```cpp
const auto result = value.high();
```

<a id="low"></a>
## `low`

Returns the low 64-bit word by value or mutable reference.

Signatures:

```cpp
constexpr std::uint64_t& low() noexcept;
constexpr std::uint64_t low() const noexcept;
```

Example:

```cpp
const auto result = value.low();
```

<a id="operator-bool"></a>
## `operator bool`

Returns true when any bit is set.

Signatures:

```cpp
constexpr explicit operator bool() const noexcept;
```

Example:

```cpp
if (value) { /* at least one bit is set */ }
```

<a id="operator-t"></a>
## `operator T`

Explicitly converts the low word to a supported integral type.

Signatures:

```cpp
template <std::integral T> constexpr explicit operator T() const noexcept;
```

Example:

```cpp
const auto low = static_cast<std::uint64_t>(value);
```

<a id="operator-minus"></a>
## `operator-`

Subtracts or negates with unsigned wraparound.

Signatures:

```cpp
constexpr uint128_t operator-(const uint128_t& rhs) const noexcept;
constexpr uint128_t operator-() const noexcept;
```

Example:

```cpp
const auto result = value - other;
```

<a id="operator-decrement"></a>
## `operator--`

Decrements the value; prefix and postfix forms are available.

Signatures:

```cpp
constexpr uint128_t& operator--() noexcept;
constexpr uint128_t operator--(int) noexcept;
```

Example:

```cpp
--value;
```

<a id="operator-minus-assign"></a>
## `operator-=`

Subtracts another value in place.

Signatures:

```cpp
constexpr uint128_t& operator-=(const uint128_t& rhs) noexcept;
```

Example:

```cpp
value -= other;
```

<a id="operator-literal-u128"></a>
## `operator""_u128`

Constructs a 128-bit value from an unsigned long long literal.

Signatures:

```cpp
[[nodiscard]] constexpr uint128_t operator""_u128(const unsigned long long value) noexcept
```

Example:

```cpp
using namespace SimdLib; const auto result = 42_u128;
```

<a id="operator-and"></a>
## `operator&`

Computes bitwise AND.

Signatures:

```cpp
constexpr uint128_t operator&(const uint128_t& rhs) const noexcept;
```

Example:

```cpp
const auto result = value & other;
```

<a id="operator-and-assign"></a>
## `operator&=`

Applies bitwise AND in place.

Signatures:

```cpp
constexpr uint128_t& operator&=(const uint128_t& rhs) noexcept;
```

Example:

```cpp
value &= other;
```

<a id="operator-xor"></a>
## `operator^`

Computes bitwise exclusive OR.

Signatures:

```cpp
constexpr uint128_t operator^(const uint128_t& rhs) const noexcept;
```

Example:

```cpp
const auto result = value ^ other;
```

<a id="operator-xor-assign"></a>
## `operator^=`

Applies bitwise exclusive OR in place.

Signatures:

```cpp
constexpr uint128_t& operator^=(const uint128_t& rhs) noexcept;
```

Example:

```cpp
value ^= other;
```

<a id="operator-plus"></a>
## `operator+`

Adds two 128-bit values with unsigned wraparound.

Signatures:

```cpp
constexpr uint128_t operator+(const uint128_t& rhs) const noexcept;
```

Example:

```cpp
const auto result = value + other;
```

<a id="operator-increment"></a>
## `operator++`

Increments the value; prefix and postfix forms are available.

Signatures:

```cpp
constexpr uint128_t& operator++() noexcept;
constexpr uint128_t operator++(int) noexcept;
```

Example:

```cpp
++value;
```

<a id="operator-plus-assign"></a>
## `operator+=`

Adds another value in place.

Signatures:

```cpp
constexpr uint128_t& operator+=(const uint128_t& rhs) noexcept;
```

Example:

```cpp
value += other;
```

<a id="operator-shift-left"></a>
## `operator<<`

Whole-value left shift. Negative counts are treated as zero; counts of 128 or more produce zero.

Signatures:

```cpp
template <std::integral T> [[nodiscard]] constexpr uint128_t operator<<(const T count) const noexcept
template <std::integral T> constexpr uint128_t operator<<(T count) const noexcept;
```

Example:

```cpp
const auto result = value << 4;
```

<a id="operator-shift-left-assign"></a>
## `operator<<=`

Shifts left in place.

Signatures:

```cpp
template <std::integral T> constexpr uint128_t& operator<<=(T count) noexcept;
```

Example:

```cpp
value <<= 4;
```

<a id="operator-compare"></a>
## `operator<=>`

Provides strong ordering against another 128-bit value or a supported integral value.

Signatures:

```cpp
constexpr std::strong_ordering operator<=>(const uint128_t& rhs) const noexcept;
template <std::integral T> constexpr std::strong_ordering operator<=>(T rhs) const noexcept;
```

Example:

```cpp
const auto ordering = value <=> other;
```

<a id="operator-assign"></a>
## `operator=`

Copies or moves another value into this object.

Signatures:

```cpp
constexpr uint128_t& operator=(const uint128_t&) noexcept = default;
constexpr uint128_t& operator=(uint128_t&&) noexcept = default;
```

Example:

```cpp
value = other;
```

<a id="operator-equal"></a>
## `operator==`

Tests equality against another 128-bit value or a supported integral value.

Signatures:

```cpp
constexpr bool operator==(const uint128_t& rhs) const noexcept;
template <std::integral T> constexpr bool operator==(T rhs) const noexcept;
```

Example:

```cpp
const bool result = value == other;
```

<a id="operator-shift-right"></a>
## `operator>>`

Returns the value shifted right; out-of-range counts produce zero.

Signatures:

```cpp
template <std::integral T> constexpr uint128_t operator>>(T count) const noexcept;
```

Example:

```cpp
const auto result = value >> 4;
```

<a id="operator-shift-right-assign"></a>
## `operator>>=`

Shifts right in place.

Signatures:

```cpp
template <std::integral T> constexpr uint128_t& operator>>=(T count) noexcept;
```

Example:

```cpp
value >>= 4;
```

<a id="operator-or"></a>
## `operator|`

Computes bitwise OR.

Signatures:

```cpp
constexpr uint128_t operator|(const uint128_t& rhs) const noexcept;
```

Example:

```cpp
const auto result = value | other;
```

<a id="operator-or-assign"></a>
## `operator|=`

Applies bitwise OR in place.

Signatures:

```cpp
constexpr uint128_t& operator|=(const uint128_t& rhs) noexcept;
```

Example:

```cpp
value |= other;
```

<a id="operator-not"></a>
## `operator~`

Complements all 128 bits.

Signatures:

```cpp
constexpr uint128_t operator~() const noexcept;
```

Example:

```cpp
const auto result = ~value;
```

<a id="popcount"></a>
## `popcount`

Returns the number of one bits.

Signatures:

```cpp
[[nodiscard]] constexpr int popcount(const uint128_t value) noexcept
```

Example:

```cpp
const auto result = SimdLib::popcount(value);
```

<a id="to-register"></a>
## `to_register`

Loads the stored words into a backend register through Api.

Signatures:

```cpp
template <class Dependency = void> requires(simd_available<Dependency>) [[nodiscard]] auto to_register() const noexcept -> typename simd<Dependency>::vector_t
```

Example:

```cpp
const auto result = value.to_register();
```

<a id="uint128-t"></a>
## `uint128_t`

Constructs a value from low and high words, in that order.

Signatures:

```cpp
constexpr uint128_t(const std::uint64_t lower, const std::uint64_t upper) noexcept : m_data
constexpr uint128_t() noexcept = default;
constexpr uint128_t(const uint128_t&) noexcept = default;
constexpr uint128_t(uint128_t&&) noexcept = default;
constexpr uint128_t(std::uint64_t lower, std::uint64_t upper) noexcept;
template <std::integral T> constexpr uint128_t(T value) noexcept;
constexpr uint128_t(bool value) noexcept;
```

Example:

```cpp
SimdLib::uint128_t value{42};
```

<a id="related-types-and-constants"></a>
## Related types and constants

`block_t` is `std::uint64_t`; `block_count` is 2 and `block_width` is 64. Related free functions on this page provide bit counting, bit-width, power-of-two, and literal support. The [`Formatting`](Formatting.md) page covers `numeric_limits`, hashing, and formatting specializations.
