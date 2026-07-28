# SIMD method-flag parser evaluation

## Decision

The `SIMD_FLAGS(...)` prototype uses a fixed-vocabulary membership scan with
arity-specific validation. This is the smallest evaluated design that satisfies
all of the frozen grammar:

- one to five flags;
- arbitrary flag order;
- unknown-token rejection;
- duplicate rejection;
- one coalesced calling-convention emission for `In`, `Out`, or both;
- canonical property emission order;
- no object-like definitions for the short flag tokens;
- no preprocessing dependency.

The prototype remains isolated in
`tests/method_flags/MethodFlagsPrototype.h`. Moving the selected machinery into
the public configuration boundary belongs to the public-macro implementation
work.

## Selected design

The parser performs four bounded operations:

1. Classify invocation arity. A zero-token invocation is represented by the
   preprocessor's single empty argument and diagnosed separately; arities above
   five select the over-arity diagnostic.
2. Validate every supplied token against the five-token vocabulary.
3. Compare every supplied token pair and reject a duplicate.
4. Scan the valid set once for each emitted property and emit properties in the
   fixed order: vector calling convention, register-only mapping, force-inline,
   then flatten.

`In` and `Out` are separate membership predicates. Their Boolean union controls
one calling-convention emission, so the parser cannot emit duplicate
`__vectorcall` tokens.

For five supplied flags, the bounded work is five validity probes, ten
pair-equality probes, and four five-element property folds. This is fixed
preprocessing work rather than a combinatorial set of declaration mappings.

All implementation helpers use the `SIMDLIB_DETAIL_FLAGS_` prefix. The only
short public macro produced by the prototype is `SIMD_FLAGS`.

## MSVC preprocessing behavior

The design does not require `/Zc:preprocessor`.

MSVC's legacy preprocessor does not automatically rescan commas introduced by
an expanded probe macro or a forwarded variadic arity list. The prototype uses
parenthesized tuple-rescan helpers for both operations. The same helpers are
accepted by conforming MSVC, clang-cl, GNU-like Clang, and GCC preprocessors, so
there is no compiler-specific parser branch.

Boolean folds use complete `00`, `01`, `10`, and `11` value tables rather than
returning an unevaluated macro argument from a short-circuit helper. This avoids
another legacy-MSVC rescan ambiguity while preserving the same Boolean result.

## Evaluated alternatives

| Design | Benefit | Rejection reason |
|---|---|---|
| Direct variadic `FOR_EACH` | Smallest token emitter | Emits in caller order, cannot naturally reject duplicates, and emits the shared calling convention twice for `In, Out`. |
| Normalized flag list | Could map one canonical sequence | Sorting arbitrary identifiers in the C preprocessor requires substantially more machinery and makes unknown-token diagnostics indirect. |
| Numeric bit mask | Compact membership representation | Requires public object-like flag macros or a second syntax, and a numeric preprocessor result cannot conditionally emit declaration tokens without another dispatch layer. |
| Named bundles | Very small implementation | Replaces the requested composable promise vocabulary with a growing set of combinations and obscures individual intent. |
| Power-set mapping | Simple expansion after exact match | Requires at least 31 subset mappings before accounting for input order; accepting all permutations grows to 325 mappings. |

The power-set design is specifically rejected. The selected scanner tests the
same 325 ordered, nonempty permutations with one bounded implementation.

### Normalized-list implementation comparison

The normalized-list spike was decomposed into the concrete preprocessing
stages it requires:

1. perform the same arity, validity, and duplicate checks as the selected
   scanner;
2. test membership for each of the five vocabulary tokens;
3. construct a new comma-separated list in canonical order while handling
   every empty/nonempty boundary between optional tokens;
4. count and dispatch that generated list again;
5. run a direct emitter over the normalized list.

The first two stages are the selected membership scanner. The remaining stages
add list construction, comma management, and a second dispatch without removing
any validation or property test. Retaining that implementation would therefore
be strictly larger than emitting the four canonical properties directly from
the membership results. It was rejected before duplicating the shared scanner
into a second permanent prototype header.

The direct `FOR_EACH` emitter is the only materially smaller implementation
found. It fails the frozen behavior because `In, Out` emits the shared calling
convention twice, caller order becomes output order, and duplicate rejection
requires adding the membership machinery back. Named bundles and a numeric bit
mask are smaller only by changing the accepted public grammar.

## Collision evaluation

The parser does not define `In`, `Out`, `RegisterOnly`, `ForceInline`, or
`Flatten`. Function-like macros with one of those names do not expand when the
bare token is supplied as a flag and therefore do not conflict.

An object-like macro with one of the five exact names is an unavoidable
collision at the invocation site. The C preprocessor expands an object-like
macro argument before a variadic forwarding layer can classify it. The result
is rejected as an unknown flag rather than silently acquiring another meaning.

Downstream code must therefore ensure that no object-like macro with an exact
flag spelling is active where `SIMD_FLAGS(...)` is invoked. This restriction is
preferable to globally defining the short names, adopting longer prefixed flag
tokens, or changing the accepted call syntax. It must appear in the eventual
public macro documentation.

## Verification fixture

`cmake/VerifyMethodFlagsPreprocessor.cmake` generates a preprocessing-only
translation unit in the build tree. It covers:

- all 325 ordered permutations without repeated flags;
- all 31 nonempty flag subsets as a consequence of that permutation set;
- one function-like macro collision case;
- exact canonical output-token comparison for every case;
- absence of leaked short flag macros;
- absence of prototype header dependencies;
- rejection of any non-`SIMDLIB_DETAIL_` helper definition;
- focused failures for empty, unknown, duplicate, over-arity, and object-like
  collision inputs.

The verifier invokes the configured compiler directly in preprocessing mode and
then invokes its syntax checker for the negative fixtures. It is registered as
the `MethodFlagsPreprocessor` CTest entry when configuration probes are enabled.

The focused command for a configured build tree is:

```text
ctest --test-dir <build-directory> -R ^MethodFlagsPreprocessor$ --output-on-failure
```

The same CMake verifier can be called directly with a compiler path, driver
style, source directory, and writable binary directory. This keeps the Linux
container checks identical to the native Windows checks.
