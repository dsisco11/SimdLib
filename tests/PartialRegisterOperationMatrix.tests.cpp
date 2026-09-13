#include <SimdLib/IRegister.h>
#include <SimdLib/PartialRegister.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#ifndef SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
#define SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256 SIMDLIB_HAS_AVX2
#endif

namespace
{

/** @brief Reports whether the adjacent-result alias accepts one source geometry. */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
concept has_partial_adjacent_alias = requires { typename SimdLib::partial_multiply_add_adjacent_result_t<element_t, bits, active_lane_count>; };

/** @brief Reports whether the byte-multiply-add result alias accepts one source geometry. */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
concept has_partial_byte_multiply_add_alias = requires { typename SimdLib::partial_byte_multiply_add_result_t<element_t, bits, active_lane_count>; };

/** @brief Reports whether the SAD result alias accepts one source geometry. */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
concept has_partial_sad_alias = requires { typename SimdLib::partial_sad_result_t<element_t, bits, active_lane_count>; };

/** @brief Reports whether the checked-magnitude result alias accepts one source geometry. */
template <class element_t, std::size_t bits, std::size_t active_lane_count>
concept has_partial_checked_magnitude_alias = requires { typename SimdLib::partial_magnitude_checked_result_t<element_t, bits, active_lane_count>; };

/** @brief Selects the independently expected complete result when a useful partial target cannot be formed. */
template <class element_t, std::size_t bits, std::size_t result_lane_count,
		  bool complete =
			  result_lane_count == SimdLib::Api<bits, element_t>::element_count || (bits == 256 && result_lane_count * sizeof(element_t) * 8 <= 128)>
struct expected_partial_result;

/** @brief Provides the expected complete-register result spelling. */
template <class element_t, std::size_t bits, std::size_t result_lane_count> struct expected_partial_result<element_t, bits, result_lane_count, true>
{
	using type = SimdLib::Register<element_t, bits>;
};

/** @brief Provides the expected useful partial-register result spelling. */
template <class element_t, std::size_t bits, std::size_t result_lane_count> struct expected_partial_result<element_t, bits, result_lane_count, false>
{
	using type = SimdLib::PartialRegister<element_t, bits, result_lane_count>;
};

/** @brief Expected public result type for one meaningful contiguous result extent. */
template <class element_t, std::size_t bits, std::size_t result_lane_count>
using expected_partial_result_t = typename expected_partial_result<element_t, bits, result_lane_count>::type;

/** @brief Audits arithmetic and specialized-operation availability for one valid geometry. */
template <class element_t, std::size_t bits, std::size_t active_lane_count> [[nodiscard]] consteval bool has_complete_arithmetic_surface() noexcept
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_lane_count>;
	using api_t = typename value_t::api_type;
	using other_t = std::conditional_t<std::same_as<element_t, std::int8_t>, std::uint8_t, std::int8_t>;

