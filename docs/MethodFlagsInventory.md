# Method flags declaration inventory

`MethodFlagsInventory.csv` is the exhaustive migration and review ledger for
active uses of `VECTORCALL`, `SIMDLIB_REGISTER_ONLY`,
`SIMDLIB_FORCE_INLINE`, and `SIMDLIB_FLATTEN` under `include`, `tests`, and
`examples`.

The inventory deliberately treats the return type as ordinary, independent C++
syntax. `TargetFlags` contains only the attribute and calling-convention macro
that belongs immediately before the function name. For example:

```cpp
Register SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
combine(Register rhs) const noexcept;
```

The generator removes comments while retaining source positions, groups all
legacy tokens belonging to one declaration, and verifies that the sum of
`LegacyOccurrenceCount` equals the complete active-token count. Regenerate or
verify the ledger with:

```powershell
./tools/Generate-MethodFlagsInventory.ps1
./tools/Generate-MethodFlagsInventory.ps1 -Verify
```

## Classification totals

The ledger contains 88 reviewed exception records accounting for all 186 active
legacy occurrences. All individually classified migratable declarations have
left the active ledger; the implementation plan retains their completed-group
counts and validation evidence.

| Classification | Count |
| --- | ---: |
| Deferred runtime-path repairs | 24 |
| Compiler-adapter definitions | 19 |
| Intentional legacy comparison baselines | 17 |
| Grammar exceptions | 15 |
| Low-level configuration probes | 13 |

Migrated declarations no longer appear in this active exception ledger. Their
independently reviewed input/output directions and exact unified spellings are
preserved by the implementation-plan evidence.

## Modifier decisions

All 88 active records are reviewed exceptions, so their target-modifier fields
remain `Exception`. Completed modifier decisions and their validation evidence
are retained in the implementation plan rather than duplicated in the active
ledger.

Twenty-four exceptions use `KeepLegacyPendingSourceRepair`. They retain the
existing `RegisterOnly` promise and legacy declaration spelling; the inventory
does not silently relax the promise or misrepresent them as migrated. Their
runtime paths are the deferred immediate-control blend and shuffle families:

- implementation `blend`, `blend_slow`, and `shuffle_32_slow` methods that reach
  reference-writing or array-backed portable helpers;
- the corresponding generic `Api::shuffle`, `Api::blend`,
  `Api::shuffle_lo_slow`, and `Api::shuffle_hi_slow` forwarding declarations.

These declarations require their separately planned non-storage runtime
implementations before migration, or explicit approval before any
`RegisterOnly` promise is relaxed. Focused SSE4.2 and AVX2 tests own correctness
coverage for the deferred declarations in their retained form.

The exception reasons distinguish compiler adapters, comparison baselines,
grammar limitations, low-level probes, and declarations pending source repair;
none of those categories implies a new optimization promise.

## Constant-evaluation and call-path review

For pending source repairs, `ConstexprAudit`, `Memory`, `DirectCalls`, and
`TransitiveAudit` preserve the distinction between constant-evaluation and
runtime paths, including direct writes, addressable local storage, and
transitive writer families. Other exception categories record why those fields
are not applicable.

## Reviewed exceptions

The unified macro remains inapplicable to constructors, destructors, and
conversion operators because those declaration categories have no ordinary
return type before the function name. Compiler-adapter definitions,
low-level configuration probes, pending runtime-path repairs, and the
intentional legacy half of ABI or generated-code comparisons keep their legacy
spelling for their stated test, configuration, or deferred-repair purpose. Each
exception has its exact reason in `Disposition` and `Reason`.

## CSV fields

- `Path`, `Line`, `Symbol`, `Context`, and `Kind` identify the declaration,
  containing implementation specialization where applicable, or exception.
- `Existing` and `LegacyOccurrenceCount` record the present legacy surface.
- `SimdInput`, `SimdOutput`, and `Boundary` record the call-boundary contract.
- `Memory`, `ConstexprAudit`, `DirectCalls`, and `TransitiveAudit` record the
  no-write review evidence.
- `RegisterOnlyTarget`, `ForceInlineTarget`, and `FlattenTarget` record each
  modifier decision independently.
- `ForceInlineAudit` and `FlattenAudit` state why the optimization modifier is
  retained or omitted.
- `TargetFlags` provides the exact unified macro invocation while leaving the
  return type independent.
- `Disposition` and `Reason` record migration eligibility or the reviewed
  exception.
