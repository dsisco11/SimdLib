# Semantic Function Flags Proposal

Status: proposed public declaration contract.

## Summary

SimdLib should provide one public `SIMDLIB_FLAGS(...)` macro for declaring the
SIMD-related behavioral promises made by a function. SimdLib and downstream
projects would name those promises instead of spelling compiler attributes and
calling conventions individually.

The proposed flags are not cosmetic aliases. They are developer assertions
about the function's signature and implementation. SimdLib translates the
assertions into the calling convention, stack-protection override, and inlining
attributes supported by the active compiler.

```cpp
[[nodiscard]]
SimdLib::Register<float, 256>
SIMDLIB_FLAGS(In, Out, RegisterOnly, ForceInline, Flatten)
add(
	SimdLib::Register<float, 256> lhs,
	SimdLib::Register<float, 256> rhs) noexcept;
```

The macro belongs immediately before the function name. That position is
required because MSVC and Windows-targeting Clang place `__vectorcall` between
the return type and the function declarator. The compiler-specific attribute
spellings selected by SimdLib must therefore also be valid in that position.

## Motivation

Register-oriented functions currently repeat independent declarations such as:

```cpp
[[nodiscard]]
SIMDLIB_FLATTEN
SIMDLIB_FORCE_INLINE
SIMDLIB_REGISTER_ONLY
Register VECTORCALL add(Register lhs, Register rhs) noexcept;
```

This exposes compiler mechanics at every call boundary and asks downstream
authors to understand several independent rules:

- Windows register arguments and results require `VECTORCALL` at surviving
  function boundaries.
- A function audited as unable to write addressable storage may suppress stack
  protection that would otherwise be emitted by a compiler heuristic.
- Force-inline and flatten control different directions of inlining.
- Memory-writing paths must retain normal stack protection.
- Calling-convention declarations must match across translation units.

The repeated spelling also permits internally inconsistent declarations. A
function may return a register but omit `VECTORCALL`, or may receive
`SIMDLIB_REGISTER_ONLY` without an explicit source-level promise explaining why
the security override is safe.

`SIMDLIB_FLAGS(...)` makes the semantic contract the public surface and leaves
compiler selection to SimdLib:

```cpp
Register
SIMDLIB_FLAGS(In, Out, RegisterOnly, ForceInline, Flatten)
add(Register lhs, Register rhs) noexcept;
```

## Goals

- Give SimdLib and downstream projects one concise declaration system for
  register-oriented functions.
- Express developer intent rather than compiler-specific syntax.
- Derive `__vectorcall` once when either register input or register output
  requires it.
- Emit attributes in a compiler-tested canonical order regardless of flag
  order.
- Preserve stack protection on functions that write addressable storage.
- Support free functions, static members, ordinary members, templates,
  operators, and C++23 explicit-object members.
- Preserve direct register argument and result boundaries where the platform
  ABI supports them.
- Keep language contracts such as `constexpr`, `noexcept`, and `requires`
  visible in ordinary C++.
- Allow downstream compiler support to improve without rewriting downstream
  function declarations.

## Non-goals

- Inspecting a function signature or body to prove that its flags are true.
- Guaranteeing that a compiler never spills a register or creates a stack
  frame.
- Replacing `constexpr`, `consteval`, `static`, `noexcept`, `requires`, or
  explicit alignment declarations.
- Encoding parameter-specific alignment, aliasing, or access bounds.
- Enabling runtime CPU dispatch or changing instruction-family availability.
- Making arbitrary aggregates register-passable merely by adding `In` or
  `Out`.
- Applying a calling convention to variadic functions.
- Hiding standard API contracts such as `[[nodiscard]]` inside an attribute
  bundle whose required declarator position cannot represent them portably.

## Public spelling

The proposed exported spelling is:

```cpp
SIMDLIB_FLAGS(flag, ...)
```

The `SIMDLIB_` prefix is retained because macros occupy the global preprocessor
namespace even when included through `SimdLib`. `SIMD_FLAGS` is shorter but is
too broad for a public header and is more likely to collide with another SIMD
library or application macro.

At least one flag is required. A function with no relevant promise omits the
macro. Flag order does not affect the generated declaration, and repeated
capabilities are emitted only once.

## Initial flag vocabulary

