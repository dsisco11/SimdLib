# PartialRegister operation ledger

## Purpose

This document fixes the public contract for `PartialRegister` before its
implementation begins. It applies to every method and operator currently
exposed by `Register`; no Register surface may be silently omitted.

`PartialRegister` is a value containing a fixed prefix of logical SIMD lanes.
It is not a partial load helper, a dynamically sized value, a mask, or a
subclass of `Register`.

## Type and representation contract

The primary value type is:

`PartialRegister<element_t, register_width, active_lane_count>`

It is available exactly when all of the following are true:

- `RegisterAvailable<element_t, register_width>` is satisfied.
- `active_lane_count` is greater than zero.
- `active_lane_count` is smaller than the native lane count for
  `Api<register_width, element_t>`.
- For a 256-bit register, the active payload extends into the upper 128-bit
  group: `active_lane_count * sizeof(element_t) * 8 > 128`.

The strict upper bound deliberately reserves a fully populated native register
for `Register<element_t, register_width>`.

`PartialRegister` is `final`, non-polymorphic, non-allocating, and contains
exactly one `Api<register_width, element_t>::vector_t`. It has no base class,
runtime metadata, active-lane mask member, proxy, or secondary native value.
The active count is compile-time metadata only.

The public metadata has these meanings:

| Name | Meaning |
| --- | --- |
| `register_width` | Physical native-register width in bits. |
| `byte_count` | Physical native-register size in bytes. |
| `native_lane_count` | Number of `element_t` lanes in the physical register. |
| `lane_count` | Number of logical active lanes. |
| `active_byte_count` | `lane_count * sizeof(element_t)`. |
| `inactive_lane_count` | `native_lane_count - lane_count`. |

Logical lanes are the contiguous low prefix with indices
`[0, lane_count)`. Native lanes at indices `[lane_count, native_lane_count)`
are inactive and must have the all-bits-zero representation. For floating
point lanes this is positive zero, not merely a value that compares equal to
zero.

The complete-register aliases `element_type`, `api_type`, `native_type`, and
`mask_type` remain available. `mask_type` names
`PartialRegisterMask<element_t, register_width, active_lane_count>`.

## Construction, native interoperation, and conversions

The native value is the sole public non-static data member, matching
`Register`'s aggregate object model and avoiding custom-constructor codegen.
Direct aggregate initialization has a documented precondition that the native
value already contains an all-bits-zero inactive suffix. Checked observation
boundaries validate that precondition; ordinary operation results and imports
establish the invariant themselves.

The implementation provides two explicit native boundaries:

- `from_native(native_type)` accepts one native value and clears its inactive
  suffix before constructing the result.
- `to_native()` returns a by-value native copy whose inactive suffix is known
  to be zero.

These are the sanitizing and validating native interoperation paths. Direct
access to `native` remains available for Register-equivalent low-level use,
subject to the aggregate-initialization precondition above.

All element and byte transfer APIs use the logical extent. `load_bytes()` and
`store_bytes()` respectively consume and produce exactly `active_byte_count`
bytes. The aligned transfer APIs retain the existing `byte_count` alignment
precondition, even though they transfer only the active logical extent.

Conversions between public value types are explicit:

| Source | Destination | Contract |
| --- | --- | --- |
| PartialRegister | Register with the same `element_t` and width | `to_register()` returns the complete register whose high lanes are the known zero suffix. |
| Register with the same `element_t` and width | PartialRegister | `from_register()` copies the low logical prefix and discards every remaining lane. |
| SimdVector | PartialRegister | No direct conversion. Callers stage through `to_array()` and `from_array()` so the width and logical extent remain explicit. |
| PartialRegister | SimdVector | No direct conversion. Callers stage through `to_array()` and an explicitly matching SimdVector construction path. |

`NativePartialRegister<element_t, active_lane_count>` is the target-selected
alias. It selects 256 bits only when that width is available and the active
payload crosses into its upper 128-bit group; otherwise it selects an available
valid 128-bit specialization.

`SimdLib::Register` owns both register-shaped public types. It already carries
the C++23 explicit-object and compiler-boundary requirements needed by
`PartialRegister`, so a separate CMake interface target would only duplicate
the same contract. The core `SimdLib::SimdLib` target remains C++20.

## Evaluation rule

For a lane-preserving value operation, the active result is the corresponding
`Api` result for the active lanes. Inactive inputs are not logical operands.
The implementation must therefore supply safe inactive native operands where
the `Api` instruction would otherwise observe an invalid value, and it must
clear every inactive result lane before constructing a PartialRegister result.

