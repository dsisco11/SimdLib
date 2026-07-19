#include "ApiConstexprContracts.h"

using namespace SimdLib::Tests::Constexpr;

static_assert(construction_contract<128, std::int8_t>());
static_assert(construction_contract<128, std::uint8_t>());
static_assert(construction_contract<128, std::int16_t>());
static_assert(construction_contract<128, std::uint16_t>());
static_assert(construction_contract<128, std::int32_t>());
static_assert(construction_contract<128, std::uint32_t>());
static_assert(construction_contract<128, std::int64_t>());
static_assert(construction_contract<128, std::uint64_t>());
static_assert(construction_contract<128, float>());
static_assert(construction_contract<128, double>());

static_assert(comparison_contract<128, std::int8_t>());
static_assert(comparison_contract<128, std::uint8_t>());
static_assert(comparison_contract<128, std::int16_t>());
static_assert(comparison_contract<128, std::uint16_t>());
static_assert(comparison_contract<128, std::int32_t>());
static_assert(comparison_contract<128, std::uint32_t>());
static_assert(comparison_contract<128, std::int64_t>());
static_assert(comparison_contract<128, std::uint64_t>());
static_assert(comparison_contract<128, float>());
static_assert(comparison_contract<128, double>());

static_assert(movemask_contract<128, std::int8_t>());
static_assert(movemask_contract<128, std::uint8_t>());
static_assert(movemask_contract<128, std::int16_t>());
static_assert(movemask_contract<128, std::uint16_t>());
static_assert(movemask_contract<128, std::int32_t>());
static_assert(movemask_contract<128, std::uint32_t>());
static_assert(movemask_contract<128, std::int64_t>());
static_assert(movemask_contract<128, std::uint64_t>());
static_assert(movemask_contract<128, float>());
static_assert(movemask_contract<128, double>());

static_assert(extrema_position_contract<128, std::int8_t>());
static_assert(extrema_position_contract<128, std::uint8_t>());
static_assert(extrema_position_contract<128, std::int16_t>());
static_assert(extrema_position_contract<128, std::uint16_t>());
static_assert(extrema_position_contract<128, std::int32_t>());
static_assert(extrema_position_contract<128, std::uint32_t>());
static_assert(extrema_position_contract<128, std::int64_t>());
static_assert(extrema_position_contract<128, std::uint64_t>());

static_assert(lane_shift_contract<128, std::int8_t>());
static_assert(lane_shift_contract<128, std::uint8_t>());
static_assert(lane_shift_contract<128, std::int16_t>());
static_assert(lane_shift_contract<128, std::uint16_t>());
static_assert(lane_shift_contract<128, std::int32_t>());
static_assert(lane_shift_contract<128, std::uint32_t>());
static_assert(lane_shift_contract<128, std::int64_t>());
static_assert(lane_shift_contract<128, std::uint64_t>());
static_assert(whole_register_shift_contract());
static_assert(simd_vector_contract<4>());