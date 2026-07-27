# Api Operation and Type Matrix

This matrix records the public `SimdLib::Api` contract. A checkmark (**✓**) means
a runtime public-API test exists at both 128 and 256 bits unless the cell names a
specific width. An X (**✗**) means the operation is intentionally constrained
away for that lane family. A shared marker identifies types that use the same
generic overload as the separately tested cell rather than a type-specific
implementation.

`Api` remains the controlling backend-availability record and the supported
C++20 surface. In a supported C++23 translation unit, the preferred spelling
for an operation on exactly one complete register is `Register<T, Bits>` or
`NativeRegister<T>`. Register availability intentionally follows the
corresponding `Api` cell rather than inventing a second implementation policy.

| Public operation family | `i8` | `u8` | `i16` | `u16` | `i32` | `u32` | `i64` | `u64` | `float` | `double` |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Construction, transfer, `set1`, and lane extraction/replacement | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Addition, subtraction, multiplication, and bitwise operations | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Logical lane shifts | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✗ | ✗ |
| Arithmetic lane shifts | ✓ | ✗ | ✓ | ✗ | ✓ | ✗ | ✓ | ✗ | ✗ | ✗ |
| Integer divide, remainder, absolute value, minimum, and maximum | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✗ | ✗ |
| Integer comparisons and `min_position`/`max_position` | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✗ | ✗ |
| Integer conversion | ✗ | ✗ | ✗ | ✗ | ✓ | ✓ | ✗ | ✗ | ✗ | ✗ |
| Floating absolute value, comparison helpers, and element extraction | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✓ | ✓ |
| Floating `set1` and bitwise operations | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✓ | ✓ |
| Compile-time logical `shuffle<indices...>` | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `uint64_t::multiply_add_adjacent` | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✓ | ✗ | ✗ |
| Whole-register byte shifts | 128 ✓ / 256 ✗ | 128 ✓ / 256 ✗ | 128 ✓ / 256 ✗ | 128 ✓ / 256 ✗ | 128 ✓ / 256 ✗ | 128 ✓ / 256 ✗ | 128 ✓ / 256 ✗ | 128 ✓ / 256 ✗ | ✗ | ✗ |
| `transform_pack` | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✗ | ✗ |
| Span transforms (in-place unary, separate-output unary, and binary) | shared¹ | shared¹ | shared¹ | shared¹ | shared¹ | ✓ | shared¹ | shared¹ | shared¹ | shared¹ |

¹ The span-transform entry point is one generic overload. Its runtime gate uses
`u32`; the other element columns do not represent separate implementations or
separately tested overloads.

## Backend-routing audit

The source audit found no test or shared test-support source that directly
references `SimdLib::Detail`, `Detail::Implementations`,
`Detail::Extensions`, or an implementation mapping. Existing tests route
through public `Api`, `SimdVector`, `SimdAlgo`, `SimdResample`, `Bmi`,
or `uint128_t` entry points. No direct `Detail` test is retained as a
supported test seam.

## Register migration boundary

| Operation category | Preferred supported surface |
| --- | --- |
| Complete-register construction, exact-width transfer, arithmetic, bitwise operations, shifts, comparisons, masks, selection, reductions, rearrangements, and constrained conversions | `Register<T, Bits>` or `NativeRegister<T>` in C++23 |
| Target-selected backend access in C++20 | `NativeApi<T>` |
| Explicit-width backend access, specialized low-level operations, and compatibility call sites | `Api<Bits, T>` |
| Span transforms, packed transforms, collection tails, and partial-register staging | `Api`, `SimdAlgo`, or the owning higher-level algorithm |
| Partial lane lists, partial or dynamic-extent transfers, native-order construction, generic implementation-specific shuffles, and runtime extraction | Intentionally absent from `Register`; retain the existing owning abstraction where available |

The complete one-register mapping is audited by
`tests/RegisterOperationMatrix.tests.cpp`. Behavioral correctness remains
independently checked against scalar references so agreement between
`Register` and `Api` cannot hide a shared defect. Execution results and exact
compiler counts belong in [Validation.md](Validation.md), not in this enduring
availability matrix.
