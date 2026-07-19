# Production Static-Assertion Inventory

The production-header audit classifies every retained `static_assert` and rejects any new occurrence that is not listed with a justification in `cmake/PublicHeaderStaticAssertAllowlist.txt`. The CMake build target and CTest entry both execute `cmake/AuditPublicHeaderAssertions.cmake`.

## Extraction result

- `Bmi.h`: 121 test-example assertions moved verbatim to `tests/constexpr/BmiConstexpr.tests.cpp`.
- `UInt128.h`: six arithmetic, shift, and bit-helper examples moved verbatim to `tests/constexpr/UInt128Constexpr.tests.cpp`.
- Other production headers contained no namespace-scope or function-adjacent test examples.
- The dedicated sources keep the migrated assertions before separately labelled expanded contracts, so the original proof is preserved independently of later additions.

## Retained assertions

| Header | Count | Classification | Why evaluation must remain in production |
| --- | ---: | --- | --- |
| `UInt128.h` | 5 | Four ABI/layout invariants; one template-width constraint | Register conversion requires a 16-byte, 16-byte-aligned, standard-layout, trivially-copyable representation; invalid mask widths must fail at instantiation. |
| `Bmi.h` | 3 | Two template control-field constraints; one implementation safety invariant | Invalid immediate controls must be diagnosed and the 64-bit product split must retain its word-size assumption. |
| `Api.h` | 17 | Fourteen template constraints, one dependent unsupported-mapping diagnostic, two implementation safety invariants | Invalid widening, conversion, packed-result, shift, endian, and callable shapes must fail at the caller instantiation. |
| `SimdAlgo.h` | 2 | Template constraints | Invalid packed comparison result widths and storage shapes must fail at instantiation. |
| `Detail/Implementations.h` | 19 | Sixteen dependent unsupported-mapping diagnostics, two extraction-index constraints, one implementation safety invariant | Unsupported widening shapes need dependent diagnostics; extraction and scalar lane-size assumptions must be checked where instantiated. |
| `Detail/Extensions.h` | 2 | Template constraints | Negative immediate whole-register shifts must fail at instantiation. |
| **Total** | **48** | 23 template constraints, 17 unsupported-instantiation diagnostics, four ABI invariants, and four implementation safety invariants | No test-example assertion remains in production headers. |

The allowlist has 30 entries because one justified rule covers repeated assertions with the same contract, such as the sixteen backend-dependent widening diagnostics.