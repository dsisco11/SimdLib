# SIMD method-flag contract

## Scope

`SIMD_FLAGS(...)` is the public declaration macro for stating the SIMD ABI and
optimization promises of an ordinary function. It is intended for both SimdLib
and downstream code.

The initial declaration form keeps the return type independent:

```cpp
Result SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) transform(Input value);
```

Every invocation starts with exactly one SIMD boundary mode: `Neither`, `In`,
`Out`, or `InOut`. It is followed by only the modifiers that apply to that
function, in the canonical order `RegisterOnly`, `ForceInline`, then `Flatten`.
The fixed order is part of the grammar rather than a formatting preference.

The macro records developer intent. The preprocessor can validate the flag
grammar, but it cannot inspect C++ parameter types, return types, function
bodies, template instantiations, or transitive callees. Correct flag selection
therefore remains a source-review responsibility.

## Boundary-mode semantics

### `Neither`

`Neither` promises that no native SIMD value, `Register`, or `RegisterMask`
crosses the function boundary by value as either an input or result.

- Pointers, references, spans, and arrays do not themselves violate `Neither`.
- An ordinary implicit `this` pointer does not violate `Neither`.
- A scalar input or result does not violate `Neither`.
- For dependent parameter or return types, every supported instantiation
  described by the declaration must satisfy the promise.

`Neither` emits no vector calling convention. It makes no memory-effect or
optimization promise; those properties remain explicit modifiers.

### `In`

`In` promises that at least one native SIMD value, `Register`, or
`RegisterMask` enters the function by value.

- A C++23 explicit-object parameter taken by value counts as an input.
- An ordinary implicit `this` pointer does not count as a by-value SIMD input.
- A pointer, reference, span, array, or scalar does not count as a SIMD input.
- For a dependent parameter type, every supported instantiation described by
  the declaration must satisfy the promise.

`In` requests the configured vector calling convention where one is supported.
It does not promise that every input remains in a physical register after
register allocation.

### `Out`

`Out` promises that the function returns a native SIMD value, `Register`, or
`RegisterMask` by value.

- Scalar, pointer, reference, span, and array returns do not satisfy `Out`.
- For a dependent or deduced return type, every supported instantiation
  described by the declaration must satisfy the promise.

`Out` requests the configured vector calling convention where one is supported.
It does not independently guarantee that a platform ABI will avoid hidden
return storage.

### `InOut`

`InOut` promises that the function satisfies both the `In` and `Out` contracts.
It describes one bidirectional SIMD call boundary and emits the configured
vector calling convention exactly once where one is supported.

`In, Out` is not an alternate spelling. A declaration that satisfies both
directions uses the single `InOut` boundary mode.

## Modifier semantics

### `RegisterOnly`

`RegisterOnly` promises that every runtime-evaluated path is authored as
register/scalar computation and does not intentionally write a value to
addressable memory.

The promise allows:

- SIMD and scalar computation;
- extraction from and insertion into SIMD registers;
- returning SIMD or scalar values;
- reading through const pointers, const references, and read-only spans;
- intrinsic loads from input memory;
- non-addressable scalar temporaries;
- calls whose relevant paths independently satisfy the same no-write contract;
- storage used exclusively by an `if consteval` branch that cannot be evaluated
  at runtime.

The promise prohibits:

- writes through pointers, references, spans, iterators, or array parameters;
- stores to globals, static storage, thread-local storage, or volatile storage;
- runtime local arrays or other explicit addressable local buffers;
- a `memcpy`, `memmove`, memory intrinsic, or library call with a destination;
- SIMD store, scatter, streaming-store, masked-store, or similar intrinsics;
- inline assembly with a memory output, memory clobber, or unreviewed memory
  side effect;
- calls that perform a prohibited write on behalf of the function;
- returning an array or another result whose authored contract requires output
  storage.

A read-only volatile access and inline assembly without a memory output require
individual review rather than automatic acceptance.

Compiler-created spills, stack frames, unwind records, instrumentation, and
hidden ABI storage do not falsify the source-level promise. They also are not
prevented by it. ABI and generated-code tests remain responsible for detecting
those effects.

On supported Microsoft C++ configurations, `RegisterOnly` may map to
`__declspec(safebuffers)` after this audit. That mapping suppresses the
function's `/GS` security-cookie instrumentation and is the reason the promise
must never be applied speculatively. An empty mapping on another compiler does
not weaken the semantic promise.

### `ForceInline`

