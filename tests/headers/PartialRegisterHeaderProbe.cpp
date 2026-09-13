#include <SimdLib/PartialRegister.h>

#include <cstdint>

#ifndef SIMDLIB_PARTIAL_REGISTER_HEADER_PROBE_BITS
#define SIMDLIB_PARTIAL_REGISTER_HEADER_PROBE_BITS 128
#endif

constexpr std::size_t partial_register_header_probe_lane_count = SIMDLIB_PARTIAL_REGISTER_HEADER_PROBE_BITS == 256 ? 5 : 3;
using PartialRegisterHeaderProbe =
	SimdLib::PartialRegister<std::uint32_t, SIMDLIB_PARTIAL_REGISTER_HEADER_PROBE_BITS, partial_register_header_probe_lane_count>;
using CompleteRegisterHeaderProbe = SimdLib::Register<std::uint32_t, SIMDLIB_PARTIAL_REGISTER_HEADER_PROBE_BITS>;

static_assert(PartialRegisterHeaderProbe::lane_count == partial_register_header_probe_lane_count);
static_assert(SimdLib::IRegister::CoreSurface<PartialRegisterHeaderProbe>);
static_assert(SimdLib::IRegister::CoreSurface<CompleteRegisterHeaderProbe>);
static_assert(SimdLib::IPartialRegisterMask::Type<typename PartialRegisterHeaderProbe::mask_type>);
