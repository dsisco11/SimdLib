# API availability

These compile-time queries let generic code determine whether an `Api<register_width, element_t>` specialization is supported.

## Contents

- [`is_api_available_v`](#is-api-available-v)
- [`ApiAvailable`](#apiavailable)

<a id="is-api-available-v"></a>
## `is_api_available_v`

Reports whether the requested register width and element type are supported by the current compile target.

Definition:

```cpp
template <std::size_t register_width, class element_t>
constexpr bool is_api_available_v = /* target-dependent */;
```

Example:

```cpp
SimdLib::
    is_api_available_v<128, float>; // => true when the 128-bit float facade is enabled
```

<a id="apiavailable"></a>
## `ApiAvailable`

The concept form of `is_api_available_v`, useful for constraining templates.

Definition:

```cpp
template <std::size_t register_width, class element_t>
concept ApiAvailable = is_api_available_v<register_width, element_t>;
```

Example:

```cpp
SimdLib::ApiAvailable<128, float>; // => true when `Api<128, float>` is supported
```
