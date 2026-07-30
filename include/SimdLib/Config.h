#pragma once

// Configuration macros are caller-overridable except
// SIMDLIB_REGISTER_INTERFACE_AVAILABLE, which reports a language capability
// computed by SimdLib. Instruction-family values describe compiler-enabled
// code-generation features, not runtime CPU support.

#ifndef SIMDLIB_COMPILER_CLANG
#if defined(__clang__)
#define SIMDLIB_COMPILER_CLANG 1
#else
#define SIMDLIB_COMPILER_CLANG 0
#endif
#endif

#ifndef SIMDLIB_COMPILER_MSVC
#if defined(_MSC_VER) && !defined(__clang__)
#define SIMDLIB_COMPILER_MSVC 1
#else
#define SIMDLIB_COMPILER_MSVC 0
#endif
#endif

#ifndef SIMDLIB_COMPILER_GCC
#if defined(__GNUC__) && !defined(__clang__)
#define SIMDLIB_COMPILER_GCC 1
#else
#define SIMDLIB_COMPILER_GCC 0
#endif
#endif

#if defined(SIMDLIB_REGISTER_INTERFACE_AVAILABLE)
#error "SIMDLIB_REGISTER_INTERFACE_AVAILABILITY_IS_COMPUTED: do not define SIMDLIB_REGISTER_INTERFACE_AVAILABLE"
#undef SIMDLIB_REGISTER_INTERFACE_AVAILABLE
#endif

#if defined(__cpp_explicit_this_parameter) && __cpp_explicit_this_parameter >= 202110L
#define SIMDLIB_REGISTER_INTERFACE_AVAILABLE 1
#elif defined(_MSC_VER) && !defined(__clang__) && _MSC_VER >= 1944 && defined(_MSVC_LANG) && _MSVC_LANG > 202002L
#define SIMDLIB_REGISTER_INTERFACE_AVAILABLE 1
#else
#define SIMDLIB_REGISTER_INTERFACE_AVAILABLE 0
#endif

// This caller-controlled signal requires the computed Register capability; it
// cannot enable or override that capability.
#ifndef SIMDLIB_REQUIRE_REGISTER_INTERFACE
#define SIMDLIB_REQUIRE_REGISTER_INTERFACE 0
#endif

#if SIMDLIB_REQUIRE_REGISTER_INTERFACE && !SIMDLIB_REGISTER_INTERFACE_AVAILABLE
#error "SIMDLIB_REGISTER_INTERFACE_UNAVAILABLE: SimdLib::Register requires C++23 explicit object parameter support"
#endif

#ifndef SIMDLIB_TARGET_X86
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#define SIMDLIB_TARGET_X86 1
#else
#define SIMDLIB_TARGET_X86 0
#endif
#endif

#ifndef SIMDLIB_TARGET_X64
#if defined(_M_X64) || defined(__x86_64__)
#define SIMDLIB_TARGET_X64 1
#else
#define SIMDLIB_TARGET_X64 0
#endif
#endif

#ifndef SIMDLIB_HAS_SSE
#if SIMDLIB_TARGET_X86 && (defined(__SSE__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1) || defined(_M_X64))
#define SIMDLIB_HAS_SSE 1
#else
#define SIMDLIB_HAS_SSE 0
#endif
#endif

#ifndef SIMDLIB_HAS_SSE2
#if SIMDLIB_TARGET_X86 && (defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(_M_X64))
#define SIMDLIB_HAS_SSE2 1
#else
#define SIMDLIB_HAS_SSE2 0
#endif
#endif

#ifndef SIMDLIB_HAS_SSE3
#if SIMDLIB_TARGET_X86 && (defined(__SSE3__) || defined(__AVX__) || defined(__AVX2__) || defined(_M_AVX) || defined(_M_AVX2))
#define SIMDLIB_HAS_SSE3 1
#else
#define SIMDLIB_HAS_SSE3 0
#endif
#endif

#ifndef SIMDLIB_HAS_SSSE3
#if SIMDLIB_TARGET_X86 && (defined(__SSSE3__) || defined(__AVX__) || defined(__AVX2__) || defined(_M_AVX) || defined(_M_AVX2))
#define SIMDLIB_HAS_SSSE3 1
#else
#define SIMDLIB_HAS_SSSE3 0
#endif
#endif

