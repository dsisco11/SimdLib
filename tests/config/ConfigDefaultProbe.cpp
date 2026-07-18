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

using ConfigFunctionPointer = int(VECTORCALL*)(int);

SIMDLIB_FORCE_INLINE int ForceInlineFunction(const int value) noexcept
{
	return value + 1;
}

static_assert(SimdLib::Config::target_x86 == (SIMDLIB_TARGET_X86 != 0));
static_assert(SimdLib::Config::has_avx2 == (SIMDLIB_HAS_AVX2 != 0));
static_assert(SimdLib::version_major == 0 && SimdLib::version_minor == 2 && SimdLib::version_patch == 0);
#if SIMDLIB_COMPILER_MSVC && SIMDLIB_TARGET_X86
static_assert(SimdLib::Config::vectorcall_enabled);
#endif

int ConfigDefaultProbe() noexcept
{
	const ConfigFunctionPointer function = &ConfigFreeFunction;
	return function(ConfigProbe::StaticFunction(ConfigProbe::TemplateFunction(ForceInlineFunction(0))));
}