	static_assert(SimdLib::IRegister::Add<value_t> == SimdLib::IApi::Add<api_t>);
	static_assert(SimdLib::IRegister::Subtract<value_t> == SimdLib::IApi::Subtract<api_t>);
	static_assert(SimdLib::IRegister::Multiply<value_t> == SimdLib::IApi::Multiply<api_t>);
	static_assert(SimdLib::IRegister::Divide<value_t> == SimdLib::IApi::Divide<api_t>);
	static_assert(SimdLib::IRegister::Modulus<value_t> == SimdLib::IApi::Modulus<api_t>);
	static_assert(SimdLib::IRegister::Negate<value_t> == SimdLib::IApi::Negate<api_t>);
	static_assert(SimdLib::IRegister::Min<value_t> == SimdLib::IApi::Min<api_t>);
	static_assert(SimdLib::IRegister::Max<value_t> == SimdLib::IApi::Max<api_t>);
	static_assert(SimdLib::IRegister::Absolute<value_t> == SimdLib::IApi::Absolute<api_t>);
	static_assert(SimdLib::IRegister::Sqrt<value_t> == SimdLib::IApi::Sqrt<api_t>);
	static_assert(SimdLib::IRegister::Average<value_t> == SimdLib::IApi::Average<api_t>);
	static_assert(SimdLib::IRegister::MultiplyAdd<value_t> == SimdLib::IApi::MultiplyAdd<api_t>);
	static_assert(SimdLib::IRegister::Magnitude<value_t> == SimdLib::IApi::Magnitude<api_t>);
	constexpr bool has_magnitude_checked = requires(value_t value) { value.magnitude_checked(); };
	static_assert(has_magnitude_checked == SimdLib::IApi::MagnitudeChecked<api_t>);
	static_assert(SimdLib::IRegister::Normalize<value_t> == SimdLib::IApi::Normalize<api_t>);
	static_assert(SimdLib::IRegister::HorizontalAdd<value_t> == SimdLib::IApi::HorizontalAdd<api_t>);
	static_assert(SimdLib::IRegister::HorizontalSubtract<value_t> == SimdLib::IApi::HorizontalSubtract<api_t>);
	static_assert(SimdLib::IRegister::MultiplyAddAdjacent<value_t> == SimdLib::IApi::MultiplyAddAdjacent<api_t>);
	static_assert(SimdLib::IRegister::MultiplyAddUnsignedSignedBytes<value_t> == SimdLib::IApi::ByteMultiplyAdd<api_t>);
	static_assert(SimdLib::IRegister::SumAbsoluteByteDifferences<value_t> == SimdLib::IApi::Sad<api_t>);
	static_assert(SimdLib::IRegister::MultiSumAbsoluteByteDifferences<value_t, 0> == SimdLib::IApi::MultiSad<api_t, 0>);
	static_assert(SimdLib::IRegister::MultiSumAbsoluteByteDifferences<value_t, 255> == SimdLib::IApi::MultiSad<api_t, 255>);
	static_assert(SimdLib::IRegister::MinPosition<value_t> == SimdLib::IApi::MinPosition<api_t>);
	static_assert(SimdLib::IRegister::MaxPosition<value_t> == SimdLib::IApi::MaxPosition<api_t>);
	static_assert(SimdLib::IRegister::AddSaturated<value_t> == SimdLib::IApi::AddSaturated<api_t>);
	static_assert(SimdLib::IRegister::SubtractSaturated<value_t> == SimdLib::IApi::SubtractSaturated<api_t>);
	static_assert(SimdLib::IRegister::HorizontalAddSaturated<value_t> == SimdLib::IApi::HorizontalAddSaturated<api_t>);
	static_assert(SimdLib::IRegister::HorizontalSubtractSaturated<value_t> == SimdLib::IApi::HorizontalSubtractSaturated<api_t>);
	static_assert(SimdLib::IRegister::AddSubtract<value_t> == SimdLib::IApi::AddSubtract<api_t>);
	static_assert(SimdLib::IRegister::DotProduct<value_t, 0> == SimdLib::IApi::DotProduct<api_t, 0>);
	static_assert(!SimdLib::IRegister::DotProduct<value_t, -1>);
	static_assert(!SimdLib::IRegister::DotProduct<value_t, 256>);
	static_assert(!SimdLib::IRegister::MultiSumAbsoluteByteDifferences<value_t, -1>);
	static_assert(!SimdLib::IRegister::MultiSumAbsoluteByteDifferences<value_t, 256>);
	static_assert(!SimdLib::IRegister::MultiplyAddAdjacent<value_t, other_t>);
	static_assert(!SimdLib::IRegister::MultiplyAddUnsignedSignedBytes<value_t, other_t>);
	static_assert(!SimdLib::IRegister::SumAbsoluteByteDifferences<value_t, other_t>);
	static_assert(!SimdLib::IRegister::MultiSumAbsoluteByteDifferences<value_t, 0, other_t>);
	return true;
}

/** @brief Audits the last invalid and first valid 256-bit active extents for one element type. */
template <class element_t> [[nodiscard]] consteval bool has_upper_group_geometry_boundary() noexcept
{
	constexpr std::size_t low_group_lane_count = 128 / (sizeof(element_t) * 8);
	static_assert(!SimdLib::PartialRegisterAvailable<element_t, 256, low_group_lane_count>);
	static_assert(SimdLib::PartialRegisterAvailable<element_t, 256, low_group_lane_count + 1>);
	return true;
}

/** @brief Audits exact specialized-result aliases for one valid integral source geometry. */
template <class element_t, std::size_t bits, std::size_t active_lane_count> [[nodiscard]] consteval bool has_exact_specialized_results() noexcept
{
	using value_t = SimdLib::PartialRegister<element_t, bits, active_lane_count>;
	using api_t = typename value_t::api_type;
	if constexpr (SimdLib::IApi::MultiplyAddAdjacent<api_t>)
	{
		using result_element_t = SimdLib::partial_multiply_add_adjacent_element_t<element_t>;
		using expected_t = expected_partial_result_t<result_element_t, bits, (active_lane_count + 1) / 2>;
		static_assert(std::same_as<SimdLib::partial_multiply_add_adjacent_result_t<element_t, bits, active_lane_count>, expected_t>);
		static_assert(std::same_as<decltype(std::declval<value_t>().multiply_add_adjacent(std::declval<value_t>())), expected_t>);
	}
	if constexpr (SimdLib::IApi::ByteMultiplyAdd<api_t>)
	{
		constexpr std::size_t result_lane_count = (active_lane_count * sizeof(element_t) + 1) / 2;
		using expected_t = expected_partial_result_t<std::int16_t, bits, result_lane_count>;
		static_assert(std::same_as<SimdLib::partial_byte_multiply_add_result_t<element_t, bits, active_lane_count>, expected_t>);
		static_assert(std::same_as<decltype(std::declval<value_t>().multiply_add_unsigned_signed_bytes(std::declval<value_t>())), expected_t>);
	}
	if constexpr (SimdLib::IApi::Sad<api_t>)
	{
		constexpr std::size_t result_lane_count = (active_lane_count * sizeof(element_t) + 7) / 8;
		using expected_t = expected_partial_result_t<std::uint64_t, bits, result_lane_count>;
		static_assert(std::same_as<SimdLib::partial_sad_result_t<element_t, bits, active_lane_count>, expected_t>);
		static_assert(std::same_as<decltype(std::declval<value_t>().sum_absolute_byte_differences(std::declval<value_t>())), expected_t>);
	}
	if constexpr (SimdLib::IApi::MagnitudeChecked<api_t>)
	{
		constexpr std::size_t group_lane_count = 128 / (sizeof(element_t) * 8);
		constexpr std::size_t result_lane_count = ((active_lane_count - 1) / group_lane_count) * group_lane_count + 2;
		using expected_t = expected_partial_result_t<element_t, bits, result_lane_count>;
		static_assert(std::same_as<SimdLib::partial_magnitude_checked_result_t<element_t, bits, active_lane_count>, expected_t>);
		static_assert(std::same_as<decltype(std::declval<value_t>().magnitude_checked()), expected_t>);
	}
	return true;
}

