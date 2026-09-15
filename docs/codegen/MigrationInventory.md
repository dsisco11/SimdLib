# Instruction-contract migration inventory

This is the source-owned migration specification for
[RegisterCodegenPolicy.todo](../RegisterCodegenPolicy.todo), not an execution
receipt or a replacement qualification claim. Existing gates remain authoritative
until cutover. The [proposal](../RegisterCodegenPolicyProposal.md) controls the
replacement design; [Register qualification](../RegisterQualification.md) and
[PartialRegister qualification](../PartialRegisterQualification.md) describe the
legacy evidence. Repository paths below are relative to the repository root.

## Case identity and expansion

The finite sets below are declarations, not disassembler search patterns. Expand
each listed suffix and type/geometry combination into one primary case. Braces
denote a Cartesian product; a stated restriction removes only that combination.
No unlisted suffix is implicitly covered. Each expanded case has these fields:

| Field | Definition |
| --- | --- |
| `case_id` | `<family>.<expanded-suffix>`; preserve the suffix spelling below, including type tokens already present. The partial ABI matrix uses `partial-abi.identity.<type>` and puts N in geometry. Compiler, ISA, width, and optimization are execution dimensions, not copies of primary definitions. |
| `geometry` | Source/target element types and widths, active count, immediate/selector values, signature shape. Values below are mandatory, not examples. |
| `legacy_symbol` | Prefix plus suffix below; template ABI identities additionally include their exact type/count specialization and `pass` member. |
| `fixture` | Register families use `tests/codegen/register/<family>.cpp`; partial families use `tests/codegen/partial-register/<family>.cpp`; adjacent families use `tests/codegen/api-transfer/transfer.cpp` and `tests/method_flags/codegen/MethodFlagsFlagged.cpp`. One production expression per case; no raw mirror. |
| `emitted_symbol` | New non-template observation entry `simdlib_contract_<case_id with dots and hyphens replaced by underscores>`. Geometry-specific objects may reuse the same C identifier. An ABI group adds `_caller` and `_callee`. Preserve the actual aggregate signatures and boundary attributes. |
| `matrix` | The named matrix below, intersected with the row's geometry restriction and the independent availability oracle. |
| `contracts` | Selected reusable families below plus the method expectation implied by the named operation, types, extent, and immediate. Families never replace method requirements. |
| `supplemental` | Applicable facts from the supplemental ledger; none can cancel a primary assertion. |
| `owner` | The suite owner below registers the primary and all applicable supplemental checks; independent correctness owners remain separate. |

Before object inspection, registration must materialize the exact emitted-symbol
list from these declarations. A missing, duplicated, ambiguous, or empty function
fails. A symbol appearing in disassembly does not add itself to the expected
list. `zero`, identity imports, bit casts, special members, zero-count shifts, and
ABI identity boundaries may have minimal or return-only bodies; those still
require a present, complete nonempty instruction body. Constant results need no
artificial runtime dependency.

`configuration_id` is
`<driver>-<version>-<target>-<isa>-w<width>-<optimization>-<checks>-<instrumentation>-<fma>-<abi>-<stack-policy>`.
Record the resolved executable and effective flags separately. Driver values are
`msvc`, `clang-cl`, `clang`, and `gcc`; Clang's compiler ID alone is insufficient.
CTest identities identify the logical case, geometry and configuration in one
composite Catch2 result. Primary and supplemental rule identities appear within
that result; they are not separately scheduled CTest cases. Shared and
supplemental FileCheck invocations inspect the same object/body independently
with independent captures. An independently authored required-rule ledger detects
omitted or incorrectly selected assertions, and discovery is checked against an
independent expected-case inventory.

`geometry-id` serializes `<source>-w<source-width>-n<active-count>-to<target>-w<target-width>-imm<control>-shape<signature>`.
Use `all` for complete active count and `none` for absent target/control. Encode
immediates as unsigned decimal, runtime controls as `runtime`, and selector lists
as ordered decimal values joined by underscores. Use the row suffix for the
signature shape; partial ABI matrix identities use `identity`. New observation
functions have C linkage, with their declared calling convention retained;
object-format decoration is resolved explicitly by the extraction owner, never
by substring matching. For multiple N values in one object, append `_n<N>` to
the partial ABI identity symbol before `_caller`/`_callee`.