#ifndef SIMDLIB_HAS_SSE41
#if SIMDLIB_TARGET_X86 && (defined(__SSE4_1__) || defined(__AVX__) || defined(__AVX2__) || defined(_M_AVX) || defined(_M_AVX2))
#define SIMDLIB_HAS_SSE41 1
#else
#define SIMDLIB_HAS_SSE41 0
#endif
#endif

#ifndef SIMDLIB_HAS_SSE42
#if SIMDLIB_TARGET_X86 && (defined(__SSE4_2__) || defined(__AVX__) || defined(__AVX2__) || defined(_M_AVX) || defined(_M_AVX2))
#define SIMDLIB_HAS_SSE42 1
#else
#define SIMDLIB_HAS_SSE42 0
#endif
#endif

#ifndef SIMDLIB_HAS_AVX
#if SIMDLIB_TARGET_X86 && (defined(__AVX__) || defined(__AVX2__) || defined(_M_AVX) || defined(_M_AVX2))
#define SIMDLIB_HAS_AVX 1
#else
#define SIMDLIB_HAS_AVX 0
#endif
#endif

#ifndef SIMDLIB_HAS_AVX2
#if SIMDLIB_TARGET_X86 && (defined(__AVX2__) || defined(_M_AVX2))
#define SIMDLIB_HAS_AVX2 1
#else
#define SIMDLIB_HAS_AVX2 0
#endif
#endif

#ifndef SIMDLIB_HAS_FMA
#if SIMDLIB_TARGET_X86 && (defined(__FMA__) || defined(_M_AVX2))
#define SIMDLIB_HAS_FMA 1
#else
#define SIMDLIB_HAS_FMA 0
#endif
#endif

#ifndef SIMDLIB_HAS_BMI1
#if SIMDLIB_TARGET_X86 && (defined(__BMI__) || defined(_M_BMI))
#define SIMDLIB_HAS_BMI1 1
#else
#define SIMDLIB_HAS_BMI1 0
#endif
#endif

#ifndef SIMDLIB_HAS_BMI2
#if SIMDLIB_TARGET_X86 && (defined(__BMI2__) || defined(_M_BMI2))
#define SIMDLIB_HAS_BMI2 1
#else
#define SIMDLIB_HAS_BMI2 0
#endif
#endif

#ifndef SIMDLIB_VECTORCALL_ENABLED
#if SIMDLIB_TARGET_X86 && (SIMDLIB_COMPILER_MSVC || (SIMDLIB_COMPILER_CLANG && defined(_WIN32)))
#define SIMDLIB_VECTORCALL_ENABLED 1
#else
#define SIMDLIB_VECTORCALL_ENABLED 0
#endif
#endif

/*
 * Internal adapter: SIMDLIB_METHOD_FLAGS_HAS_VECTORCALL
 * @brief Reports whether the method-flags vector calling-convention adapter is active.
 * @details A custom toolchain may override this capability together with
 * SIMDLIB_METHOD_FLAGS_VECTORCALL before including this header.
 */
#ifndef SIMDLIB_METHOD_FLAGS_HAS_VECTORCALL
#define SIMDLIB_METHOD_FLAGS_HAS_VECTORCALL SIMDLIB_VECTORCALL_ENABLED
#endif

/*
 * Internal adapter: SIMDLIB_METHOD_FLAGS_HAS_SAFE_BUFFERS
 * @brief Reports whether RegisterOnly can suppress compiler stack-cookie instrumentation.
 * @details A custom toolchain may override this capability together with
 * SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS before including this header.
 */
#ifndef SIMDLIB_METHOD_FLAGS_HAS_SAFE_BUFFERS
#define SIMDLIB_METHOD_FLAGS_HAS_SAFE_BUFFERS SIMDLIB_COMPILER_MSVC
#endif

/*
 * Internal adapter: SIMDLIB_METHOD_FLAGS_HAS_FORCE_INLINE
 * @brief Reports whether ForceInline has an active compiler enforcement attribute.
 * @details The adapter retains ordinary inline semantics when this capability is zero.
 * A custom toolchain may override this capability together with
 * SIMDLIB_METHOD_FLAGS_FORCE_INLINE before including this header.
 */
#ifndef SIMDLIB_METHOD_FLAGS_HAS_FORCE_INLINE
#if SIMDLIB_COMPILER_MSVC || SIMDLIB_COMPILER_CLANG || SIMDLIB_COMPILER_GCC
#define SIMDLIB_METHOD_FLAGS_HAS_FORCE_INLINE 1
#else
#define SIMDLIB_METHOD_FLAGS_HAS_FORCE_INLINE 0
#endif
#endif