/** @brief Audits every valid active extent for one integral element/width cell. */
template <class element_t, std::size_t bits, std::size_t... indices>
[[nodiscard]] consteval bool has_all_exact_specialized_results(std::index_sequence<indices...>) noexcept
{
	return (
		[]<std::size_t active_lane_count>() consteval
		{
			if constexpr (SimdLib::PartialRegisterAvailable<element_t, bits, active_lane_count>)
				return has_exact_specialized_results<element_t, bits, active_lane_count>();
			else
				return true;
		}.template operator()<indices + 1>() &&
		...);
}

/** @brief Audits every valid specialized-result geometry for all integral element types at one width. */
template <std::size_t bits> [[nodiscard]] consteval bool has_exact_specialized_result_matrix() noexcept
{
#define SIMDLIB_PARTIAL_EXACT_RESULTS(element_type)                                                                                                            \
	has_all_exact_specialized_results<element_type, bits>(std::make_index_sequence<SimdLib::Api<bits, element_type>::element_count - 1>{})
	return SIMDLIB_PARTIAL_EXACT_RESULTS(std::int8_t) && SIMDLIB_PARTIAL_EXACT_RESULTS(std::uint8_t) && SIMDLIB_PARTIAL_EXACT_RESULTS(std::int16_t) &&
		   SIMDLIB_PARTIAL_EXACT_RESULTS(std::uint16_t) && SIMDLIB_PARTIAL_EXACT_RESULTS(std::int32_t) && SIMDLIB_PARTIAL_EXACT_RESULTS(std::uint32_t) &&
		   SIMDLIB_PARTIAL_EXACT_RESULTS(std::int64_t) && SIMDLIB_PARTIAL_EXACT_RESULTS(std::uint64_t);
#undef SIMDLIB_PARTIAL_EXACT_RESULTS
}

#define SIMDLIB_PARTIAL_VALIDATE_SURFACE(element_type, width, count) static_assert(has_complete_arithmetic_surface<element_type, width, count>())
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::int8_t, 128, 15);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::uint8_t, 128, 15);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::int16_t, 128, 7);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::uint16_t, 128, 7);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::int32_t, 128, 3);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::uint32_t, 128, 3);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::int64_t, 128, 1);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::uint64_t, 128, 1);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(float, 128, 3);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(double, 128, 1);
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::int8_t, 256, 17);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::uint8_t, 256, 17);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::int16_t, 256, 9);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::uint16_t, 256, 9);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::int32_t, 256, 5);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::uint32_t, 256, 5);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::int64_t, 256, 3);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(std::uint64_t, 256, 3);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(float, 256, 5);
SIMDLIB_PARTIAL_VALIDATE_SURFACE(double, 256, 3);
#endif
#undef SIMDLIB_PARTIAL_VALIDATE_SURFACE

static_assert(has_exact_specialized_result_matrix<128>());
#if SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256
static_assert(has_exact_specialized_result_matrix<256>());
static_assert(has_upper_group_geometry_boundary<std::int8_t>() && has_upper_group_geometry_boundary<std::uint8_t>());
static_assert(has_upper_group_geometry_boundary<std::int16_t>() && has_upper_group_geometry_boundary<std::uint16_t>());
static_assert(has_upper_group_geometry_boundary<std::int32_t>() && has_upper_group_geometry_boundary<std::uint32_t>());
static_assert(has_upper_group_geometry_boundary<std::int64_t>() && has_upper_group_geometry_boundary<std::uint64_t>());
static_assert(has_upper_group_geometry_boundary<float>() && has_upper_group_geometry_boundary<double>());
#endif

static_assert(!has_partial_checked_magnitude_alias<float, 128, 3>);
static_assert(!has_partial_checked_magnitude_alias<double, 128, 1>);
static_assert(!has_partial_adjacent_alias<std::int32_t, 128, 0>);
static_assert(!has_partial_byte_multiply_add_alias<std::int32_t, 128, 4>);
static_assert(!has_partial_sad_alias<std::int32_t, 128, 5>);
static_assert(!has_partial_checked_magnitude_alias<std::int32_t, 64, 1>);

} // namespace
