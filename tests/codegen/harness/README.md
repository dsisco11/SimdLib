# Extraction regression harness

Configure this standalone project with explicit FileCheck, llvm-objdump, and
llvm-readobj paths (or `SIMDLIB_CODEGEN_LLVM_ROOT`). Also select
`SIMDLIB_HARNESS_CLANG` and `SIMDLIB_HARNESS_OBJCOPY` as absolute executable
paths. Clang's integrated assembler creates controlled ELF and COFF fixtures;
LLVM objcopy creates duplicate and invalid symbol metadata. These two tools are
harness-only dependencies. No Python or lit is used.

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

Each test owns its output directory. `harness.txt` retains wrapper exit status
and diagnostics for adversarial cases; ordinary extraction artifacts retain
metadata, full/selected disassembly, exact constant section dumps, and receipts.
