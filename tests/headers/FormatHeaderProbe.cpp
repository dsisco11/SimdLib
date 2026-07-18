#include <SimdLib/Format.h>

#include <format>
#include <string>

static_assert(SimdLib::version_major == 0);

namespace
{
[[maybe_unused]] const std::string formatted_uint128 = std::format("{}", SimdLib::uint128_t{1});
}
