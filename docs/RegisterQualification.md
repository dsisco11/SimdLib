# Register Qualification Contract

This document defines the supported `Register<T, Bits>` and
`RegisterMask<T, Bits>` qualification matrix, the evidence required for each
supported cell, and the exclusions that bound the zero-overhead claim. Generated
artifacts and individual execution results are intentionally not committed; the
commands below reproduce them under `build*/register-codegen` or
`out/container`.

## Supported matrix

| Dimension | Supported cells |
| --- | --- |
| Architecture | x86-64 |
| Register widths | 128 and 256 bits |
| Availability floor | SSE4.2 exposes the 128-bit specialization; AVX2 additionally exposes the 256-bit specialization |
| Optimized zero-overhead profile | AVX2 for the complete 128-bit and 256-bit wrapper/raw corpus |
| Element types | `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, `int32_t`, `uint32_t`, `int64_t`, `uint64_t`, `float`, and `double` |
| Windows compilers | MSVC 19.44 and clang-cl 22 |
| Linux compilers | GCC 14 and Clang 22 on the pinned Alpine/musl images |
| Optimized configuration | Release with strict wrapper/raw generated-code comparison |
| Diagnostic configurations | Debug on every supported compiler; ASan+UBSan on Clang 22 |
| FMA profiles | Explicitly enabled and explicitly disabled specialized-operation corpora |

Every supported compiler must compile the C++23 interface, the complete runtime
and constexpr corpus, both register widths, and the external consumer. An
optimized cell is supported only when its applicable wrapper/raw profiles are
instruction-identical after allocation-independent normalization, except for an
exact exception listed below.

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
  and both widths. Construction, load, store, byte transfer, and array
  observation are separate symbols.
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

Debug and sanitizer builds compile the same wrapper/raw objects with identical
flags and write disassembly, normalized profiles, provenance, and a
`recorded-difference` result. These configurations establish visibility of
diagnostic-only differences; optimized Release remains the zero-overhead gate.

## Exception and exclusion ledger

| Cell | Disposition | Justification |
| --- | --- | --- |
| MSVC 19.44, 128-bit `Register<double>::from_array` | Exact accepted Release exception | MSVC adds one `/GS` cookie prologue/epilogue to the wrapper path. The comparator accepts only the complete known instruction sequence and requires every remaining instruction to match the raw mirror. |
| MSVC memory-capable aggregate corpus | Recorded, outside the zero-overhead claim when `/GS` differs | Stores, transfers, array returns, mutating references, and other addressable paths intentionally retain `/GS`; applying `SIMDLIB_REGISTER_ONLY` would suppress protection for functions that can write memory. |
| MSVC constexpr bit-cast value matrix | Frontend evaluation excluded | MSVC 19.44 terminates with an internal compiler error when evaluating the first Register bit-cast cell. MSVC still compiles the complete availability matrix and validates runtime bit-cast values; GCC and both Clang drivers perform the complete constexpr value matrix. |
| clang-cl Windows platform-default aggregate ABI | Diagnostic only; failing signatures excluded | The platform-default convention may use hidden return storage for aggregate Register results. `VECTORCALL` wrapper/raw parity is the supported clang-cl boundary. |
| MSVC Windows platform-default aggregate ABI | Diagnostic only; hidden-return signatures excluded | The platform-default convention also returns aggregate Register results through caller-provided storage. The supported non-inline boundary uses `VECTORCALL`; default-convention disassembly remains available without expanding the guarantee. |
| Debug wrapper/raw differences | Recorded, not accepted as Release overhead | Disabled optimization preserves abstraction structure and may add wrapper-only calls, temporaries, or stack traffic. Both sides are compiled with identical Debug flags so the difference remains inspectable. |
| ASan+UBSan wrapper/raw differences | Recorded, not accepted as Release overhead | Sanitizer instrumentation intentionally changes memory and control-flow code. Correctness and absence of sanitizer diagnostics are required; instruction identity is not. |
| SSE4.2-only Register configuration | Excluded from this zero-overhead contract | The 128-bit type follows the existing SSE4.2 `Api` availability boundary, but no standalone complete wrapper/raw and ABI corpus is defined for that compiler profile. The AVX2 profile is the only optimized machine-code claim made here. |
| 32-bit targets, non-x86 architectures, 512-bit registers, AVX-512, and compilers below the listed versions | Unsupported | No complete correctness, ABI, and zero-overhead matrix exists for these cells. |

No other optimized Release performance exception is accepted. Adding one
requires an exact recognizer, a written justification here, and review of why
the operation cannot satisfy the supported zero-overhead contract.

## Reproduction commands

Native Windows Release and Debug builds use the ordinary CMake targets with
`SIMDLIB_BUILD_REGISTER_CODEGEN=ON`. Debug additionally sets
`SIMDLIB_REGISTER_CODEGEN_RECORD_ONLY=ON`.

The pinned Linux matrix is reproduced with:

```powershell
.\tools\Run-ContainerMatrix.ps1 -Mode Full -Compiler All
.\tools\Run-ContainerMatrix.ps1 -Mode Codegen -Compiler All -NoBuild
.\tools\Run-ContainerMatrix.ps1 -Mode Debug -Compiler All -NoBuild
.\tools\Run-ContainerMatrix.ps1 -Mode Sanitizer -Compiler Clang22 -NoBuild
.\tools\Run-ContainerMatrix.ps1 -Mode Benchmark -Compiler All -NoBuild
```

Benchmarks are supplemental and run only after strict generated-code gates. The
Register benchmark operands derive from a runtime clock seed and are returned
from each measured expression so constant folding and dead-code elimination
cannot replace the work. Benchmark timing never overrides an assembly failure
and has no pass/fail performance threshold.
