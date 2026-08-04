#include <SimdLib/Aliases.h>

#include <concepts>
#include <cstdint>

static_assert(std::same_as<SimdLib::VectorInt8, SimdLib::Register<std::int8_t, 128>>);
static_assert(std::same_as<SimdLib::VectorUInt64, SimdLib::Register<std::uint64_t, 256>>);

static_assert(std::same_as<SimdLib::uint8x16, SimdLib::Register<std::uint8_t, 128>>);
static_assert(std::same_as<SimdLib::uint16x16, SimdLib::Register<std::uint16_t, 256>>);
static_assert(std::same_as<SimdLib::uint32x8, SimdLib::Register<std::uint32_t, 256>>);
static_assert(std::same_as<SimdLib::uint64x4, SimdLib::Register<std::uint64_t, 256>>);

static_assert(std::same_as<SimdLib::int8x32, SimdLib::Register<std::int8_t, 256>>);
static_assert(std::same_as<SimdLib::int16x8, SimdLib::Register<std::int16_t, 128>>);
static_assert(std::same_as<SimdLib::int32x4, SimdLib::Register<std::int32_t, 128>>);
static_assert(std::same_as<SimdLib::int64x2, SimdLib::Register<std::int64_t, 128>>);
