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
| Diagnostic configurations | Debug on every supported compiler; ASan+UBSan on Clang 22 |
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
  oracle. Unsupported operations do not become supported merely because a
  code-generation fixture can instantiate a no-op fallback cell.

## Generated-code and ABI evidence

`cmake/CompareRegisterCodegen.cmake` disassembles separately compiled wrapper
and raw objects, normalizes allocation-dependent details, and compares complete
instruction profiles. Optimized Release comparisons reject wrapper-only
instructions, moves, spills, reloads, stack traffic, return buffers, branches,
temporaries, and indirection.

The corpus is divided so one optimization decision cannot hide another:

- `RegisterCodegenFixture.h` covers common expression and overload shapes.
- `RegisterTypeMatrixCodegenFixture.h` emits an isolated no-inline function for
  each common Register and RegisterMask operation across all ten element types
  and every width available in the selected ISA profile. Construction, load,
  store, byte transfer, and array observation are separate symbols.
- `RegisterSpecializedCodegenFixture.h` covers specialized arithmetic and both
  FMA modes across the supported type matrix.
- `RegisterRearrangementCodegenFixture.h` covers selectors, rearrangements,
  conversions, bit casts, and width changes.
- `RegisterAbi.cpp` and `RegisterAbiRaw.cpp` mirror Register, RegisterMask,
  native-vector, scalar-result, native-result, store, mutating-reference, and
  downstream-consumer signatures as separately compiled no-inline functions.

MSVC and clang-cl supported call-boundary claims use `VECTORCALL`. On GCC and
GNU-like Clang the macro is empty, so the paired raw/default platform ABI is the
supported boundary. Windows platform-default calling-convention artifacts are
recorded separately by `RecordRegisterDefaultAbi.cmake`; they are diagnostic and
do not participate in the Windows call-boundary guarantee.

SSE4.2, Debug, and sanitizer builds compile the same wrapper/raw objects with
identical flags and write disassembly, normalized profiles, provenance, and a
`recorded-difference` result when the profiles diverge. These configurations
establish visibility of diagnostic-only differences; optimized Release AVX2
remains the zero-overhead gate. Every artifact records `isa_profile` in addition
to the compiler, configuration, width, calling convention, and stack-protector
mode. Artifacts are separated under `register-codegen/sse42/128`,
`register-codegen/avx2/128`, and `register-codegen/avx2/256`.

## Exception and exclusion ledger

| Cell | Disposition | Justification |
| --- | --- | --- |
| SSE4.2 generated-code corpus | Optimized diagnostic; excluded from the zero-overhead claim | Legacy two-operand SSE can expose aggregate-sensitive instruction selection and register coalescing. The complete 128-bit corpus is retained for compiler-by-compiler inspection without treating a recorded difference as an accepted optimized exception. |
| MSVC 19.44, 128-bit `Register<double>::from_array` under SSE4.2 and AVX2 | Exact accepted Release exception | MSVC adds one `/GS` cookie prologue/epilogue to the wrapper path. The comparator separately recognizes the exact legacy `movdqu` SSE4.2 sequence and exact `vmovdqu` AVX2 sequence, then requires every remaining instruction to match the raw mirror. |
| MSVC memory-capable aggregate corpus | Recorded, outside the zero-overhead claim when `/GS` differs | Stores, transfers, array returns, mutating references, and other addressable paths intentionally retain `/GS`; applying `SIMDLIB_REGISTER_ONLY` would suppress protection for functions that can write memory. |
| MSVC constexpr bit-cast value matrix | Frontend evaluation excluded | MSVC 19.44 terminates with an internal compiler error when evaluating the first Register bit-cast cell. MSVC still compiles the complete availability matrix and validates runtime bit-cast values; GCC and both Clang drivers perform the complete constexpr value matrix. |
| clang-cl Windows platform-default aggregate ABI | Diagnostic only; failing signatures excluded | The platform-default convention may use hidden return storage for aggregate Register results. `VECTORCALL` wrapper/raw parity is the supported clang-cl boundary. |
| MSVC Windows platform-default aggregate ABI | Diagnostic only; hidden-return signatures excluded | The platform-default convention also returns aggregate Register results through caller-provided storage. The supported non-inline boundary uses `VECTORCALL`; default-convention disassembly remains available without expanding the guarantee. |
| Debug wrapper/raw differences | Recorded, not accepted as Release overhead | Disabled optimization preserves abstraction structure and may add wrapper-only calls, temporaries, or stack traffic. Both sides are compiled with identical Debug flags so the difference remains inspectable. |
| ASan+UBSan wrapper/raw differences | Recorded, not accepted as Release overhead | Sanitizer instrumentation intentionally changes memory and control-flow code. Correctness and absence of sanitizer diagnostics are required; instruction identity is not. |
| 32-bit targets, non-x86 architectures, 512-bit registers, AVX-512, and compilers below the listed versions | Unsupported | No complete correctness, ABI, and zero-overhead matrix exists for these cells. |

No other optimized Release performance exception is accepted. Adding one
requires an exact recognizer, a written justification here, and review of why
the operation cannot satisfy the supported zero-overhead contract.

## Reproduction commands

The formal scoped commands reproduce the native and pinned Linux Register
qualification. Release fingerprints enforce generated-code policy; Debug and
sanitizer fingerprints record diagnostics:

```powershell
tools/Build.ps1 -Scope Native -Compiler Msvc,ClangCl
tools/Run-Tests.ps1 -Scope Native -Compiler Msvc,ClangCl -SkipBuild
tools/Build.ps1 -Scope Containers -Compiler Gcc14,Clang22
tools/Run-Tests.ps1 -Scope Containers -Compiler Gcc14,Clang22 -SkipBuild
tools/Build-Benchmarks.ps1 -Scope All -Compiler Msvc,ClangCl,Gcc14,Clang22
tools/Run-Benchmarks.ps1 -Scope All -Compiler Msvc,ClangCl,Gcc14,Clang22
```

Benchmarks are supplemental and run only after strict generated-code gates. The
Register benchmark operands derive from a runtime clock seed and are returned
from each measured expression so constant folding and dead-code elimination
cannot replace the work. Benchmark timing never overrides an assembly failure
and has no pass/fail performance threshold.
