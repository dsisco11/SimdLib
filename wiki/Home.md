# SimdLib API wiki

This wiki documents every supported public SimdLib facade, class, namespace utility, and compatibility alias. Start with `SimdVector` for small math vectors or `NativeApi` for register-oriented bulk processing.

## Contents

- [Friendly APIs](#friendly-apis)
- [Low-level and bulk APIs](#low-level-and-bulk-apis)
- [Utilities and integration](#utilities-and-integration)
- [Compatibility](#compatibility)

## Friendly APIs

- [`SimdVector`](SimdVector.md) — position-vector-like arithmetic and vector math.
- [`NativeApi`](NativeApi.md) — the widest compile-time-supported SIMD register facade without spelling `128` or `256`.

## Low-level and bulk APIs

- [`Api`](Api.md) — complete register facade, with every method and overload.
- [`SimdAlgo`](SimdAlgo.md) — bulk comparisons and bitwise algorithms.
- [`SimdResample`](SimdResample.md) — packed-bit/byte-mask conversion.
- [`Bmi`](Bmi.md) — integer bit-manipulation helpers.
- [`uint128_t`](UInt128.md) — portable unsigned 128-bit integer.

## Utilities and integration

- [API availability](Api-Availability.md) — compile-time support queries.
- [Configuration](Config.md) — version, compiler, target, and instruction constants.
- [Template tools](TemplateTools.md) — public compile-time helpers and tag types.
- [Formatting](Formatting.md) — `std::format` and hashing integration.
- [Technical reference](Technical-Reference.md) — build profiles, contracts, and lower-level details.

## Compatibility

- [`SimdApi`](SimdApi.md) — deprecated spellings retained for existing callers.

