#include <SimdLib/PartialRegister.h>

#include <cstdint>
#include <type_traits>

static_assert(SIMDLIB_REGISTER_INTERFACE_AVAILABLE == 1);
static_assert(SIMDLIB_REQUIRE_REGISTER_INTERFACE == 1);
static_assert(std::is_final_v<SimdLib::PartialRegister<std::uint32_t, 128, 3>>);
static_assert(SimdLib::PartialRegister<std::uint32_t, 128, 3>::native_lane_count == 4);