## Execution matrices and availability

| Matrix | Existing executions to preserve |
| --- | --- |
| R | x64 C++23 Register compilers: MSVC 19.44, clang-cl 20.1.8 compatibility floor and newer qualified 22.1.7/22.1.8 identities, GNU-like Clang 22, GCC 14; SSE4.2/128, AVX2/128, AVX2/256. |
| P | Same Register compiler families; SSE4.2/128 and AVX2/256. There is no existing AVX2/128 PartialRegister codegen registration. |
| A | API partial transfer: P's registered cells, through C++20 `SimdLib::SimdLib`; current registration is nevertheless guarded by Register compiler support. Do not silently expand it to GCC 13. |
| M | Method attributes: x86/x64 configuration-probe/codegen gate, 128-bit SSE4.2, core C++20, all configured core compiler drivers including GCC 13. The Register x64 restriction does not redefine this adjacent gate. |

`T = {i8,u8,i16,u16,i32,u32,i64,u64,f32,f64}`; `I = T - {f32,f64}`.
Tokens mean fixed-width standard integers, `float`, and `double` respectively.
For complete registers every lane is active. For partial ABI identities, enumerate
`1 <= N < width/element_bits` at 128 bits and
`128/element_bits < N < 256/element_bits` at 256 bits: 56 cells per width.

[RegisterImplementationMatrix.md](../RegisterImplementationMatrix.md),
[ApiOperationMatrix.md](../ApiOperationMatrix.md), and
`tests/RegisterOperationMatrix.tests.cpp` own availability independently of emitted
code. Floating modulus and lane shifts are absent, not identity cases. Partial
availability/result geometry follows
[PartialRegisterOperationLedger.md](../PartialRegisterOperationLedger.md) and its
runtime/compile-time surface matrices. No disabled API becomes a positive codegen
case merely because a fallback branch emits a return.

Current Register gate options add `/O2 /GS` for Microsoft-style drivers or
`-O2 -fstack-protector-strong` for GNU-like drivers. SSE4.2 explicitly disables
AVX/AVX2/FMA through configuration macros; clang-cl also uses
`/clang:-msse4.2 /clang:-mno-avx /clang:-mno-avx2 /clang:-mno-fma`; GNU-like
drivers use the equivalent unprefixed options. AVX2 uses `/arch:AVX2` or `-mavx2`.
Partial and API targets inherit Release optimization and target configuration;
do not attribute Register's explicit stack options to those targets. Method
attributes use `/O2 /Ob2 /GS` or `-O2 -msse4.2 -fstack-protector-strong`.
See `cmake/development/TargetConfiguration.cmake` and the three suite CMake files.

Optimization, checks, instrumentation, FMA, and ABI are independent dimensions.
Existing optimized outcomes do not qualify proposed low optimization. Low,
production, and wholly unoptimized flag selection and applicability remain the
optimization-policy work in tasklist section 4. Preserve explicitly requested
Debug/ASan diagnostic executions as observations, not optimized passes.

## Initial reusable contracts

| Key | Shared responsibility and applicability |
| --- | --- |
| R | Register-only: allowed instruction families and method-specific counts/data flow; no unexpected calls, stack traffic, or data-memory operations. Select only when the operation permits these restrictions; constant pools must be explicit. |
| L | Load/operate/store: exact logical memory widths and offsets, required operation, observable result; forbid temporary traffic/calls only where established. Aligned and unaligned operations remain distinct. |
| I | Immediate: instruction family, actual immediate or selector/constant content, operand relationship, and no fallback sequence. Zero/overshift cases use their identity/zero requirements instead. |
| F | FMA: require fused work for enabled cases and forbid it for disabled cases, per function rather than aggregate profile. Keep input/output roles and operation count. |
| B | ABI: caller and callee argument/result locations, convention, reference/scalar/native/mask result shapes, and expected targets; prohibit hidden return storage only for the supported boundary. |
| G | Targeted regression: specifically absent redundant frame, move, spill, branch, projection, or helper call. Pressure/opaque-call cases allow necessary preservation and the named opaque target; they are not universal R cases. |
| P | Partial invariant requirements added to the applicable family: import projection, inactive divisor neutralization, active-only reduction/transfer, and correct complete versus partial results. Preserve checks-enabled observation work. |

