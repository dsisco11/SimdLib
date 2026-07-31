#define SIMDLIB_HAS_SSE42 1
#define SIMDLIB_HAS_AVX2 1
#include <SimdLib/Api.h>
#include <SimdLib/IApi.h>
#include <SimdLib/IImpl.h>

#include <cstddef>
#include <cstdint>

namespace
{

/** @brief Selects the implementation mapping for one public Api specialization. */
template <std::size_t Width, class Element> using implementation_t = SimdLib::Detail::SimdMappings<Width, Element>;

/**
 * @brief Verifies runtime-selected lane slow paths in both public and implementation layers.
 * @tparam Width SIMD register width in bits.
 * @tparam Element Logical lane type.
 * @return `true` when extraction and insertion slow signatures are available in both layers.
 */
template <std::size_t Width, class Element> consteval bool lane_slow_paths_available()
{
	using api = SimdLib::Api<Width, Element>;
	using implementation = implementation_t<Width, Element>;
	return SimdLib::IApi::ExtractSlow<api> && SimdLib::IApi::InsertSlow<api> && SimdLib::IImpl::ExtractSlow<implementation, int> &&
		   SimdLib::IImpl::InsertSlow<implementation, typename implementation::vector_t, Element, int>;
}

/**
 * @brief Verifies scalar-controlled blend slow paths in both public and implementation layers.
 * @tparam Width SIMD register width in bits.
 * @tparam Element Logical lane type.
 * @return `true` when both slow blend signatures are available.
 */
template <std::size_t Width, class Element> consteval bool blend_slow_path_available()
{
	using api = SimdLib::Api<Width, Element>;
	using implementation = implementation_t<Width, Element>;
	using vector = typename implementation::vector_t;
	return SimdLib::IApi::BlendSlow<api> && SimdLib::IImpl::BlendSlow<implementation, vector, vector, int>;
}

/**
 * @brief Verifies scalar-controlled floating shuffle slow paths in both layers.
 * @tparam Width SIMD register width in bits.
 * @tparam Element Floating-point lane type.
 * @return `true` when both slow shuffle signatures are available.
 */
template <std::size_t Width, class Element> consteval bool floating_shuffle_slow_path_available()
{
	using api = SimdLib::Api<Width, Element>;
	using implementation = implementation_t<Width, Element>;
	using vector = typename implementation::vector_t;
	return SimdLib::IApi::ShuffleSlow<api> && SimdLib::IImpl::ShuffleSlow<implementation, vector, vector, int>;
}

/**
 * @brief Verifies low- and high-half shuffle slow paths in both layers.
 * @tparam Width SIMD register width in bits.
 * @return `true` when all four slow signatures are available.
 */
template <std::size_t Width> consteval bool half_shuffle_slow_paths_available()
{
	using api = SimdLib::Api<Width, std::uint16_t>;
	using implementation = implementation_t<Width, std::uint16_t>;
	using vector = typename implementation::vector_t;
	return SimdLib::IApi::ShuffleLowSlow<api> && SimdLib::IApi::ShuffleHighSlow<api> && SimdLib::IImpl::ShuffleLowSlow<implementation, vector, int> &&
		   SimdLib::IImpl::ShuffleHighSlow<implementation, vector, int>;
}

/**
 * @brief Verifies 32-bit shuffle slow paths in both layers.
 * @tparam Width SIMD register width in bits.
 * @return `true` when both slow signatures are available.
 */
template <std::size_t Width> consteval bool shuffle_32_slow_path_available()
{
	using api = SimdLib::Api<Width, std::uint32_t>;
	using implementation = implementation_t<Width, std::uint32_t>;
	return SimdLib::IApi::Shuffle32Slow<api> && SimdLib::IImpl::Shuffle32Slow<implementation>;
}

} // namespace

