# Register Qualification Contract

This document defines the supported `Register<T, Bits>` and
`RegisterMask<T, Bits>` qualification matrix, the evidence required for each
supported cell, and the exclusions that bound the zero-overhead claim. Generated
artifacts and individual execution results are intentionally not committed; the
commands below reproduce them under `build*/register-codegen` or
`out/pipeline`.

## Supported matrix

| Dimension | Supported cells |
| --- | --- |
| Architecture | x86-64 |
| Register widths | 128 and 256 bits |
| Availability floor | SSE4.2 exposes the 128-bit specialization; AVX2 additionally exposes the 256-bit specialization |
| Optimized zero-overhead profile | AVX2 for the complete 128-bit and 256-bit wrapper/raw corpus |
| Optimized diagnostic profile | SSE4.2 for the complete 128-bit wrapper/raw corpus |
| Element types | `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, `int32_t`, `uint32_t`, `int64_t`, `uint64_t`, `float`, and `double` |
| Windows compilers | MSVC 19.44 and clang-cl 22 |
| Linux compilers | GCC 14 and Clang 22 on the pinned Alpine/musl images |
| Optimized configuration | Release with strict wrapper/raw generated-code comparison |
| Optional diagnostic configurations | Explicitly selected Debug compiler; ASan+UBSan on Clang 22 only for an instrumentation investigation |
| FMA profiles | Explicitly disabled under SSE4.2; explicitly enabled and disabled under AVX2 |

Every supported compiler must compile the C++23 interface, the complete runtime
and constexpr corpus for each ISA-available width, and the external consumer.
AVX2 participates in the strict optimized wrapper/raw gate. SSE4.2 compiles the
same 128-bit fixtures with Release optimization and records any differential;
it is a correctness-supported profile but is excluded from the zero-overhead
claim. An optimized zero-overhead cell is supported only when its applicable
wrapper/raw profiles are instruction-identical after allocation-independent
normalization, except for an exact exception listed below.

## Correctness evidence

- `tests/Register.tests.cpp`, `tests/RegisterBasicOperations.tests.cpp`,
  `tests/RegisterSpecializedOperations.tests.cpp`, and
  `tests/RegisterRearrangementConversion.tests.cpp` compare results with
  independent scalar references. `Api` results are secondary migration checks,
  not the sole oracle.
- `tests/constexpr/RegisterConstexpr.tests.cpp` instantiates both widths and all
  element types for every Register and RegisterMask operation backed by a
  constant-evaluable `Api` operation. Conversion, widening, and bit-cast cells
  are evaluated across the complete source/target matrix subject to the MSVC
  frontend exclusion below.
- `tests/RegisterPreconditionFailure.tests.cpp` runs alignment and invalid
  runtime-shift failures in isolated processes. Valid boundary transfers,
  conversions, shifts, rearrangements, and mask paths run in the ordinary test
  corpus and in the Clang ASan+UBSan configuration.
- `tests/RegisterOperationMatrix.tests.cpp` is the compile-time availability
  oracle. The generated-code type matrix emits a symbol only when the matching
  `IRegister` operation is available, so unavailable floating modulus and shift
  cells cannot be mistaken for supported identity operations.

## Generated-code and ABI evidence

`cmake/CompareRegisterCodegen.cmake` disassembles separately compiled wrapper
and raw objects, normalizes allocation-dependent details, and compares complete
instruction profiles. Optimized Release comparisons reject wrapper-only
instructions, moves, spills, reloads, stack traffic, return buffers, branches,
temporaries, and indirection.

The permanent corpus assigns one contract to each fixture and one public raw
`Api` baseline to each parity comparison:

The per-symbol ownership, category, baseline, validation owner, retention
decision, and rationale are recorded in
`RegisterCodegenSymbolAudit.csv`; `RegisterCodegenAudit.md` inventories the
source, target, record, CTest, CI-artifact, and documentation boundaries.

- `RegisterCodegenFixture.h` retains composed expressions, mask composition and
  reduction, broadcast reuse, nonzero lane extraction, immediate and complete
  shifts, memory transfers, mutation, special members, reassignment, register
  pressure, and opaque-call behavior. Its register-only, reassignment, and
  memory/composition records use nonoverlapping symbol filters.
- `RegisterTypeMatrixCodegenFixture.h` is the canonical isolated-operation suite.
  It emits one no-inline symbol for every available Register and RegisterMask
  operation across all ten element types and every supported width. Construction,
  load, store, byte transfer, and array observation are separate symbols; dynamic
  indexing is excluded because it is not part of the Register surface. Its
  comparison is partitioned into common non-modulus and integer-modulus records
  so a compiler-specific scalar-remainder diagnostic cannot weaken unrelated
  exact gates.
- `RegisterSpecializedCodegenFixture.h` covers the FMA-independent specialized
  operation matrix once per width and ISA profile.
- `RegisterFmaCodegenFixture.h` contains only the single- and double-precision
  multiply-add symbols and is compiled with FMA explicitly enabled and disabled
  where the ISA profile permits it.
- `RegisterRearrangementCodegenFixture.h` covers selectors, rearrangements,
  conversions, bit casts, width changes, and the public `Register::shuffle`
  versus `Api::shuffle` baseline.

Handwritten intrinsic or scalar mirrors are algorithm-evaluation tools, not
permanent codegen baselines, unless they protect a documented instruction
property that the public `Api` baseline cannot express. Behavioral tests and
benchmarks retain independent scalar oracles where correctness or performance
requires them.

- `RegisterAbi.cpp` and `RegisterAbiRaw.cpp` mirror Register, RegisterMask,
  native-vector, scalar-result, native-result, store, mutating-reference, and
  downstream-consumer signatures as separately compiled no-inline functions.

MSVC and clang-cl supported call-boundary claims use the appropriate
`SIMD_FLAGS(...)` boundary mode. On GCC and GNU-like Clang its
vector-calling-convention adapter is empty, so the paired raw/default platform
ABI is the supported boundary. Windows platform-default calling-convention artifacts are
recorded separately by `RecordRegisterDefaultAbi.cmake`; they are diagnostic and
do not participate in the Windows call-boundary guarantee.

SSE4.2 Release builds compile the same wrapper/raw objects with identical flags
and record diagnostic-only differences. Debug and sanitizer wrapper/raw
comparisons are available only through the explicit `Record-Codegen.ps1`
operation for a selected investigation; ordinary runtime builds do not compile
their fixtures. Optimized Release AVX2 remains the zero-overhead gate except for
the exact diagnostic subsets listed below. Every artifact records `isa_profile` in addition
to the compiler, configuration, width, calling convention, and stack-protector
mode. Artifacts are separated under `register-codegen/sse42/128`,
`register-codegen/avx2/128`, and `register-codegen/avx2/256`. Each profile's
`RegisterExpressionCodegen` and `RegisterConsumerAbi` targets remain build
conveniences; the single `RegisterCodegen.<profile>` CTest owns validation of
every record in that profile exactly once.

Method-attribute records and text evidence live under `method-flags-codegen` and
are published with the Register artifact roots. Their single validation owner is
the `MethodFlagsCodegen` CTest.

## Exception and exclusion ledger

| Cell | Disposition | Justification |
| --- | --- | --- |
| SSE4.2 generated-code corpus | Optimized diagnostic; excluded from the zero-overhead claim | Legacy two-operand SSE can expose aggregate-sensitive instruction selection and register coalescing. The complete 128-bit corpus is retained for compiler-by-compiler inspection without treating a recorded difference as an accepted optimized exception. |
| MSVC 19.44, 128-bit `Register<double>::from_array` under SSE4.2 and AVX2 | Exact accepted Release exception | MSVC adds one `/GS` cookie prologue/epilogue to the wrapper path. The comparator separately recognizes the exact legacy `movdqu` SSE4.2 sequence and exact `vmovdqu` AVX2 sequence, then requires every remaining instruction to match the raw mirror. |
| MSVC memory-capable aggregate corpus | Recorded, outside the zero-overhead claim when `/GS` differs | Stores, transfers, array returns, mutating references, and other addressable paths intentionally retain `/GS`; applying the `RegisterOnly` modifier would suppress protection for functions that can write memory. |
| MSVC 19.44, AVX2/256 integer modulus | Recorded scheduling diagnostic; excluded from the strict parity claim | The `Register::operator%` and `Api::modulus` paths inline the same scalar lane-remainder algorithm, but MSVC schedules independent extract, divide, and insert operations differently after the aggregate operator boundary. The modulus symbols have their own record so this diagnostic cannot relax any other type-matrix operation. |
| MSVC constexpr bit-cast value matrix | Frontend evaluation excluded | MSVC 19.44 terminates with an internal compiler error when evaluating the first Register bit-cast cell. MSVC still compiles the complete availability matrix and validates runtime bit-cast values; GCC and both Clang drivers perform the complete constexpr value matrix. |
| clang-cl Windows platform-default aggregate ABI | Diagnostic only; failing signatures excluded | The platform-default convention may use hidden return storage for aggregate Register results. `SIMD_FLAGS(...)` wrapper/raw parity is the supported clang-cl boundary. |
| MSVC Windows platform-default aggregate ABI | Diagnostic only; hidden-return signatures excluded | The platform-default convention also returns aggregate Register results through caller-provided storage. The supported non-inline boundary uses the appropriate `SIMD_FLAGS(...)` mode; default-convention disassembly remains available without expanding the guarantee. |
| Debug wrapper/raw differences | Optional record, not accepted as Release overhead | Disabled optimization preserves abstraction structure and may add wrapper-only calls, temporaries, or stack traffic. An explicit diagnostic compiles both sides with identical Debug flags when that difference needs investigation. |
| ASan+UBSan wrapper/raw differences | Optional record, not accepted as Release overhead | An explicit Clang 22 diagnostic exposes instrumentation-induced wrapper/raw memory, control-flow, or ABI differences. Runtime sanitizer tests own correctness and absence of sanitizer diagnostics; instruction identity is not a default requirement. |
| 32-bit targets, non-x86 architectures, 512-bit registers, AVX-512, and compilers below the listed versions | Unsupported | No complete correctness, ABI, and zero-overhead matrix exists for these cells. |

No other optimized Release performance exception is accepted. Adding one
requires an exact recognizer, a written justification here, and review of why
the operation cannot satisfy the supported zero-overhead contract.

## Reproduction commands

The formal scoped commands reproduce the native and pinned Linux Register
qualification. Release fingerprints enforce generated-code policy; ordinary
Debug and sanitizer fingerprints contain no Register generated-code workload:

```powershell
tools/Build.ps1 -Scope Native -Compiler Msvc,ClangCl
tools/Run-Tests.ps1 -Scope Native -Compiler Msvc,ClangCl
tools/Build.ps1 -Scope Containers -Compiler Gcc14,Clang22
tools/Run-Tests.ps1 -Scope Containers -Compiler Gcc14,Clang22
tools/Record-Codegen.ps1 -Scope Native -Compiler Msvc -Cell Debug
tools/Record-Codegen.ps1 -Scope Containers -Compiler Clang22 -Cell Debug
tools/Record-Codegen.ps1 -Scope Containers -Compiler Clang22 -Cell AsanUbsan
tools/Build-Benchmarks.ps1 -Scope All -Compiler Msvc,ClangCl,Gcc14,Clang22
tools/Run-Benchmarks.ps1 -Scope All -Compiler Msvc,ClangCl,Gcc14,Clang22
```

The record command requires an explicit compiler and cell, builds only the
generated-code fixture and comparison targets, and writes dedicated provenance.
Its record-only outputs cannot satisfy a missing Release enforcement result.

Benchmarks are supplemental and run only after strict generated-code gates. The
Register benchmark operands derive from a runtime clock seed and are returned
from each measured expression so constant folding and dead-code elimination
cannot replace the work. Benchmark timing never overrides an assembly failure
and has no pass/fail performance threshold.