The following terms are used in the ledger:

| Term | Meaning |
| --- | --- |
| Clean | The operation is zero-closed for inactive inputs. It may construct directly from its Api result after that property is qualified for every supported cell. |
| Project | Clear the inactive result suffix after the Api operation. |
| Neutralize | Replace inactive input lanes with the operation's neutral, valid values before the Api operation, then project the result. |
| Logical | Apply the operation only to the documented logical prefix, rather than reducing or selecting complete native lanes. |
| Re-map | Produce a PartialRegister result with the documented destination active count and clear its inactive suffix. |
| Complete result | Return the existing complete Register result type because the intrinsic result layout is sparse or not a logical prefix. |

Every returned `PartialRegister` preserves the invariant. Where Register
documents an active result lane as unspecified, PartialRegister retains that
same specified-versus-unspecified status for the corresponding logical lane;
it never permits an inactive lane to be unspecified or nonzero.

## Constructors, transfer, and observation

| Register surface | PartialRegister contract | Ledger action |
| --- | --- | --- |
| `zero()` | All physical lanes are zero. | Clean |
| `broadcast(value)` | Writes `value` to the logical prefix and zero to the inactive suffix. | Project |
| `from_lanes(lanes...)` | Requires exactly `lane_count` arguments in low-to-high logical order. | Project |
| `from_array(source)` | Requires `std::array<element_t, lane_count>`. | Project |
| `load(source)` | Consumes exactly `std::span<const element_t, lane_count>`. | Project |
| `load_aligned(source)` | Same logical extent as `load`; pointer alignment remains `byte_count`. | Project |
| `load_bytes(source)` | Consumes exactly `std::span<const std::byte, active_byte_count>`. | Project |
| `store(destination)` | Produces exactly `lane_count` elements. | Logical |
| `store_aligned(destination)` | Same logical extent as `store`; pointer alignment remains `byte_count`. | Logical |
| `store_bytes(destination)` | Produces exactly `active_byte_count` bytes. | Logical |
| `to_array()` | Returns `std::array<element_t, lane_count>` in low-to-high logical order. | Logical |
| `lane<index>()` | Requires `index < lane_count`. | Logical |
| `with_lane<index>(replacement)` | Requires `index < lane_count`; replaces only that logical lane. | Project |

## Arithmetic and specialized operations

| Register surface | PartialRegister contract | Ledger action |
| --- | --- | --- |
| binary `operator+`, binary `operator-`, `operator*`, unary `operator-` | Operate pairwise over logical lanes and return the same PartialRegister specialization. | Clean |
| `operator/`, `operator%` | Operate pairwise over logical lanes. Inactive divisors are replaced with one before calling Api so they cannot create an inactive divide-by-zero or remainder-by-zero path. | Neutralize |
| `min(rhs)`, `max(rhs)` | Retain the Register operation's active-lane intrinsic semantics. Zero inactive inputs remain zero, so no suffix projection is required. | Clean |
| `absolute()`, `sqrt()`, `average(rhs)`, `multiply_add(rhs, addend)`, `add_saturated(rhs)`, `subtract_saturated(rhs)`, `add_subtract(rhs)` | Retain the Register operation's active-lane intrinsic semantics and return the same PartialRegister specialization. | Project |
| `magnitude()` | Retains the Register operation's documented 128-bit-group semantics for the logical prefix. Inactive lanes are zero inputs, do not contribute to group values, and are cleared after the operation. Register-specified sparse logical lanes retain their documented status. | Project |
| `magnitude_checked()` | Returns `partial_magnitude_checked_result_t`, whose logical prefix ends after the magnitude and overflow lane of the final occupied 128-bit group. Intermediate lanes retain the underlying API's documented unspecified status. It becomes `Register` when the final required status lane fills the native result. | Re-map |
| `normalize()` | Computes each occupied 128-bit group's magnitude through `Api` from zero-padded inputs, then clears the suffix. Every permitted 256-bit geometry occupies both physical groups, so there is no wholly inactive group requiring a fabricated divisor. Floating-environment status remains subject to the selected compiler's floating-point model, matching the existing `Register` contract. | Project |
| `horizontal_add(rhs)`, `horizontal_subtract(rhs)`, `horizontal_add_saturated(rhs)`, `horizontal_subtract_saturated(rhs)` | Retain the underlying intrinsic order, expose its low `lane_count` output lanes as the logical result prefix, and clear the suffix. | Project |
| `multiply_add_adjacent(rhs)` | Normally returns a contiguous promoted result with `ceil(lane_count / 2)` logical lanes; a final unmatched source lane is paired with zero. For 256-bit 64-bit sources, the intrinsic instead places per-group results in physical lanes zero and two, so the result is a complete `Register<element_t, 256>` preserving that sparse layout. | Re-map or complete result |
| `multiply_add_unsigned_signed_bytes(rhs)` | Returns a contiguous signed 16-bit result with `ceil(active_byte_count / 2)` logical lanes; a final unmatched byte is paired with zero. | Re-map |
| `sum_absolute_byte_differences(rhs)` | Returns a contiguous unsigned 64-bit result with `ceil(active_byte_count / 8)` logical lanes; incomplete final eight-byte groups use zero for their inactive input bytes. | Re-map |
| `multi_sum_absolute_byte_differences<imm8>(rhs)` | Returns the existing `multi_sad_result_t` complete Register type. Its immediate-selected output layout is not a contiguous logical-prefix layout. Inactive source bytes are zero. | Complete result |
| `min_position()`, `max_position()` | Consider only logical lanes and return an index in `[0, lane_count)`, choosing the first logical occurrence on ties. | Logical |
| `dot_product<imm8>(rhs)` | Retains Register's immediate-controlled active-lane semantics and rejects immediate controls that select an inactive output lane. That restriction itself guarantees a zero suffix, so no redundant projection is performed. | Clean |

