#include <SimdLib/Config.h>

#include <immintrin.h>

namespace SimdLibMethodFlagsContract
{
/// Declares the Neither boundary with no modifiers.
[[nodiscard]] int SIMD_FLAGS(Neither) contract_neither_plain(int value) noexcept;

/// Declares the Neither boundary with RegisterOnly.
[[nodiscard]] int SIMD_FLAGS(Neither, RegisterOnly) contract_neither_registeronly(int value) noexcept;

/// Declares the Neither boundary with ForceInline.
[[nodiscard]] int SIMD_FLAGS(Neither, ForceInline) contract_neither_forceinline(int value) noexcept;

/// Declares the Neither boundary with Flatten.
[[nodiscard]] int SIMD_FLAGS(Neither, Flatten) contract_neither_flatten(int value) noexcept;

/// Declares the Neither boundary with RegisterOnly, ForceInline.
[[nodiscard]] int SIMD_FLAGS(Neither, RegisterOnly, ForceInline) contract_neither_registeronly_forceinline(int value) noexcept;

/// Declares the Neither boundary with RegisterOnly, Flatten.
[[nodiscard]] int SIMD_FLAGS(Neither, RegisterOnly, Flatten) contract_neither_registeronly_flatten(int value) noexcept;

/// Declares the Neither boundary with ForceInline, Flatten.
[[nodiscard]] int SIMD_FLAGS(Neither, ForceInline, Flatten) contract_neither_forceinline_flatten(int value) noexcept;

/// Declares the Neither boundary with RegisterOnly, ForceInline, Flatten.
[[nodiscard]] int SIMD_FLAGS(Neither, RegisterOnly, ForceInline, Flatten) contract_neither_registeronly_forceinline_flatten(int value) noexcept;

/// Declares the In boundary with no modifiers.
[[nodiscard]] int SIMD_FLAGS(In) contract_in_plain(__m128 value) noexcept;

/// Declares the In boundary with RegisterOnly.
[[nodiscard]] int SIMD_FLAGS(In, RegisterOnly) contract_in_registeronly(__m128 value) noexcept;

/// Declares the In boundary with ForceInline.
[[nodiscard]] int SIMD_FLAGS(In, ForceInline) contract_in_forceinline(__m128 value) noexcept;

/// Declares the In boundary with Flatten.
[[nodiscard]] int SIMD_FLAGS(In, Flatten) contract_in_flatten(__m128 value) noexcept;

/// Declares the In boundary with RegisterOnly, ForceInline.
[[nodiscard]] int SIMD_FLAGS(In, RegisterOnly, ForceInline) contract_in_registeronly_forceinline(__m128 value) noexcept;

/// Declares the In boundary with RegisterOnly, Flatten.
[[nodiscard]] int SIMD_FLAGS(In, RegisterOnly, Flatten) contract_in_registeronly_flatten(__m128 value) noexcept;

/// Declares the In boundary with ForceInline, Flatten.
[[nodiscard]] int SIMD_FLAGS(In, ForceInline, Flatten) contract_in_forceinline_flatten(__m128 value) noexcept;

/// Declares the In boundary with RegisterOnly, ForceInline, Flatten.
[[nodiscard]] int SIMD_FLAGS(In, RegisterOnly, ForceInline, Flatten) contract_in_registeronly_forceinline_flatten(__m128 value) noexcept;

/// Declares the Out boundary with no modifiers.
[[nodiscard]] __m128 SIMD_FLAGS(Out) contract_out_plain(int value) noexcept;

/// Declares the Out boundary with RegisterOnly.
[[nodiscard]] __m128 SIMD_FLAGS(Out, RegisterOnly) contract_out_registeronly(int value) noexcept;

/// Declares the Out boundary with ForceInline.
[[nodiscard]] __m128 SIMD_FLAGS(Out, ForceInline) contract_out_forceinline(int value) noexcept;

/// Declares the Out boundary with Flatten.
[[nodiscard]] __m128 SIMD_FLAGS(Out, Flatten) contract_out_flatten(int value) noexcept;

/// Declares the Out boundary with RegisterOnly, ForceInline.
[[nodiscard]] __m128 SIMD_FLAGS(Out, RegisterOnly, ForceInline) contract_out_registeronly_forceinline(int value) noexcept;

/// Declares the Out boundary with RegisterOnly, Flatten.
[[nodiscard]] __m128 SIMD_FLAGS(Out, RegisterOnly, Flatten) contract_out_registeronly_flatten(int value) noexcept;

/// Declares the Out boundary with ForceInline, Flatten.
[[nodiscard]] __m128 SIMD_FLAGS(Out, ForceInline, Flatten) contract_out_forceinline_flatten(int value) noexcept;

/// Declares the Out boundary with RegisterOnly, ForceInline, Flatten.
[[nodiscard]] __m128 SIMD_FLAGS(Out, RegisterOnly, ForceInline, Flatten) contract_out_registeronly_forceinline_flatten(int value) noexcept;

/// Declares the InOut boundary with no modifiers.
[[nodiscard]] __m128 SIMD_FLAGS(InOut) contract_inout_plain(__m128 value) noexcept;

/// Declares the InOut boundary with RegisterOnly.
[[nodiscard]] __m128 SIMD_FLAGS(InOut, RegisterOnly) contract_inout_registeronly(__m128 value) noexcept;

/// Declares the InOut boundary with ForceInline.
[[nodiscard]] __m128 SIMD_FLAGS(InOut, ForceInline) contract_inout_forceinline(__m128 value) noexcept;

/// Declares the InOut boundary with Flatten.
[[nodiscard]] __m128 SIMD_FLAGS(InOut, Flatten) contract_inout_flatten(__m128 value) noexcept;

/// Declares the InOut boundary with RegisterOnly, ForceInline.
[[nodiscard]] __m128 SIMD_FLAGS(InOut, RegisterOnly, ForceInline) contract_inout_registeronly_forceinline(__m128 value) noexcept;

/// Declares the InOut boundary with RegisterOnly, Flatten.
[[nodiscard]] __m128 SIMD_FLAGS(InOut, RegisterOnly, Flatten) contract_inout_registeronly_flatten(__m128 value) noexcept;

/// Declares the InOut boundary with ForceInline, Flatten.
[[nodiscard]] __m128 SIMD_FLAGS(InOut, ForceInline, Flatten) contract_inout_forceinline_flatten(__m128 value) noexcept;

/// Declares the InOut boundary with RegisterOnly, ForceInline, Flatten.
[[nodiscard]] __m128 SIMD_FLAGS(InOut, RegisterOnly, ForceInline, Flatten) contract_inout_registeronly_forceinline_flatten(__m128 value) noexcept;
} // namespace SimdLibMethodFlagsContract