| Flag | Developer promise | Derived capability |
| --- | --- | --- |
| `In` | At least one native SIMD value or supported SIMD carrier is accepted by value. | Request the supported vector calling convention. |
| `Out` | A native SIMD value or supported SIMD carrier is returned by value. | Request the supported vector calling convention. |
| `RegisterOnly` | The runtime path does not perform programmer-directed writes to addressable storage. | Suppress the function's stack protector where the compiler provides a qualified per-function override. |
| `ForceInline` | The function definition is intended to be incorporated into each eligible caller. | Apply the supported always-inline declaration and the C++ `inline` property. |
| `Flatten` | Eligible calls made from the function are intended to be incorporated into the function. | Apply the supported flatten declaration. |

The flags are orthogonal:

- `In` and `Out` both derive the vector calling convention, but the convention
  is emitted only once.
- `Out` does not imply `RegisterOnly`; a function may return a register and
  also write to memory.
- `Out` does not imply `[[nodiscard]]`.
- `RegisterOnly` does not imply `In` or `Out`; a scalar reduction or helper may
  satisfy the same storage restriction.
- `ForceInline` does not imply `Flatten`.
- `Flatten` does not require the containing function itself to be inlined into
  its caller.

### `In`

`In` applies when a function accepts at least one by-value value whose ABI is
intended to use a SIMD register:

- A native intrinsic vector such as `__m128`, `__m256`, or the corresponding
  integer and double forms.
- `Register<T, Bits>`.
- `RegisterMask<T, Bits>`.
- Another explicitly qualified aggregate or homogeneous vector aggregate used
  as a SIMD carrier by downstream code.

A pointer or reference to one of these values does not by itself satisfy `In`;
the ABI passes the pointer or reference rather than the contained register.
`In` also does not promise that every argument remains in a register. Register
availability, argument count, ABI classification, and register pressure may
still require memory.

### `Out`

`Out` applies when the function returns a native SIMD value or supported SIMD
carrier by value. On supported Windows x64 boundaries, the derived
`__vectorcall` declaration allows qualifying vector and aggregate results to be
returned through XMM or YMM registers instead of platform-default hidden return
storage.

`Out` does not claim that any arbitrary class becomes a vector result. The
returned type must independently satisfy the compiler ABI's vector or
homogeneous-vector-aggregate rules. SimdLib's `Register` and `RegisterMask`
qualification remains responsible for proving their supported boundaries.

### `RegisterOnly`

`RegisterOnly` is preferred over `NoStack`. No source annotation can promise
that optimization, register pressure, debugging, instrumentation, or ABI
requirements will never create a stack frame or compiler-generated spill.

The `RegisterOnly` promise permits:

- Reading from const pointers, references, spans, or other input storage.
- Producing native vector, register-wrapper, mask, and scalar results.
- Scalar temporaries that remain ordinary compiler values.
- Compiler-generated spills and reloads.
- Calls to intrinsics or functions whose relevant runtime paths satisfy the
  same contract.

The promise prohibits:

- Writing through pointers, references, spans, iterators, or output objects.
- Mutating an explicit object through an addressable reference.
- Storing a vector into a local or caller-provided array as an implementation
  technique.
- Using `memcpy` or equivalent staging to materialize an addressable vector
  buffer.
- Creating addressable local buffers whose presence makes the runtime function
  eligible for stack-buffer protection.
- Calling a helper whose inlined runtime path violates these restrictions.

Compile-time-only array or byte manipulation should remain isolated in a
dedicated constant-evaluation helper. The attributed runtime-facing function
must not directly contain storage constructs that can affect its generated
runtime body.

An incorrect `RegisterOnly` promise removes a security mitigation. It therefore
requires individual source and generated-code review; it must never be added by
bulk inference from a return type or method name.

### `ForceInline`

`ForceInline` requests that the attributed function be inlined into eligible
callers. The definition must be visible where inlining is required. A
declaration in a public header followed by an unavailable definition in another
translation unit cannot create an ordinary non-LTO force-inline guarantee.

The compiler may still reject or diagnose an impossible request. The flag does
not relax semantic correctness, target-feature, recursion, or unavailable-body
constraints.

### `Flatten`

`Flatten` requests recursive inlining of eligible calls made by the attributed
function. It does not override an unavailable definition, a `noinline`
contract, recursion, or another compiler restriction.

`Flatten` remains distinct from `ForceInline`:

```text
caller -> function -> helper
          ^             ^
          |             |
     ForceInline     Flatten
```

