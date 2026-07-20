# SimdAlgo

`SimdAlgo<type>` adapts common algorithms to arrays, spans, containers, and scalar values while selecting SIMD work when the type and target permit it.

## Contents

- [Overview](#overview)
- [Example setup](#example-setup)
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
## Example setup

```cpp
#include <SimdLib/SimdLib.h>
#include <array>
#include <span>

std::array<std::uint32_t, 64> input{};
std::array<std::uint32_t, 64> bitwiseOutput{};
std::array<std::uint8_t, 8> packedOutput{};
const std::span<const std::uint32_t, 64> inputView{input};
std::span<std::uint32_t, 64> bitwiseView{bitwiseOutput};
std::span<std::uint8_t, 8> packedView{packedOutput};
using Algo = SimdLib::SimdAlgo<32, 1>;
```

<a id="allequal"></a>
## `AllEqual`

 Returns true if all elements in equal . Intended for fast "uniform" checks. 

Signatures:

```cpp
template <std::size_t count> [[nodiscard]] constexpr static inline bool AllEqual(std::span<const read_t, count> read, const read_t predicate) noexcept
```

Example:

```cpp
const bool result = Algo::AllEqual(inputView, 0U);
```

<a id="anyequal"></a>
## `AnyEqual`

 Returns true if any element in equals . Intended for fast membership checks. 

Signatures:

```cpp
template <std::size_t count> [[nodiscard]] constexpr static inline bool AnyEqual(std::span<const read_t, count> read, const read_t predicate) noexcept
```

Example:

```cpp
const bool result = Algo::AnyEqual(inputView, 0U);
```

<a id="bitwiseand"></a>
## `BitwiseAnd`

Writes the lane-wise bitwise AND of two equally sized inputs.

Signatures:

```cpp
template <std::size_t count> static void BitwiseAnd(std::span<const read_t, count> lhs, std::span<const read_t, count> rhs, std::span<read_t, count> write) noexcept;
static void BitwiseAnd(std::span<const read_t> lhs, std::span<const read_t> rhs, std::span<read_t> write) noexcept;
```

Example:

```cpp
Algo::BitwiseAnd(inputView, inputView, bitwiseView);
```

<a id="bitwiseandnot"></a>
## `BitwiseAndNot`

Writes `~lhs & rhs` for every input lane.

Signatures:

```cpp
template <std::size_t count> static void BitwiseAndNot(std::span<const read_t, count> lhs, std::span<const read_t, count> rhs, std::span<read_t, count> write) noexcept;
static void BitwiseAndNot(std::span<const read_t> lhs, std::span<const read_t> rhs, std::span<read_t> write) noexcept;
```

Example:

```cpp
Algo::BitwiseAndNot(inputView, inputView, bitwiseView);
```

<a id="bitwisenot"></a>
## `BitwiseNot`

Writes the lane-wise bitwise complement of an input.

Signatures:

```cpp
template <std::size_t count> static void BitwiseNot(std::span<const read_t, count> lhs, std::span<read_t, count> write) noexcept;
static void BitwiseNot(std::span<const read_t> lhs, std::span<read_t> write) noexcept;
```

Example:

```cpp
Algo::BitwiseNot(inputView, bitwiseView);
```

<a id="bitwiseor"></a>
## `BitwiseOr`

Writes the lane-wise bitwise OR of two equally sized inputs.

Signatures:

```cpp
template <std::size_t count> static void BitwiseOr(std::span<const read_t, count> lhs, std::span<const read_t, count> rhs, std::span<read_t, count> write) noexcept;
static void BitwiseOr(std::span<const read_t> lhs, std::span<const read_t> rhs, std::span<read_t> write) noexcept;
```

Example:

```cpp
Algo::BitwiseOr(inputView, inputView, bitwiseView);
```

<a id="bitwisexor"></a>
## `BitwiseXor`

Writes the lane-wise bitwise exclusive OR of two equally sized inputs.

Signatures:

```cpp
template <std::size_t count> static void BitwiseXor(std::span<const read_t, count> lhs, std::span<const read_t, count> rhs, std::span<read_t, count> write) noexcept;
static void BitwiseXor(std::span<const read_t> lhs, std::span<const read_t> rhs, std::span<read_t> write) noexcept;
```

Example:

```cpp
Algo::BitwiseXor(inputView, inputView, bitwiseView);
```

<a id="compare"></a>
## `Compare`

Compares every input value with a predicate and writes a packed one-bit result.

Signatures:

```cpp
template <std::size_t count> static void Compare(std::span<const read_t, count> read, std::span<write_t, count / write_data_size> write, read_t predicate) noexcept;
```

Example:

```cpp
Algo::Compare(inputView, packedView, 0U);
```

<a id="related-types-and-constants"></a>
## Related types and constants

`read_t` and `write_t` are the integer types selected from the template bit widths. `read_width`, `write_width`, `read_data_size`, and `write_data_size` expose those choices, while `SimdImpl<count>` selects the register facade used for a fixed extent.
