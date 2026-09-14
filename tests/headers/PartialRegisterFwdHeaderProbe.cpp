#include <SimdLib/PartialRegisterFwd.h>

#include <cstdint>

#ifndef SIMDLIB_PARTIAL_REGISTER_HEADER_PROBE_BITS
#define SIMDLIB_PARTIAL_REGISTER_HEADER_PROBE_BITS 128
#endif

constexpr std::size_t partial_register_fwd_probe_lane_count = SIMDLIB_PARTIAL_REGISTER_HEADER_PROBE_BITS == 256 ? 5 : 3;
using PartialRegisterFwdProbe = SimdLib::PartialRegister<std::uint32_t, SIMDLIB_PARTIAL_REGISTER_HEADER_PROBE_BITS, partial_register_fwd_probe_lane_count>;
