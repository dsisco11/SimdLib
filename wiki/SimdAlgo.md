# SimdAlgo

`SimdAlgo<type>` adapts common algorithms to arrays, spans, containers, and scalar values while selecting SIMD work when the type and target permit it.

## Contents

- [Overview](#overview)
- [Example alias](#example-setup)
- [`AllEqual`](#allequal)
- [`AnyEqual`](#anyequal)
- [`BitwiseAnd`](#bitwiseand)
- [`BitwiseAndNot`](#bitwiseandnot)
- [`BitwiseNot`](#bitwisenot)
- [`BitwiseOr`](#bitwiseor)
- [`BitwiseXor`](#bitwisexor)
- [`Compare`](#compare)
- [Related types and constants](#related-types-and-constants)

<a id="overview"></a>
## Overview

Include `<SimdLib/SimdAlgo.h>`. Overloads with the same name are collected in one subsection; every public overload is listed below.

<a id="example-setup"></a>
## Example alias

```cpp
#include <SimdLib/SimdLib.h>

using Algo = SimdLib::SimdAlgo<32, 1>;
```
<a id="allequal"></a>
## `AllEqual`

Returns true when every input element equals the predicate. Intended for fast uniformity checks.

Signatures:

```cpp
template <std::size_t count> static bool AllEqual(std::span<const read_t, count> read, read_t predicate)
```

Example:

```cpp
Algo::AllEqual(
    std::span<const std::uint32_t, 4>{std::array{3U, 3U, 3U, 3U}}, 3U); // => true
```

<a id="anyequal"></a>
## `AnyEqual`

Returns true when at least one input element equals the predicate. Intended for fast membership checks.

Signatures:

```cpp
template <std::size_t count> static bool AnyEqual(std::span<const read_t, count> read, read_t predicate)
```

Example:

```cpp
Algo::AnyEqual(
    std::span<const std::uint32_t, 4>{std::array{1U, 2U, 3U, 4U}}, 3U); // => true
```

<a id="bitwiseand"></a>
## `BitwiseAnd`

Writes the lane-wise bitwise AND of two equally sized inputs.

Signatures:

```cpp
template <std::size_t count> static void BitwiseAnd(std::span<const read_t, count> lhs, std::span<const read_t, count> rhs, std::span<read_t, count> write)
static void BitwiseAnd(std::span<const read_t> lhs, std::span<const read_t> rhs, std::span<read_t> write)
```

Example:

```cpp
std::array<std::uint32_t, 2> result{};
Algo::BitwiseAnd(
    std::span<const std::uint32_t, 2>{std::array{0b1100U, 0b0011U}},
    std::span<const std::uint32_t, 2>{std::array{0b1010U, 0b0110U}},
    std::span<std::uint32_t, 2>{result}); // => {0b1000U, 0b0010U} (&)
```

<a id="bitwiseandnot"></a>
## `BitwiseAndNot`

Writes `~lhs & rhs` for every input lane.

Signatures:

```cpp
template <std::size_t count> static void BitwiseAndNot(std::span<const read_t, count> lhs, std::span<const read_t, count> rhs, std::span<read_t, count> write)
static void BitwiseAndNot(std::span<const read_t> lhs, std::span<const read_t> rhs, std::span<read_t> write)
```

Example:

```cpp
std::array<std::uint32_t, 2> result{};
Algo::BitwiseAndNot(
    std::span<const std::uint32_t, 2>{std::array{0b1100U, 0b0011U}},
    std::span<const std::uint32_t, 2>{std::array{0b1010U, 0b0110U}},
    std::span<std::uint32_t, 2>{result}); // => {0b0010U, 0b0100U} (~lhs & rhs)
```

<a id="bitwisenot"></a>
## `BitwiseNot`

Writes the lane-wise bitwise complement of an input.

Signatures:

```cpp
template <std::size_t count> static void BitwiseNot(std::span<const read_t, count> lhs, std::span<read_t, count> write)
static void BitwiseNot(std::span<const read_t> lhs, std::span<read_t> write)
```

Example:

```cpp
std::array<std::uint32_t, 2> result{};
Algo::BitwiseNot(
    std::span<const std::uint32_t, 2>{std::array{0b1100U, 0b0011U}},
    std::span<std::uint32_t, 2>{result}); // => {0xFFFFFFF3U, 0xFFFFFFFCU} (~)
```

<a id="bitwiseor"></a>
## `BitwiseOr`

Writes the lane-wise bitwise OR of two equally sized inputs.

Signatures:

```cpp
template <std::size_t count> static void BitwiseOr(std::span<const read_t, count> lhs, std::span<const read_t, count> rhs, std::span<read_t, count> write)
static void BitwiseOr(std::span<const read_t> lhs, std::span<const read_t> rhs, std::span<read_t> write)
```

Example:

```cpp
std::array<std::uint32_t, 2> result{};
Algo::BitwiseOr(
    std::span<const std::uint32_t, 2>{std::array{0b1100U, 0b0011U}},
    std::span<const std::uint32_t, 2>{std::array{0b1010U, 0b0110U}},
    std::span<std::uint32_t, 2>{result}); // => {0b1110U, 0b0111U} (|)
```

<a id="bitwisexor"></a>
## `BitwiseXor`

Writes the lane-wise bitwise exclusive OR of two equally sized inputs.

Signatures:

```cpp
template <std::size_t count> static void BitwiseXor(std::span<const read_t, count> lhs, std::span<const read_t, count> rhs, std::span<read_t, count> write)
static void BitwiseXor(std::span<const read_t> lhs, std::span<const read_t> rhs, std::span<read_t> write)
```

Example:

```cpp
std::array<std::uint32_t, 2> result{};
Algo::BitwiseXor(
    std::span<const std::uint32_t, 2>{std::array{0b1100U, 0b0011U}},
    std::span<const std::uint32_t, 2>{std::array{0b1010U, 0b0110U}},
    std::span<std::uint32_t, 2>{result}); // => {0b0110U, 0b0101U} (^)
```

<a id="compare"></a>
## `Compare`

Compares every input value with a predicate and writes a packed one-bit result.

Signatures:

```cpp
template <std::size_t count> static void Compare(std::span<const read_t, count> read, std::span<write_t, count / write_data_size> write, read_t predicate)
```

Example:

```cpp
std::array<std::uint8_t, 1> result{};
Algo::Compare(
    std::span<const std::uint32_t, 8>{std::array{3U, 1U, 3U, 2U, 0U, 0U, 0U, 0U}},
    std::span<std::uint8_t, 1>{result},
    3U); // => result[0] is 0b0000'0101
```

<a id="related-types-and-constants"></a>
## Related types and constants

`read_t` and `write_t` are the integer types selected from the template bit widths. `read_width`, `write_width`, `read_data_size`, and `write_data_size` expose those choices, while `SimdImpl<count>` selects the register facade used for a fixed extent.
