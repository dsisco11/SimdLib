# SimdResample

`SimdLib::SimdResample` converts between byte-per-element masks and compact bit masks. Size relationships are part of each function's contract.

## Contents

- [Overview](#overview)
- [Example setup](#example-setup)
- [`ExpandBitsToBytesBy8`](#expandbitstobytesby8)
- [`ReduceBytesToBitsBy8_All`](#reducebytestobitsby8-all)
- [`ReduceBytesToBitsBy8_Any`](#reducebytestobitsby8-any)
- [`ReduceBytesToBitsBy8_Parity`](#reducebytestobitsby8-parity)

<a id="overview"></a>
## Overview

Include `<SimdLib/SimdResample.h>`. Overloads with the same name are collected in one subsection; every public overload is listed below.

<a id="example-setup"></a>
## Example setup

```cpp
#include <SimdLib/SimdResample.h>
#include <array>

std::array<std::uint8_t, 16> source{};
std::array<std::uint8_t, 2> packed{};
auto destination = std::span{packed};
```

<a id="expandbitstobytesby8"></a>
## `ExpandBitsToBytesBy8`

Expands each packed source bit to one byte (`1 -> 0xFF`, `0 -> 0x00`).

Signatures:

```cpp
inline void ExpandBitsToBytesBy8( const std::span<const std::uint8_t> src, const std::span<std::uint8_t> dst) noexcept
```

Example:

```cpp
SimdLib::SimdResample::ExpandBitsToBytesBy8(source, destination);
```

<a id="reducebytestobitsby8-all"></a>
## `ReduceBytesToBitsBy8_All`

Packs one bit per source byte, set when the byte is exactly `0xFF`.

Signatures:

```cpp
inline void ReduceBytesToBitsBy8_All( const std::span<const std::uint8_t> src, const std::span<std::uint8_t> dst) noexcept
```

Example:

```cpp
SimdLib::SimdResample::ReduceBytesToBitsBy8_All(source, destination);
```

<a id="reducebytestobitsby8-any"></a>
## `ReduceBytesToBitsBy8_Any`

Resamples packed-bit masks where each byte represents eight logical elements.

Signatures:

```cpp
/// Packs one bit per source byte, set when the byte is nonzero. inline void ReduceBytesToBitsBy8_Any( const std::span<const std::uint8_t> src, const std::span<std::uint8_t> dst) noexcept
inline void ReduceBytesToBitsBy8_Any( const std::span<const std::uint8_t> src, const std::span<std::uint8_t> dst) noexcept
```

Example:

```cpp
SimdLib::SimdResample::ReduceBytesToBitsBy8_Any(source, destination);
```

<a id="reducebytestobitsby8-parity"></a>
## `ReduceBytesToBitsBy8_Parity`

Packs one bit per source byte, set when the byte has odd parity.

Signatures:

```cpp
inline void ReduceBytesToBitsBy8_Parity( const std::span<const std::uint8_t> src, const std::span<std::uint8_t> dst) noexcept
```

Example:

```cpp
SimdLib::SimdResample::ReduceBytesToBitsBy8_Parity(source, destination);
```

