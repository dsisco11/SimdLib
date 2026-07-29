# Register Implementation Matrix

This document makes the accepted design in `RegisterProposal.md` executable and
traceable. The proposal controls semantics; `ApiOperationMatrix.md` controls the
current backend availability matrix; `RegisterImplementation.todo` controls the
order and completion gates. A disagreement is resolved by correcting these
documents before implementing the affected operation.

The supported compiler, configuration, generated-code, ABI, and exception
boundaries are defined by `RegisterQualification.md`.

## Contract identity

| Field | Value |
| --- | --- |
| Register widths | 128-bit SSE4.2 and AVX2; 256-bit AVX2 |
| Element types | `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, `int32_t`, `uint32_t`, `int64_t`, `uint64_t`, `float`, `double` |
| Existing language baseline | C++20 through `SimdLib::SimdLib` |
| Register language baseline | C++23 explicit object parameters through the opt-in `SimdLib::Register` target |

### Portability requirements

The cross-platform compiler boundary requires these C++20 portability rules:

- `tests/Api128.tests.cpp` now passes the fixed-extent output of
  `Api::transform_pack<1>()` as an explicit `std::span<uint8_t, 1>`. The count
  also appears in the declared span extent and cannot be deduced portably through
  an implicit `std::array` conversion.
- `Api::transform_pack()` compiles its native-word store only when the output can
  contain a complete native word. This states the existing size invariant and
  prevents GCC from diagnosing an unreachable eight-byte store into a smaller
  result object.
- The transform tail tests retain their immediate canaries but give the backing
  objects at least one full-register access of physical capacity. This prevents
  GCC's inliner from diagnosing the unreachable full-register path against a
  three-element allocation while preserving the logical one-element span.
- `SimdVector` default construction delegates unconditionally to the existing
  constexpr `Api::setzero()` path. This preserves intrinsic runtime zeroing and
  avoids assigning `{}` directly to GCC's native vector extension type.

These portability rules do not change a public declaration.

## Contract traceability

| Contract | Accepted implementation requirement | Owning task | Required evidence |
| --- | --- | ---: | --- |
| Template identity | All new public templates, concepts, aliases, and examples use `<T, Bits>`; only internal delegation uses `Api<Bits, T>` | 3, 9 | Compile probes and public-source audit |
| Availability | `SIMDLIB_REGISTER_INTERFACE_AVAILABLE` is computed from the standard explicit-object feature macro or the documented MSVC 19.44 fallback and cannot be overridden | 1 | Positive and negative configuration probes |
| Build boundary | `SimdLib::SimdLib` remains C++20; `SimdLib::Register` requests C++23, requires Register availability, and selects `/std:c++latest` for Microsoft C++ | 1 | CMake consumer probes and generated command inspection |
| Reproducible toolchains | GCC and GNU-like Clang container environments are pinned, locally and CI reusable, aggregate failures reliably, and remain explicitly separate from native Windows ABI evidence | 2 | Dockerfile provenance, Compose/orchestrator comparison, clean/failing matrix demonstrations |
| Supported geometry | A specialization owns one complete 128-bit or 256-bit native register and has no logical active count | 3 | Availability, size, alignment, and lane-count assertions |
| Representation | Register and RegisterMask are aggregates with one public native vector member each; neither type has bases, metadata, allocation, proxies, or address-dependent state | 3 | Aggregate/layout traits and ABI inspection |
| Special members | Register and RegisterMask use implicit trivial copy/move construction, assignment, and destruction; their member initializers explicitly use the intrinsic-backed zero operation | 3, 4 | Type traits and zero-construction code generation |
| All-active invariant | Every lane participates in transfer, arithmetic, comparison, rearrangement, and reduction behavior | 4-9 | Distinctive highest-lane runtime and constexpr tests |
| Transfer extent | Element and byte loads/stores use fixed extents equal to `lane_count` or `byte_count`; partial and unsafe forms do not exist | 4 | Compile rejection, canaries, and sanitizers |
| Alignment | Aligned loads/stores require `byte_count` alignment and follow the existing SimdLib precondition configuration | 4, 10 | Checks-enabled failures and release code generation |
| Scalar operands | Arithmetic and bitwise operations initially accept only the same Register type; scalar use requires explicit `broadcast()` | 4, 6 | Compile rejection and broadcast code generation |
| Integer division | Because x86 has no packed integer divide instruction, the named `_ext128_div_{epi,epu}{8,16,32,64}` methods explicitly extract, divide, and reinsert every lane with constant-index intrinsics; the matching `_ext256_` methods divide two 128-bit halves and reassemble them without a fold helper, runtime selector, or addressable array | 6, 10 | Scalar-oracle correctness and register-only wrapper-versus-raw generated-code parity for every integer type and width |
| Native interoperation | Register and RegisterMask support explicit aggregate-brace initialization from one complete native value and expose their representation through the public `native` member; direct mask initialization requires canonical predicate lanes | 4, 5 | Aggregate/constructibility assertions and native-result ABI probes |
| Explicit object parameters | Active non-static members take the explicit object by value; compound assignment is intentionally disabled and its implementations remain preserved in source comments | 3-9 | Declaration audit, constraint rejection, and reassignment code-generation probes |
| Calling convention | Register-shaped members use `VECTORCALL` where supported; consumer-defined non-inlined boundaries must opt in separately | 3, 10 | Vector/default convention wrapper-versus-raw mirrors |
| Mask invariant | Comparisons and mask operations produce all-zero/all-one predicate lanes; direct aggregate initialization has the same canonical-lane precondition | 5 | Constraint tests, predicate-bit tests, and documented aggregate precondition |
| Compact mask bits | `bits_type` is normalized from lane count, is `uint32_t` for initial widths, maps bit `i` to lane `i`, and clears unused bits | 5 | Static assertions and mask-pattern tests |
| Comparison semantics | Named comparisons reproduce the selected intrinsic, including signedness, NaNs, signed zero, ordered/unordered predicates, and lane bit patterns | 5 | Runtime, portable, emulated, and constexpr parity |
| Whole equality | `operator==` means all lanes compare equal; `operator!=` is its Boolean negation; relational operators are absent | 5 | Boolean and compile-rejection tests |
| Shift counts | Per-lane negative counts are invalid; logical overshifts zero, arithmetic overshifts sign-fill, and byte/whole-register shifts follow the proposal boundary table | 6 | Boundary, precondition, constexpr, and codegen tests |
| Immediate controls | Every `imm8` is constrained to `0..255`; logical element and byte shuffles require exactly one selector per output lane or byte, permit repeated selectors, and reject selectors outside the complete source register | 7, 8 | Compile-success/failure boundaries |
| Rearrangement order | `lower_half()`, unpacking, and shuffling use logical low-to-high lanes or bytes. The 256-bit logical element and byte shuffles may select from the complete source register across the 128-bit boundary; lane-group restrictions remain only on operations whose names or intrinsic contracts specify them | 8 | Independent lane and byte oracles, cross-half selectors, highest-position sentinels, and exact code-generation parity |
| Type-changing results | Public operations name the exact constrained namespace-level result alias and never expose a raw intrinsic result | 7 | Type assertions and unsupported-combination rejection |
| Conversion split | `bit_cast()` preserves bits; `convert()` changes numeric values; `widen_low()` explicitly consumes only low source lanes | 8 | Independent bit/numeric/lane-consumption tests |
| Zero overhead | No supported register-only wrapper expression or call boundary adds instructions, moves, spills, reloads, stack traffic, temporaries, return buffers, branches, or indirection relative to the identical raw baseline | 3, 10 | Mandatory exact-parity generated-code and ABI gates with provenance |
| MSVC `/GS` boundary | Register-only fixture subsets and ABI mirrors retain strict wrapper-versus-raw gates. The sole accepted Release exception is the exact 128-bit `Register<double>::from_array` cookie sequence recognized by the comparator; all remaining instructions must match. Store, transfer, mutating-reference, opaque-call, and array-return fixtures that can write memory retain `/GS`, stay outside the general zero-overhead claim when they differ, and preserve their paired disassembly as review evidence | 3, 10 | Register-only, lane, type-matrix, and ABI comparison stamps; paired memory-writing profiles; comparison result; provenance; and `RegisterQualification.md` exception ledger |
| Compatibility | `Api` remains supported; collection transforms and compatibility-only operations do not migrate | 9, 11 | Final ledger audit and unchanged C++20 matrix |
| Public exposure | `SimdLib.h` conditionally includes `Register.h` when `SIMDLIB_REGISTER_INTERFACE_AVAILABLE` is nonzero; C++20 translation units retain the existing umbrella surface | 1, 11 | C++20 exclusion, C++23 umbrella, isolated-header, ODR, and external-consumer gates |

## Explicit exclusions

| Excluded surface | Classification | Reason |
| --- | --- | --- |
| Partial load/store or lane construction | Higher-level responsibility | Register has no inactive lanes or fill policy |
| Dynamic-extent `load_unsafe` | `Api` compatibility-only | Its precondition is unsuitable for the restrictive value type |
| Native-order `set` | `Api` compatibility-only | Public lane order is logical low-to-high |
| Implicit scalar broadcast | Excluded | Broadcast cost and intent remain explicit |
| Implicit native conversion or mutable native reference | Excluded | Native access is an explicit by-value boundary |
| Scalar mask construction or `from_bits()` | Excluded | Native aggregate interoperation stays explicit and scalar expansion policy remains deferred |
| Runtime `extract_slow` | Api-only slow path | Register deliberately exposes only compile-time lane access |
| Register-selector `shuffle(value, selector)` | Api-only native control | Register exposes portable logical and byte shuffles instead of the backend register signature |
| `expand` and `compress` | Compatibility-only | Result width, lane consumption, and saturation are ambiguous |
| Multi-register widening/narrowing | Separate future design | One Register operation produces one complete result Register |
| Scalar arithmetic overloads | Deferred additive API | Real call sites and code generation must first justify them |
| Compound assignment overloads | Excluded | Reassignment is equally expressive, while mutable wrapper references cause a redundant 32-byte stack-alignment frame for 256-bit values under MSVC 19.44 |
| `RegisterMask::from_bits()` | Deferred additive API | Scalar-to-vector expansion cost and demand are not established |
| 512-bit registers and AVX-512 predicate registers | Future extension | Initial storage and mask contract is limited to 128/256-bit vectors |
| Span transforms and `transform_pack` | Collection-owned | Iteration and tail policy remain outside Register |
| `FinishIntegerMagnitudeFromPairSums`, `TransformForMaxPosition`, `compare_each_element` | Internal-only | These remain backend or compatibility helpers |

## Type-changing result matrix

| Public alias | Exact result | Availability rule |
| --- | --- | --- |
| `multiply_add_adjacent_result_t<T, Bits>` | Same signedness at twice the lane width through 64 bits; 64-bit lanes remain 64-bit | Alias and method exist only for backend-supported source types/widths |
| `byte_multiply_add_result_t<T, Bits>` | `Register<int16_t, Bits>` | Supported signed/unsigned byte input combinations only |
| `sad_result_t<T, Bits>` | `Register<uint64_t, Bits>` | Backend-supported SAD combinations only |
| `multi_sad_result_t<T, Bits>` | `Register<uint16_t, Bits>` | Backend-supported multi-SAD combinations only |

## Public operation migration matrix

The disposition column records whether the preferred Register surface implements
the operation or intentionally leaves it in a compatibility or collection layer.

| Current public `Api` operation | Register result | Disposition |
| --- | --- | --- |
| `load` | `Register::load(fixed_span)` | Implemented |
| `load_aligned` | `Register::load_aligned(fixed_span)` | Implemented |
| `load_unaligned` | Canonicalized to `Register::load(fixed_span)` | Implemented |
| `load_partial` | No Register operation | Compatibility |
| `load_unsafe` | No Register operation | Compatibility |
| Element `store` | `value.store(fixed_span)` | Implemented |
| `store_aligned` | `value.store_aligned(fixed_span)` | Implemented |
| `store_unaligned` | Canonicalized to `value.store(fixed_span)` | Implemented |
| Fixed-byte `store` | `value.store_bytes(fixed_byte_span)` | Implemented |
| Dynamic-byte `store` | No Register operation | Compatibility |
| Fixed-byte `load` | `Register::load_bytes(fixed_byte_span)` | Implemented |
| `construct(array)` | `Register::from_array(array)` | Implemented |
| `to_array` | `value.to_array()` | Implemented |
| `setzero` | Default construction and `Register::zero()` | Implemented |
| `set1` | `Register::broadcast(value)` | Implemented |
| `setr` | `Register::from_lanes(...)` | Implemented |
| `set`, `set_partial`, `setr_partial` | No Register operation | Compatibility |
| `add` | `lhs + rhs` | Implemented |
| `subtract` | `lhs - rhs` | Implemented |
| `multiply` | `lhs * rhs` | Implemented |
| `divide` | `lhs / rhs` | Implemented |
| `modulus` | `lhs % rhs` | Implemented |
| `negate` | `-value` | Implemented |
| `min` | `lhs.min(rhs)` | Implemented |
| `max` | `lhs.max(rhs)` | Implemented |
| `multiply_add` | `lhs.multiply_add(rhs, addend)` | Implemented |
| `widen` | `value.widen_low<target_t, target_bits>()` | Implemented |
| `absolute` | `value.absolute()` | Implemented |
| `sqrt` | `value.sqrt()` | Implemented |
| `magnitude` | `value.magnitude()` | Implemented |
| `magnitude_checked` | `value.magnitude_checked()` | Implemented |
| `normalize` | `value.normalize()` | Implemented |
| `avg` | `lhs.average(rhs)` | Implemented |
| `add_horizontal` | `lhs.horizontal_add(rhs)` | Implemented |
| `subtract_horizontal` | `lhs.horizontal_subtract(rhs)` | Implemented |
| `multiply_add_adjacent` | `lhs.multiply_add_adjacent(rhs)` with named result alias | Implemented |
| `multiply_add_unsigned_signed_bytes` | Same named member with byte-multiply-add result alias | Implemented |
| `sum_absolute_byte_differences` | Same named member with SAD result alias | Implemented |
| `multi_sum_absolute_byte_differences` | Same named immediate member with multi-SAD result alias | Implemented |
| `min_position` | `value.min_position()` | Implemented |
| `max_position` | `value.max_position()` | Implemented |
| `add_saturated` | `lhs.add_saturated(rhs)` | Implemented |
| `subtract_saturated` | `lhs.subtract_saturated(rhs)` | Implemented |
| `hadd_saturated` | `lhs.horizontal_add_saturated(rhs)` | Implemented |
| `hsubtract_saturated` | `lhs.horizontal_subtract_saturated(rhs)` | Implemented |
| `add_subtract` | `lhs.add_subtract(rhs)` | Implemented |
| `dot_product` | `lhs.dot_product<imm8>(rhs)` | Implemented |
| `bitwise_and` | `lhs & rhs` | Implemented |
| `bitwise_or` | `lhs \| rhs` | Implemented |
| `bitwise_xor` | `lhs ^ rhs` | Implemented |
| `bitwise_not` | `~value` | Implemented |
| `bitwise_andnot` | `lhs.andnot(rhs)` with preserved polarity | Implemented |
| `select` | `mask.select(when_true, when_false)` | Implemented |
| `movemask` | `value.movemask()` with intrinsic-native granularity | Implemented |
| `movemask_slim` | `value.lane_sign_bits()` with one bit per lane | Implemented |
| `compare_equal`, `compare_greater`, `compare_greater_equal`, `compare_less`, `compare_less_equal` | Corresponding named comparison | Implemented |
| `cmp_eq_mask`, `cmp_gt_mask`, `cmp_ge_mask`, `cmp_lt_mask`, `cmp_le_mask` | No compact-mask Register counterpart | Compatibility |
| `cmp_eq_slim`, `cmp_gt_slim`, `cmp_ge_slim`, `cmp_lt_slim`, `cmp_le_slim` | Corresponding named comparison followed by `.bits()` | Implemented |
| Deprecated `cmp_eq`, `cmp_gt`, `cmp_ge`, `cmp_lt`, `cmp_le` | Corresponding explicitly named `cmp_*_mask` method | Compatibility |
| `expand`, `compress` | No Register operation | Compatibility |
| `extract<index>` | `value.lane<index>()` | Implemented |
| Runtime `extract_slow` | No Register operation | Explicit Api slow path |
| `lower_half` | `value.lower_half()` | Implemented |
| `insert<index>` | `value.with_lane<index>(lane)` | Implemented |
| `unpack_lo` | `lhs.unpack_low(rhs)` | Implemented |
| `unpack_hi` | `lhs.unpack_high(rhs)` | Implemented |
| `shuffle<indices...>` | `value.shuffle<indices...>()` | Implemented for every arithmetic element type at 128 and 256 bits |
| `Api<Bits, std::uint8_t>::shuffle<indices...>` | `value.shuffle_bytes<indices...>()` | Implemented for every arithmetic element type at 128 and 256 bits; result retains its element type |
| Register-selector `shuffle(value, selector)` | No generic Register operation | Native Api control; Register exposes `shuffle_bytes<indices...>()` |
| `shuffle_lo<imm8>`; `shuffle_lo_slow` | `value.shuffle_low<imm8>()` | Compile-time form implemented; scalar runtime control remains Api-only |
| `shuffle_hi<imm8>`; `shuffle_hi_slow` | `value.shuffle_high<imm8>()` | Compile-time form implemented; scalar runtime control remains Api-only |
| `blend<imm8>`; register-mask `blend`; `blend_slow` | `lhs.blend<imm8>(rhs)`; predicate selection uses `mask.select()` | Compile-time and native-register controls mapped; scalar runtime control remains Api-only |
| `shift_left` | `value << count` | Implemented |
| `shift_right` | `value.logical_shift_right(count)`; unsigned `operator>>` | Implemented |
| `shift_right_arithmetic` | Signed `value >> count` | Implemented |
| `byte_shift_left_slow` | `value.byte_shift_left_slow(count)` | Implemented |
| `byte_shift_right_slow` | `value.byte_shift_right_slow(count)` | Implemented |
| Runtime `bit_shift_left_slow` | `value.bit_shift_left_slow(count)` | Implemented |
| Compile-time `bit_shift_left` | `value.bit_shift_left<count>()` | Implemented |
| Runtime `bit_shift_right_slow` | `value.bit_shift_right_slow(count)` | Implemented |
| Compile-time `bit_shift_right` | `value.bit_shift_right<count>()` | Implemented |
| `bit_cast` | `value.bit_cast<target_t>()` | Implemented |
| `convert_to_float` | `value.convert<float>()` | Implemented |
| `convert_to_int` | `value.convert<int32_t>()` | Implemented |
| Explicit-target `convert<target_t>` | `value.convert<target_t>()` | Implemented |
| Inferred-target `convert` | No Register operation | Compatibility |
| `transform_pack` | No Register operation | Collection |
| Unary and binary span `transform` overloads | No Register operation | Collection |
| `TransformForMaxPosition` | No Register operation | Internal |
| `compare_each_element` | Internal comparison fallback only | Internal |

### Inventory audit

A Clang AST declaration audit of `include/SimdLib/Api.h` identifies 92 unique
public static-operation names after excluding compiler-generated lambda call
helpers. The six additional operations exposed through inherited
`using impl::...` declarations—`add`, `divide`, `max`, `min`, `multiply`, and
`subtract`—produce 98 unique public operation names. Every name is classified
above. Overloaded `load`, `store`, `extract`, `insert`, `shuffle`,
`shuffle_lo`, `shuffle_hi`, `blend`, `bit_shift_*`, `convert`, and span
`transform` families are split whenever their Register dispositions differ.
The protected `TransformForMaxPosition` and `compare_each_element` helpers are
classified separately as internal operations.

### Register evidence matrix

The supported-cell oracle is executable rather than hand-maintained:
`tests/RegisterOperationMatrix.tests.cpp` instantiates all ten element types at
128 and 256 bits, compares every conditional `IRegister` concept against its
`IApi` counterpart, verifies every unconditional `IRegister` and `IRegisterMask`
declaration, and audits every source/target cell for `bit_cast`, `convert`, and
`widen_low`. A supported cell is therefore exactly a cell accepted by that
compile-time audit; no prose-only availability list can drift independently.

| Public family | Runtime semantics | Constexpr semantics | Constraints and exclusions | Generated code | ABI |
| --- | --- | --- | --- | --- | --- |
| Construction, observation, and full-width transfer | [`Register.tests.cpp`](../tests/Register.tests.cpp) | [`RegisterConstexpr.tests.cpp`](../tests/constexpr/RegisterConstexpr.tests.cpp) | [`RegisterOperationMatrix.tests.cpp`](../tests/RegisterOperationMatrix.tests.cpp), [`RegisterDynamicTransfer.cpp`](../tests/compile_fail/register/RegisterDynamicTransfer.cpp), and the lane-list/native/scalar/uninitialized probes in [`tests/compile_fail/register`](../tests/compile_fail/register) | [`RegisterCodegenFixture.h`](../tests/codegen/RegisterCodegenFixture.h) and [`RegisterTypeMatrixCodegenFixture.h`](../tests/codegen/RegisterTypeMatrixCodegenFixture.h) | [`RegisterAbi.cpp`](../tests/codegen/RegisterAbi.cpp), [`RegisterAbiRaw.cpp`](../tests/codegen/RegisterAbiRaw.cpp), [`RegisterDefaultAbi.cpp`](../tests/codegen/RegisterDefaultAbi.cpp), and [`RegisterDefaultAbiRaw.cpp`](../tests/codegen/RegisterDefaultAbiRaw.cpp) |
| RegisterMask, comparisons, reductions, and predicate selection | [`Register.tests.cpp`](../tests/Register.tests.cpp) | [`RegisterConstexpr.tests.cpp`](../tests/constexpr/RegisterConstexpr.tests.cpp) | [`RegisterOperationMatrix.tests.cpp`](../tests/RegisterOperationMatrix.tests.cpp) | [`RegisterCodegenFixture.h`](../tests/codegen/RegisterCodegenFixture.h) and [`RegisterTypeMatrixCodegenFixture.h`](../tests/codegen/RegisterTypeMatrixCodegenFixture.h) | Register and mask signatures in the paired ABI fixtures above |
| Basic arithmetic, bitwise operations, compact masks, and shifts | [`RegisterBasicOperations.tests.cpp`](../tests/RegisterBasicOperations.tests.cpp) and [`RegisterPreconditionFailure.tests.cpp`](../tests/RegisterPreconditionFailure.tests.cpp) | [`RegisterConstexpr.tests.cpp`](../tests/constexpr/RegisterConstexpr.tests.cpp) for the Api-constexpr subset | [`RegisterOperationMatrix.tests.cpp`](../tests/RegisterOperationMatrix.tests.cpp) and [`RegisterPreconditionFailure.tests.cpp`](../tests/RegisterPreconditionFailure.tests.cpp) | [`RegisterCodegenFixture.h`](../tests/codegen/RegisterCodegenFixture.h) and [`RegisterTypeMatrixCodegenFixture.h`](../tests/codegen/RegisterTypeMatrixCodegenFixture.h) | Paired Register/native unary, binary, scalar-result, and mutating-signature ABI fixtures above |
| Specialized arithmetic and reductions | [`RegisterSpecializedOperations.tests.cpp`](../tests/RegisterSpecializedOperations.tests.cpp) | Not a constant-evaluated `Api` surface unless a method is separately covered by the constexpr fixture | [`RegisterOperationMatrix.tests.cpp`](../tests/RegisterOperationMatrix.tests.cpp) | [`RegisterSpecializedCodegenFixture.h`](../tests/codegen/RegisterSpecializedCodegenFixture.h) | Type-changing and scalar-result signatures in the paired ABI fixtures above |
| Rearrangement, immediate controls, and lower-half extraction | [`RegisterRearrangementConversion.tests.cpp`](../tests/RegisterRearrangementConversion.tests.cpp) and [`LogicalShuffleRegister.tests.cpp`](../tests/LogicalShuffleRegister.tests.cpp) | [`RegisterConstexpr.tests.cpp`](../tests/constexpr/RegisterConstexpr.tests.cpp) | [`RegisterOperationMatrix.tests.cpp`](../tests/RegisterOperationMatrix.tests.cpp), [`RegisterInvalidByteShuffleSelector.cpp`](../tests/compile_fail/register/RegisterInvalidByteShuffleSelector.cpp), [`RegisterWrongByteShuffleSelectorCount.cpp`](../tests/compile_fail/register/RegisterWrongByteShuffleSelectorCount.cpp), and the other selector/immediate/compatibility probes in [`tests/compile_fail/register`](../tests/compile_fail/register) | [`RegisterRearrangementCodegenFixture.h`](../tests/codegen/RegisterRearrangementCodegenFixture.h) | Register/native return signatures in the paired ABI fixtures above |
| Bit reinterpretation, numeric conversion, and explicit low-lane widening | [`RegisterRearrangementConversion.tests.cpp`](../tests/RegisterRearrangementConversion.tests.cpp) | [`RegisterConstexpr.tests.cpp`](../tests/constexpr/RegisterConstexpr.tests.cpp) | All source/target cells in [`RegisterOperationMatrix.tests.cpp`](../tests/RegisterOperationMatrix.tests.cpp), plus unsupported-target and unavailable-width probes in [`tests/compile_fail/register`](../tests/compile_fail/register) | [`RegisterRearrangementCodegenFixture.h`](../tests/codegen/RegisterRearrangementCodegenFixture.h) | Type-changing Register/native return signatures in the paired ABI fixtures above |
| Compatibility-only partial, unsafe, scalar, native-order, runtime-selector, inferred-target, generic-selector, and collection operations | Not part of Register | Not part of Register | Dedicated compile-failure probes in [`tests/compile_fail/register`](../tests/compile_fail/register), including [`RegisterCollectionOperations.cpp`](../tests/compile_fail/register/RegisterCollectionOperations.cpp) | Not part of Register | Not part of Register |

### Public-surface invariants

- `Register<T, Bits>` and `RegisterMask<T, Bits>` are constrained at the class
  boundary by `RegisterAvailable<T, Bits>`. Operations available for every
  valid specialization inherit that constraint; conditional operations add an
  `IApi` concept or an immediate/index/width constraint before the body.
- Register-facing traits, concepts, aliases, examples, diagnostics, and result
  types use `<T, Bits>` order. Only internal delegation uses `Api<Bits, T>`.
- Public operation results are Register, RegisterMask, or documented scalar
  types. Register has no base class, inherited backend members, public
  implementation selector, or public `SimdLib::Detail` dependency.
- All ordinary operations consume every active input lane. `widen_low()` names
  and documents its consumed source prefix; `lower_half()` explicitly names its
  lower-half result; sparse integer magnitude layouts document every defined
  result lane and still consume every input lane.
- Every production class and active method in `Register.h`, `RegisterMask.h`,
  and `RegisterFwd.h` has a Doxygen contract. Conditional methods document
  availability, selectors and lane-moving methods document logical order, and
  preconditioned methods document their valid domains.
- Partial and dynamic transfer, implicit scalar/native construction,
  native-order construction, runtime extraction, generic implementation
  selectors, inferred conversion targets, and collection algorithms are
  rejected by the registered compile-failure sources under
  `tests/compile_fail/register`.

## Precondition and selector matrix

| Surface | Contract | Failure evidence |
| --- | --- | --- |
| Full element transfer | Fixed extent equals `lane_count` | Compile rejection |
| Raw-byte transfer | Fixed extent equals `byte_count` | Compile rejection and canaries |
| Aligned transfer | Address is aligned to `byte_count` | Checks-enabled negative test |
| Lane access/replacement | `index < lane_count` | Constraint rejection |
| Logical shuffle | Exactly one selector per output lane; repeated selectors permitted; every selector names a lane in the complete source register; no zero-fill sentinel | Count/range constraint rejection and positive cross-half coverage |
| Logical byte shuffle | Exactly `byte_count` selectors; repeated selectors permitted; every selector is less than `byte_count`; no zero-fill sentinel | Count/range constraint rejection and positive cross-half coverage |
| Immediate operations | `0 <= imm8 <= 255` | Constraint rejection at `-1` and `256` |
| Per-lane logical/left shift | Runtime count is nonnegative; count at least lane width yields zero | Negative precondition and boundary tests |
| Per-lane arithmetic shift | Runtime count is nonnegative; oversized count clamps to `lane_width - 1` | Negative precondition and sign-fill tests |
| 128-bit byte shift | Count at most zero is identity; count at least 16 is zero | Runtime and constexpr boundaries |
| Runtime 128-bit whole-register shift | Count at most zero is identity; count at least 128 is zero | Runtime and constexpr boundaries |
| Compile-time whole-register shift | Negative rejected; count at least 128 is zero | Compile rejection and constexpr test |
| Unsupported operation/type/width | Removed from overload resolution | Requires-expression and compile-failure probes |

## Compiler and configuration matrix

| Surface | Compiler | Architecture/configuration | Requirement |
| --- | --- | --- | --- |
| C++20 core | MSVC 19.44 | Windows x64; Debug and Release | Existing full public matrix remains supported |
| C++20 core | clang-cl 22.1.8 | Windows x64; Debug and Release | Existing full public matrix remains supported |
| C++20 core | Clang 22.1.8 | Linux x64; Debug and Release | Existing full public matrix remains supported |
| C++20 core | GCC 13.2 | Linux x64; Debug and Release | Existing full public matrix remains supported; Register unavailable |
| C++20 core sanitizer | Clang 22.1.8 | Linux x64 Debug, `-O1`, ASan/UBSan, frame pointers | No sanitizer diagnostics |
| Register | MSVC 19.44 | Windows x64, `/std:c++latest`; supported ISA profiles | SSE4.2 diagnostics and strict AVX2 gates; memory-writing fixtures retain `/GS` and the exact documented exception |
| Register | clang-cl 22.1.8 | Windows x64, C++23; supported ISA profiles | SSE4.2 diagnostics and strict AVX2 correctness, ABI, and generated-code gates |
| Register | Clang 22.1.8 | Linux x64, C++23; supported ISA profiles | SSE4.2 diagnostics and strict AVX2 correctness, ABI, and generated-code gates |
| Register | GCC 14 or newer | Linux x64, C++23; supported ISA profiles | SSE4.2 diagnostics and strict AVX2 correctness, ABI, and generated-code gates |

Linux x64 GCC 13.2 remains the required unavailable-interface probe; it is not
a Register compiler. A Register compiler floor is lowered or expanded only after
the complete correctness, layout, ABI, and generated-code gates pass.

## Test and evidence ownership

| Evidence family | Source owner | CMake/CTest owner |
| --- | --- | --- |
| Runtime Register correctness | `tests/Register.tests.cpp` | `RegisterSse42Tests`, `RegisterAvx2Tests` |
| Runtime mask/comparison correctness | `tests/Register.tests.cpp` | `RegisterSse42Tests`, `RegisterAvx2Tests` |
| Complete public-surface and availability audit | `tests/RegisterOperationMatrix.tests.cpp` | `RegisterSse42Tests`, `RegisterAvx2Tests` |
| Shared independent scalar oracles | Focused helpers in each Register runtime test source | Included only by public Register tests |
| Constexpr contracts | `tests/constexpr/RegisterConstexpr.tests.cpp` | `RegisterConstexpr128Probe`, `RegisterConstexpr256Probe` |
| Availability and language modes | `tests/availability/Register*.cpp` | Compile-only Register availability targets |
| Configuration fallback/exclusion | `tests/config/Register*.cpp` | Compile-only Register configuration targets |
| First-and-only headers | `tests/headers/RegisterHeaderProbe.cpp`, `tests/headers/RegisterMaskHeaderProbe.cpp`, and `tests/headers/SimdLibRegisterHeaderProbe.cpp` | `HeaderRegisterProbe`, `HeaderRegisterMaskProbe`, `HeaderSimdLibRegisterProbe` |
| Invalid declarations | `tests/compile_fail/register/*.cpp` | CMake `try_compile`/CTest compile-failure driver |
| ODR and multi-TU use | `tests/register_odr/main.cpp`, `tests/register_odr/second_translation_unit.cpp` | `RegisterOdr` |
| External consumer | `tests/consumer/register.cpp` and consumer CMake target | Existing consumer CTest project linked through `SimdLib::Register` |
| Forced-inline code generation | `tests/codegen/RegisterCodegen.cpp` and `RegisterCodegenFixture.h` | `RegisterCodegen` plus compiler-specific extraction scripts |
| Raw code-generation baselines | `tests/codegen/RegisterCodegenRaw.cpp` and `RegisterCodegenFixture.h` | Paired with `RegisterCodegen` under identical flags |
| Non-inlined ABI mirrors | `tests/codegen/RegisterAbi.cpp`, `tests/codegen/RegisterAbiRaw.cpp` | ABI records owned by `RegisterCodegen128Sse42`, `RegisterCodegen128Avx2`, and `RegisterCodegen256Avx2` |
| Register pressure and opaque calls | `tests/codegen/RegisterCodegenFixture.h` | Register code-generation gate |
| Code-generation comparison | `cmake/CompareRegisterCodegen.cmake` and checked-in allowlisted normalization rules | CTest mandatory performance gate |
| Checks-enabled preconditions | `tests/RegisterPreconditionFailure.tests.cpp` | Existing precondition death-test infrastructure |
| Sanitizers | Runtime Register and mask sources | Fresh Clang ASan/UBSan configuration |
| Supplemental benchmarks | `benchmarks/Register.benchmarks.cpp` | `Benchmarks`; never a correctness/codegen substitute |
| Final evidence | This document and `docs/Validation.md` | Updated after each completed task |

Every production class and method has Doxygen documentation. Test
and generated-code sources use only public SimdLib declarations except the
proposal-approved narrow internal comparison adapter tests.

## Validation ownership

Compiler commands and result files are runtime artifacts rather than durable
documentation. CMake presets, CI workflows, and `ContainerValidation.md` own
the reproducible invocation contract; generated build trees, JUnit reports,
provenance files, and logs own individual outcomes.

## Language and build-integration design

This work introduces only the language boundary. `Register.h` deliberately
contains no Register or RegisterMask declaration until the representation work
begins. It also remains absent from `SimdLib.h`.

| Requirement | Contract |
| --- | --- |
| Computed availability | `Config.h` computes `SIMDLIB_REGISTER_INTERFACE_AVAILABLE` from `__cpp_explicit_this_parameter >= 202110L`, or from non-clang Microsoft C++ 19.44 with `_MSVC_LANG > 202002L` |
| Non-overridable result | Defining the availability macro is rejected with `SIMDLIB_REGISTER_INTERFACE_AVAILABILITY_IS_COMPUTED` |
| Requirement signal | `SIMDLIB_REQUIRE_REGISTER_INTERFACE` defaults to zero and diagnoses unavailable required use without changing availability |
| Core target | `SimdLib::SimdLib` retains only `cxx_std_20` |
| Opt-in target | `SimdLib::Register` links the core target, requests `cxx_std_23`, and publishes `SIMDLIB_REQUIRE_REGISTER_INTERFACE=1` |
| Microsoft language selection | Only Microsoft C++ receives `/std:c++latest`; clang-cl and GNU-like Clang use their CMake-selected C++23 modes |
| Focused header | Direct unsupported inclusion of `Register.h` emits `SIMDLIB_REGISTER_HEADER_REQUIRES_CXX23` |
| Positive syntax | The enabled probe compiles named, arithmetic, comparison, and reference-mutating explicit-object members using `VECTORCALL` |
| Reproducible negative probes | The compile-failure inputs and public headers are configure dependencies; every fresh or affected configuration reruns each `try_compile` and records its compiler output |
| External consumers | The core consumer explicitly remains C++20; the separate Register consumer receives C++23 only by linking `SimdLib::Register` |

## Container-environment design

The container environment uses Alpine Linux for both GNU-like compiler services. The complete
Release, feature-labelled, sanitizer, constexpr, configuration, header,
consumer, and C++23 availability gates are required to remain on Alpine/musl.
A larger distribution is considered only after a concrete incompatibility is
recorded and the next-smallest maintained option is evaluated.

| Environment | Immutable base | Toolchain contract |
| --- | --- | --- |
| `gcc14` | Alpine 3.22.5 manifest digest `sha256:14358309a308569c32bdc37e2e0e9694be33a9d99e68afb0f5ff33cc1f695dce` | GCC 14.2.0, CMake 4.4.0, Ninja 1.12.1, musl 1.2.5 |
| `clang22` | Alpine 3.24.1 manifest digest `sha256:28bd5fe8b56d1bd048e5babf5b10710ebe0bae67db86916198a6eec434943f8b` | Clang 22.1.3, libc++, LLD, compiler-rt, libunwind, binutils 2.45.1, CMake 4.4.0, Ninja 1.13.2, musl 1.2.6 |

Both multi-stage Dockerfiles verify the CMake 4.4.0 source checksum and bake in
the peeled Catch2 v3.8.1 commit. Exact runtime package versions are pinned. The
BuildKit Dockerfile frontend is pinned to the digest used by the no-cache proof.
The containers run as a non-root user with a read-only root and source mount,
dropped capabilities, an executable temporary filesystem, and explicit
writable outputs. The Clang image intentionally omits the GCC compiler after
the CMake builder stage. A build operation configures each fingerprint-owned
tree once and CI applies CMake's fresh-toolchain behavior during that configure
step. Test and benchmark-execution operations validate the completed manifest
and never configure, clear, or rebuild the tree.

The exhaustive build and test operations collectively cover the complete
Linux-supported C++20/C++23 suite, not a platform-independent subset. Portable
header repairs guard the Windows-only `<intrin.h>` boundary, include x86
intrinsics only on x86, disable `VECTORCALL` for GNU-like Linux Clang, and
value-initialize the temporary used by `register_set_constexpr`. Native Windows jobs
remain authoritative for MSVC, clang-cl, Windows ABI, and calling-convention
evidence.

### Compose and orchestration decision

`compose.yml` is the single declarative environment used locally and in CI. It
uses a shared service anchor and one compiler-service profile.
`tools/Run-ContainerMatrix.ps1` owns the build-cell matrix: it builds selected
images once, starts compiler cells with bounded concurrency, waits for every
exit, retains separate logs, and removes only that invocation's unique Compose
project.

A separate Compose healthcheck is intentionally absent: these are one-shot
`compose run` jobs, for which Compose does not wait on the service's own health
state. The canonical entrypoint instead performs synchronous compiler, CMake,
CPU-feature, and argument preflight before any configure or test work.
Generated-code comparisons are ordinary artifacts of each owning build cell.

Direct parallel `docker compose up` interleaves logs, retains stopped service
containers, and cannot provide deterministic all-service failure attribution.
Direct `compose run` provides isolation but requires repeated arguments. The
wrapper therefore remains the smallest interface satisfying aggregate status,
cancellation, and artifact requirements while Compose remains the environment
definition. Its intentional-failure and cancellation switches exercise
one-service failure, multi-service failure, partial-log retention, and
unique-project cleanup.

The scheduled reproducibility workflow runs the environment inspection action
with Docker caching disabled and records image inspection output. Normal CI
first builds every Linux cell and then runs a compile-free test operation
through the same wrapper and Dockerfiles. There is no CI-only Linux dependency
installation path. Exact local commands, artifact conventions,
refresh/security procedure, and project-owned cleanup are recorded in
`ContainerValidation.md`.
