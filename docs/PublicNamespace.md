# Public namespace surface

This document records the public namespace inventory taken from commit
`95c2118efee480b61c2c955caae73f03909eec9b` before the namespace migration,
and the resulting supported surface for SimdLib 0.2.0.

## Pre-migration inventory

| Header | Public declarations before migration | Customizations and notes |
| --- | --- | --- |
| `Config.h` | `SimdLib::Config` feature constants and the root `vectorcall_enabled` constant | Compiler-control macros remain global preprocessor configuration points. |
| `TemplateTools.h` | `sorted_unique_t`, `select_unsigned_integer_t`, `select_signed_integer_t`, `is_specialization`, `constexpr_for_each`, `constexpr_for`, and `constexpr_for_tuple` | Root utility declarations are unchanged. |
| `SimdApi.h` | `is_simd_api_available_v`, `SimdApiAvailable`, and `SimdApi<register_width, element_t>` | `SimdApi` owns the complete static register-operation member surface; backend mappings remain in `Detail`. |
| `SimdVector.h` | Root `SimdVector<element_t, element_count>`, eight `Vector*` aliases, and sixteen fixed-width signed/unsigned aliases | `std::hash<SimdLib::SimdVector<...>>` is provided. |
| `SimdAlgo.h` | Root `SimdAlgo<read_width, write_width>` | The algorithm facade remains unchanged pending the Tensor design work. |
| `SimdResample.h` | The `SimdLib::SimdResample` namespace and its four byte-mask reduction/expansion functions | The namespace remains unchanged to keep this migration about naming topology rather than behavior. |
| `Bmi.h` | The `SimdLib::Bmi` namespace, `integer_like`, generic bit helpers, BMI1 operations, BMI2 operations, and portable fallbacks hidden in `Bmi::Detail` | Wide `uint128_t` overloads were also reachable through redundant root forwarding functions in `UInt128.h`. |
| `UInt128.h` | Root `uint128_t`, `_u128`, `popcount`, count/width/power-of-two helpers, and root BMI forwarding functions | `std::numeric_limits<SimdLib::uint128_t>` and `std::hash<SimdLib::uint128_t>` are provided. |
| `Format.h` | No new SimdLib names | Provides `std::formatter` specializations for root `SimdVector` and root `uint128_t`; parsing/rendering helpers remain in `SimdLib::Detail`. |
| `SimdLib.h` | The union of the focused public headers | This is the opt-in umbrella header and retains the deprecated facade aliases during the compatibility window; formatting remains separately opt-in through `Format.h`. |

Public class members remain members of their owning root types and are not
separate namespace declarations. The inventory includes their owning type so
the rename preserves the complete member API rather than selecting a subset.

## Final namespace map

| Responsibility | Supported name |
| --- | --- |
| Automatically sized SIMD facade | `SimdLib::NativeApi<element_t>` |
| Register-width SIMD facade | `SimdLib::Api<register_width, element_t>` |
| Availability query and constraint | `SimdLib::is_api_available_v` and `SimdLib::ApiAvailable` |
| Automatically sized complete-register value | `SimdLib::NativeRegister<element_t>` |
| Explicit-width complete-register value | `SimdLib::Register<element_t, register_width>` |
| Complete-register predicate value | `SimdLib::RegisterMask<element_t, register_width>` |
| Automatically sized partial-register value | `SimdLib::NativePartialRegister<element_t, active_lane_count>` |
| Explicit-width partial-register value | `SimdLib::PartialRegister<element_t, register_width, active_lane_count>` |
| Partial-register predicate value | `SimdLib::PartialRegisterMask<element_t, register_width, active_lane_count>` |
| Fixed logical SIMD value | `SimdLib::SimdVector<element_t, element_count>` |
| Fixed-width register aliases | C++23 root complete-register aliases and `partial_*` alias templates |
| Bit manipulation | `SimdLib::Bmi` |
| Unsigned wide integer | `SimdLib::uint128_t` |
| Byte-mask resampling | `SimdLib::SimdResample` |
| Low-level backends and formatter helpers | `SimdLib::Detail` |

`Api` is accepted because its containing namespace and focused include,
`<SimdLib/Api.h>`, already provide the SIMD-library context. Repeating `Simd`
in `SimdLib::SimdApi` adds length without distinguishing another public API.
The short name also reads clearly in aliases such as
`using u32x4_api = SimdLib::Api<128, std::uint32_t>`.

For C++23 complete-register expressions, `NativeRegister<element_t>` is the
preferred entry point when consumers do not require a fixed register width.
Explicit `Register<element_t, register_width>` is required when storage layout
or an ABI contract must remain stable across target configurations.

`PartialRegister<element_t, register_width, active_lane_count>` is the sibling
value type for one native register whose contiguous low-lane prefix is logical
data and whose remaining lanes are always all-bits-zero. Use
`NativePartialRegister<element_t, active_lane_count>` when target-selected width
is appropriate, and use the explicit-width spelling at storage or ABI
boundaries. `PartialRegisterMask` provides the matching active-prefix predicate.

`SimdVector<element_t, element_count>` remains the fixed logical vector type;
it is not a register-tail policy. Collection operations such as `SimdAlgo`
continue to own iteration and dynamic final-batch handling. In short,
`Register` means every native lane is active, `PartialRegister` means a
compile-time low-lane prefix is active in one native register, `SimdVector`
means one fixed logical value, and collection algorithms decide how a sequence
is divided into complete and partial work.

`NativeApi<element_t>` remains the preferred backend facade for C++20,
collection helpers, compatibility code, and specialized low-level operations.
It selects the 256-bit facade when the compile target enables it and otherwise
selects the 128-bit facade. Explicit `Api<register_width, element_t>` remains
the supported form for width-specific backend algorithms.

`SimdResample` remains unchanged. It names a cohesive, existing operation
family and changing it would add churn without improving the requested type
topology.

## Compatibility and versioning

`<SimdLib/Api.h>` is the primary header. `<SimdLib/SimdApi.h>` remains a
compatibility forwarding header and supplies deprecated `SimdApi`,
`SimdApiAvailable`, and `is_simd_api_available_v` names. They are scheduled for
removal in 1.0.0; new code must use the final names.

The redundant root `blsmsk`, `blsr`, `blsi`, `bzhi`, `andn`, and `bextr`
overloads for `uint128_t` are removed. Their supported spellings are under
`SimdLib::Bmi`, alongside the integral overloads. This is an intentional
pre-1.0 source break, so the project version advances from 0.1.0 to 0.2.0.

`Detail` is not a consumer API. Focused headers may use it internally, but
public examples, tests acting as consumers, and downstream projects must not
name it.
