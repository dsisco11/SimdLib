#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/Register.h>

#include <cstdint>

using register_type = SimdLib::Register<std::int32_t, 128>;

/** @brief Reports whether an incomplete logical lane list is accepted. */
template <class value_t>
concept accepts_partial_lane_list = requires { value_t::from_lanes(1, 2, 3); };

static_assert(accepts_partial_lane_list<register_type>, "SIMDLIB_REGISTER_REJECTS_PARTIAL_LANE_LIST");