## Declaration grammar

For a function with an ordinary return type, the macro appears after the return
type and immediately before the function name:

```cpp
[[nodiscard]]
static constexpr Register
SIMDLIB_FLAGS(Out, RegisterOnly, ForceInline, Flatten)
zero() noexcept;
```

```cpp
[[nodiscard]]
Register
SIMDLIB_FLAGS(In, Out, RegisterOnly, ForceInline, Flatten)
operator+(this Register lhs, Register rhs) noexcept;
```

```cpp
void
SIMDLIB_FLAGS(In, ForceInline, Flatten)
store(Register value, std::span<float> destination) noexcept;
```

This placement intentionally differs from the existing prefix placement of
`SIMDLIB_FORCE_INLINE` and `SIMDLIB_FLATTEN`. MSVC rejects `__vectorcall` before
the return type. MSVC and clang-cl accept the shared pre-name location only
when SimdLib selects attribute spellings valid after the return type.

Conversion operators, constructors, destructors, deduction guides, trailing
return types, function-pointer declarations, and other declarations without a
conventional return-type/name boundary require explicit syntax probes before
they enter the supported surface. The initial migration must not assume that a
spelling validated for an ordinary function is valid for every declarator
grammar.

## Representative contracts

### Register arithmetic

```cpp
[[nodiscard]]
Register
SIMDLIB_FLAGS(In, Out, RegisterOnly, ForceInline, Flatten)
add(Register lhs, Register rhs) noexcept;
```

The function accepts and returns register carriers, performs no addressable
write, and requests both directions of inlining.

### Read-only load

```cpp
[[nodiscard]]
Register
SIMDLIB_FLAGS(Out, RegisterOnly, ForceInline, Flatten)
load(std::span<const float, lane_count> source) noexcept;
```

Reading memory does not violate `RegisterOnly`. The absence of a by-value SIMD
argument means `In` is unnecessary; `Out` still derives the Windows vector
calling convention.

### Memory store

```cpp
void
SIMDLIB_FLAGS(In, ForceInline, Flatten)
store(Register value, std::span<float, lane_count> destination) noexcept;
```

The function accepts a register carrier and writes addressable storage.
`RegisterOnly` is intentionally absent, so normal stack protection remains
available.

### Scalar reduction

```cpp
[[nodiscard]]
bool
SIMDLIB_FLAGS(In, RegisterOnly, ForceInline, Flatten)
any(RegisterMask<float, 256> value) noexcept;
```

`In` derives the calling convention. `Out` is absent because the result is an
ordinary scalar.

### Non-inlined consumer boundary

```cpp
[[nodiscard]]
SimdLib::Register<float, 256>
SIMDLIB_FLAGS(In, Out, RegisterOnly)
transform_register(SimdLib::Register<float, 256> value) noexcept;
```

The ABI and storage promises remain useful even when inlining is deliberately
not requested.

## Compiler mapping

The reducer emits properties in this conceptual order:

1. Flatten.
2. Force-inline and C++ inline semantics.
3. Register-only stack-protection override.
4. Vector calling convention.

The exact tokens are compiler-specific and must be valid immediately before the
function name.

| Compiler and target | `In` or `Out` | `RegisterOnly` | `ForceInline` | `Flatten` |
| --- | --- | --- | --- | --- |
| MSVC x64 | `__vectorcall` | `__declspec(safebuffers)` | `__forceinline` | `[[msvc::flatten]]` |
| Clang using the Windows MSVC ABI | `__vectorcall` | `__declspec(safebuffers)` | `__attribute__((always_inline)) inline` | `__attribute__((flatten))` |
| GCC x64 Linux | Empty; use the platform ABI | `__attribute__((no_stack_protector))` | `__attribute__((always_inline)) inline` | `__attribute__((flatten))` |
| Clang x64 Linux | Empty; use the platform ABI | `__attribute__((no_stack_protector))` | `__attribute__((always_inline)) inline` | `__attribute__((flatten))` |

The Linux `RegisterOnly` mapping is part of the proposed complete contract, not
an assumption that every register-only function would otherwise receive a
stack protector. Qualification must compile annotated production paths with
stack protection enabled and must retain unannotated audit mirrors where needed
to detect accidental addressable-buffer implementations.

