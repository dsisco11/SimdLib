# SimdLib

SimdLib is a small, header-only library for working with SIMD data and bit-heavy
code without scattering compiler intrinsics throughout your project. Its core
surface remains C++20; supporting C++23 translation units can additionally use
the complete-register value interface.

There is no library binary to build or ship. Add the headers to your project,
link the CMake interface target, and use only the pieces you need.

## What is included?

- `NativeRegister<T>` is the preferred C++23 value interface for operations on
  one complete target-selected SIMD register.
- `Register<T, Bits>` selects an explicit 128-bit or 256-bit representation for
  stable storage and ABI contracts.
- `RegisterMask<T, Bits>` preserves native comparison predicates and provides
  composition, reduction, observation, and selection operations.
- `NativeApi<T>` and `Api<Bits, T>` remain supported for C++20, compatibility,
  specialized low-level access, collection helpers, and operations intentionally
  excluded from `Register`.
- `SimdVector<T, ElementCount>` wraps a register in a fixed-size, value-like container.
- `SimdAlgo` applies common operations to arrays and spans.
- `SimdResample` packs and expands byte masks, with a scalar fallback when the
  SIMD path is unavailable.
- `Bmi` collects portable and hardware-assisted bit-manipulation helpers.
- `uint128_t` provides an unsigned 128-bit value type with formatting support.

SimdLib targets Windows x64 with MSVC or clang-cl and Linux x64 with Clang or
GCC. The core requires C++20. GCC 13.2 qualifies the Linux core-only surface;
GCC 14 or newer qualifies both the core and `Register`. `Register` otherwise
requires a supported C++23 compiler with explicit-object member support.

## Add it to a project

Place SimdLib in your source tree, for example as a Git submodule, and add it
with CMake:

```cmake
add_subdirectory(external/SimdLib)
target_link_libraries(MyTarget PRIVATE SimdLib::SimdLib)
```

Link the opt-in target for a C++23 translation unit that uses `Register`:

```cmake
target_link_libraries(MyRegisterTarget PRIVATE SimdLib::Register)
```

Then include the complete public surface. The umbrella exposes `Register` only
when `SIMDLIB_REGISTER_INTERFACE_AVAILABLE` is nonzero:

```cpp
#include <SimdLib/SimdLib.h>
```

Or include a focused header such as `<SimdLib/SimdVector.h>` or
`<SimdLib/Bmi.h>` to keep dependencies explicit. Formatting support is opt-in
through `<SimdLib/Format.h>`.

## A small example

With SSE4.2 enabled, a three-component `SimdVector` can be used much like a
familiar position or math-vector class:

```cpp
#include <SimdLib/SimdVector.h>

using Vector3 = SimdLib::SimdVector<float, 3>;

Vector3 position{10.0F, 4.0F, 2.0F};
const Vector3 velocity{6.0F, -2.0F, 1.0F};
const float frameTime = 0.5F;

position += velocity * frameTime;
position.z() = 0.0F;

const Vector3 cameraOffset{-3.0F, 2.0F, 1.5F};
const Vector3 cameraPosition = position + cameraOffset;

// cameraPosition is {10.0F, 5.0F, 1.5F}
const float cameraHeight = cameraPosition.z();
```

### Operating on one complete register

Use `NativeRegister<T>` when the register width may follow the compile target:

```cpp
#include <SimdLib/SimdLib.h>

using FloatRegister = SimdLib::NativeRegister<float>;

const FloatRegister values = FloatRegister::broadcast(3.0F);
const FloatRegister scale = FloatRegister::broadcast(2.0F);
const FloatRegister offset = FloatRegister::broadcast(1.0F);
const FloatRegister transformed = values * scale + offset;
```

`NativeRegister<T>` resolves to 128 bits in an SSE4.2-only translation unit and
256 bits when AVX2 is enabled. Do not store it in an ABI or exchange it across
translation units that may use incompatible ISA or SimdLib configuration
settings. Use explicit `Register<T, Bits>` for stable storage, interfaces, and
ABI contracts.

On platforms where SimdLib enables a vector calling convention, a non-inlined
consumer function must declare `VECTORCALL` itself. The annotations on Register
members do not propagate to a surrounding function:

```cpp
using StableFloatRegister = SimdLib::Register<float, 128>;

/**
 * @brief Applies a consumer-defined complete-register transformation.
 * @param value Input register.
 * @return Transformed register.
 */
StableFloatRegister VECTORCALL add_one(StableFloatRegister value) noexcept
{
    return value + StableFloatRegister::broadcast(1.0F);
}
```

### Working with RegisterMask

Comparisons create `RegisterMask<T, Bits>` values. Masks can be combined with
`&`, `|`, `^`, and `~`; reduced with `any()`, `all()`, or `none()`; observed as
compact lane bits with `bits()` or as a by-value native predicate through the
public `native` member; and applied with `select()`:

```cpp
#include <cstdint>

using IntRegister = SimdLib::Register<std::int32_t, 128>;

const IntRegister values = IntRegister::from_lanes(-2, 0, 4, 9);
const auto positive = values.compare_greater(IntRegister::zero());
const auto not_nine = ~values.compare_equal(IntRegister::broadcast(9));
const auto selected_lanes = positive & not_nine;
const auto compact_bits = selected_lanes.bits();
const auto observed_native = selected_lanes.native;
const IntRegister selected =
    selected_lanes.select(values, IntRegister::zero());
```

