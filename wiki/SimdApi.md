# SimdApi compatibility aliases

`SimdApi.h` preserves the old public names for source compatibility. New code should use `Api`, `ApiAvailable`, and `is_api_available_v`.

## Contents

- [`SimdApi`](#simdapi)
- [`SimdApiAvailable`](#simdapiavailable)
- [`is_simd_api_available_v`](#is-simd-api-available-v)

<a id="simdapi"></a>
## `SimdApi`

Deprecated alias for `Api<register_width, element_t>`. Its methods are documented on [`Api`](Api.md).

```cpp
using OldApi [[deprecated]] = SimdLib::SimdApi<128, float>;
using NewApi = SimdLib::Api<128, float>;
```

<a id="simdapiavailable"></a>
## `SimdApiAvailable`

Deprecated alias for the `ApiAvailable` concept.

```cpp
static_assert(SimdLib::SimdApiAvailable<128, float>);
```

<a id="is-simd-api-available-v"></a>
## `is_simd_api_available_v`

Deprecated alias for `is_api_available_v`.

```cpp
static_assert(SimdLib::is_simd_api_available_v<128, float>);
```

