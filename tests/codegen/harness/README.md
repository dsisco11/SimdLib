# Extraction and instruction-contract regression harness

Configure this standalone project with explicit FileCheck, llvm-objdump, and
llvm-readobj paths (or `SIMDLIB_CODEGEN_LLVM_ROOT`). Also select
`SIMDLIB_HARNESS_CLANG` and `SIMDLIB_HARNESS_OBJCOPY` as absolute executable
paths. Clang's integrated assembler creates controlled ELF and COFF fixtures;
LLVM objcopy creates duplicate and invalid symbol metadata. These two tools are
harness-only dependencies. Supply an installed Catch2 3 package or
`SIMDLIB_CATCH2_SOURCE` pointing to an existing source checkout. No Python or lit is used.

Example additions to a Windows CMake configure command:

```text
-DSIMDLIB_HARNESS_CLANG=C:/Program Files/LLVM/bin/clang.exe
-DSIMDLIB_HARNESS_OBJCOPY=C:/Program Files/LLVM/bin/llvm-objcopy.exe
```

Quote each complete `-D...` argument when its path contains spaces. Build before
running CTest. The fixture compiler uses provisional `/O1 /Gy /arch:AVX2` or
`-O1 -ffunction-sections -mavx2`; these are extraction probes, not a qualified
production optimization policy.

The suite checks six compiler-generated functions, three tool behaviors, and
assembled object regressions. The assembly contains interior labels after early
returns, same-address non-function aliases, ELF alignment padding, repeated COFF
code/data section names, malformed objects, and invalid symbol identities.
Assertions inspect the wrapper's intact LLVM output and explicit rejection
messages. They do not parse or reconstruct instruction boundaries.

`Instructions.*` additionally tests whole-body forbidden work, exact counts,
widths, immediates, operand relationships, exact targets, and bounded cookie/ABI
sequences. Deliberate mutations must fail for the expected reason through the
shared Catch2/FileCheck checker. Isolated Catch2 executions prove that either a
failed primary or supplement fails the composite test and that later rules still
execute. Independent case/rule ledgers reject missing definitions, omitted rules,
and incorrect applicability. Receipt/cache tests modify only disposable copies.
Process tests cover Unicode paths, quoted arguments, captured output, exit status,
and bounded timeouts. The standalone harness also includes the production pilot.
`SIMDLIB_HARNESS_BUILD_CONTRACTS=OFF` retains extraction-only operation without
Catch2; this is the default on core-only GCC 13.

See [instruction contracts](../../../docs/codegen/InstructionContracts.md) for
the production pilot and shared development registration interface.

Each test owns its output directory. `harness.txt` retains wrapper exit status
and diagnostics for adversarial cases; ordinary extraction artifacts retain
metadata, full/selected disassembly, exact constant section dumps, and receipts.
