# Production Static-Assertion Inventory

The production-header audit classifies every retained `static_assert` and
rejects any new occurrence that is not listed with a justification in
`cmake/PublicHeaderStaticAssertAllowlist.txt`. `tools/Run-RepositoryAudit.ps1`
runs this source-revision-wide contract once for the canonical source digest,
also rejects implementation-detail use in public-consumer fixtures, and writes
the machine-readable result bound into the unified build receipt. Compiler
configure trees do not repeat the audit as a target or CTest.

## Extraction result

- `Bmi.h`: 121 test-example assertions moved verbatim to `tests/constexpr/BmiConstexpr.tests.cpp`.
- `UInt128.h`: six arithmetic, shift, and bit-helper examples moved verbatim to `tests/constexpr/UInt128Constexpr.tests.cpp`.
- Other production headers contained no namespace-scope or function-adjacent test examples.
- The dedicated sources keep the migrated assertions before separately labelled expanded contracts, so the original proof is preserved independently of later additions.

## Retained assertions

| Header | Classification | Why evaluation must remain in production |
| --- | --- | --- |
| `UInt128.h` | ABI/layout invariants and template-width constraints | Register conversion requires a 16-byte, 16-byte-aligned, standard-layout, trivially-copyable representation; invalid mask widths must fail at instantiation. |
| `Bmi.h` | Template control-field constraints and implementation safety invariants | Invalid immediate controls must be diagnosed and the 64-bit product split must retain its word-size assumption. |
| `Api.h` | Template constraints, dependent unsupported-mapping diagnostics, and implementation safety invariants | Invalid widening, conversion, packed-result, shift, endian, and callable shapes must fail at the caller instantiation. |
| `SimdAlgo.h` | Template constraints | Invalid packed comparison result widths and storage shapes must fail at instantiation. |
| `Detail/Implementations.h` | Dependent unsupported-mapping diagnostics, extraction-index constraints, and implementation safety invariants | Unsupported widening shapes need dependent diagnostics; extraction and scalar lane-size assumptions must be checked where instantiated. |
| `Detail/Extensions.h` | Template constraints | Negative immediate whole-register shifts must fail at instantiation. |

The allowlist is the canonical assertion inventory. The audit requires every
retained entry to match at least one production assertion and rejects stale
entries, so duplicated counts are intentionally not maintained here.