Counts must exclude extra occurrences throughout the complete body. Negative
checks cover all blocks, including blocks after an early return. Captures retain
register identity, aliases, widths, and operand relationships. Constants and
relocations must remain inspectable when used by a contract. Exact instruction
spellings/bounds require real-object validation during fixture migration; the
families above are not a claim that their FileCheck implementation exists.

## Register primary declarations

All rows use R matrix unless restricted. Legacy sources are under `tests/codegen`.
The owner is the Catch2 declaration beside its operation family, built through
thin `cmake/development/RegisterCodegen.cmake` configuration, replacing the listed
record under `RegisterCodegen.<profile>`.
The fixture family determines the new production-only source responsibility.

### Expressions (`RegisterCodegenFixture.h`, prefix `simdlib_codegen_`)

| Family / suffix set | Geometry | Legacy record | Contracts and method facts |
| --- | --- | --- | --- |
| expression / `ternary` | f32 | register-only | R: composed multiply/add, preserve operand flow |
| expression / `mask_combine,mask_select,mask_bits,mask_any,mask_all` | f32 predicates | register-only | R: composition, selection, compact bits and reductions respectively |
| expression / `native,special_members` | f32 | register-only | R+G: identity/special-member copy and move elision |
| expression / `broadcast_reuse,basic_broadcast_chain` | f32 scalar inputs | register-only | R+G: broadcast reuse and scale/offset composition |
| expression / `lane_last` | f32 index `width/32-1` | register-only | R+I: nonzero highest-lane extraction |
| expression / `basic_bitwise` | f32 representation | register-only | R: composed bitwise operations |
| expression / `pressure` | eight f32 native inputs | register-only | G: preserve eight-input expression; do not invent an unconditional spill prohibition |
| expression / `reassignment_arithmetic` | f32 | reassignment | R+G: by-value reassignment, no redundant wrapper alignment frame |
| transfer / `load_operate_store,aligned_transfer,byte_transfer,mutate` | f32; full width / `width/8` bytes; mutate uses native reference | primary-composition | L: respective unaligned, aligned, byte, and reference mutation boundaries |
| shift / `basic_shift_left_immediate` | u32, count 3 | register-only | R+I: immediate lane shift |
| shift / `complete_shift_static` | u32, width 128, bit count 19 | primary-composition | I: complete-register left bit shift |
| shift / `complete_shift_runtime,complete_byte_shift` | u32, width 128, runtime count | primary-composition | I+G: right bit / left byte slow paths and count branches |
| shift / `shift_bytes_{left,right}_{0,1,7,15,16,17}` | u32, all R cells | complete-byte-shift-immediate | R+I+G: exact requirements below |
| shift / `shift_bytes_{left,right}_{31,32}` | u32, width 256 only | complete-byte-shift-immediate | R+I+G: exact requirements below |
| expression / `opaque` | f32 live across external call | primary-composition | B+G: preserve value across `simdlib_codegen_opaque_sink`; sink is an external target, not a missing emitted definition |

Immediate byte shifts retain the separate
`cmake/VerifyCompleteRegisterShiftCodegen.cmake` responsibility currently run on
the raw object in addition to parity. All counts forbid call/push/pop, rsp/rbp,
`pshufb`, and `por` (including VEX forms). Count zero forbids byte shifts, align,
cross-half permutation, and zeroing. At 128 bits, counts 1/7/15 require the
directional byte shift; 16/17 require zeroing and forbid shifts/align/permutation.
At 256 bits, 1/7/15 require left cross-half permutation or right
permutation/extraction plus `vpalignr`; 16 requires the cross-half step without
align/byte shift; 17/31 additionally require directional byte shift; 32 requires
zeroing without align/permutation/byte shift. These are already enforced even
for SSE4.2 and the legacy RECORD path: the CMake command hardcodes
`RECORD_ONLY=OFF`. Qualification prose's blanket SSE4.2 diagnostic description
does not remove this distinct gate.