Floating comparisons use the selected hardware intrinsic's ordered semantics.
A NaN lane is false for the five named comparisons, including
`compare_equal`; positive and negative zero compare equal. Predicate lanes
retain their native all-zero or all-one bit patterns.

### Transforming a collection

`Api::transform` applies a register operation across an entire span, including
a final partial register. This example converts more than one million local
terrain heights into world-space values, then clamps them to the supported
vertical range:

```cpp
#include <SimdLib/Api.h>

#include <numeric>
#include <span>
#include <vector>

using FloatApi = SimdLib::NativeApi<float>;

// Create a large set of example terrain heights.
std::vector<float> localHeights(1'000'003);
std::iota(localHeights.begin(), localHeights.end(), -500'000.0F);

// Prepare values that will be reused for every SIMD batch.
const auto heightScale = FloatApi::set1(0.02F);
const auto seaLevel = FloatApi::set1(64.0F);
const auto minimumHeight = FloatApi::set1(-500.0F);
const auto maximumHeight = FloatApi::set1(8'000.0F);

// Update the entire collection in place, one SIMD batch at a time.
FloatApi::transform(
    std::span<float>{localHeights.data(), localHeights.size()},
    [heightScale, seaLevel, minimumHeight, maximumHeight](
        const FloatApi::vector_t heights) noexcept
    {
        // Convert local heights to world heights.
        const auto scaled = FloatApi::multiply(heights, heightScale);
        const auto worldHeights = FloatApi::add(scaled, seaLevel);

        // Keep every result inside the world's allowed height range.
        const auto aboveMinimum = FloatApi::max(worldHeights, minimumHeight);
        return FloatApi::min(aboveMinimum, maximumHeight);
    });

// Each value is now clamp(localHeight * 0.02F + 64.0F, -500.0F, 8'000.0F).
```

The executable [Register example](examples/RegisterExamples.cpp) demonstrates
the preferred C++23 complete-register and mask workflows. The separate
[API example](examples/ApiExamples.cpp) demonstrates the supported C++20
facade, vectors, collection algorithms, bit helpers, `uint128_t`, resampling,
and formatting.

## MSVC stack-cookie behavior

> [!WARNING]
> [MSVC's default `/GS` heuristic](https://learn.microsoft.com/en-us/cpp/build/reference/gs-buffer-security-check?view=msvc-170)
> treats any pointer-free data structure larger than eight bytes as a
> security-sensitive buffer. Consequently, a non-inlined
> function that creates or accepts `Register<T, bits>` by value may receive a
> security-cookie prologue and epilogue even when `__vectorcall` transports the
> value entirely in SIMD registers. This is compiler-generated overhead, not a
> spill required by the `Register` representation.

SimdLib marks narrowly audited functions with `SIMDLIB_REGISTER_ONLY` when
their runtime path cannot write through pointers, references, spans, arrays,
or addressable local buffers. The macro expands to
[`__declspec(safebuffers)`](https://learn.microsoft.com/en-us/cpp/cpp/safebuffers?view=msvc-170)
on MSVC and to nothing on other compilers. It is deliberately separate from
`VECTORCALL`: stores, transforms, dynamic array-backed fallbacks, and other
memory-writing functions retain normal `/GS` protection.

The operational methods in the `Api`, `Register`, `RegisterMask`, and legacy
`SimdVector` facades use `SIMDLIB_FLATTEN` to make their transitive-inlining
intent explicit. The mapping facades do the same for paths inherited directly
by `Api`. Flattening is an optimization request rather than proof of generated
code, so the mandatory codegen gates still compare wrapper and raw-intrinsic
objects.

Consumer-defined, non-inlined functions can therefore still encounter this
MSVC behavior. Keep `/GS` enabled globally. Only after reviewing an individual
hot function and its generated code should a consumer consider applying
`__declspec(safebuffers)` to that function; the annotation disables `/GS`
protection for the entire annotated function.

The mandatory MSVC generated-code gates compare SSE4.2 and AVX2 wrapper objects
with raw-intrinsic mirrors. SSE4.2 is an optimized diagnostic profile; AVX2 is
the strict zero-overhead profile. The pure register-only AVX2 subset permits no
cookie exception. Its sole optimized exception is the exact 128-bit
`Register<double>::from_array` `/GS` sequence. The SSE4.2 diagnostic recognizes
the corresponding legacy-instruction cookie sequence so the remainder stays
comparable. Store, transfer, mutating-reference, opaque-call, and array-return
fixtures retain normal `/GS` protection and paired disassembly for review.

## Learn more

- The [wiki](wiki/Home.md) contains API documentation for every
  public method, more examples and usage guidance, supported environments,
  configuration details, formatting, and development commands.
- [Public namespace and compatibility](docs/PublicNamespace.md) describes the
  supported API boundary.
- [Validation record](docs/Validation.md) documents the compiler, sanitizer,
  consumer, and test evidence.

## License

SimdLib is available under the [MIT License](LICENSE).
