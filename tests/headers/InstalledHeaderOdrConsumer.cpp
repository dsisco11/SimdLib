#include "InstalledHeaderOdrFixture.h"

/**
 * @brief Verifies declaration and definition agreement across translation units.
 * @return Zero when the copied-header declaration linked and executed correctly.
 */
int main()
{
	return installed_header_odr_value(41) == 42 ? 0 : 1;
}
