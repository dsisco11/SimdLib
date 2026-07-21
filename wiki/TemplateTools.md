# Template tools

`TemplateTools.h` contains small public types and compile-time helpers used by SimdLib and available to callers.

## Contents

- [`sorted_unique_t::sorted_unique_t`](#sorted-unique-constructor)
- [`sorted_unique`](#sorted-unique-value)
- [`select_unsigned_integer_t`](#select-unsigned-integer-t)
- [`select_signed_integer_t`](#select-signed-integer-t)
- [`integer_like`](#integer-like)
- [`force_consteval`](#force-consteval)
- [`constexpr_for_each`](#constexpr-for-each)
- [`constexpr_for`](#constexpr-for)
- [`constexpr_for_tuple`](#constexpr-for-tuple)

<a id="sorted-unique-constructor"></a>
## `sorted_unique_t::sorted_unique_t`

Constructs the disambiguation tag used by APIs that accept already-sorted unique input.

```cpp
constexpr SimdLib::sorted_unique_t result{}; // => a sorted-unique disambiguation tag
```

<a id="sorted-unique-value"></a>
## `sorted_unique`

A ready-made `sorted_unique_t` value.

```cpp
SimdLib::sorted_unique; // => a sorted_unique_t value
```

<a id="select-unsigned-integer-t"></a>
## `select_unsigned_integer_t`

Selects the smallest standard unsigned integer type that can hold the requested number of bits, up to 64.

```cpp
using Result = SimdLib::select_unsigned_integer_t<8>; // => std::uint8_t
```

<a id="select-signed-integer-t"></a>
## `select_signed_integer_t`

Selects the smallest standard signed integer type for the requested bit width, up to 64.

```cpp
using Result = SimdLib::select_signed_integer_t<16>; // => std::int16_t
```

<a id="integer-like"></a>
## `integer_like`

Matches numeric-limits-aware integer types other than `bool`, including SimdLib integer-like extensions.

```cpp
SimdLib::integer_like<std::uint32_t>; // => true
```

<a id="force-consteval"></a>
## `force_consteval`

Forces an expression through immediate constant evaluation.

```cpp
SimdLib::force_consteval(2 + 3); // => 5
```

<a id="constexpr-for-each"></a>
## `constexpr_for_each`

Invokes a callable once for each supplied argument.

```cpp
int result = 0;
SimdLib::constexpr_for_each(
    [&](auto value) { result += value; },
    1,
    2,
    3); // => result is 6
```

<a id="constexpr-for"></a>
## `constexpr_for`

Unrolls a compile-time integer range and invokes the callable with each index as a template argument.

```cpp
std::array<int, 4> result{};
SimdLib::constexpr_for<0, 4, 1>([&]<auto index>() {
  result[index] = index;
}); // => result is {0, 1, 2, 3}
```

<a id="constexpr-for-tuple"></a>
## `constexpr_for_tuple`

Visits every element in a tuple and supplies its index and value.

```cpp
int result = 0;
SimdLib::constexpr_for_tuple(std::tuple{2, 3, 4}, [&](auto, int value) {
  result += value;
}); // => result is 9
```
