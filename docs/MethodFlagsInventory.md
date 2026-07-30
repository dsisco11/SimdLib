# Method-flags source inventories

The method-flags source audit maintains two generated ledgers:

- `MethodFlagsInventory.csv` records active uses of the retired declaration
  macros under `include`, `tests`, and `examples`. Its normal completed state is
  a header-only CSV: any new record represents declaration boilerplate that
  must be removed or explicitly rejected by the audit.
- `MethodFlagsRegisterOnly.csv` lists every canonical `SIMD_FLAGS(...)`
  declaration containing `RegisterOnly`, with its path, line, symbol, and full
  flag list. This makes the promise reviewable without claiming that a source
  scanner can prove the function body or its transitive callees are free of
  memory writes.

Generate or verify both ledgers with:

```powershell
./tools/Generate-MethodFlagsInventory.ps1
./tools/Generate-MethodFlagsInventory.ps1 -Verify
```

The repository audit runs the verifier and binds the count and SHA-256 digest
of each ledger into its result. A source change cannot reuse an audit result
whose inventories do not match.

## Enforced source policy

The scanner removes C++ comments while preserving line positions, then rejects:

- active `VECTORCALL`, `SIMDLIB_REGISTER_ONLY`, `SIMDLIB_FORCE_INLINE`, or
  `SIMDLIB_FLATTEN` tokens;
- object-like macros named `Neither`, `In`, `Out`, `InOut`, `RegisterOnly`,
  `ForceInline`, or `Flatten`;
- unknown, duplicated, reordered, or otherwise noncanonical
  `SIMD_FLAGS(...)` token lists;
- internal compiler-adapter use outside the configuration and raw compiler
  fixtures that require it;
- internal method-flags helper names exposed through Doxygen comments.

Intentional compile-failure fixtures named `Invalid*.cpp` remain available to
exercise the public preprocessor diagnostics. They are not treated as
production declarations by the inventory.

`Test-MethodFlagsSourceAudit.ps1` creates isolated disposable source trees and
proves that the scanner accepts canonical syntax while rejecting each policy
violation above.

## Internal compiler fixtures

`SIMD_FLAGS(...)` is the only supported declaration spelling. Configuration,
ABI-placement, and generated-code fixtures may compose the internal
`SIMDLIB_METHOD_FLAGS_*` adapters directly when the raw compiler spelling is
the subject of the test. Those files are kept on an exact allowlist; the
adapters are not downstream API and cannot be used from another source file
without failing the audit.

## RegisterOnly ledger fields

- `Path` and `Line` locate the declaration.
- `Symbol` identifies the declared function or method.
- `Flags` preserves the complete canonical invocation so reviewers can assess
  the boundary mode and the other optimization promises together.