`ForceInline` promises that optimized generated code is intended to inline the
annotated function into its caller. Its compiler mapping includes the C++
`inline` specifier needed for a header definition.

The flag is an optimization request, not a claim that every compiler,
configuration, recursion pattern, or invalid program shape can perform the
inlining. A function that only requires the C++ ODR meaning of `inline` uses the
language specifier directly and does not claim `ForceInline`.

### `Flatten`

`Flatten` promises that calls made by the annotated function are intended to be
recursively inlined where the compiler provides a flattening attribute.

`Flatten` does not request that the annotated function itself be inlined into
its caller. A declaration that requires both behaviors specifies both
`ForceInline` and `Flatten`.

## Grammar

### Accepted boundary modes, modifiers, and arity

The initial grammar accepts one boundary mode and zero to three modifiers:

```text
SIMD_FLAGS(boundary-mode [, modifier ...])

boundary-mode:
  Neither
  In
  Out
  InOut

modifier sequence:
  [RegisterOnly] [ForceInline] [Flatten]

modifier:
  RegisterOnly
  ForceInline
  Flatten
```

Four is the initial maximum argument count. Modifier omission is allowed, but
the selected modifiers remain an ordered subsequence of `RegisterOnly`,
`ForceInline`, `Flatten`.

The following rules are mandatory:

- `SIMD_FLAGS()` is invalid.
- A modifier-only invocation is invalid; use `Neither` as the boundary mode.
- More than four arguments is invalid.
- An unknown or misspelled token is invalid.
- A boundary mode in a modifier position is invalid.
- A modifier in the boundary-mode position is invalid.
- A duplicate modifier is invalid.
- A noncanonical modifier order is invalid.
- No invalid token may be silently ignored.
- No underlying attribute or calling convention may be emitted more than once.

Invalid input must fail at the declaration. Empty and over-arity invocations
use these stable diagnostic identifiers:

- `SIMDLIB_FLAGS_ERROR_EMPTY`
- `SIMDLIB_FLAGS_ERROR_TOO_MANY`

Other invalid tokens or token sequences fail through an unresolved
`SIMDLIB_DETAIL_FLAGS_BOUNDARY_...` or
`SIMDLIB_DETAIL_FLAGS_MODIFIERS_...` mapping. This deliberately avoids a
general-purpose membership parser solely to improve diagnostic spelling.

No public object-like macros named `Neither`, `In`, `Out`, `InOut`,
`RegisterOnly`, `ForceInline`, or `Flatten` may be defined to implement the
grammar.

No object-like macro with one of those exact names may be active at a
`SIMD_FLAGS(...)` invocation. Macro arguments are expanded before a variadic
forwarding layer can classify them, so such a collision makes the invocation
invalid. A function-like macro with the same name does not expand when passed
as a bare token and is not a collision.

### Canonical declaration position

`SIMD_FLAGS(...)` follows the independently specified return type and immediately
precedes the function name. The macro never selects, replaces, or deduces the
return type.

The canonical order is:

1. template head and any leading `requires` clause;
2. standard declaration attributes such as `[[nodiscard]]`;
3. `friend`, `static`, ordinary `inline`, and then `constexpr`, when
   applicable; `consteval` declarations are rejected by the initial contract;
4. independently specified return type, including `auto` when selected by the
   declaration;
5. `SIMD_FLAGS(...)`;
6. function name and parameter list;
7. member cv/ref qualifiers;
8. exception specification;
9. an independently specified trailing return type, when applicable;
10. trailing `requires` clause.

The macro emits placement-safe optimization attributes followed by the
configured vector calling convention. This order and position are required
because MSVC accepts `__vectorcall` after the return type and immediately before
the function name, but rejects it before the return type. GNU-style compilers
accept their corresponding function attributes in the same pre-name position.
An ordinary return type, a deduced `auto` return, and `auto` with an explicit
trailing return remain normal C++ syntax outside the macro.

`ForceInline` already supplies the header-definition `inline` specifier.
Ordinary `inline` is therefore omitted when `ForceInline` is present.
Declarations and out-of-line definitions repeat the same complete flag list.
Every overload is classified independently.

Compiler qualification must prove this pre-name placement before the
public macro is implemented. A compiler-specific warning suppression is not a
substitute for accepted placement.

## Canonical declaration forms

### Free function

```cpp
[[nodiscard]] constexpr
Result
SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
transform(Input lhs) noexcept;
```

### Static member

```cpp
[[nodiscard]] static constexpr
Register
SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten)
zero() noexcept;
```

### Non-static member

An implicit object does not itself satisfy `In`.

