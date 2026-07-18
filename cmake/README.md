# CMake support

The root build defines only the header-only `SimdLib` interface target as a
consumer requirement. Test, benchmark, example, warning-policy, and compiler
probe targets are development-only.

The scripts in this directory compare portable and optimized deterministic
result sets. `tests/consumer` is a standalone CMake project that imports the
root project through `add_subdirectory` and asserts that the imported target is
an `INTERFACE_LIBRARY`.
