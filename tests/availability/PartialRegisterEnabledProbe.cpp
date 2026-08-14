#include <SimdLib/PartialRegister.h>

#include <cstdint>
#include <type_traits>

#ifndef SIMDLIB_PARTIAL_REGISTER_AVAILABILITY_BITS
#define SIMDLIB_PARTIAL_REGISTER_AVAILABILITY_BITS 128
#endif

constexpr std::size_t partial_register_availability_lane_count = SIMDLIB_PARTIAL_REGISTER_AVAILABILITY_BITS == 256 ? 5 : 3;

static_assert(SIMDLIB_REGISTER_INTERFACE_AVAILABLE == 1);
static_assert(SIMDLIB_REQUIRE_REGISTER_INTERFACE == 1);
using PartialRegisterAvailabilityProbe =
	SimdLib::PartialRegister<std::uint32_t, SIMDLIB_PARTIAL_REGISTER_AVAILABILITY_BITS, partial_register_availability_lane_count>;
static_assert(std::is_final_v<PartialRegisterAvailabilityProbe>);
static_assert(SimdLib::IRegister::CoreSurface<PartialRegisterAvailabilityProbe>);