/*
 * Internal adapter: SIMDLIB_METHOD_FLAGS_HAS_FLATTEN
 * @brief Reports whether Flatten has an active recursive-inlining attribute.
 * @details A custom toolchain may override this capability together with
 * SIMDLIB_METHOD_FLAGS_FLATTEN before including this header.
 */
#ifndef SIMDLIB_METHOD_FLAGS_HAS_FLATTEN
#if SIMDLIB_COMPILER_MSVC || SIMDLIB_COMPILER_CLANG || SIMDLIB_COMPILER_GCC
#define SIMDLIB_METHOD_FLAGS_HAS_FLATTEN 1
#else
#define SIMDLIB_METHOD_FLAGS_HAS_FLATTEN 0
#endif
#endif

/*
 * Internal adapter: SIMDLIB_METHOD_FLAGS_VECTORCALL
 * @brief Placement-safe vector calling-convention adapter used by SIMD_FLAGS.
 */
#ifndef SIMDLIB_METHOD_FLAGS_VECTORCALL
#if SIMDLIB_METHOD_FLAGS_HAS_VECTORCALL
#define SIMDLIB_METHOD_FLAGS_VECTORCALL __vectorcall
#else
#define SIMDLIB_METHOD_FLAGS_VECTORCALL
#endif
#endif

/*
 * Internal adapter: SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS
 * @brief Placement-safe safe-buffer adapter used by the RegisterOnly flag.
 */
#ifndef SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS
#if SIMDLIB_METHOD_FLAGS_HAS_SAFE_BUFFERS
#define SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS __declspec(safebuffers)
#else
#define SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS
#endif
#endif

/*
 * Internal adapter: SIMDLIB_METHOD_FLAGS_FORCE_INLINE
 * @brief Placement-safe force-inline adapter used by the ForceInline flag.
 */
#ifndef SIMDLIB_METHOD_FLAGS_FORCE_INLINE
#if !SIMDLIB_METHOD_FLAGS_HAS_FORCE_INLINE
#define SIMDLIB_METHOD_FLAGS_FORCE_INLINE inline
#elif SIMDLIB_COMPILER_MSVC
#define SIMDLIB_METHOD_FLAGS_FORCE_INLINE __forceinline
#elif SIMDLIB_COMPILER_CLANG || SIMDLIB_COMPILER_GCC
#define SIMDLIB_METHOD_FLAGS_FORCE_INLINE inline __attribute__((always_inline))
#else
#define SIMDLIB_METHOD_FLAGS_FORCE_INLINE inline
#endif
#endif

/*
 * Internal adapter: SIMDLIB_METHOD_FLAGS_FLATTEN
 * @brief Placement-safe recursive-inlining adapter used by the Flatten flag.
 */
#ifndef SIMDLIB_METHOD_FLAGS_FLATTEN
#if !SIMDLIB_METHOD_FLAGS_HAS_FLATTEN
#define SIMDLIB_METHOD_FLAGS_FLATTEN
#elif SIMDLIB_COMPILER_MSVC
#define SIMDLIB_METHOD_FLAGS_FLATTEN [[msvc::flatten]]
#elif SIMDLIB_COMPILER_CLANG || SIMDLIB_COMPILER_GCC
#define SIMDLIB_METHOD_FLAGS_FLATTEN __attribute__((flatten))
#else
#define SIMDLIB_METHOD_FLAGS_FLATTEN
#endif
#endif

#define SIMDLIB_DETAIL_FLAGS_CAT_RAW(left, right) left##right
#define SIMDLIB_DETAIL_FLAGS_CAT(left, right) SIMDLIB_DETAIL_FLAGS_CAT_RAW(left, right)

#define SIMDLIB_DETAIL_FLAGS_BOUNDARY_ static_assert(false, "SIMDLIB_FLAGS_ERROR_EMPTY");
#define SIMDLIB_DETAIL_FLAGS_BOUNDARY_Neither
#define SIMDLIB_DETAIL_FLAGS_BOUNDARY_In SIMDLIB_METHOD_FLAGS_VECTORCALL
#define SIMDLIB_DETAIL_FLAGS_BOUNDARY_Out SIMDLIB_METHOD_FLAGS_VECTORCALL
#define SIMDLIB_DETAIL_FLAGS_BOUNDARY_InOut SIMDLIB_METHOD_FLAGS_VECTORCALL

