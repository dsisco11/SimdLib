#include <SimdLib/Aliases.h>

#include <concepts>
#include <cstdint>

static_assert(std::same_as<SimdLib::VectorInt8, SimdLib::SimdVector<std::int8_t, 4>>);
static_assert(std::same_as<SimdLib::VectorUInt64, SimdLib::SimdVector<std::uint64_t, 4>>);

static_assert(std::same_as<SimdLib::uint8x16, SimdLib::SimdVector<std::uint8_t, 16>>);
static_assert(std::same_as<SimdLib::uint16x16, SimdLib::SimdVector<std::uint16_t, 16>>);
static_assert(std::same_as<SimdLib::uint32x8, SimdLib::SimdVector<std::uint32_t, 8>>);
static_assert(std::same_as<SimdLib::uint64x4, SimdLib::SimdVector<std::uint64_t, 4>>);

static_assert(std::same_as<SimdLib::int8x32, SimdLib::SimdVector<std::int8_t, 32>>);
static_assert(std::same_as<SimdLib::int16x8, SimdLib::SimdVector<std::int16_t, 8>>);
static_assert(std::same_as<SimdLib::int32x4, SimdLib::SimdVector<std::int32_t, 4>>);
static_assert(std::same_as<SimdLib::int64x2, SimdLib::SimdVector<std::int64_t, 2>>);