### Isolated operations (`RegisterTypeMatrixCodegenFixture.h`)

Prefix `simdlib_type_matrix_`; family `operation`; symbol suffix is
`<operation>_<type>`. There are 41 common operations per T and four additional
integer operations per I (442 primary geometry cells per width). Each is
independently emitted; do not replace these with one composition.

| Operations | Types | Contracts |
| --- | --- | --- |
| `zero,broadcast,add,subtract,multiply,divide,negate,bitwise_and,bitwise_or,bitwise_xor,bitwise_not,bitwise_andnot` | T | R where applicable; scalar integer division retains its native-lane algorithm and permitted instructions |
| `compare_equal,compare_greater,compare_greater_equal,compare_less,compare_less_equal,mask_and,mask_or,mask_xor,mask_not,select,insert_last` | T | R; insertion index `width/element_bits-1` |
| `movemask,lane_sign_bits,mask_bits,mask_any,mask_all,mask_none,equal,not_equal,extract_first` | T | R; extraction index 0; scalar result width retained |
| `construct_array,construct_lanes,load,load_aligned,load_bytes,store,store_aligned,store_bytes,observe_array` | T | L; full native extent, fixed lane-list order; MSVC array supplemental ledger applies |
| `modulus,shift_left,logical_shift_right,shift_right` | I | R/I as appropriate; runtime shift count, signedness retained |

`modulus` alone maps to `modulus-type-matrix`; all others map to
`common-type-matrix`. Floating shifts/modulus remain negative availability
assertions. Specialization requirements are checked before registering symbols.

### Specialized operations (`RegisterSpecializedCodegenFixture.h`)

Prefix `simdlib_specialized_codegen_`; family `specialized`; suffix
`<operation>_<type>`; record `specialized`. These select R with method-specific
allowed work/constant access, I for immediate controls, and G for targeted
redundancy. This is a finite operation/type list, not a claim that every method
exists on every type.

| Operations | Types / parameters |
| --- | --- |
| `min,max,absolute,sqrt,magnitude` | T |
| `magnitude_checked,min_position,max_position,multiply_add_adjacent,byte_multiply_add,sum_absolute_byte_differences,multi_sad` | I; `multi_sad` immediate 0x1B |
| `normalize,add_subtract,dot_product` | f32,f64; dot immediate 0xD3 |
| `average` | u8,u16 |
| `horizontal_add,horizontal_subtract` | i16,u16,i32,u32,f32,f64 |
| `add_saturated,subtract_saturated` | i8,u8,i16,u16 |
| `horizontal_add_saturated,horizontal_subtract_saturated` | i16,u16 |

### FMA (`RegisterFmaCodegenFixture.h`)

Family `fma`; prefix `simdlib_fma_codegen_`; suffix `multiply_add_{f32,f64}`.
One primary source across `fma-disabled` (all R) and `fma-enabled` (AVX2 only).
`SIMDLIB_HAS_FMA=0/1` is explicit; GNU-like drivers additionally select
`-mno-fma/-mfma`. F owns fused/nonfused expectations without cloning the primary
definition by compiler. The legacy aggregate FMA check does not prove a count or
per-function data flow; replacement method expectations must do so.

### Rearrangement/conversion (`RegisterRearrangementCodegenFixture.h`)

Family `rearrangement`; prefix `simdlib_rearrangement_codegen_`; record
`rearrangement-conversion`; R+I as applicable. Exact closed expansions:

| Suffix | Geometry |
| --- | --- |
| `{unpack_low,unpack_high}_<T>` | all T |
| `{shuffle_low,shuffle_high}_{i16,u16}` | immediate 0x1B |
| `blend_{i16,u16,i32,u32,f32,f64}` | immediate 0xA5 |
| `logical_shuffle_<T>` | 128: reverse all lanes. 256: reverse lanes except i8/u8/i16/u16, which keep even positions and select the opposite 128-bit half for odd positions. |
| `lower_half_<T>` | 256 -> 128 only |
| `byte_shuffle_i32` | 128: reverse bytes 15..0 |
| `byte_shuffle_i32_local` | 256: reverse bytes separately in each half |
| `byte_shuffle_i32_cross` | 256: reverse bytes 31..0 |
| `byte_shuffle_i32_mixed` | 256: exchange byte positions 0 and 16, retain all others |
| `bit_cast_<T>_<T>` | all 100 ordered source/target pairs, same width; identity bodies valid |
| `convert_i32_f32,convert_u32_f32,convert_f32_i32` | same source/target width |
| `widen_<source>_<target>_<target-width>` | source width 128; i8 -> i16/i32/i64, u8 -> u16/u32/u64, i16 -> i32/i64, u16 -> u32/u64, i32 -> i64, u32 -> u64; target 128 on SSE4.2, targets 128 and 256 on AVX2 |

### ABI declarations

| Family / source / legacy prefix | Suffixes | Matrix / contract |
| --- | --- | --- |
| abi / `RegisterAbi.cpp` / `simdlib_abi_` | `unary,binary,ternary,scalar,mask,native,store,mutate` | R, f32; B, with original explicit-object aggregate boundary and native/reference/scalar/mask result shape. Each gets caller/callee group. |
| consumer-abi / `RegisterAbi.cpp` / `simdlib_consumer_abi_` | `register_return,register_pass,mask_return,mask_pass` | R, f32; B caller/callee group, actual public Register/RegisterMask signatures |
| default-abi / `RegisterDefaultAbi.cpp` / `simdlib_codegen_` | `default` | R, f32 binary aggregate boundary, B observation group; Windows platform-default hidden return storage remains diagnostic |

Legacy records are `abi`, `consumer-abi`, and `default-abi`, respectively. The
explicit-object fixture's `AbiRegister`/`AbiMask` shapes are ABI probes, not a
second production algorithm. New groups must call production operations and
preserve those tested signatures. Windows flagged convention and platform-default
observations stay separate; GNU-like drivers use the platform ABI because the
vectorcall adapter is empty.

## PartialRegister primary declarations

Owner: thin `cmake/development/PartialRegisterCodegen.cmake`; matrix P. Sources
below are under `tests/codegen`; each suffix has one primary definition. Every
row adds P invariant requirements to its applicable contract.

| Family / source / prefix | Suffixes and geometry | Legacy owner / contracts |
| --- | --- | --- |
| predicate / `PartialRegisterMaskCodegen.cpp` / `simdlib_partial_mask_codegen_` | `compose`: u32 active 3/5 for 128/256; native imports, XOR, NOT, native observation | `PartialRegisterMaskCodegen.<profile>` / R or bounded memory policy + P |
| value / same source / `simdlib_partial_register_codegen_` | `import,add`: u32 active 3/5; `divide`: i32 active 3/5 | `PartialRegisterValueCodegen.<profile>` / R+P; import projects, add is closed, divide neutralizes |
| arithmetic / `PartialRegisterArithmeticCodegen.cpp` / `simdlib_partial_arithmetic_codegen_` | `add,multiply,divide,modulus,min,max,absolute,magnitude`: i32 active `width/32-1` | `PartialRegisterArithmeticCodegen.<profile>` / R+P |
| arithmetic / same | `subtract,negate,sqrt,multiply_add,normalize,add_subtract,dot_product`: f32 active `width/32-1`; dot 0x11 | same / R+P, F for multiply_add, I for dot |
| arithmetic / same | `magnitude_checked,horizontal_add,horizontal_subtract,multiply_add_adjacent,min_position,max_position,horizontal_add_saturated,horizontal_subtract_saturated`: i16 active `width/16-1` | same / R+P; preserve mapped result and active-only reductions |
| arithmetic / same | `average,byte_multiply_add,sad,multi_sad,add_saturated,subtract_saturated`: u8 active `width/8-1`; multi-SAD 0x35 | same / R+P, I for multi-SAD |
| arithmetic / same | `sparse_adjacent`: i64/256/3 only, complete sparse result | same / R+P |
| general / `PartialRegisterGeneralCodegen.cpp` / `simdlib_partial_general_codegen_` | `transfer,unary_shift,payload_shift,payload_shift_right,rearrange,blend,convert,numeric_convert,native_transfer,composed`: u32 active 3/5 | `PartialRegisterGeneral.<profile>` / L for transfer, R/I/P for others; unary runtime count; byte shifts 1; blend 0b0101; convert bit-casts to i32, numeric_convert to f32; unpack_low and compare/select/add composition |
| general / same | `widen_low`: u16/128/3 -> u32/128/3 only | same / R+P |
| partial-abi / `PartialRegisterAbi.cpp` / `simdlib_partial_abi_` | `return,pass,reassign,mask,opaque,pressure`: f32 active 3/5; `scalar`: u32 active 3/5; `type_change`: f32 -> u32 active 3/5 | `PartialRegisterAbi.<profile>` / B+P, G for opaque/pressure; opaque calls the return boundary; pressure has four inputs |
| partial-abi / same / `simdlib_partial_abi_matrix_cell<T,N>::pass` | all T and every valid N as defined above | same / B+P; 56 identity boundaries plus eight named shapes = 64 per width |