#define SIMDLIB_DETAIL_FLAGS_MODIFIERS_1_RegisterOnly SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS
#define SIMDLIB_DETAIL_FLAGS_MODIFIERS_1_ForceInline SIMDLIB_METHOD_FLAGS_FORCE_INLINE
#define SIMDLIB_DETAIL_FLAGS_MODIFIERS_1_Flatten SIMDLIB_METHOD_FLAGS_FLATTEN
#define SIMDLIB_DETAIL_FLAGS_MODIFIERS_2_RegisterOnly_ForceInline SIMDLIB_METHOD_FLAGS_FORCE_INLINE SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS
#define SIMDLIB_DETAIL_FLAGS_MODIFIERS_2_RegisterOnly_Flatten SIMDLIB_METHOD_FLAGS_FLATTEN SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS
#define SIMDLIB_DETAIL_FLAGS_MODIFIERS_2_ForceInline_Flatten SIMDLIB_METHOD_FLAGS_FLATTEN SIMDLIB_METHOD_FLAGS_FORCE_INLINE
#define SIMDLIB_DETAIL_FLAGS_MODIFIERS_3_RegisterOnly_ForceInline_Flatten                                                                                      \
	SIMDLIB_METHOD_FLAGS_FLATTEN SIMDLIB_METHOD_FLAGS_FORCE_INLINE SIMDLIB_METHOD_FLAGS_SAFE_BUFFERS

#define SIMDLIB_DETAIL_FLAGS_BOUNDARY_RAW(mode) SIMDLIB_DETAIL_FLAGS_BOUNDARY_##mode
#define SIMDLIB_DETAIL_FLAGS_BOUNDARY(mode) SIMDLIB_DETAIL_FLAGS_BOUNDARY_RAW(mode)
#define SIMDLIB_DETAIL_FLAGS_MODIFIERS_1_RAW(a) SIMDLIB_DETAIL_FLAGS_MODIFIERS_1_##a
#define SIMDLIB_DETAIL_FLAGS_MODIFIERS_1(a) SIMDLIB_DETAIL_FLAGS_MODIFIERS_1_RAW(a)
#define SIMDLIB_DETAIL_FLAGS_MODIFIERS_2_RAW(a, b) SIMDLIB_DETAIL_FLAGS_MODIFIERS_2_##a##_##b
#define SIMDLIB_DETAIL_FLAGS_MODIFIERS_2(a, b) SIMDLIB_DETAIL_FLAGS_MODIFIERS_2_RAW(a, b)
#define SIMDLIB_DETAIL_FLAGS_MODIFIERS_3_RAW(a, b, c) SIMDLIB_DETAIL_FLAGS_MODIFIERS_3_##a##_##b##_##c
#define SIMDLIB_DETAIL_FLAGS_MODIFIERS_3(a, b, c) SIMDLIB_DETAIL_FLAGS_MODIFIERS_3_RAW(a, b, c)

#define SIMDLIB_DETAIL_FLAGS_1(boundary) SIMDLIB_DETAIL_FLAGS_BOUNDARY(boundary)
#define SIMDLIB_DETAIL_FLAGS_2(boundary, a) SIMDLIB_DETAIL_FLAGS_MODIFIERS_1(a) SIMDLIB_DETAIL_FLAGS_BOUNDARY(boundary)
#define SIMDLIB_DETAIL_FLAGS_3(boundary, a, b) SIMDLIB_DETAIL_FLAGS_MODIFIERS_2(a, b) SIMDLIB_DETAIL_FLAGS_BOUNDARY(boundary)
#define SIMDLIB_DETAIL_FLAGS_4(boundary, a, b, c) SIMDLIB_DETAIL_FLAGS_MODIFIERS_3(a, b, c) SIMDLIB_DETAIL_FLAGS_BOUNDARY(boundary)
#define SIMDLIB_DETAIL_FLAGS_5(...) static_assert(false, "SIMDLIB_FLAGS_ERROR_TOO_MANY");

#define SIMDLIB_DETAIL_FLAGS_ARITY_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, count, ...) count
#define SIMDLIB_DETAIL_FLAGS_ARITY_EXPAND(arguments) SIMDLIB_DETAIL_FLAGS_ARITY_IMPL arguments
#define SIMDLIB_DETAIL_FLAGS_ARITY(...) SIMDLIB_DETAIL_FLAGS_ARITY_EXPAND((__VA_ARGS__, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2, 1))

