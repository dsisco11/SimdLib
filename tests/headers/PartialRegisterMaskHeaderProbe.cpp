#include <SimdLib/PartialRegisterMask.h>

#include <cstdint>

using PartialRegisterMaskHeaderProbe = SimdLib::PartialRegisterMask<std::uint32_t, 128, 3>;

static_assert(PartialRegisterMaskHeaderProbe::lane_count == 3);