`normalize()` retains `Register`'s absence of a checks-enabled precondition:
zero-magnitude active groups follow the selected floating-point behavior and
are covered as an exceptional-result case, while the inactive suffix is still
projected to bitwise zero. Division and modulus retain their active-lane
nonzero and signed-minimum preconditions; checks-enabled coverage first proves
that inactive zero divisor lanes are neutralized and then triggers each active
failure case.

The promoted type in `multiply_add_adjacent()` is the same type selected by
`multiply_add_adjacent_result_t`; the distinct logical-result alias is
`partial_multiply_add_adjacent_result_t`. The other two contiguous specialized
results similarly use `partial_byte_multiply_add_result_t` and
`partial_sad_result_t`. Checked magnitude uses
`partial_magnitude_checked_result_t` so an occupied group's overflow lane is
never discarded merely because its final source lane was the group's first,
without exposing source-prefix lanes beyond the final defined status lane.
Each alias selects `PartialRegister` while an inactive target suffix remains
and the result crosses every required physical 128-bit group. It selects
`Register` when the meaningful result count fills the target register or when
a 256-bit result is sparse or would otherwise be confined to the low 128-bit
group. This avoids discarding a valid final result lane or forming a forbidden
`PartialRegister` specialization. `multi_sad_result_t`
intentionally remains complete because its immediate-selected output layout is
not a contiguous prefix.

## Bitwise, masks, and comparisons

| Register surface | PartialRegister contract | Ledger action |
| --- | --- | --- |
| `operator&`, `operator|`, `operator^`, `andnot(rhs)` | Operate over the logical prefix and return the same PartialRegister specialization. | Clean |
| `operator~` | Complements logical lanes and clears the inactive suffix. | Project |
| `movemask()` | Returns the underlying native-granularity mask with every bit sourced solely from active bytes; high result bits are zero. | Logical |
| `lane_sign_bits()` | Returns one sign bit for each logical lane; unused high bits are zero. | Logical |
| `compare_equal(rhs)`, `compare_greater(rhs)`, `compare_greater_equal(rhs)`, `compare_less(rhs)`, `compare_less_equal(rhs)` | Return `PartialRegisterMask` with false inactive predicate lanes. | Project |
| `operator==`, `operator!=` | Compare logical lanes only. Floating NaN and signed-zero behavior matches Register for those lanes. | Logical |

`PartialRegisterMask<element_t, register_width, active_lane_count>` has one
native predicate register, the same compile-time active prefix, and an
all-zero inactive suffix. Its RegisterMask-compatible surface is `any()`,
`all()`, `none()`, `bits()`, `operator&`, `operator|`, `operator^`,
`operator~`, and `select(when_true, when_false)`. `all()` considers only the
logical prefix. `operator~` clears its inactive suffix, and `select()` always
returns a projected PartialRegister. The mask keeps RegisterMask's deliberate
absence of compound assignment operators.

## Shift operations

