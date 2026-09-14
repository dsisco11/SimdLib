#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/PartialRegister.h>

#include <cstdint>

using partial_register_type = SimdLib::PartialRegister<std::int32_t, 128, 3>;

/** @brief Reports whether observation accepts the first inactive lane index. */
template <class value_t>
concept observes_inactive_lane = requires(value_t value) { value.template lane<value_t::lane_count>(); };

/** @brief Reports whether replacement accepts the first inactive lane index. */
template <class value_t>
concept replaces_inactive_lane = requires(value_t value) { value.template with_lane<value_t::lane_count>(typename value_t::element_type{}); };

static_assert(observes_inactive_lane<partial_register_type> || replaces_inactive_lane<partial_register_type>,
			  "SIMDLIB_PARTIAL_REGISTER_REJECTS_INACTIVE_LANE_INDEX");
