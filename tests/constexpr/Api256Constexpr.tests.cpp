#include "ApiConstexprContracts.h"

using namespace SimdLib::Tests::Constexpr;

static_assert(construction_contract<256, std::int8_t>());
static_assert(construction_contract<256, std::uint8_t>());
static_assert(construction_contract<256, std::int16_t>());
static_assert(construction_contract<256, std::uint16_t>());
static_assert(construction_contract<256, std::int32_t>());
static_assert(construction_contract<256, std::uint32_t>());
static_assert(construction_contract<256, std::int64_t>());
static_assert(construction_contract<256, std::uint64_t>());
static_assert(construction_contract<256, float>());
static_assert(construction_contract<256, double>());

static_assert(comparison_contract<256, std::int8_t>());
static_assert(comparison_contract<256, std::uint8_t>());
static_assert(comparison_contract<256, std::int16_t>());
static_assert(comparison_contract<256, std::uint16_t>());
static_assert(comparison_contract<256, std::int32_t>());
static_assert(comparison_contract<256, std::uint32_t>());
static_assert(comparison_contract<256, std::int64_t>());
static_assert(comparison_contract<256, std::uint64_t>());
static_assert(comparison_contract<256, float>());
static_assert(comparison_contract<256, double>());

static_assert(bitwise_contract<256, std::int8_t>());
static_assert(bitwise_contract<256, std::uint8_t>());
static_assert(bitwise_contract<256, std::int16_t>());
static_assert(bitwise_contract<256, std::uint16_t>());
static_assert(bitwise_contract<256, std::int32_t>());
static_assert(bitwise_contract<256, std::uint32_t>());
static_assert(bitwise_contract<256, std::int64_t>());
static_assert(bitwise_contract<256, std::uint64_t>());
static_assert(bitwise_contract<256, float>());
static_assert(bitwise_contract<256, double>());

static_assert(movemask_contract<256, std::int8_t>());
static_assert(movemask_contract<256, std::uint8_t>());
static_assert(movemask_contract<256, std::int16_t>());
static_assert(movemask_contract<256, std::uint16_t>());
static_assert(movemask_contract<256, std::int32_t>());
static_assert(movemask_contract<256, std::uint32_t>());
static_assert(movemask_contract<256, std::int64_t>());
static_assert(movemask_contract<256, std::uint64_t>());
static_assert(movemask_contract<256, float>());
static_assert(movemask_contract<256, double>());

static_assert(extrema_position_contract<256, std::int8_t>());
static_assert(extrema_position_contract<256, std::uint8_t>());
static_assert(extrema_position_contract<256, std::int16_t>());
static_assert(extrema_position_contract<256, std::uint16_t>());
static_assert(extrema_position_contract<256, std::int32_t>());
static_assert(extrema_position_contract<256, std::uint32_t>());
static_assert(extrema_position_contract<256, std::int64_t>());
static_assert(extrema_position_contract<256, std::uint64_t>());

static_assert(lane_shift_contract<256, std::int8_t>());
static_assert(lane_shift_contract<256, std::uint8_t>());
static_assert(lane_shift_contract<256, std::int16_t>());
static_assert(lane_shift_contract<256, std::uint16_t>());
static_assert(lane_shift_contract<256, std::int32_t>());
static_assert(lane_shift_contract<256, std::uint32_t>());
static_assert(lane_shift_contract<256, std::int64_t>());
static_assert(lane_shift_contract<256, std::uint64_t>());
static_assert(logical_shuffle_contract<256, std::int8_t>());
static_assert(logical_shuffle_contract<256, std::uint8_t>());
static_assert(logical_shuffle_contract<256, std::int16_t>());
static_assert(logical_shuffle_contract<256, std::uint16_t>());
static_assert(logical_shuffle_contract<256, std::int32_t>());
static_assert(logical_shuffle_contract<256, std::uint32_t>());
static_assert(logical_shuffle_contract<256, std::int64_t>());
static_assert(logical_shuffle_contract<256, std::uint64_t>());
static_assert(logical_shuffle_contract<256, float>());
static_assert(logical_shuffle_contract<256, double>());

static_assert(simd_vector_contract<8>());