An unsupported compiler may provide approved leaf overrides. Without a
qualified calling-convention mapping, `In` and `Out` do not create a
register-boundary guarantee merely because the source declaration compiles.

## Preprocessor design

The C++ type system cannot apply a calling convention or declaration attribute
after inspecting a parameter pack of enum values. The flag system must
therefore be implemented by the preprocessor.

Each public flag maps to a private descriptor:

```cpp
// (vector_call, register_only, force_inline, flatten)
#define SIMDLIB_DETAIL_FLAG_In           (1, 0, 0, 0)
#define SIMDLIB_DETAIL_FLAG_Out          (1, 0, 0, 0)
#define SIMDLIB_DETAIL_FLAG_RegisterOnly (0, 1, 0, 0)
#define SIMDLIB_DETAIL_FLAG_ForceInline  (0, 0, 1, 0)
#define SIMDLIB_DETAIL_FLAG_Flatten      (0, 0, 0, 1)
```

A bounded reducer:

1. Counts between one and eight arguments.
2. Resolves every token to its descriptor.
3. ORs each descriptor column independently.
4. Checks defined incompatibilities.
5. Emits every derived capability once in canonical order.

For example:

```cpp
SIMDLIB_FLAGS(In, Out, RegisterOnly, ForceInline, Flatten)
```

reduces to:

```text
vector_call   = 1
register_only = 1
force_inline  = 1
flatten       = 1
```

`In` and `Out` therefore request the calling convention independently without
duplicating `__vectorcall`.

The initial implementation should use fixed-arity reducers rather than
recursive `__VA_OPT__` machinery. SimdLib headers must remain usable under the
supported MSVC preprocessing modes without requiring a downstream project to
enable a new preprocessor option.

Unknown flag names must produce a stable diagnostic containing the unknown
token. Future contradictory flags must produce focused diagnostics rather than
emitting conflicting compiler attributes.

## Header and customization boundary

The public macro and flag descriptors should live in a focused
`<SimdLib/FunctionFlags.h>` header. That header may include `Config.h` for
compiler and target detection. Headers declaring flagged functions include the
focused header directly; the umbrella header also exposes it.

Existing compiler leaves should adopt spellings that are valid both before a
return type and immediately before a function name:

- MSVC force-inline should use `__forceinline`.
- Clang and GCC force-inline should use
  `__attribute__((always_inline)) inline`.
- Clang and GCC flatten should use `__attribute__((flatten))`.

Downstream code should use only `SIMDLIB_FLAGS(...)`. Leaf overrides remain an
advanced toolchain-adaptation boundary and must satisfy the documented
pre-name placement contract. Ordinary downstream code must not assemble the
leaf macros manually.

Because `In` and `Out` affect ABI, every declaration visible to a caller and
every separately compiled definition must use a consistent flag contract.
Projects must not compile linked translation units with contradictory
`SIMDLIB_VECTORCALL_ENABLED` or leaf overrides.

## Safety and correctness consequences

The compiler cannot verify these developer promises:

- An incorrect `In` or `Out` declaration can produce an ABI mismatch between
  callers and callees.
- An incorrect `RegisterOnly` declaration can remove stack-buffer protection
  from code that needs it.
- An incorrect purity-like future flag could permit optimizer transformations
  that change observable behavior.
- Force-inline and flatten may substantially increase generated code size.

The proposal therefore treats flags similarly to `noexcept`, `restrict`,
alignment assumptions, and intrinsic preconditions: concise and useful, but
requiring precise documentation and qualification.

`RegisterOnly` must be reviewed per function. Neither a native vector return
type nor the absence of an obvious store operation is sufficient evidence.
Every runtime branch and eligible inlined callee belongs to the audit.

## Deferred flags

The initial surface should remain limited to promises already required by
SimdLib's register abstractions.

Potential later additions include:

| Candidate | Reason to defer |
| --- | --- |
| `NoInline` | Requires conflict diagnostics with `ForceInline` and deliberate interaction rules with `Flatten`. |
| `Hot` and `Cold` | Compiler support and code-layout effects require separate qualification. |
| `Pure` | An incorrect promise may cause miscompilation; MSVC, Clang, and GCC do not expose identical semantics. |
| `NoReturn` | Standard `[[noreturn]]` is already clear and occupies a different portable attribute position. |
| `NoDiscard` | Standard `[[nodiscard]]` should remain a visible API contract before the return type. |
| `NoThrow` | C++ `noexcept` is clearer, stronger, and belongs after the declarator. |
| `Read` and `Write` | Bare function flags cannot express parameter index, extent, aliasing, or read/write mode precisely enough for compiler access attributes. |
| `Aligned` and `NoAlias` | These are parameter- or result-specific promises rather than whole-function SIMD transport properties. |

