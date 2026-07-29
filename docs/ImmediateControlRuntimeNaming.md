# Runtime controls for immediate-mode operations

Many x86 SIMD instructions encode their control value directly in the instruction. That control must therefore be known while the caller is compiled. SimdLib reserves an unsuffixed operation name for this compile-time form and for genuinely native runtime-control instructions.

A name ending in `_slow` is a deliberate runtime substitute for an operation whose native counterpart normally requires a compile-time immediate. The substitute preserves the operation's semantics for a runtime scalar control, but it may require dispatch, branching, or a longer synthesized instruction sequence. The suffix describes the control mechanism; it does not mean that every call is necessarily slow after inlining and constant propagation.

## Operation inventory

| Operation family | Unsuffixed compile-time or native runtime form | Runtime immediate substitute | Exposed layers |
| --- | --- | --- | --- |
| Lane extraction | `extract<index>(value)`; `Register::lane<index>()` | `extract_slow(value, index)` | `Api`, implementation; `SimdVector` uses the Api slow path internally |
| Lane insertion | `insert<index>(value, lane)`; `Register::with_lane<index>(lane)` | `insert_slow(value, lane, index)` | `Api`, implementation |
| Immediate blend | `blend<imm8>(lhs, rhs)` | `blend_slow(lhs, rhs, control)` | `Api`, implementation, extension helper |
| Register-mask blend | `blend(lhs, rhs, mask)` | Not applicable; the mask register is a native runtime control | `Api`, implementation |
| Floating shuffle | Immediate or compile-time logical `shuffle` forms | `shuffle_slow(lhs, rhs, control)` | `Api`, implementation, extension helper |
| Byte shuffle | `shuffle(value, selector_register)` | Not applicable; the selector register is a native runtime control | `Api`, implementation |
| Low 16-bit half shuffle | `shuffle_lo<imm8>(value)` | `shuffle_lo_slow(value, control)` | `Api`, implementation, extension helper |
| High 16-bit half shuffle | `shuffle_hi<imm8>(value)` | `shuffle_hi_slow(value, control)` | `Api`, implementation, extension helper |
| 32-bit group shuffle | `shuffle_32<imm8>(value)` | `shuffle_32_slow(value, control)` | `Api` through its implementation mapping, implementation, extension helper |
| Complete-register byte shift | No public immediate spelling is currently exposed | `byte_shift_left_slow(value, count)`, `byte_shift_right_slow(value, count)` | `Api`, `Register`, implementation, extension helper |
| Complete-register bit shift | `bit_shift_left<count>(value)`, `bit_shift_right<count>(value)` | `bit_shift_left_slow(value, count)`, `bit_shift_right_slow(value, count)` | `Api`, `Register`, implementation, extension helper |
| Ordinary per-lane shift | `shift_left(value, count)`, `shift_right(value, count)`, and arithmetic variants | Not applicable; the runtime count uses native variable-count instructions | `Api`, `Register`, `SimdVector`, implementation |

`Register` intentionally exposes compile-time lane access and immediate rearrangement, but it does not add dynamic lane extraction, dynamic lane insertion, or scalar-control blend and shuffle members. `SimdVector` likewise has no public immediate-control emulation surface; its reductions use `Api::extract_slow` internally when a lane is selected at runtime.

## Choosing a form

Use the unsuffixed template form whenever the control is part of the algorithm and can be expressed as a template argument. Use an unsuffixed register-control overload when the instruction family natively accepts a selector or mask register. Use `_slow` only when the control is genuinely determined at runtime and the immediate-mode operation's semantics are required.