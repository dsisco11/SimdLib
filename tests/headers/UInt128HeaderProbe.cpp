#include <SimdLib/UInt128.h>

static_assert(SimdLib::version_major == 0);
static_assert(std::numeric_limits<SimdLib::uint128_t>::has_denorm == std::denorm_absent);
static_assert(!std::numeric_limits<SimdLib::uint128_t>::has_denorm_loss);
