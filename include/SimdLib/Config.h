#pragma once

#include <cassert>

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

// VECTORCALL is intentionally unprefixed: it is the library's externally
// configurable ABI-affecting calling convention. MSVC and Clang both accept
// the __vectorcall keyword in the same declarator positions. For a
// caller-supplied empty VECTORCALL also set
// SIMDLIB_VECTORCALL_ENABLED=0.
#ifndef VECTORCALL
#if SIMDLIB_VECTORCALL_ENABLED
#define VECTORCALL __vectorcall
#else
#define VECTORCALL
#endif
#endif

#ifndef SIMDLIB_FORCE_INLINE
#if SIMDLIB_COMPILER_MSVC
#define SIMDLIB_FORCE_INLINE [[msvc::forceinline]] inline
#elif SIMDLIB_COMPILER_CLANG
#define SIMDLIB_FORCE_INLINE [[clang::always_inline]] inline
#elif SIMDLIB_COMPILER_GCC
#define SIMDLIB_FORCE_INLINE [[gnu::always_inline]] inline
#else
#define SIMDLIB_FORCE_INLINE inline
#endif
#endif

#ifndef SIMDLIB_PRECONDITION
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