#define SIMDLIB_DETAIL_FLAGS_DISPATCH(count) SIMDLIB_DETAIL_FLAGS_CAT(SIMDLIB_DETAIL_FLAGS_, count)
#define SIMDLIB_DETAIL_FLAGS_EXPAND(...) __VA_ARGS__

/**
 * @def SIMD_FLAGS
 * @brief Declares a function's SIMD boundary and optimization promises.
 * @param ... One required boundary mode followed by zero to three modifiers.
 * @details The boundary is one of Neither, In, Out, or InOut. Modifiers are an
 * ordered subsequence of RegisterOnly, ForceInline, and Flatten. Place the macro
 * after the independently specified return type and immediately before the
 * function name. Constructors, destructors, conversion operators, deduction
 * guides, lambdas, virtual functions, explicit function-pointer types,
 * coroutines, C-style variadic functions, extern-C functions, allocation
 * functions, defaulted or deleted functions, and consteval functions are not
 * supported. The macro records developer intent; it cannot inspect function
 * signatures, bodies, template instantiations, or transitive callees.
 */
#define SIMD_FLAGS(...) SIMDLIB_DETAIL_FLAGS_EXPAND(SIMDLIB_DETAIL_FLAGS_DISPATCH(SIMDLIB_DETAIL_FLAGS_ARITY(__VA_ARGS__))(__VA_ARGS__))

#ifndef SIMDLIB_PRECONDITION
#include <cassert>
#define SIMDLIB_PRECONDITION(condition, message) assert((condition) && (message))
#endif

#ifndef SIMDLIB_ENABLE_CHECKS
#if defined(NDEBUG)
#define SIMDLIB_ENABLE_CHECKS 0
#else
#define SIMDLIB_ENABLE_CHECKS 1
#endif
#endif

namespace SimdLib::Config
{
inline constexpr int version_major = 0;
inline constexpr int version_minor = 2;
inline constexpr int version_patch = 0;

inline constexpr bool compiler_clang = SIMDLIB_COMPILER_CLANG != 0;
inline constexpr bool compiler_msvc = SIMDLIB_COMPILER_MSVC != 0;
inline constexpr bool compiler_gcc = SIMDLIB_COMPILER_GCC != 0;
inline constexpr bool target_x86 = SIMDLIB_TARGET_X86 != 0;
inline constexpr bool target_x64 = SIMDLIB_TARGET_X64 != 0;
inline constexpr bool vectorcall_enabled = SIMDLIB_VECTORCALL_ENABLED != 0;
inline constexpr bool method_flags_has_vectorcall = SIMDLIB_METHOD_FLAGS_HAS_VECTORCALL != 0;
inline constexpr bool method_flags_has_safe_buffers = SIMDLIB_METHOD_FLAGS_HAS_SAFE_BUFFERS != 0;
inline constexpr bool method_flags_has_force_inline = SIMDLIB_METHOD_FLAGS_HAS_FORCE_INLINE != 0;
inline constexpr bool method_flags_has_flatten = SIMDLIB_METHOD_FLAGS_HAS_FLATTEN != 0;

inline constexpr bool has_sse = SIMDLIB_HAS_SSE != 0;
inline constexpr bool has_sse2 = SIMDLIB_HAS_SSE2 != 0;
inline constexpr bool has_sse3 = SIMDLIB_HAS_SSE3 != 0;
inline constexpr bool has_ssse3 = SIMDLIB_HAS_SSSE3 != 0;
inline constexpr bool has_sse41 = SIMDLIB_HAS_SSE41 != 0;
inline constexpr bool has_sse42 = SIMDLIB_HAS_SSE42 != 0;
inline constexpr bool has_avx = SIMDLIB_HAS_AVX != 0;
inline constexpr bool has_avx2 = SIMDLIB_HAS_AVX2 != 0;
inline constexpr bool has_fma = SIMDLIB_HAS_FMA != 0;
inline constexpr bool has_bmi1 = SIMDLIB_HAS_BMI1 != 0;
inline constexpr bool has_bmi2 = SIMDLIB_HAS_BMI2 != 0;
} // namespace SimdLib::Config

namespace SimdLib
{
inline constexpr int version_major = Config::version_major;
inline constexpr int version_minor = Config::version_minor;
inline constexpr int version_patch = Config::version_patch;
} // namespace SimdLib
