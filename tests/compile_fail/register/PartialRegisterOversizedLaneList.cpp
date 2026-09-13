#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/PartialRegister.h>

#include <cstdint>

using partial_register_type = SimdLib::PartialRegister<std::int32_t, 128, 3>;

/** @brief Reports whether an oversized active lane list is accepted. */
template <class value_t>
concept accepts_oversized_lane_list = requires { value_t::from_lanes(1, 2, 3, 4); };

static_assert(accepts_oversized_lane_list<partial_register_type>, "SIMDLIB_PARTIAL_REGISTER_REJECTS_OVERSIZED_LANE_LIST");