Each partial ABI case becomes a caller/callee fixture group, including every
type/count identity. Legacy layout assertions remain independent compile-time
evidence. Checked observation validates direct aggregate imports; replacing it
with unchecked native-member access to erase invariant work is forbidden.
Partial arithmetic's multiply_add currently inherits the ISA/FMA configuration;
it is not an existing separate enabled/disabled pair. Record that effective mode
and qualify any expanded FMA matrix deliberately.

## Adjacent primary declarations

| Family / source / prefix | Suffixes | Matrix / owner / contract |
| --- | --- | --- |
| api-transfer / `ApiPartialTransferCodegenFixture.h` / `simdlib_api_partial_codegen_` | `broadcast_partial` | A / `ApiPartialTransferCodegen.<profile>` / R+P: u32, exactly 3 active lanes |
| api-transfer / same | `load,load_aligned,load_bytes,store,store_aligned,store_bytes` | A / same / L+P: u32 low half; 2/4 elements or 8/16 bytes for 128/256 |
| api-transfer / same | `load_aligned_full,store_aligned_full` | A / same / L: full-width branch, 4/8 u32 elements |
| api-transfer / same | `to_array` | A / same future owner / L+B observation: low-half array return; explicitly excluded by legacy `EXCLUDE_SYMBOL_PATTERN`, retain as mapped observation rather than claiming legacy enforcement |
| method-attributes / `tests/method_flags/codegen/MethodFlagsFlagged.cpp` / `simdlib_method_flags_codegen_` | `unary,binary,ternary,scalar_result,register_result,load,store,forceinline,flatten` | M / `MethodFlagsCodegen` / B+G plus L for load/store; __m128/f32 |

API fixtures call the public Api method once, not a raw intrinsic mirror.
Method-attribute fixtures retain actual `SIMD_FLAGS` declarations instead of
becoming Register tests. Their primary requirements preserve absence of cookie
references for all eight non-store symbols, absence of calls to
`simdlib_method_flags_force_leaf` and `simdlib_method_flags_flatten_leaf`, and
explicit stack-protection configuration. `store` is not given the register-only
cookie prohibition. The two leaves are support functions/targets, not additional
primary cases. `VerifyMethodFlagsCodegen.cmake` currently checks both objects;
the new check owns the flagged production declaration only.

## Supplemental facts and legacy policy