| Register surface | PartialRegister contract | Ledger action |
| --- | --- | --- |
| `operator<<(count)`, `logical_shift_right(count)`, `operator>>(count)` | Apply the corresponding per-lane Register behavior to logical lanes. Existing count preconditions and signedness behavior are retained. | Clean |
| `shift_bytes_left_slow(count)`, `shift_bytes_left<count>()` | Shift the active byte payload toward higher logical byte indices. Bytes shifted beyond `active_byte_count` are discarded; newly introduced low bytes are zero. | Logical |
| `shift_bytes_right_slow(count)`, `shift_bytes_right<count>()` | Shift the active byte payload toward lower logical byte indices. Bytes shifted beyond the low logical boundary are discarded; newly introduced high logical bytes are zero. | Logical |
| `shift_bits_left_slow(count)`, `shift_bits_left<count>()` | Shift the active bit payload toward higher logical bit indices. Bits shifted beyond `active_byte_count * CHAR_BIT` are discarded. | Logical |
| `shift_bits_right_slow(count)`, `shift_bits_right<count>()` | Shift the active bit payload toward lower logical bit indices. Bits shifted beyond the low logical boundary are discarded. | Logical |

The slow and immediate forms retain Register's width, availability, and count
constraints. Their logical payload extent replaces Register's complete native
byte or bit extent.

## Rearrangement and conversion operations

| Register surface | PartialRegister contract | Ledger action |
| --- | --- | --- |
| `lower_half()` | A valid 256-bit PartialRegister necessarily has more than 128 active bits, so extraction always returns the complete `Register<element_t, 128>` low half. Counts below or equal to the destination capacity cannot produce a partial result under the source type's upper-half constraint; counts above it are truncated to the complete low half. | Re-map |
| `unpack_low(rhs)`, `unpack_high(rhs)` | Retain the underlying intrinsic order and expose its low `lane_count` output lanes as the logical result prefix. `unpack_low` clears values that can spill into the suffix; `unpack_high` is zero-closed for active-prefix operands and needs no cleanup. | Project |
| `shuffle<indices...>()` | Requires exactly `lane_count` selectors and every selector to be less than `lane_count`; returns the selected logical prefix. | Logical |
| `shuffle_bytes<indices...>()` | Requires exactly `active_byte_count` selectors and every selector to be less than `active_byte_count`; returns the selected logical byte prefix. | Logical |
| `shuffle_low<imm8>()`, `shuffle_high<imm8>()` | Retain Register's immediate selector semantics where they affect logical lanes, then clear the suffix. | Project |
| `blend<imm8>(rhs)` | Retains Register's immediate selector semantics for logical lanes, then clears the suffix. | Project |
| `bit_cast<target_t>()` | Requires `active_byte_count` to be divisible by `sizeof(target_t)` and returns a partial or complete result exposing `active_byte_count / sizeof(target_t)` lanes. Because bit-casting preserves the active bit extent, every valid 256-bit source also produces a result whose active extent reaches the upper half. | Re-map |
| `convert<target_t>()` | Returns a partial or complete target with one meaningful target lane per active source lane for the same supported Api conversion cells. | Re-map |
| `widen_low<target_t, target_bits>()` | Returns a partial or complete target exposing `min(lane_count, Api<target_bits, target_t>::element_count)` consumed source lanes. A derived 256-bit result that would leave its upper half entirely inactive is unavailable. Register exposes no narrowing counterpart, so PartialRegister does not add one. | Re-map |

Any derived result that fills its destination register is represented by the
corresponding complete `Register`. Any derived partial result must satisfy
`PartialRegisterAvailable`, including the 256-bit upper-half rule.

## Surface and performance commitments

Every counterpart preserves Register's static/member form, template parameter
order, `[[nodiscard]]`, `constexpr`, `noexcept`, `SIMD_FLAGS`, and Api-based
availability requirements unless this ledger explicitly changes a logical
extent, selector domain, partial result type, or partial-only precondition.

PartialRegister also preserves Register's deliberate absence of
`operator+=`, `operator-=`, `operator*=`, `operator/=`, `operator%=`,
`operator&=`, `operator|=`, `operator^=`, `operator<<=`, and `operator>>=`.

The accepted performance baseline is one native register passed and returned
with the same calling-convention intent as Register. A PartialRegister wrapper
may emit the instructions required to build safe inactive operands or to clear
the inactive suffix. It may not introduce allocations, runtime lane-count
loads, proxy objects, hidden aggregate members, or wrapper-only work unrelated
to the documented invariant.
