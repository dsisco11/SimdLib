# Method-flags source audit

`tools/Audit-MethodFlagsSource.ps1` scans active C++ declarations under
`include`, `tests`, and `examples`. It enforces the canonical method-flags
surface directly; no generated declaration inventory is required.

`tools/Run-RepositoryAudit.ps1` invokes the source audit once for the canonical
source digest. Run it directly for a focused check:

```powershell
./tools/Audit-MethodFlagsSource.ps1
```

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
exercise the public preprocessor diagnostics. They are excluded from production-source policy checks.

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
