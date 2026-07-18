inline int precondition_failures = 0;

#define SIMDLIB_PRECONDITION(condition, message) \
	do \
	{ \
		(void)(message); \
		if (!(condition)) \
			++precondition_failures; \
	} while (false)

#include <SimdLib/Config.h>

int ConfigOverridePreconditionProbe() noexcept
{
	SIMDLIB_PRECONDITION(false, "probe");
	return precondition_failures;
}
