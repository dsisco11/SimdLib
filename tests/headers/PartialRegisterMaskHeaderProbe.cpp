#include <SimdLib/PartialRegisterMask.h>

#include <cstdint>

#ifndef SIMDLIB_PARTIAL_REGISTER_HEADER_PROBE_BITS
#define SIMDLIB_PARTIAL_REGISTER_HEADER_PROBE_BITS 128
#endif

constexpr std::size_t partial_register_mask_header_probe_lane_count = SIMDLIB_PARTIAL_REGISTER_HEADER_PROBE_BITS == 256 ? 5 : 3;
using PartialRegisterMaskHeaderProbe =
	SimdLib::PartialRegisterMask<std::uint32_t, SIMDLIB_PARTIAL_REGISTER_HEADER_PROBE_BITS, partial_register_mask_header_probe_lane_count>;

static_assert(PartialRegisterMaskHeaderProbe::lane_count == partial_register_mask_header_probe_lane_count);
static_assert(SimdLib::IPartialRegisterMask::Type<PartialRegisterMaskHeaderProbe>);
