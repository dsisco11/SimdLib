#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/Register.h>

#include <cstdint>

using register_type = SimdLib::Register<std::int32_t, 128>;

/** @brief Reports whether an oversized logical lane list is accepted. */
template <class value_t>
concept accepts_oversized_lane_list = requires { value_t::from_lanes(1, 2, 3, 4, 5); };

static_assert(accepts_oversized_lane_list<register_type>, "SIMDLIB_REGISTER_REJECTS_OVERSIZED_LANE_LIST");
