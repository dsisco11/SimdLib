#include <SimdLib/Api.h>
#include <SimdLib/Bmi.h>
#include <SimdLib/SimdVector.h>
#include <SimdLib/UInt128.h>

#include <concepts>
#include <cstdint>

static_assert(std::same_as<decltype(SimdLib::Bmi::blsi(SimdLib::uint128_t{8})), SimdLib::uint128_t>);
static_assert(std::same_as<decltype(SimdLib::Bmi::bextr(SimdLib::uint128_t{0xF0}, 4, 4)), SimdLib::uint128_t>);
static_assert(std::same_as<decltype(SimdLib::Bmi::bextr(SimdLib::uint128_t{0xF0}, std::uint32_t{4u | (4u << 8u)})), SimdLib::uint128_t>);

#if SIMDLIB_HAS_SSE42
static_assert(SimdLib::ApiAvailable<128, std::uint32_t>);
static_assert(SimdLib::Api<128, std::uint32_t>::element_count == 4);
#else
static_assert(!SimdLib::is_api_available_v<128, std::uint32_t>);
#endif