Adding a flag requires a documented semantic contract, mappings for every
supported compiler, conflict rules, negative tests, and generated-code or ABI
evidence appropriate to its effect.

## Validation requirements

The declaration system is qualified only when the following categories are
covered.

### Preprocessor behavior

- Every individual flag.
- Every supported arity.
- Different flag orders producing the same canonical expansion.
- `In`, `Out`, and `In` plus `Out` emitting one calling convention.
- Duplicate capabilities remaining idempotent.
- Unknown flags producing a stable failure marker.
- Every defined contradictory pair producing a focused diagnostic.
- Caller overrides preserving the required declaration position.

### Declaration grammar

- Free functions.
- Static and ordinary member functions.
- Function templates and constrained templates.
- Operators.
- C++23 explicit-object members.
- Native vector and register-wrapper parameters and results.
- Aligned aggregate and homogeneous-vector-aggregate carriers.
- Separate declarations and definitions.
- Function pointers and callable aliases where the grammar permits the public
  macro.
- Explicit rejection or separate syntax for unsupported declarator forms.

### ABI

- MSVC and clang-cl `In`, `Out`, and combined non-inlined boundaries.
- Native-vector versus `Register` and `RegisterMask` mirrors.
- Aggregate results checked for hidden return storage.
- Name decoration and function-pointer type compatibility.
- Default-convention diagnostic mirrors retained separately.
- GCC and Clang System V argument and result mirrors.

### Generated code

- Force-inline functions compared with direct intrinsic expressions.
- Flattened call chains checked for remaining helper calls.
- Register-only paths compiled with `/GS` or
  `-fstack-protector-strong` enabled.
- Memory-writing paths checked to retain normal protection eligibility.
- Annotated and unannotated audit mirrors used where suppression would
  otherwise hide a storage regression.
- 128-bit and 256-bit register widths.
- Representative floating, signed-integer, and unsigned-integer types.
- Optimized and diagnostic configurations where their purposes differ.

### Downstream consumption

- A separate consumer target including only public headers.
- Header-defined force-inline functions.
- Separately compiled ABI boundaries without force-inline.
- Consistent declarations across multiple translation units.
- Consumer functions using native vectors, `Register`, and `RegisterMask`.
- An override probe for a supported alternate toolchain mapping.

## Migration strategy

Migration must classify functions individually rather than replacing text
mechanically.

1. Add the focused public header, descriptor reducer, compiler leaves, and
   configuration probes.
2. Qualify the declaration position and compiler spellings before changing
   production declarations.
3. Inventory every existing use of `VECTORCALL`, `SIMDLIB_REGISTER_ONLY`,
   `SIMDLIB_FORCE_INLINE`, and `SIMDLIB_FLATTEN`.
4. Record `In`, `Out`, `RegisterOnly`, `ForceInline`, and `Flatten`
   independently for each function.
5. Review every proposed `RegisterOnly` assignment with its complete runtime
   call path.
6. Migrate `Api`, implementation, `Register`, and `RegisterMask` declarations
   in reviewable groups.
7. Migrate other algorithms only after their own input, output, storage, and
   inlining contracts are established.
8. Add downstream examples that use only `SIMDLIB_FLAGS(...)`.
9. Remove direct leaf-macro use from ordinary public documentation.
10. Retain leaf macros only as documented advanced compiler-adaptation hooks.

No compatibility alias for `SIMD_FLAGS` is proposed. SimdLib has not published
a stable release, and introducing two public spellings would create permanent
global macro surface without a compatibility requirement.

## Acceptance criteria

The proposal is ready for implementation when:

- The public spelling and initial five contracts are approved.
- The `RegisterOnly` compiler mappings are approved.
- The pre-name declaration grammar is accepted for downstream use.
- Every supported compiler has a position-compatible leaf spelling.
- The bounded reducer design has a defined maximum arity and diagnostic
  strategy.
- The validation requirements cover every ABI- or security-affecting emitted
  property.
- The migration inventory requires individual review of every
  `RegisterOnly` assignment.
