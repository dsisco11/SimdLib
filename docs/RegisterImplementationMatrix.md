# Register Implementation Matrix

This document makes the accepted design in `RegisterProposal.md` executable and
traceable. The proposal controls semantics; `ApiOperationMatrix.md` controls the
current backend availability matrix; `RegisterImplementation.todo` controls the
order and completion gates. A disagreement is resolved by correcting these
documents before implementing the affected operation.

## Contract identity

| Field | Value |
| --- | --- |
| Register widths | 128-bit SSE4.2 and 256-bit AVX2 |
| Element types | `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, `int32_t`, `uint32_t`, `int64_t`, `uint64_t`, `float`, `double` |
| Existing language baseline | C++20 through `SimdLib::SimdLib` |
| Register language baseline | C++23 explicit object parameters through the future `SimdLib::Register` target |

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

| Contract | Accepted implementation requirement | Owning phase | Required evidence |
| --- | --- | ---: | --- |
| Template identity | All new public templates, concepts, aliases, and examples use `<T, Bits>`; only internal delegation uses `Api<Bits, T>` | 3, 9 | Compile probes and public-source audit |
| Availability | `SIMDLIB_REGISTER_INTERFACE_AVAILABLE` is computed from the standard explicit-object feature macro or the documented MSVC 19.44 fallback and cannot be overridden | 1 | Positive and negative configuration probes |
| Build boundary | `SimdLib::SimdLib` remains C++20; `SimdLib::Register` requests C++23, requires Register availability, and selects `/std:c++latest` for Microsoft C++ | 1 | CMake consumer probes and generated command inspection |
| Reproducible toolchains | GCC and GNU-like Clang container environments are pinned, locally and CI reusable, aggregate failures reliably, and remain explicitly separate from native Windows ABI evidence | 2 | Dockerfile provenance, Compose/orchestrator comparison, clean/failing matrix demonstrations |
| Supported geometry | A specialization owns one complete 128-bit or 256-bit native register and has no logical active count | 3 | Availability, size, alignment, and lane-count assertions |
| Representation | Register and RegisterMask each contain exactly one native vector member and no bases, metadata, allocation, proxies, or address-dependent state | 3 | Layout traits and ABI inspection |
| Special members | Copy/move construction and assignment and destruction remain trivial; default construction is explicitly intrinsic-zeroed | 3, 4 | Type traits and zero-construction code generation |
| All-active invariant | Every lane participates in transfer, arithmetic, comparison, rearrangement, and reduction behavior | 4-9 | Distinctive highest-lane runtime and constexpr tests |
| Transfer extent | Element and byte loads/stores use fixed extents equal to `lane_count` or `byte_count`; partial and unsafe forms do not exist | 4 | Compile rejection, canaries, and sanitizers |
| Alignment | Aligned loads/stores require `byte_count` alignment and follow the existing SimdLib precondition configuration | 4, 10 | Checks-enabled failures and release code generation |
| Scalar operands | Arithmetic and bitwise operations initially accept only the same Register type; scalar use requires explicit `broadcast()` | 4, 6 | Compile rejection and broadcast code generation |
| Native interoperation | Register and RegisterMask expose by-value `native()` observers; Register has an explicit native constructor; mask native construction remains private | 4, 5 | Constructibility assertions and native-result ABI probes |
| Explicit object parameters | Non-mutating members take the explicit object by value; compound assignment takes it by reference | 3-9 | Declaration audit and forced-inline/no-inline probes |
| Calling convention | Register-shaped members use `VECTORCALL` where supported; consumer-defined non-inlined boundaries must opt in separately | 3, 10 | Vector/default convention wrapper-versus-raw mirrors |
| Mask invariant | Each predicate lane is all-zero or all-one; arbitrary numeric/native values cannot publicly construct a mask | 5 | Constraint tests and predicate-bit tests |
| Compact mask bits | `bits_type` is normalized from lane count, is `uint32_t` for initial widths, maps bit `i` to lane `i`, and clears unused bits | 5 | Static assertions and mask-pattern tests |
| Comparison semantics | Named comparisons reproduce the selected intrinsic, including signedness, NaNs, signed zero, ordered/unordered predicates, and lane bit patterns | 5 | Runtime, portable, emulated, and constexpr parity |
| Whole equality | `operator==` means all lanes compare equal; `operator!=` is its Boolean negation; relational operators are absent | 5 | Boolean and compile-rejection tests |
| Shift counts | Per-lane negative counts are invalid; logical overshifts zero, arithmetic overshifts sign-fill, and byte/whole-register shifts follow the proposal boundary table | 6 | Boundary, precondition, constexpr, and codegen tests |
| Immediate controls | Every `imm8` is constrained to `0..255`; logical selectors have exact counts and valid source indices | 7, 8 | Compile-success/failure boundaries |
| Type-changing results | Public operations name the exact constrained namespace-level result alias and never expose a raw intrinsic result | 7 | Type assertions and unsupported-combination rejection |
| Conversion split | `bit_cast()` preserves bits; `convert()` changes numeric values; `widen_low()` explicitly consumes only low source lanes | 8 | Independent bit/numeric/lane-consumption tests |
| Zero overhead | No supported wrapper expression or call boundary adds instructions, moves, spills, reloads, stack traffic, temporaries, return buffers, branches, or indirection relative to the identical raw baseline, except for an explicitly recorded compiler-generated security protection | 3, 10 | Mandatory generated-code and ABI gates with provenance |
| MSVC `/GS` exception | The MSVC wrapper `simdlib_codegen_scalar` fixture may contain the exact documented security-cookie prologue and epilogue at 128 or 256 bits while its raw mirror remains register-only; the gate removes only that sequence for comparison, preserves the original artifacts, records the exception, and rejects every additional difference | 3, 10 | `CompareRegisterCodegen.cmake`, paired profiles, comparison result, and provenance |
| Compatibility | `Api` remains supported; collection transforms and compatibility-only operations do not migrate | 9, 11 | Final ledger audit and unchanged C++20 matrix |
| Public exposure | `Register.h` remains out of the umbrella until correctness and zero-overhead qualification succeeds | 1, 11 | Header and migration gates |

## Explicit exclusions

| Excluded surface | Classification | Reason |
| --- | --- | --- |
| Partial load/store or lane construction | Higher-level responsibility | Register has no inactive lanes or fill policy |
| Dynamic-extent `load_unsafe` | `Api` compatibility-only | Its precondition is unsuitable for the restrictive value type |
| Native-order `set` | `Api` compatibility-only | Public lane order is logical low-to-high |
| Implicit scalar broadcast | Excluded | Broadcast cost and intent remain explicit |
| Implicit native conversion or mutable native reference | Excluded | Native access is an explicit by-value boundary |
| Public unchecked mask construction | Excluded | It would break the canonical predicate invariant |
| Runtime `extract` | Initial compatibility-only | Backend selector semantics are implementation-specific |
| Generic `shuffle(args...)` | Initial compatibility-only | Implementation-specific signatures are not a portable value API |
| `expand` and `compress` | Compatibility-only | Result width, lane consumption, and saturation are ambiguous |
| Multi-register widening/narrowing | Separate future design | One Register operation produces one complete result Register |
| Scalar arithmetic overloads | Deferred additive API | Real call sites and code generation must first justify them |
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

The phase column is the implementation owner. “Compatibility” and “internal”
rows are verified absent from the preferred surface in Phase 9.

| Current public `Api` operation | Register result | Owner |
| --- | --- | --- |
| `load` | `Register::load(fixed_span)` | Phase 4 |
| `load_aligned` | `Register::load_aligned(fixed_span)` | Phase 4 |
| `load_unaligned` | Canonicalized to `Register::load(fixed_span)` | Phase 4 |
| `load_partial` | No Register operation | Compatibility |
| `load_unsafe` | No Register operation | Compatibility |
| Element `store` | `value.store(fixed_span)` | Phase 4 |
| `store_aligned` | `value.store_aligned(fixed_span)` | Phase 4 |
| `store_unaligned` | Canonicalized to `value.store(fixed_span)` | Phase 4 |
| Byte `store` | `value.store_bytes(fixed_byte_span)` | Phase 4 |
| No byte-load counterpart | `Register::load_bytes(fixed_byte_span)` | Phase 4 |
| `construct(array)` | `Register::from_array(array)` | Phase 4 |
| `to_array` | `value.to_array()` | Phase 4 |
| `setzero` | Default construction and `Register::zero()` | Phase 4 |
| `set1` | `Register::broadcast(value)` | Phase 4 |
| `setr` | `Register::from_lanes(...)` | Phase 4 |
| `set`, `set_partial`, `setr_partial` | No Register operation | Compatibility |
| `add` | `lhs + rhs`, `lhs += rhs` | Phase 6 |
| `subtract` | `lhs - rhs`, `lhs -= rhs` | Phase 6 |
| `multiply` | `lhs * rhs`, `lhs *= rhs` | Phase 6 |
| `divide` | `lhs / rhs`, `lhs /= rhs` | Phase 6 |
| `modulus` | `lhs % rhs`, `lhs %= rhs` | Phase 6 |
| `negate` | `-value` | Phase 6 |
| `min` | `lhs.min(rhs)` | Phase 7 |
| `max` | `lhs.max(rhs)` | Phase 7 |
| `multiply_add` | `lhs.multiply_add(rhs, addend)` | Phase 7 |
| `widen` | `value.widen_low<target_t, target_bits>()` | Phase 8 |
| `absolute` | `value.absolute()` | Phase 7 |
| `sqrt` | `value.sqrt()` | Phase 7 |
| `magnitude` | `value.magnitude()` | Phase 7 |
| `normalize` | `value.normalize()` | Phase 7 |
| `avg` | `lhs.average(rhs)` | Phase 7 |
| `add_horizontal` | `lhs.horizontal_add(rhs)` | Phase 7 |
| `subtract_horizontal` | `lhs.horizontal_subtract(rhs)` | Phase 7 |
| `multiply_add_adjacent` | `lhs.multiply_add_adjacent(rhs)` with named result alias | Phase 7 |
| `multiply_add_unsigned_signed_bytes` | Same named member with byte-multiply-add result alias | Phase 7 |
| `sum_absolute_byte_differences` | Same named member with SAD result alias | Phase 7 |
| `multi_sum_absolute_byte_differences` | Same named immediate member with multi-SAD result alias | Phase 7 |
| `min_position` | `value.min_position()` | Phase 7 |
| `max_position` | `value.max_position()` | Phase 7 |
| `add_saturated` | `lhs.add_saturated(rhs)` | Phase 7 |
| `subtract_saturated` | `lhs.subtract_saturated(rhs)` | Phase 7 |
| `hadd_saturated` | `lhs.horizontal_add_saturated(rhs)` | Phase 7 |
| `hsubtract_saturated` | `lhs.horizontal_subtract_saturated(rhs)` | Phase 7 |
| `add_subtract` | `lhs.add_subtract(rhs)` | Phase 7 |
| `dot_product` | `lhs.dot_product<imm8>(rhs)` | Phase 7 |
| `bitwise_and` | `lhs & rhs`, `lhs &= rhs` | Phase 6 |
| `bitwise_or` | `lhs \| rhs`, `lhs \|= rhs` | Phase 6 |
| `bitwise_xor` | `lhs ^ rhs`, `lhs ^= rhs` | Phase 6 |
| `bitwise_not` | `~value` | Phase 6 |
| `bitwise_andnot` | `lhs.andnot(rhs)` with preserved polarity | Phase 6 |
| `movemask` | `value.movemask()` with intrinsic-native granularity | Phase 6 |
| `movemask_slim` | `value.lane_sign_bits()` with one bit per lane | Phase 6 |
| `cmp_eq`, `cmp_eq_mask` | `lhs.compare_equal(rhs)` and `.bits()` | Phase 5 |
| `cmp_gt` | `lhs.compare_greater(rhs)` | Phase 5 |
| `cmp_ge` | `lhs.compare_greater_equal(rhs)` | Phase 5 |
| `cmp_lt` | `lhs.compare_less(rhs)` | Phase 5 |
| `cmp_le` | `lhs.compare_less_equal(rhs)` | Phase 5 |
| `expand`, `compress` | No Register operation | Compatibility |
| `extract<index>` | `value.lane<index>()` | Phase 4 |
| Runtime `extract` | No initial Register operation | Compatibility |
| `lower_half` | `value.lower_half()` | Phase 8 |
| `insert` | `value.with_lane<index>(lane)` | Phase 4 |
| `unpack_lo` | `lhs.unpack_low(rhs)` | Phase 8 |
| `unpack_hi` | `lhs.unpack_high(rhs)` | Phase 8 |
| `shuffle<indices...>` | `value.shuffle<indices...>()` | Phase 8 |
| Generic `shuffle(args...)` | No initial Register operation | Compatibility |
| `shuffle_lo` | `value.shuffle_low<imm8>()` | Phase 8 |
| `shuffle_hi` | `value.shuffle_high<imm8>()` | Phase 8 |
| `blend` | `lhs.blend<imm8>(rhs)`; predicate selection uses `mask.select()` | Phase 8 and Phase 5 |
| `shift_left` | `value << count`, `value <<= count` | Phase 6 |
| `shift_right` | `value.logical_shift_right(count)`; unsigned `operator>>` | Phase 6 |
| `shift_right_arithmetic` | Signed `value >> count`, `value >>= count` | Phase 6 |
| `byte_shift_left` | `value.byte_shift_left(count)` | Phase 6 |
| `byte_shift_right` | `value.byte_shift_right(count)` | Phase 6 |
| Runtime `bit_shift_left` | `value.bit_shift_left(count)` | Phase 6 |
| Compile-time `bit_shift_left` | `value.bit_shift_left<count>()` | Phase 6 |
| Runtime `bit_shift_right` | `value.bit_shift_right(count)` | Phase 6 |
| Compile-time `bit_shift_right` | `value.bit_shift_right<count>()` | Phase 6 |
| `convert_to_float` | `value.convert<float>()` | Phase 8 |
| `convert_to_int` | `value.convert<int32_t>()` | Phase 8 |
| `convert` | `value.convert<target_t>()` | Phase 8 |
| `transform_pack` | No Register operation | Collection |
| Unary and binary span `transform` overloads | No Register operation | Collection |
| `FinishIntegerMagnitudeFromPairSums` | No Register operation | Internal |
| `TransformForMaxPosition` | No Register operation | Internal |
| `compare_each_element` | Internal comparison fallback only | Internal |

### Inventory audit

A declaration audit of `include/SimdLib/Api.h` found 76 unique public or
documented internal static-operation names declared with the SimdLib inline
surface. Every name appears in the matrix above. The six operations exposed
through inherited `using impl::...` declarations—`add`, `divide`, `max`, `min`,
`multiply`, and `subtract`—also appear explicitly. Overloaded `load`, `store`,
`extract`, `shuffle`, `bit_shift_*`, and span `transform` families are split or
collapsed only where their Register disposition is identical. Phase 9 repeats
this mechanical audit against the then-current `Api.h` so later additions cannot
escape classification.

## Precondition and selector matrix

| Surface | Contract | Failure evidence |
| --- | --- | --- |
| Full element transfer | Fixed extent equals `lane_count` | Compile rejection |
| Raw-byte transfer | Fixed extent equals `byte_count` | Compile rejection and canaries |
| Aligned transfer | Address is aligned to `byte_count` | Checks-enabled negative test |
| Lane access/replacement | `index < lane_count` | Constraint rejection |
| Logical shuffle | Exact selector count; each selector in documented input range | Constraint rejection |
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
| C++20 core | MSVC 19.44 | x64 and x86; Debug and Release | Existing full public matrix remains supported |
| C++20 core | clang-cl 22.1.8 | x64 and x86; Debug and Release | Existing full public matrix remains supported |
| C++20 core | Clang 22.1.8 | x64 and x86; Debug and Release | Existing full public matrix remains supported |
| C++20 core | GCC 13.2 | x64 and CI x86; Debug and Release | Existing full public matrix remains supported; Register unavailable |
| C++20 core sanitizer | Clang 22.1.8 | x64 Debug, `-O1`, ASan/UBSan, frame pointers | No sanitizer diagnostics |
| Register | MSVC 19.44 | `/std:c++latest`; supported x64/x86 profiles | Complete Register gates must pass; the generated-code gate may record only the exact documented `/GS` scalar-cookie exception |
| Register | clang-cl 22.1.8 | C++23; supported x64/x86 profiles | Standard feature macro and complete Register gates pass |
| Register | Clang 22.1.8 | C++23; supported x64/x86 profiles | Standard feature macro and complete Register gates pass |
| Register | GCC 14 or newer | C++23; supported x64/x86 profiles | Standard feature macro and complete Register gates pass |

GCC 13.2 remains the required local unavailable-interface probe; it is not a
Register compiler. A Register compiler floor is lowered or expanded only after
the complete correctness, layout, ABI, and generated-code gates pass.

## Test and evidence ownership

| Evidence family | Planned source owner | Planned CMake/CTest owner |
| --- | --- | --- |
| Runtime Register correctness | `tests/Register.tests.cpp` | `SimdLibTestsRegister128`, `SimdLibTestsRegister256` |
| Runtime mask/comparison correctness | `tests/RegisterMask.tests.cpp` | Register runtime targets, split by width/profile |
| Shared independent scalar oracles | `tests/RegisterTestSupport.h` | Included only by public Register tests |
| Constexpr contracts | `tests/constexpr/Register128Constexpr.tests.cpp`, `Register256Constexpr.tests.cpp` | `SimdLibConstexprRegister128`, `SimdLibConstexprRegister256` |
| Availability and language modes | `tests/availability/Register*.cpp` | Compile-only Register availability targets |
| Configuration fallback/exclusion | `tests/config/Register*.cpp` | Compile-only Register configuration targets |
| First-and-only header | `tests/headers/RegisterHeaderProbe.cpp` | `SimdLibHeaderRegisterProbe` |
| Invalid declarations | `tests/compile_fail/register/*.cpp` | CMake `try_compile`/CTest compile-failure driver |
| ODR and multi-TU use | `tests/smoke/register_*.cpp` | `SimdLibHeaderOnlySmoke` extension |
| External consumer | `tests/consumer/register.cpp` and consumer CMake target | Existing consumer CTest project linked through `SimdLib::Register` |
| Forced-inline code generation | `tests/codegen/RegisterCodegen.cpp` generated from the operation matrix | `SimdLibRegisterCodegen` plus compiler-specific extraction scripts |
| Raw code-generation baselines | `tests/codegen/RegisterCodegenRaw.cpp` generated from the same matrix | Paired with `SimdLibRegisterCodegen` under identical flags |
| Non-inlined ABI mirrors | `tests/codegen/RegisterAbi.cpp`, `RegisterAbiRaw.cpp` | `SimdLibRegisterAbi` comparison gate |
| Register pressure and opaque calls | `tests/codegen/RegisterPressure.cpp`, `RegisterPressureRaw.cpp` | Register code-generation gate |
| Code-generation comparison | `cmake/CompareRegisterCodegen.cmake` and checked-in allowlisted normalization rules | CTest mandatory performance gate |
| Checks-enabled preconditions | `tests/RegisterPreconditionFailure.tests.cpp` | Existing precondition death-test infrastructure |
| Sanitizers | Runtime Register and mask sources | Fresh Clang ASan/UBSan configuration |
| Supplemental benchmarks | `benchmarks/Register.benchmarks.cpp` | `SimdLibBenchmarks`; never a correctness/codegen substitute |
| Final evidence | This document and `docs/Validation.md` | Updated after each completed phase |

Every planned production class and method receives Doxygen documentation. Test
and generated-code sources use only public SimdLib declarations except the
proposal-approved narrow internal comparison adapter tests.

## Validation ownership

Compiler commands and result files are runtime artifacts rather than durable
documentation. CMake presets, CI workflows, and `ContainerValidation.md` own
the reproducible invocation contract; generated build trees, JUnit reports,
provenance files, and logs own individual outcomes.

## Phase 1 language and build-integration design

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

## Phase 2 container-environment design

Phase 2 selects Alpine Linux for both GNU-like compiler services. The complete
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
the CMake builder stage. CMake configures fresh on each invocation so a cached
missing-tool result cannot survive an image refresh.

The full profiles compile and run the complete Linux-supported C++20/C++23
suite, not a platform-independent subset. Portable header repairs guard the
Windows-only `<intrin.h>` boundary, include x86 intrinsics only on x86, disable
`VECTORCALL` for GNU-like Linux Clang, and value-initialize the temporary used
by `register_set`. Native Windows jobs remain authoritative for MSVC, clang-cl,
Windows ABI, and calling-convention evidence.

### Compose and orchestration decision

`compose.yml` is the single declarative environment used locally and in CI. It
uses a shared service anchor and explicit focused, full, feature, sanitizer, and
code-generation profiles. `tools/Run-ContainerMatrix.ps1` is the accepted thin
aggregator: it selects the complete service set, pre-creates one unique project
network, starts compiler services concurrently with `docker compose run --rm`,
waits for every exit, retains separate logs, and removes only that run's unique
Compose project.

A separate Compose healthcheck is intentionally absent: these are one-shot
`compose run` jobs, for which Compose does not wait on the service's own health
state. The canonical entrypoint instead performs synchronous compiler, CMake,
CPU-feature, and argument preflight before any configure or test work.
The reserved code-generation profile runs that preflight for both compilers;
generated-code comparison targets remain owned by Phase 3.

Direct parallel `docker compose up` interleaves logs, retains stopped service
containers, and cannot provide deterministic all-service failure attribution.
Direct `compose run` provides isolation but requires repeated arguments. The
wrapper therefore remains the smallest interface satisfying aggregate status,
cancellation, and artifact requirements while Compose remains the environment
definition. Its intentional-failure and cancellation switches exercise
one-service failure, multi-service failure, partial-log retention, and
unique-project cleanup.

The scheduled reproducibility workflow runs the canonical `Focused -NoCache`
command and records image inspection output. Normal CI runs Full, Feature, and
Sanitizer through the same wrapper and Dockerfiles;
there is no CI-only Linux dependency installation path. Exact local commands,
artifact conventions, refresh/security procedure, and project-owned cleanup
are recorded in `ContainerValidation.md`.
