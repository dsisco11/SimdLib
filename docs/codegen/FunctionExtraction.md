# LLVM function inspection

`cmake/codegen/ExtractFunction.cmake` runs LLVM tools on one existing object.
It does not decode instructions, slice disassembly text, or parse constant bytes.
FileCheck owns instruction expectations in the subsequent checking layer.

## Inputs and outputs

Required CMake script variables are `OBJECT_FILE`, `EXPECTED_SYMBOL`,
`OUTPUT_DIRECTORY`, `LLVM_OBJDUMP`, and `LLVM_READOBJ`. Tool paths must be
explicit absolute paths. Optional `CONSTANT_SYMBOLS` names exact defined data
symbols whose contents a contract needs. Each invocation owns a separate output
directory; callers must treat a nonzero exit as failure.

The script retains full disassembly, raw object metadata, LLVM's selected
disassembly in `body.txt`, constant symbol metadata and LLVM hex dumps in
`constants.txt`, selection arguments, tool diagnostics, and an extraction receipt
with object/tool hashes. The object must remain unchanged during inspection.

## Complete selection

The metadata adapter rejects missing or duplicate entry symbols, unsupported
formats, empty functions, and invalid section bounds. ELF function sizes provide
the end address. COFF requires exclusive executable `/Gy` COMDAT function
sections because ordinary COFF function symbols do not supply sizes.
The supported extraction formats are x86-64 COFF and ELF. The adjacent legacy
method-attribute gate also permits x86 configurations; that gate remains
unchanged and has not been migrated or qualified by these extraction results.

For uniquely named sections, llvm-objdump receives the section and exact
start/stop addresses. For repeated COFF section names, it also receives the entry
symbol and all interior symbols. These names must be unambiguous. Selecting only
the entry would stop at an interior label and could hide a later call. Explicit
end bounds prevent ELF padding from becoming part of a function's checks.

LLVM's complete selected output is retained unchanged, including operands,
register widths, labels and relocations. Decoding failures and missing output
fail inspection. No return instruction is treated as an end marker.

Constants use `llvm-readobj --hex-dump=<numeric-section-index>` after exact
symbol resolution. Numeric selection distinguishes identically named COFF
sections; LLVM formats the bytes directly.

## Responsibility and evidence

The standalone harness in `tests/codegen/harness` exercises actual LLVM object
inspection and explicit failure cases. Its optimization settings are provisional
extraction probes, not a qualification of production optimization policy.

Configure it with `SIMDLIB_CODEGEN_LLVM_ROOT` (or the three explicit tool paths),
plus `SIMDLIB_HARNESS_CLANG` and `SIMDLIB_HARNESS_OBJCOPY`. These last two tools
assemble COFF/ELF edge cases and construct deliberate duplicate symbols; the
extraction wrapper itself needs only llvm-readobj and llvm-objdump. Build the
harness before running CTest. For example, from a configured compiler environment:

```powershell
cmake -S tests/codegen/harness -B out/extraction -G Ninja `
  "-DSIMDLIB_CODEGEN_LLVM_ROOT=$PWD/out/codegen-tools-provisioned" `
  '-DSIMDLIB_HARNESS_CLANG=C:/Program Files/LLVM/bin/clang.exe' `
  '-DSIMDLIB_HARNESS_OBJCOPY=C:/Program Files/LLVM/bin/llvm-objcopy.exe'
cmake --build out/extraction
ctest --test-dir out/extraction --output-on-failure
```

The design evaluation and reproducible commands are retained locally in
`out/codegen-option1-evaluation/REPORT.md`. That evaluation demonstrated ordinary
MSVC/clang-cl COFF and GCC/Clang ELF selection, interior-label truncation with
entry-only selection, explicit bounds excluding padding, and numeric constant
section selection. The regression harness validates the implemented wrapper.

### Contract traceability and validation

The controlling requirements are [complete function inspection in the proposal](../RegisterCodegenPolicyProposal.md#complete-function-inspection)
and the extraction items in [the tasklist](../RegisterCodegenPolicy.todo).

| Requirement | Implementation and evidence |
| --- | --- |
| Exact identity and complete bounds | Metadata validation; real missing/duplicate/empty-function regressions and ELF padding exclusion |
| Blocks after early returns; operands and relocations | LLVM bounded selection; ELF and repeated-name COFF interior-label regressions, same-address COFF alias, and six compiler-generated functions |
| Referenced constant contents | Numeric LLVM hex dumps; repeated-name COFF data sections, missing/duplicate constants and invalid constant offset rejection |
| Invalid inputs fail | Real empty/malformed objects, unsupported object format, missing object and explicit tool rejection tests |

The replacement passed 27/27 CTest checks on MSVC 19.44.35228 and clang-cl
22.1.8 with LLVM inspection tools 22.1.8. Logs are retained locally at
`out/codegen-extraction-probe/harness-{msvc,clang-cl}-wrapper-ctest.log` alongside
configure/build logs and per-case artifacts. Six real-object checks also passed
on the GNU-style Clang 22.1.8 ELF fixture. Initial retained GCC 14.2.0 object
inspection was followed by fresh container compilation and six real-object
checks each on GCC 13.2.1, GCC 14.2.0 and Clang 22.1.3. Those independent
receipts live in `out/codegen-container-evidence/{gcc13,gcc14,clang22}`.
All settings remain provisional extraction evidence. See
[tool provisioning](ToolProvisioning.md) for distribution and environment proof.