#define SIMDLIB_ASSERT_LANE_SLOW_PATHS(width)                                                                                                                  \
	static_assert(lane_slow_paths_available<width, std::int8_t>());                                                                                            \
	static_assert(lane_slow_paths_available<width, std::uint8_t>());                                                                                           \
	static_assert(lane_slow_paths_available<width, std::int16_t>());                                                                                           \
	static_assert(lane_slow_paths_available<width, std::uint16_t>());                                                                                          \
	static_assert(lane_slow_paths_available<width, std::int32_t>());                                                                                           \
	static_assert(lane_slow_paths_available<width, std::uint32_t>());                                                                                          \
	static_assert(lane_slow_paths_available<width, std::int64_t>());                                                                                           \
	static_assert(lane_slow_paths_available<width, std::uint64_t>());                                                                                          \
	static_assert(lane_slow_paths_available<width, float>());                                                                                                  \
	static_assert(lane_slow_paths_available<width, double>())

#define SIMDLIB_ASSERT_BLEND_SLOW_PATHS(width)                                                                                                                 \
	static_assert(blend_slow_path_available<width, std::int16_t>());                                                                                           \
	static_assert(blend_slow_path_available<width, std::uint16_t>());                                                                                          \
	static_assert(blend_slow_path_available<width, std::int32_t>());                                                                                           \
	static_assert(blend_slow_path_available<width, std::uint32_t>());                                                                                          \
	static_assert(blend_slow_path_available<width, float>());                                                                                                  \
	static_assert(blend_slow_path_available<width, double>())

SIMDLIB_ASSERT_LANE_SLOW_PATHS(128);
SIMDLIB_ASSERT_LANE_SLOW_PATHS(256);
SIMDLIB_ASSERT_BLEND_SLOW_PATHS(128);
SIMDLIB_ASSERT_BLEND_SLOW_PATHS(256);
static_assert(floating_shuffle_slow_path_available<128, float>());
static_assert(floating_shuffle_slow_path_available<128, double>());
static_assert(floating_shuffle_slow_path_available<256, float>());
static_assert(floating_shuffle_slow_path_available<256, double>());
static_assert(half_shuffle_slow_paths_available<128>());
static_assert(half_shuffle_slow_paths_available<256>());
static_assert(shuffle_32_slow_path_available<128>());
static_assert(shuffle_32_slow_path_available<256>());
static_assert(SimdLib::IApi::ShiftBytesSlow<SimdLib::Api<128, std::uint8_t>>);
static_assert(SimdLib::IApi::ShiftBitsSlow<SimdLib::Api<128, std::uint64_t>>);
static_assert(SimdLib::IApi::ShiftBits<SimdLib::Api<128, std::uint64_t>, 1>);
static_assert(SimdLib::IImpl::ShiftBytesSlow<implementation_t<128, std::uint8_t>>);
static_assert(SimdLib::IImpl::ShiftBitsSlow<implementation_t<128, std::uint64_t>>);
static_assert(SimdLib::IImpl::ShiftBits<implementation_t<128, std::uint64_t>, 1>);

static_assert(SimdLib::IApi::RegisterShuffle<SimdLib::Api<128, std::uint8_t>>);
static_assert(SimdLib::IApi::RegisterShuffle<SimdLib::Api<256, std::uint8_t>>);
static_assert(SimdLib::IApi::RegisterBlend<SimdLib::Api<128, std::int8_t>>);
static_assert(SimdLib::IApi::RegisterBlend<SimdLib::Api<256, std::int8_t>>);
static_assert(requires(SimdLib::Api<128, std::uint32_t>::vector_t value) { SimdLib::Api<128, std::uint32_t>::shift_left(value, 1); });
static_assert(requires(SimdLib::Api<256, std::uint32_t>::vector_t value) { SimdLib::Api<256, std::uint32_t>::shift_left(value, 1); });

#undef SIMDLIB_ASSERT_BLEND_SLOW_PATHS
#undef SIMDLIB_ASSERT_LANE_SLOW_PATHS