| Fact ID | Applicability | Requirements retained / migration boundary |
| --- | --- | --- |
| `msvc-array-cookie` | MSVC 19.44, Windows, width 128, type-matrix array construction, SSE4.2/AVX2 | Preserve bounded cookie prologue/check/epilogue and the operation's memory work. Require the actual security-cookie target, not any call. `CompareRegisterCodegen.cmake` encodes 0x28 frame, cookie slot 0x10, two qword source copies and movdqu/vmovdqu reload; its recognizer locates the second matching normalized block rather than verifying the documented symbol. Bind the new fact to the exact array case after real-object inspection; never infer a symbol identity from normalized block order. |
| `no-cookie-or-temporary-frame` | Non-MSVC array cells where the stronger property is established; compiler set may share one declaration | Add absence of cookie/frame/call work to the same primary. Do not cancel an R prohibition on MSVC: primary must allow only the genuinely common memory behavior. Exact per-symbol bounds are validated before cutover. |
| `partial-msvc-predicate` | MSVC predicate, both P cells | Bounded predicate-composition /GS work; no general call/stack exemption. |
| `partial-msvc-value` | MSVC value, both P cells | Import normalization and /GS frame, unaligned moves, scalar-divide scheduling/allocation; preserve required suffix work and bound incidental work. |
| `partial-msvc-arithmetic` | MSVC arithmetic, both P cells | Equivalent unaligned moves, commutative operands/allocation. Preserve operation counts and prohibit unrelated overhead. |
| `partial-msvc-general` | MSVC general, both P cells | Bounded /GS, equivalent vector moves, memory/register mask materialization, operand allocation. Constants must remain inspectable. |
| `partial-msvc-abi` | MSVC partial ABI, both P cells | Boundary suffix normalization and bounded /GS, original parameter/result ABI. |
| `partial-clang-arithmetic` | clang-cl 20.1.8 and 22.1.7/22.1.8, both P cells | 128: commutative operand selection/scalar division scheduling; 256: commutative operand/register selection. Do not freeze arbitrary register allocation or drop stronger absence properties. |
| `partial-clang-value` | same clang-cl versions, both P cells | 128: add operand selection/division scheduling; 256: commutative add operand selection. |

The retained profile JSON has 16 entries: four clang-cl arithmetic entries split
by old/new version and width, two clang-cl value entries, and ten MSVC entries
(predicate/value/arithmetic/general/ABI by width). It owns exact legacy hashes,
not the new instruction policy. GNU-like Clang and GCC have no retained entries.
MSVC profile version selectors are `*` in the JSON, bounded in practice by the
supported compiler gate; this is not proof for every future MSVC version.
Do not copy hashes into new acceptance rules. Preserve their stronger established
properties through primary or additive facts, not a shared weakest allowance.
Generated `symbols.txt` and `instruction-differences.txt` remain necessary inputs
when assigning each profile-level variation to exact emitted functions; tasklist
sections 5/6 own that real-object validation before retiring profiles.

Register SSE4.2 records are otherwise diagnostic. MSVC primary composition is
diagnostic at all widths; AVX2/256 integer modulus is a separate scheduling
diagnostic. Windows default ABI is observation-only. These are mapped cases,
not new exceptions to R. Scalar-cookie recognition for broad
`SYMBOL_PATTERN=simdlib_codegen_` remains comparator code but is not selected by
current partitioned Register commands. Compound-assignment recognition is
commented historical code. Neither establishes an additional active case.

Qualification-document discrepancies are recorded rather than silently resolved:
the implementation matrix contains older strict-SSE/compiler patch wording;
the current suite commands and Register qualification ledger define actual
record partitioning. Partial qualification's observed clang-cl 22.1.8 and
Clang 22.1.3 results do not update its support-table 22.1.7 spelling or prove
today's installed tools. No compiler was executed to produce this inventory.

## Independent semantic evidence

Preserve `Register.tests.cpp`, `RegisterBasicOperations.tests.cpp`,
`RegisterSpecializedOperations.tests.cpp`, `RegisterRearrangementConversion.tests.cpp`,
`LogicalShuffleRegister.tests.cpp`, and their constexpr, availability, negative,
precondition, header, ODR, and external-consumer owners listed in the implementation
matrix. Partial runtime arithmetic/specialized/general/mask suites, surface
matrices, suffix/canary checks, constexpr/negative/layout probes, and installed
consumer remain owned by PartialRegister qualification and the operation ledger.
API transfer correctness stays in the Api runtime/partial-transfer tests;
method flags retain configuration, preprocessor, compile-failure, and ABI probes.
No scalar semantic oracle or benchmark baseline is deleted as a raw codegen mirror.

See [IntegrationInventory.md](IntegrationInventory.md) for retirement consumers,
artifact ownership, provisioning, and remaining implementation decisions.
