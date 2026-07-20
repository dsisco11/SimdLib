# Api Operation and Type Matrix

This matrix records the public `SimdLib::Api` contract. Unless a cell says
otherwise, **tested** means a runtime public-API test exists at both 128 and
256 bits. **Unavailable** means the operation is intentionally constrained away
for that lane family. **Compile-time-only** identifies a contract proved only by
a compile-time probe. **Clarification needed** identifies a supported-looking
cell that cannot be classified until its intended behavior is decided.

| Public operation family | `i8` | `u8` | `i16` | `u16` | `i32` | `u32` | `i64` | `u64` | `float` | `double` |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Construction, transfer, `set1`, and lane extraction/replacement | tested | tested | tested | tested | tested | tested | tested | tested | tested | tested |
| Addition, subtraction, multiplication, and bitwise operations | tested | tested | tested | tested | tested | tested | tested | tested | tested | tested |
| Logical lane shifts | tested | tested | tested | tested | tested | tested | tested | tested | unavailable | unavailable |
| Arithmetic lane shifts | tested | unavailable | tested | unavailable | tested | unavailable | tested | unavailable | unavailable | unavailable |
| Integer divide, remainder, absolute value, minimum, and maximum | tested | tested | tested | tested | tested | tested | tested | tested | unavailable | unavailable |
| Integer comparisons and `min_position`/`max_position` | tested | tested | tested | tested | tested | tested | tested | tested | unavailable | unavailable |
| Integer conversion | unavailable | unavailable | unavailable | unavailable | tested | tested | unavailable | unavailable | unavailable | unavailable |
| Floating absolute value, comparison helpers, and element extraction | unavailable | unavailable | unavailable | unavailable | unavailable | unavailable | unavailable | unavailable | tested | tested |
| Floating `set1` and bitwise operations | unavailable | unavailable | unavailable | unavailable | unavailable | unavailable | unavailable | unavailable | tested | tested |
| `uint64_t::multiply_add_adjacent` | unavailable | unavailable | unavailable | unavailable | unavailable | unavailable | unavailable | tested | unavailable | unavailable |
| Whole-register byte shifts | 128 tested; 256 unavailable | 128 tested; 256 unavailable | 128 tested; 256 unavailable | 128 tested; 256 unavailable | 128 tested; 256 unavailable | 128 tested; 256 unavailable | 128 tested; 256 unavailable | 128 tested; 256 unavailable | unavailable | unavailable |
| `transform_pack` | tested | tested | tested | tested | tested | tested | tested | tested | unavailable | unavailable |
| Span transforms (in-place unary, separate-output unary, and binary) | same generic overload | same generic overload | same generic overload | same generic overload | same generic overload | tested | same generic overload | same generic overload | same generic overload | same generic overload |

## Backend-routing audit

The source audit found no test or shared test-support source that directly
references `SimdLib::Detail`, `Detail::Implementations`,
`Detail::Extensions`, or an implementation mapping. Existing tests route
through public `Api`, `SimdVector`, `SimdAlgo`, `SimdResample`, `Bmi`,
or `uint128_t` entry points. No direct `Detail` test is retained as a
supported test seam.

## Runtime evidence

The matrix is exercised by `tests/Api128.tests.cpp`,
`tests/Api256.tests.cpp`, and the public contract helpers in
`tests/TestSupport.h`. The focused MSVC Release and Clang coverage runs each
contain 21 SSE4.2 tests and 19 AVX2 tests; all 40 pass in both configurations.
The complete MSVC Release suite passes all 187 tests.
