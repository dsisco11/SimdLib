# NativeApi

`NativeApi<element_t>` selects the widest `Api` specialization enabled by the
compile target. It remains the supported facade for C++20, collection helpers,
and direct backend operations. C++23 complete-register expressions should use
`NativeRegister<element_t>` instead; explicit `Register<element_t, Bits>` is
required when storage or ABI must remain stable across target configurations.

## Contents

- [Selection rule](#selection-rule)
- [Example](#example)
- [Methods](#methods)

## Selection rule

```cpp
template <class element_t>
using NativeApi = Api<is_api_available_v<256, element_t> ? 256 : 128, element_t>;
```

The alias selects 256 bits when that facade is available and otherwise selects 128 bits. Availability describes features enabled for the current compilation target; it is not runtime CPU dispatch.

## Example

```cpp
#include <SimdLib/SimdLib.h>

#include <array>

using FloatApi = SimdLib::NativeApi<float>;

std::array<float, 8> output{};
FloatApi::transform(
    std::array{1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F},
    output,
    [](auto values) {
      return FloatApi::multiply(values, values);
    }); // => output is {1.0F, 4.0F, 9.0F, 16.0F, 25.0F, 36.0F, 49.0F, 64.0F}
```

## Methods

`NativeApi<element_t>` is a type alias, so it owns no separate methods. It exposes every method, nested type, and constant documented on the complete [`Api`](Api.md) page.
