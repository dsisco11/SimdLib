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

The ledger contains 1,524 declaration records accounting for 4,443 active
legacy occurrences:

| Classification | Count |
| --- | ---: |
| Migratable ordinary functions | 1,460 |
| Compiler-adapter definitions | 19 |
| Intentional legacy comparison baselines | 17 |
| Grammar exceptions | 15 |
| Low-level configuration probes | 13 |

The migratable declarations have independently recorded SIMD directions:

| Boundary | Count |
| --- | ---: |
| `Neither` | 135 |
| `In` | 221 |
| `Out` | 142 |
| `InOut` | 962 |

`SimdInput` and `SimdOutput` retain the two independent decisions behind each
boundary. A SIMD input is a native or SimdLib register value entering by value;
references, pointers, arrays, spans, and an implicit object alone do not make a
declaration `In`. A SIMD output is a native or SimdLib register value returned
by value; scalar, array, pointer, and reference results do not make it `Out`.

## Modifier decisions

`RegisterOnlyTarget` records 933 resolved existing promises, 12 existing
promises pending source repair, 117 omissions, 398 separately reviewable
additions, and 64 declaration-form exceptions. Candidate status never adds the promise
during mechanical migration. It means that the declaration has no authored
direct write, no known runtime-storage helper, and no unresolved transitive
callee in the reviewed source. Generated-code evidence and a separate approval
are still required before adding `RegisterOnly` because its Microsoft mapping
can suppress `/GS` instrumentation.

Twelve existing declarations are classified `KeepPendingSourceRepair`. Their
target spelling retains `RegisterOnly`; the inventory does not silently relax
an existing promise. Their runtime call paths presently reach one of these
authored storage forms:

- `register_blend` and `register_blend_bytes`, which reach reference-writing
  lane helpers and use a runtime array representation on non-MSVC compilers;
- `register_shuffle_32` and dependent generic shuffle or blend paths that reach
  array-backed control-mask helpers for at least one supported instantiation.

The affected operation families are recorded individually in the CSV across
`Api`, `Implementations`, `Register`, and their code-generation fixture. They
require register/scalar source repairs before migration, or explicit approval
before any `RegisterOnly` promise is relaxed.

The complete implementation-layer investigation is recorded in
`RuntimeArrayRegisterConstruction.todo`. It records the original 81 runtime
methods and the 38 deferred blend/shuffle methods that still reconstruct
registers through array-backed helpers, including methods that do not currently
claim `RegisterOnly`. `min_position` index-vector
initializers and the array-conversion branches of `construct` are excluded from
that runtime list because their relevant helper calls are evaluated only during
constant evaluation. Both `construct` implementations now accept their input
arrays by const reference, so their runtime intrinsic-load paths no longer
create by-value array parameters.

`ForceInlineTarget` retains 1,346 current optimized-code-shape promises and
omits the modifier from 114 declarations. No retained use is classified as
ODR-only: templates, in-class definitions, `constexpr`, or an ordinary
`inline` specifier already provide ODR semantics independently.

`FlattenTarget` retains 783 explicit recursive-inlining contracts and omits the
modifier from 677 declarations. Missing `Flatten` is not inferred merely from
a containing type or neighboring method. `FlattenAudit` distinguishes leaf
declarations from composed declarations that have no separately established
recursive-inlining requirement.

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
low-level configuration probes, and the intentional legacy half of ABI or
generated-code comparisons keep their legacy spelling for their stated test or
configuration purpose. Each exception has its exact reason in `Disposition`
and `Reason`.

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
