# SIMD method-flag parser evaluation

## Decision

`SIMD_FLAGS(...)` uses a fixed-position grammar:

```cpp
SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
```

The first argument is exactly one boundary mode: `Neither`, `In`, `Out`, or
`InOut`. Zero to three modifiers follow as an ordered subsequence of
`RegisterOnly`, `ForceInline`, `Flatten`.

This grammar replaces the rejected unordered five-token set and removes the
need for membership scans, pairwise duplicate comparisons, Boolean folds,
canonical sorting, and special coalescing of separate `In` and `Out` flags.

## Selected design

The dependency-free prototype in
`tests/method_flags/MethodFlagsPrototype.h` consists of:

1. four boundary-mode mappings;
2. eight canonical modifier-subset mappings;
3. arity dispatch for one through four arguments;
4. one over-arity path;
5. the small expansion indirection required by MSVC's traditional
   preprocessor.

`Neither` maps to no calling-convention token. `In`, `Out`, and `InOut` each map
to exactly one calling-convention adapter.

Canonical modifier mappings are defined directly:

```text
none
RegisterOnly
ForceInline
Flatten
RegisterOnly, ForceInline
RegisterOnly, Flatten
ForceInline, Flatten
RegisterOnly, ForceInline, Flatten
```

An unknown boundary mode leaves an unresolved
`SIMDLIB_DETAIL_FLAGS_BOUNDARY_...` token. An unknown, duplicate, or
noncanonical modifier sequence leaves an unresolved
`SIMDLIB_DETAIL_FLAGS_MODIFIERS_...` token. Compilation therefore fails at the
declaration without a general-purpose token classifier.

Empty and over-arity invocations retain explicit
`SIMDLIB_FLAGS_ERROR_EMPTY` and `SIMDLIB_FLAGS_ERROR_TOO_MANY` diagnostic
identifiers.

## Complexity comparison

| Measure | Rejected unordered prototype | Fixed-position prototype |
|---|---:|---:|
| Header size | 19,151 bytes | 4,011 bytes |
| Macro definitions | 114 | 37 |
| Valid canonical invocation forms | 325 ordered permutations | 32 boundary-and-modifier forms |

The replacement removes 15,140 bytes and 77 macro definitions from the
prototype. The eight modifier mappings represent the complete three-modifier
grammar rather than a power set that grows with arbitrary input order.

## MSVC preprocessing behavior

The design supports both MSVC preprocessors without requiring a
compiler-specific parser branch.

MSVC's traditional preprocessor does not consistently rescan a forwarded
variadic arity result before token pasting. The prototype retains two bounded
compatibility helpers:

- a parenthesized tuple rescan for the arity list;
- two-step token concatenation before selecting the arity handler.

No probe-generated commas, Boolean tables, short-circuit folds, or pairwise
token comparisons remain. The same helpers are accepted by conforming MSVC,
clang-cl, GNU-like Clang, and GCC.

SimdLib can enable `/Zc:preprocessor` in its own MSVC builds while retaining
this small compatibility path for downstream projects that use MSVC's default
traditional preprocessor.

## Collision evaluation

The implementation does not define object-like macros named `Neither`, `In`,
`Out`, `InOut`, `RegisterOnly`, `ForceInline`, or `Flatten`. A function-like
macro with one of those names is not invoked when its bare name is supplied and
does not conflict.

An active object-like macro with one of those exact names expands before the
variadic forwarding layer can dispatch it. The invocation then fails through
the expanded boundary or modifier mapping. This is an unavoidable restriction
of the chosen bare-token call syntax and must be included in the eventual
public documentation.

`Neither` was selected instead of the more collision-prone `None` spelling.
The general object-like macro restriction still applies to every boundary mode
and modifier token.

## Verification fixture

`cmake/VerifyMethodFlagsPreprocessor.cmake` generates a preprocessing-only
translation unit in the build tree. It covers:

- four boundary modes combined with all eight modifier subsets;
- exact declaration-token comparison for all 32 canonical invocations;
- one function-like macro collision case;
- absence of leaked short flag macros;
- absence of prototype header dependencies;
- rejection of any non-`SIMDLIB_DETAIL_` helper definition;
- focused invalid cases for empty input, an unknown modifier, a duplicate
  modifier, over-arity input, an object-like collision, a missing boundary
  mode, and noncanonical modifier order.

For every invalid case, the verifier first checks the preprocessed failure
token and then requires syntax compilation to fail. This avoids depending on
compiler-specific diagnostic prose.

The verifier accepts an optional focused compiler-option list, allowing the
same script to exercise traditional MSVC and `/Zc:preprocessor` explicitly.
It is also registered as the `MethodFlagsPreprocessor` CTest entry when
configuration probes are enabled.

The focused command for a configured build tree is:

```text
ctest --test-dir <build-directory> -R ^MethodFlagsPreprocessor$ --output-on-failure
```