```cpp
[[nodiscard]] constexpr
Register
SIMD_FLAGS(InOut, RegisterOnly, ForceInline)
combine(Register rhs) const noexcept;
```

### Explicit-object member

A by-value explicit object satisfies `In`.

```cpp
[[nodiscard]] constexpr
Register
SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
combine(this Register lhs, Register rhs) noexcept;
```

### Operator

Operators use the same independently specified return-type form.

```cpp
[[nodiscard]] friend constexpr
Register
SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
operator+(Register lhs, Register rhs) noexcept;
```

An explicit-object operator uses the explicit-object member form rather than
adding `friend`.

### Function template

```cpp
template<class Target>
    requires RegisterTarget<Target>
[[nodiscard]] static constexpr
Target
SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
convert(native_type value) noexcept;
```

The promises apply to every supported specialization selected by the
constraints.

### Constrained trailing-return function

```cpp
template<class Target>
[[nodiscard]] static constexpr
auto
SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten)
convert(native_type value) noexcept -> Target
    requires RegisterTarget<Target>;
```

The declaration supplies both `auto` and the resolved trailing return type.
`SIMD_FLAGS(...)` supplies neither. `Out` describes the resolved return type.

### Friend function

A friend definition follows the same flag rules as a namespace function.

```cpp
[[nodiscard]] friend constexpr
Register
SIMD_FLAGS(InOut, RegisterOnly, ForceInline)
select(RegisterMask mask, Register yes, Register no) noexcept;
```

## Unsupported declaration categories

The initial `SIMD_FLAGS(...)` surface deliberately excludes categories that
lack an ordinary return type before the function name or have incompatible ABI and
optimization rules:

- constructors and destructors;
- conversion operators;
- deduction guides;
- lambdas;
- explicit function-pointer and pointer-to-member type declarations;
- virtual functions and overriding declarations;
- coroutines;
- C-style variadic functions;
- `extern "C"` declarations;
- allocation and deallocation functions;
- defaulted or deleted functions;
- immediate-only `consteval` functions.

`constexpr` functions are supported because they can also have runtime-evaluated
paths. `consteval` functions have no runtime call boundary or generated-code
contract and therefore do not use SIMD method flags.

The address of a supported flagged function may be taken. Code that needs an
explicit callback type derives it with `decltype(&function)` so the compiler's
calling-convention type is preserved instead of placing `SIMD_FLAGS(...)`
inside a pointer declarator.

Unsupported categories must not be accepted accidentally as a documented
extension. Compile-failure probes or source audits cover categories that a
preprocessor macro cannot diagnose directly.

## Downstream declarations and definitions

Downstream functions use the same declaration form as SimdLib. Repeat an
ABI-compatible flag list on the declaration and definition:

```cpp
// Transform.h
/**
 * @brief Applies a downstream register transformation.
 * @param value Input register.
 * @return Transformed register.
 */
SimdLib::Register<float, 128>
SIMD_FLAGS(InOut)
transform(SimdLib::Register<float, 128> value) noexcept;

// Transform.cpp
SimdLib::Register<float, 128>
SIMD_FLAGS(InOut)
transform(const SimdLib::Register<float, 128> value) noexcept
{
    return value + SimdLib::Register<float, 128>::broadcast(1.0F);
}
```

All translation units that declare, define, take the address of, or call the
function must agree on the vectorcall capability and token adapter. A mismatch
is an ABI disagreement; source-level type similarity does not make it safe.
Use `decltype(&transform)` when storing the function pointer so the configured
calling convention remains part of its type where the compiler models it.

`Out` selects the configured calling convention when one exists. It does not
independently force a value into physical return registers or override a
platform ABI that uses hidden return storage for an aggregate.

Apply `RegisterOnly` only after reviewing the complete runtime call graph.
Downstream authors must not use it on stores, writable spans, output pointers
or references, addressable local buffers, array-backed algorithms, or
unreviewed transitive calls. Its Microsoft mapping suppresses `/GS` for the
whole function; an incorrect promise removes a security mitigation.

## Register-only audit procedure

Every `RegisterOnly` decision is made per function and per reachable runtime
path:

1. Identify every runtime path, separating unreachable `if consteval` storage
   from runtime storage.
2. Inspect parameters and results for writable pointers, references, spans,
   arrays, iterators, aggregate return storage, and mutable proxy types.
3. Inspect locals for arrays, address-taking, explicit buffers, destination
   objects, and memory-copy destinations.
