# Formatting and hashing

Including `Format.h` provides standard-library integration for SimdLib value types.

## Contents

- [`formatter<SimdVector>::parse`](#vector-parse)
- [`formatter<SimdVector>::format`](#vector-format)
- [`formatter<uint128_t>::parse`](#uint128-parse)
- [`formatter<uint128_t>::format`](#uint128-format)
- [`hash<SimdVector>::operator()`](#vector-hash)
- [`hash<uint128_t>::operator()`](#uint128-hash)
- [`numeric_limits<uint128_t>::min`](#uint128-min)
- [`numeric_limits<uint128_t>::lowest`](#uint128-lowest)
- [`numeric_limits<uint128_t>::max`](#uint128-max)
- [`numeric_limits<uint128_t>::epsilon`](#uint128-epsilon)
- [`numeric_limits<uint128_t>::round_error`](#uint128-round-error)
- [`numeric_limits<uint128_t>::infinity`](#uint128-infinity)
- [`numeric_limits<uint128_t>::quiet_NaN`](#uint128-quiet-nan)
- [`numeric_limits<uint128_t>::signaling_NaN`](#uint128-signaling-nan)
- [`numeric_limits<uint128_t>::denorm_min`](#uint128-denorm-min)

<a id="vector-parse"></a>
## `formatter<SimdVector>::parse`

Accepts fill, alignment, and width for the completed `{a, b, c}` container text.

```cpp
const auto text = std::format("{:>20}", SimdLib::SimdVector<float, 3>{1.0F, 2.0F, 3.0F});
```

<a id="vector-format"></a>
## `formatter<SimdVector>::format`

Formats logical lanes in order inside braces.

```cpp
const auto text = std::format("{}", SimdLib::SimdVector<float, 3>{1.0F, 2.0F, 3.0F});
// "{1, 2, 3}"
```

<a id="uint128-parse"></a>
## `formatter<uint128_t>::parse`

Accepts integer sign, alternate form, zero padding, width, and binary/octal/decimal/hex presentation options.

```cpp
const auto text = std::format("{:#x}", SimdLib::uint128_t{255});
```

<a id="uint128-format"></a>
## `formatter<uint128_t>::format`

Formats a portable 128-bit value without requiring a compiler-native 128-bit integer.

```cpp
const auto text = std::format("{}", SimdLib::uint128_t{42});
```

<a id="vector-hash"></a>
## `hash<SimdVector>::operator()`

Hashes every logical lane, making vectors usable as keys in standard unordered containers.

```cpp
const auto hash = std::hash<SimdLib::SimdVector<float, 3>>{}(position);
```

<a id="uint128-hash"></a>
## `hash<uint128_t>::operator()`

Combines the low and high words into a standard `std::size_t` hash.

```cpp
const auto hash = std::hash<SimdLib::uint128_t>{}(value);
```

<a id="uint128-min"></a>
## `numeric_limits<uint128_t>::min`

Returns zero, the smallest `uint128_t` value.

```cpp
const auto value = std::numeric_limits<SimdLib::uint128_t>::min();
```

<a id="uint128-lowest"></a>
## `numeric_limits<uint128_t>::lowest`

Returns zero because `uint128_t` is unsigned.

```cpp
const auto value = std::numeric_limits<SimdLib::uint128_t>::lowest();
```

<a id="uint128-max"></a>
## `numeric_limits<uint128_t>::max`

Returns a value with all 128 bits set.

```cpp
const auto value = std::numeric_limits<SimdLib::uint128_t>::max();
```

<a id="uint128-epsilon"></a>
## `numeric_limits<uint128_t>::epsilon`

Returns zero for this exact integer type.

```cpp
const auto value = std::numeric_limits<SimdLib::uint128_t>::epsilon();
```

<a id="uint128-round-error"></a>
## `numeric_limits<uint128_t>::round_error`

Returns zero because integer operations have no floating-point rounding error.

```cpp
const auto value = std::numeric_limits<SimdLib::uint128_t>::round_error();
```

<a id="uint128-infinity"></a>
## `numeric_limits<uint128_t>::infinity`

Returns zero; `uint128_t` has no infinity representation.

```cpp
const auto value = std::numeric_limits<SimdLib::uint128_t>::infinity();
```

<a id="uint128-quiet-nan"></a>
## `numeric_limits<uint128_t>::quiet_NaN`

Returns zero; `uint128_t` has no NaN representation.

```cpp
const auto value = std::numeric_limits<SimdLib::uint128_t>::quiet_NaN();
```

<a id="uint128-signaling-nan"></a>
## `numeric_limits<uint128_t>::signaling_NaN`

Returns zero; `uint128_t` has no signaling NaN representation.

```cpp
const auto value = std::numeric_limits<SimdLib::uint128_t>::signaling_NaN();
```

<a id="uint128-denorm-min"></a>
## `numeric_limits<uint128_t>::denorm_min`

Returns zero; integer values do not have denormal representations.

```cpp
const auto value = std::numeric_limits<SimdLib::uint128_t>::denorm_min();
```
