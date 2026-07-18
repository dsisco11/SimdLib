#include <SimdLib/Api.h>

#include <array>
#include <cstdint>

template <std::size_t Width, class Element>
consteval bool specialization_available()
{
    using simd = SimdLib::Api<Width, Element>;
    return sizeof(typename simd::vector_t) == Width / 8 && simd::element_count == Width / (sizeof(Element) * 8);
}

static_assert(specialization_available<128, std::int8_t>());
static_assert(specialization_available<128, std::uint8_t>());
static_assert(specialization_available<128, std::int16_t>());
static_assert(specialization_available<128, std::uint16_t>());
static_assert(specialization_available<128, std::int32_t>());
static_assert(specialization_available<128, std::uint32_t>());
static_assert(specialization_available<128, std::int64_t>());
static_assert(specialization_available<128, std::uint64_t>());
static_assert(specialization_available<128, float>());
static_assert(specialization_available<128, double>());
static_assert(specialization_available<256, std::int8_t>());
static_assert(specialization_available<256, std::uint8_t>());
static_assert(specialization_available<256, std::int16_t>());
static_assert(specialization_available<256, std::uint16_t>());
static_assert(specialization_available<256, std::int32_t>());
static_assert(specialization_available<256, std::uint32_t>());
static_assert(specialization_available<256, std::int64_t>());
static_assert(specialization_available<256, std::uint64_t>());
static_assert(specialization_available<256, float>());
static_assert(specialization_available<256, double>());

consteval bool constexpr_paths_match()
{
    using simd = SimdLib::Api<128, std::uint64_t>;
    constexpr auto input = simd::setr(1, 2);
    constexpr auto shifted = simd::template bit_shift_left<64>(input);
    return simd::to_array(shifted) == std::array<std::uint64_t, 2>{0, 1};
}

static_assert(constexpr_paths_match());