4. Inspect intrinsics and inline assembly for stores, scatters, memory outputs,
   memory clobbers, or undocumented side effects.
5. Inspect every call for transitive writes, including helpers hidden behind
   templates, overloads, and constant/runtime dispatch.
6. Confirm that valid runtime behavior consists only of input reads,
   register/scalar computation, and register/scalar return.
7. Retain generated-code and ABI review as a separate gate for compiler-created
   spills, hidden storage, security cookies, and other effects the source audit
   cannot prove.

An existing register-only declaration is preserved during mechanical migration.
If this audit contradicts that declaration, migration stops for explicit review;
the flag is not silently relaxed. A newly identified candidate is likewise
presented for review before `RegisterOnly` is added.

## Semantic flags and compiler mappings

The source contract is stable even when a compiler mapping is empty. The
initial mapping baseline is:

| Mode or modifier | Microsoft C++ | clang-cl | GNU-like Clang | GCC |
|---|---|---|---|---|
| `Neither` | no boundary token | no boundary token | no boundary token | no boundary token |
| `In`, `Out`, or `InOut` | configured `__vectorcall` on supported Windows x86 targets | configured `__vectorcall` on supported Windows x86 targets | no vector-calling-convention token | no vector-calling-convention token |
| `RegisterOnly` | `__declspec(safebuffers)` after audit | no emitted token | no emitted token | no emitted token |
| `ForceInline` | `__forceinline` | `inline __attribute__((always_inline))` | `inline __attribute__((always_inline))` | `inline __attribute__((always_inline))` |
| `Flatten` | `[[msvc::flatten]]` | `__attribute__((flatten))` | `__attribute__((flatten))` | `__attribute__((flatten))` |

These are adapter mappings, not definitions of the flags. A new compiler may
map the same promise differently. Changing a compiler mapping requires focused
syntax, ABI, and generated-code evidence; it does not require rewriting
correctly classified function declarations.

The placement-safe method-flags adapters may use a different spelling from a
legacy low-level adapter with the same semantic effect. In particular, the
C++11-style force-inline attributes are not accepted after every semantic
specifier by MSVC and clang-cl, while the keyword or GNU attribute spellings
above are accepted in the canonical declaration position without warnings.

### Compiler-adapter configuration

Each compiler property has a caller-overridable capability and token adapter:

| Property | Capability macro | Token adapter |
|---|---|---|
| vector calling convention | `SIMDLIB_METHOD_FLAGS_HAS_VECTORCALL` | `SIMDLIB_METHOD_FLAGS_VECTORCALL` |
| safe-buffer suppression | `SIMDLIB_METHOD_FLAGS_HAS_SAFE_BUFFERS` | `SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS` |
| forced inlining | `SIMDLIB_METHOD_FLAGS_HAS_FORCE_INLINE` | `SIMDLIB_METHOD_FLAGS_FORCE_INLINE` |
| recursive flattening | `SIMDLIB_METHOD_FLAGS_HAS_FLATTEN` | `SIMDLIB_METHOD_FLAGS_FLATTEN` |

A custom toolchain defines the relevant capability and token-adapter pair before
the first inclusion of `SimdLib/Config.h`. It does not redefine `SIMD_FLAGS(...)`
or any `SIMDLIB_DETAIL_...` parsing helper. A zero capability may produce an
empty adapter; `ForceInline` retains ordinary `inline` semantics when compiler
enforcement is unavailable. All translation units that exchange flagged
functions must agree on the ABI-affecting vectorcall configuration.

## Extension rule

A future boundary mode or modifier is admitted only after all of the following
are recorded:

1. one precise source-level promise;
2. valid and invalid usage categories;
3. interaction with every existing boundary mode and modifier;
4. canonical placement;
5. supported and empty compiler mappings;
6. configuration and downstream override behavior;
7. compile-pass and compile-failure coverage;
8. ABI or generated-code evidence when the flag can affect either.

Adding support for another compiler or changing an adapter follows the same
qualification path: define the semantic mapping, prove the canonical
post-return-type placement, cover default and overridden configuration, verify
cross-translation-unit ABI behavior, and retain generated-code evidence for
every affected optimization or stack-protection property. An empty mapping is
valid only when the semantic flag remains meaningful to source review and the
compiler lacks an applicable attribute.

Generic `Read` and `Write` modifiers are not part of the initial vocabulary
because they do not distinguish SIMD call direction from memory effects. `In`,
`Out`, and `InOut` describe SIMD values crossing the call boundary;
`RegisterOnly` describes the absence of authored runtime writes.
