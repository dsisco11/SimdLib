#include <SimdLib/PartialRegisterMask.h>

#include <cstdint>

using InstalledPartialRegisterMaskHeaderProbe = SimdLib::PartialRegisterMask<std::uint32_t, 128, 3>;

static_assert(InstalledPartialRegisterMaskHeaderProbe::native_lane_count == 4);
