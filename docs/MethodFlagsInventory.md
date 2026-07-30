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

The ledger contains 443 declaration records accounting for 1,049 active legacy
occurrences. Declarations leave this active ledger after migration; the
implementation plan retains the completed-group counts and validation evidence.

| Classification | Count |
| --- | ---: |
| Migratable ordinary functions | 355 |
| Deferred runtime-path repairs | 24 |
| Compiler-adapter definitions | 19 |
| Intentional legacy comparison baselines | 17 |
| Grammar exceptions | 15 |
| Low-level configuration probes | 13 |

The migratable declarations have independently recorded SIMD directions:

| Boundary | Count |
| --- | ---: |
| `Neither` | 106 |
| `In` | 91 |
| `Out` | 35 |
| `InOut` | 123 |

`SimdInput` and `SimdOutput` retain the two independent decisions behind each
boundary. A SIMD input is a native or SimdLib register value entering by value;
references, pointers, arrays, spans, and an implicit object alone do not make a
declaration `In`. A SIMD output is a native or SimdLib register value returned
by value; scalar, array, pointer, and reference results do not make it `Out`.

## Modifier decisions

`RegisterOnlyTarget` records 133 resolved existing promises, 87 omissions, 135
separately reviewable additions, and 88 exceptions. Candidate status never adds
the promise during mechanical migration. It means that the declaration has no
authored direct write, no known runtime-storage helper, and no unresolved
transitive callee in the reviewed source. Generated-code evidence and a separate
approval are still required before adding `RegisterOnly` because its Microsoft
mapping can suppress `/GS` instrumentation.

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

`ForceInlineTarget` retains 272 current optimized-code-shape promises and omits
the modifier from 83 declarations; 88 records are exceptions. No retained use
is classified as ODR-only: templates, in-class definitions, `constexpr`, or an
ordinary `inline` specifier already provide ODR semantics independently.

`FlattenTarget` retains 196 explicit recursive-inlining contracts and omits the
modifier from 159 declarations; 88 records are exceptions. Missing `Flatten`
is not inferred merely from a containing type or neighboring method.
`FlattenAudit` distinguishes leaf declarations from composed declarations that
have no separately established recursive-inlining requirement.

## Constant-evaluation and call-path review

`ConstexprAudit` records runtime-only declarations, shared constexpr bodies,
and explicit constant-evaluation branches separately. `Memory` and
`TransitiveAudit` distinguish direct writes, addressable local storage,
read-only inputs, known writer families, reviewed no-write callees, and the
existing promises pending source repair. `DirectCalls` keeps the reviewed call
surface visible instead of treating the containing file or operation family as
evidence.

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
