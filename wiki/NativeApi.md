# NativeApi

`NativeApi<element_t>` selects the widest `Api` specialization enabled by the compile target, so normal users do not need to choose between 128-bit and 256-bit registers.

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

using FloatApi = SimdLib::NativeApi<float>;

const auto scale = FloatApi::set1(2.0F);
const auto offset = FloatApi::set1(10.0F);
const auto result = FloatApi::multiply_add(value, scale, offset);
```

## Methods

`NativeApi<element_t>` is a type alias, so it owns no separate methods. It exposes every method, nested type, and constant documented on the complete [`Api`](Api.md) page.

