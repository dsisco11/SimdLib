#include <SimdLib/Config.h>

int VECTORCALL ConfigFreeFunction(const int value) noexcept
{
	return value;
}

struct ConfigProbe
{
	static int VECTORCALL StaticFunction(const int value) noexcept
	{
		return value;
	}

	template <typename value_t>
	static value_t VECTORCALL TemplateFunction(const value_t value) noexcept
	{
		return value;
	}
};

using ConfigFunctionPointer = int(VECTORCALL *)(int);

SIMDLIB_FORCE_INLINE int ForceInlineFunction(const int value) noexcept
{
	return value + 1;
}

/** @brief Exercises the default recursive-inlining annotation. */
SIMDLIB_FLATTEN int FlattenFunction(const int value) noexcept
{
	return ForceInlineFunction(value);
}

static_assert(SimdLib::Config::target_x86 == (SIMDLIB_TARGET_X86 != 0));
static_assert(SimdLib::Config::target_x64 == (SIMDLIB_TARGET_X64 != 0));
static_assert(SimdLib::Config::compiler_clang == (SIMDLIB_COMPILER_CLANG != 0));
static_assert(SimdLib::Config::compiler_msvc == (SIMDLIB_COMPILER_MSVC != 0));
static_assert(SimdLib::Config::compiler_gcc == (SIMDLIB_COMPILER_GCC != 0));
static_assert(SimdLib::Config::has_sse == (SIMDLIB_HAS_SSE != 0));
static_assert(SimdLib::Config::has_sse2 == (SIMDLIB_HAS_SSE2 != 0));
static_assert(SimdLib::Config::has_sse3 == (SIMDLIB_HAS_SSE3 != 0));
static_assert(SimdLib::Config::has_ssse3 == (SIMDLIB_HAS_SSSE3 != 0));
static_assert(SimdLib::Config::has_sse41 == (SIMDLIB_HAS_SSE41 != 0));
static_assert(SimdLib::Config::has_sse42 == (SIMDLIB_HAS_SSE42 != 0));
static_assert(SimdLib::Config::has_avx == (SIMDLIB_HAS_AVX != 0));
static_assert(SimdLib::Config::has_avx2 == (SIMDLIB_HAS_AVX2 != 0));
static_assert(SimdLib::Config::has_fma == (SIMDLIB_HAS_FMA != 0));
static_assert(SimdLib::Config::has_bmi1 == (SIMDLIB_HAS_BMI1 != 0));
static_assert(SimdLib::Config::has_bmi2 == (SIMDLIB_HAS_BMI2 != 0));
static_assert(SimdLib::Config::vectorcall_enabled == (SIMDLIB_VECTORCALL_ENABLED != 0));
static_assert(SimdLib::version_major == 0 && SimdLib::version_minor == 2 && SimdLib::version_patch == 0);
#if SIMDLIB_COMPILER_MSVC && SIMDLIB_TARGET_X86
static_assert(SimdLib::Config::vectorcall_enabled);
#endif
#if SIMDLIB_COMPILER_CLANG && !defined(_WIN32)
static_assert(!SimdLib::Config::vectorcall_enabled);
#endif

int ConfigDefaultProbe() noexcept
{
	const ConfigFunctionPointer function = &ConfigFreeFunction;
	return function(ConfigProbe::StaticFunction(ConfigProbe::TemplateFunction(FlattenFunction(0))));
}
