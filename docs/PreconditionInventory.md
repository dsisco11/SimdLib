# Runtime precondition inventory

This document classifies every `SIMDLIB_PRECONDITION` call reachable through
the public headers and records its failure-test coverage.

## Enforcement model

`Config.h` defines the macro as `assert((condition) && (message))` unless the
consumer supplies an override.

- Without `NDEBUG`, a failed default precondition reports an assertion and
  terminates the process.
- With `NDEBUG`, the default `assert` does not evaluate the condition. Calling
  an operation with an invalid contract is unsupported and may continue into
  an out-of-bounds access, an aligned instruction with a misaligned address, or
  another undefined operation.
- A consumer override can remain active in any configuration. The dedicated
  Catch2 precondition executable uses this replacement point and
  `SIMDLIB_ENABLE_CHECKS=1` to write `SIMDLIB_PRECONDITION_FAILURE_EXPECTED_18A7E3` to stderr, flush it, and
  exit with diagnostic status 73 before execution can continue past a failed
  condition. CTest requires the marker for success; a missing marker, access
  violation, unrelated crash, or timeout fails the discovered test.
- The `SimdVector` inactive-lane invariant is additionally controlled by
  `SIMDLIB_ENABLE_CHECKS`. Its default is enabled without `NDEBUG` and disabled
  with `NDEBUG`.

The 13 caller-facing failures are ordinary Catch2 cases discovered as separate
CTest processes. A failed precondition terminates only its selected process,
so it cannot stop the remaining test suite. The override also keeps these
contract tests meaningful in Release builds where the default assertion policy
is intentionally compiled out.

## Public-header call sites

All rows use the debug/release behavior above unless the classification says
otherwise.

| Header and operation | Condition | Classification | Failure evidence |
| --- | --- | --- | --- |
| `Api.h`: `Api::load_aligned` | Source address is aligned to `byte_count`. | Caller-facing alignment contract. | `Api load_aligned terminates for a misaligned source` passes a register-width-aligned array at a one-element offset. |
| `Api.h`: `Api::load_partial<active_count>` | Runtime source size is at least `active_count`. | Caller-facing extent contract. The check is skipped during constant evaluation; an invalid constexpr access cannot form a constant expression. The template constraint separately requires `active_count <= element_count`. | `Api load_partial terminates for an undersized source` requests two active lanes from a one-element span. |
| `Api.h`: `Api::store_aligned` | Destination address is aligned to `byte_count`. | Caller-facing alignment contract. | `Api store_aligned terminates for a misaligned destination` passes a register-width-aligned array at a one-element offset. |
| `Api.h`: raw-byte `Api::store` | Destination size is at least `byte_count`. | Caller-facing extent contract. | `Api byte store terminates for an undersized destination` passes `byte_count - 1` writable bytes. |
| `SimdAlgo.h`: dynamic `BitwiseAnd` | `lhs`, `rhs`, and `write` sizes are equal. | Caller-facing extent contract. Fixed-extent overloads encode matching sizes in their span types. | `SimdAlgo BitwiseAnd terminates for mismatched extents` uses sizes 2, 1, and 2. |
| `SimdAlgo.h`: dynamic `BitwiseOr` | `lhs`, `rhs`, and `write` sizes are equal. | Caller-facing extent contract. Fixed-extent overloads encode matching sizes in their span types. | `SimdAlgo BitwiseOr terminates for mismatched extents` uses sizes 2, 1, and 2. |
| `SimdAlgo.h`: dynamic `BitwiseXor` | `lhs`, `rhs`, and `write` sizes are equal. | Caller-facing extent contract. Fixed-extent overloads encode matching sizes in their span types. | `SimdAlgo BitwiseXor terminates for mismatched extents` uses sizes 2, 1, and 2. |
| `SimdAlgo.h`: dynamic `BitwiseNot` | `lhs` and `write` sizes are equal. | Caller-facing extent contract. Fixed-extent overloads encode matching sizes in their span types. | `SimdAlgo BitwiseNot terminates for mismatched extents` uses sizes 2 and 1. |
| `SimdAlgo.h`: dynamic `BitwiseAndNot` | `lhs`, `rhs`, and `write` sizes are equal. | Caller-facing extent contract. Fixed-extent overloads encode matching sizes in their span types. | `SimdAlgo BitwiseAndNot terminates for mismatched extents` uses sizes 2, 1, and 2. |
| `SimdResample.h`: `ReduceBytesToBitsBy8_Any` | `src.size() == dst.size() * 8`. | Caller-facing extent contract. | `SimdResample reduce any terminates for an invalid shape` uses seven source bytes and one destination byte. |
| `SimdResample.h`: `ReduceBytesToBitsBy8_All` | `src.size() == dst.size() * 8`. | Caller-facing extent contract. | `SimdResample reduce all terminates for an invalid shape` uses seven source bytes and one destination byte. |
| `SimdResample.h`: `ReduceBytesToBitsBy8_Parity` | `src.size() == dst.size() * 8`. | Caller-facing extent contract. | `SimdResample reduce parity terminates for an invalid shape` uses seven source bytes and one destination byte. |
| `SimdResample.h`: `ExpandBitsToBytesBy8` | `dst.size() == src.size() * 8`. | Caller-facing extent contract. | `SimdResample expand terminates for an invalid shape` uses one source byte and seven destination bytes. |
| `SimdVector.h`: partial-result validation | Every inactive lane in an internally produced result is zero. | Internal implementation invariant, evaluated only at runtime for partial vectors when `SIMDLIB_ENABLE_CHECKS` is enabled. It is not a caller-supplied input contract and cannot be intentionally failed through a supported public call without first introducing a library defect. | `VectorChecksTests` observes the partial divide, modulus, and clamp paths and their full-vector counterparts. |

No runtime `SIMDLIB_PRECONDITION` for an index, divisor, or overlap was found.
Compile-time width, count, and availability restrictions remain enforced by
constraints and `static_assert` declarations. Entry points explicitly named
`unsafe` do not acquire an implicit checked contract through this inventory.

## Valid boundary and sanitizer evidence

`PreconditionBoundary.tests.cpp` exercises the smallest supported valid shapes
without invoking the failure hook:

- 128- and 256-bit aligned loads/stores use exactly aligned, exact-capacity
  arrays; raw-byte stores use exactly one register of storage.
- `load_partial<0>` uses an empty span and `load_partial<1>` uses exactly one
  runtime-derived element.
- All five dynamic `SimdAlgo` bitwise operations accept both empty spans and
  matching one-element spans.
- All four resampling operations accept empty spans and their minimum nonempty
  8-to-1 or 1-to-8 shape.

On 2026-07-19, Clang 22.1.8 with Debug symbols, `-O1`, ASan/UBSan,
and frame pointers passed the complete
145/145-test Debug configuration with no sanitizer diagnostics. The focused
valid-boundary selection passed 19 assertions across three cases. This proves
the valid boundary calls do not conceal an out-of-bounds access, invalid shift,
division by zero, alignment violation, or strict-aliasing failure behind the
negative child-process tests.
