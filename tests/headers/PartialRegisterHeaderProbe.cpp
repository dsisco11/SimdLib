#include <SimdLib/PartialRegister.h>

#include <cstdint>

using PartialRegisterHeaderProbe = SimdLib::PartialRegister<std::uint32_t, 128, 3>;

static_assert(PartialRegisterHeaderProbe::lane_count == 3);
