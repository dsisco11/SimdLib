# SimdLib

SimdLib is a small, header-only C++20 library for working with SIMD data and
bit-heavy code without scattering compiler intrinsics throughout your project.
It brings register operations, fixed-size vectors, bulk algorithms, bit
manipulation helpers, and a practical 128-bit integer under one consistent API.

There is no library binary to build or ship. Add the headers to your project,
link the CMake interface target, and use only the pieces you need.

## What is included?

- `Api` provides a typed facade over 128-bit and 256-bit SIMD registers.
- `SimdVector` wraps a register in a fixed-size, value-like container.
- `SimdAlgo` applies common operations to arrays and spans.
- `SimdResample` packs and expands byte masks, with a scalar fallback when the
  SIMD path is unavailable.
- `Bmi` collects portable and hardware-assisted bit-manipulation helpers.
- `uint128_t` provides an unsigned 128-bit value type with formatting support.

SimdLib is currently aimed at x86 and x64 projects and is tested with MSVC,
clang-cl, Clang, and GCC. It requires C++20.

## Add it to a project

Place SimdLib in your source tree, for example as a Git submodule, and add it
with CMake:

```cmake
add_subdirectory(external/SimdLib)
target_link_libraries(MyTarget PRIVATE SimdLib::SimdLib)
```

Then include the complete public surface:

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

using Float4 = SimdLib::Api<128, float>;

// Create a large set of example terrain heights.
std::vector<float> localHeights(1'000'003);
std::iota(localHeights.begin(), localHeights.end(), -500'000.0F);

// Prepare values that will be reused for every group of four heights.
const auto heightScale = Float4::set1(0.02F);
const auto seaLevel = Float4::set1(64.0F);
const auto minimumHeight = Float4::set1(-500.0F);
const auto maximumHeight = Float4::set1(8'000.0F);

// Update the entire collection in place, four heights at a time.
Float4::transform(
    std::span<float>{localHeights.data(), localHeights.size()},
    [heightScale, seaLevel, minimumHeight, maximumHeight](
        const Float4::vector_t heights) noexcept
    {
        // Convert local heights to world heights.
        const auto scaled = Float4::multiply(heights, heightScale);
        const auto worldHeights = Float4::add(scaled, seaLevel);

        // Keep every result inside the world's allowed height range.
        const auto aboveMinimum = Float4::max(worldHeights, minimumHeight);
        return Float4::min(aboveMinimum, maximumHeight);
    });

// Each value is now clamp(localHeight * 0.02F + 64.0F, -500.0F, 8'000.0F).
```

The executable [API example](examples/ApiExamples.cpp) shows the register
facade, vectors, algorithms, bit helpers, `uint128_t`, resampling, and
formatting together in one short program.

## Learn more

- The [wiki](wiki/Technical-Reference.md) contains API documentation for every
  public method, more examples and usage guidance, supported environments,
  configuration details, formatting, and development commands.
- [Public namespace and compatibility](docs/PublicNamespace.md) describes the
  supported API boundary.
- [Validation record](docs/Validation.md) documents the compiler, sanitizer,
  consumer, and test evidence.

## License

SimdLib is available under the [MIT License](LICENSE).
