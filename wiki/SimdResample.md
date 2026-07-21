# SimdResample

`SimdLib::SimdResample` converts between byte-per-element masks and compact bit masks. Size relationships are part of each function's contract.

## Contents

- [Overview](#overview)
- [Example include](#example-setup)
- [`ExpandBitsToBytesBy8`](#expandbitstobytesby8)
- [`ReduceBytesToBitsBy8_All`](#reducebytestobitsby8-all)
- [`ReduceBytesToBitsBy8_Any`](#reducebytestobitsby8-any)
- [`ReduceBytesToBitsBy8_Parity`](#reducebytestobitsby8-parity)

<a id="overview"></a>
## Overview

Include `<SimdLib/SimdResample.h>`. Overloads with the same name are collected in one subsection; every public overload is listed below.

<a id="example-setup"></a>
## Example include

```cpp
#include <SimdLib/SimdResample.h>
```
<a id="expandbitstobytesby8"></a>
## `ExpandBitsToBytesBy8`

Expands each packed source bit to one byte (`1 -> 0xFF`, `0 -> 0x00`).

Signatures:

```cpp
void ExpandBitsToBytesBy8(std::span<const std::uint8_t> src, std::span<std::uint8_t> dst)
```

Example:

```cpp
std::array<std::uint8_t, 8> result{};
SimdLib::SimdResample::ExpandBitsToBytesBy8(
    std::array<std::uint8_t, 1>{0b0000'0101},
    result); // => {0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00}
```

<a id="reducebytestobitsby8-all"></a>
## `ReduceBytesToBitsBy8_All`

Packs one bit per source byte, set when the byte is exactly `0xFF`.

Signatures:

```cpp
void ReduceBytesToBitsBy8_All(std::span<const std::uint8_t> src, std::span<std::uint8_t> dst)
```

Example:

```cpp
std::array<std::uint8_t, 1> result{};
SimdLib::SimdResample::ReduceBytesToBitsBy8_All(
    std::array<std::uint8_t, 8>{0xFF, 0, 0xFF, 0, 0, 0, 0, 0},
    result); // => result[0] is 0b0000'0101
```

<a id="reducebytestobitsby8-any"></a>
## `ReduceBytesToBitsBy8_Any`

Resamples packed-bit masks where each byte represents eight logical elements.

Signatures:

```cpp
/// Packs one bit per source byte, set when the byte is nonzero. void ReduceBytesToBitsBy8_Any(std::span<const std::uint8_t> src, std::span<std::uint8_t> dst)
void ReduceBytesToBitsBy8_Any(std::span<const std::uint8_t> src, std::span<std::uint8_t> dst)
```

Example:

```cpp
std::array<std::uint8_t, 1> result{};
SimdLib::SimdResample::ReduceBytesToBitsBy8_Any(
    std::array<std::uint8_t, 8>{0, 4, 0, 2, 0, 0, 0, 0},
    result); // => result[0] is 0b0000'1010
```

<a id="reducebytestobitsby8-parity"></a>
## `ReduceBytesToBitsBy8_Parity`

Packs one bit per source byte, set when the byte has odd parity.

Signatures:

```cpp
void ReduceBytesToBitsBy8_Parity(std::span<const std::uint8_t> src, std::span<std::uint8_t> dst)
```

Example:

```cpp
std::array<std::uint8_t, 1> result{};
SimdLib::SimdResample::ReduceBytesToBitsBy8_Parity(
    std::array<std::uint8_t, 8>{1, 3, 7, 0, 0, 0, 0, 0},
    result); // => result[0] is 0b0000'0101
```
