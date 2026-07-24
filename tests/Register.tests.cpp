#include <SimdLib/Register.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>
#include <utility>

namespace
{

/** @brief Creates distinctive, exactly representable values for every lane. */
template <class register_t> [[nodiscard]] constexpr auto lane_values() noexcept
{
	std::array<typename register_t::element_type, register_t::lane_count> result{};
	for (std::size_t index = 0; index < result.size(); ++index)
		result[index] = static_cast<typename register_t::element_type>(index + 1);
	if constexpr (std::is_integral_v<typename register_t::element_type>)
	{
		result.front() = std::numeric_limits<typename register_t::element_type>::lowest();
		result.back() = std::numeric_limits<typename register_t::element_type>::max();
	}
	else
	{
		result.front() = static_cast<typename register_t::element_type>(-3.5);
		result.back() = static_cast<typename register_t::element_type>(7.25);
	}
	return result;
}

/** @brief Verifies every compile-time-selected lane against its source value. */
template <std::size_t index = 0, class register_t>
void require_all_lanes(const register_t value, const std::array<typename register_t::element_type, register_t::lane_count> &expected)
{
	if constexpr (index < register_t::lane_count)
	{
		REQUIRE(value.template lane<index>() == expected[index]);
		require_all_lanes<index + 1>(value, expected);
	}
}

/** @brief Constructs a register from an expanded low-to-high lane array. */
template <class register_t, std::size_t... indices>
[[nodiscard]] constexpr register_t from_lanes(const std::array<typename register_t::element_type, register_t::lane_count> &values,
											  std::index_sequence<indices...>) noexcept
{
	return register_t::from_lanes(values[indices]...);
}

/** @brief Verifies construction, observation, and lane replacement for one register type. */
template <class element_t, std::size_t bits> void require_value_contracts()
{
	using register_type = SimdLib::Register<element_t, bits>;
	const auto values = lane_values<register_type>();
	const std::array<element_t, register_type::lane_count> zeros{};

	REQUIRE(register_type{}.to_array() == zeros);
	REQUIRE(register_type::zero().to_array() == zeros);
	REQUIRE(register_type::broadcast(static_cast<element_t>(7)).to_array() ==
			[]
			{
				std::array<element_t, register_type::lane_count> result{};
				result.fill(static_cast<element_t>(7));
				return result;
			}());
	REQUIRE(register_type::from_array(values).to_array() == values);
	REQUIRE(from_lanes<register_type>(values, std::make_index_sequence<register_type::lane_count>{}).to_array() == values);

	const register_type wrapped{register_type::api_type::construct(values)};
	REQUIRE(register_type::api_type::to_array(wrapped.native) == values);
	require_all_lanes(wrapped, values);

	const auto first_replaced = wrapped.template with_lane<0>(static_cast<element_t>(41)).to_array();
	const auto last_replaced = wrapped.template with_lane<register_type::lane_count - 1>(static_cast<element_t>(43)).to_array();
	for (std::size_t index = 0; index < values.size(); ++index)
	{
		REQUIRE(first_replaced[index] == (index == 0 ? static_cast<element_t>(41) : values[index]));
		REQUIRE(last_replaced[index] == (index + 1 == values.size() ? static_cast<element_t>(43) : values[index]));
	}
}

/** @brief Verifies exact-width aligned, unaligned, and raw-byte transfers with canaries. */
template <class element_t, std::size_t bits> void require_transfer_contracts()
{
	using register_type = SimdLib::Register<element_t, bits>;
	const auto values = lane_values<register_type>();

	alignas(register_type::byte_count) std::array<element_t, register_type::lane_count> aligned_source = values;
	alignas(register_type::byte_count) std::array<element_t, register_type::lane_count> aligned_destination{};
	register_type::load_aligned(std::span<const element_t, register_type::lane_count>{aligned_source})
		.store_aligned(std::span<element_t, register_type::lane_count>{aligned_destination});
	REQUIRE(aligned_destination == values);

	alignas(register_type::byte_count) std::array<element_t, register_type::lane_count + 2> unaligned_source{};
	alignas(register_type::byte_count) std::array<element_t, register_type::lane_count + 2> unaligned_destination{};
	unaligned_source.front() = static_cast<element_t>(91);
	unaligned_source.back() = static_cast<element_t>(93);
	unaligned_destination.front() = static_cast<element_t>(95);
	unaligned_destination.back() = static_cast<element_t>(97);
	for (std::size_t index = 0; index < values.size(); ++index)
		unaligned_source[index + 1] = values[index];
	const auto loaded = register_type::load(std::span<const element_t, register_type::lane_count>{unaligned_source.data() + 1, register_type::lane_count});
	loaded.store(std::span<element_t, register_type::lane_count>{unaligned_destination.data() + 1, register_type::lane_count});
	REQUIRE(unaligned_destination.front() == static_cast<element_t>(95));
	REQUIRE(unaligned_destination.back() == static_cast<element_t>(97));
	for (std::size_t index = 0; index < values.size(); ++index)
		REQUIRE(unaligned_destination[index + 1] == values[index]);

	std::array<std::byte, register_type::byte_count> source_bytes{};
	for (std::size_t index = 0; index < source_bytes.size(); ++index)
		source_bytes[index] = static_cast<std::byte>((index * 37U + 11U) & 0xFFU);
	std::array<std::byte, register_type::byte_count + 2> destination_bytes{};
	destination_bytes.front() = std::byte{0xA5};
	destination_bytes.back() = std::byte{0x5A};
	register_type::load_bytes(std::span<const std::byte, register_type::byte_count>{source_bytes})
		.store_bytes(std::span<std::byte, register_type::byte_count>{destination_bytes.data() + 1, register_type::byte_count});
	REQUIRE(std::to_integer<unsigned int>(destination_bytes.front()) == 0xA5U);
	REQUIRE(std::to_integer<unsigned int>(destination_bytes.back()) == 0x5AU);
	for (std::size_t index = 0; index < source_bytes.size(); ++index)
		REQUIRE(std::to_integer<unsigned int>(destination_bytes[index + 1]) == std::to_integer<unsigned int>(source_bytes[index]));
}

/** @brief Runs all Register value and transfer contracts for one scalar type. */
template <class element_t> void require_type_contracts()
{
	require_value_contracts<element_t, 128>();
	require_value_contracts<element_t, 256>();
	require_transfer_contracts<element_t, 128>();
	require_transfer_contracts<element_t, 256>();
}

/** @brief Returns a compact low-bit mask for one RegisterMask geometry. */
template <class mask_t> [[nodiscard]] constexpr typename mask_t::bits_type logical_bits() noexcept
{
	if constexpr (mask_t::lane_count == std::numeric_limits<typename mask_t::bits_type>::digits)
		return std::numeric_limits<typename mask_t::bits_type>::max();
	else
		return (typename mask_t::bits_type{1} << mask_t::lane_count) - 1;
}

/** @brief Verifies canonical predicate bits, Boolean reductions, combination, and selection. */
template <class element_t, std::size_t bits> void require_mask_contracts()
{
	using register_type = SimdLib::Register<element_t, bits>;
	using mask_type = typename register_type::mask_type;
	using bits_type = typename mask_type::bits_type;
	constexpr bits_type all_bits = logical_bits<mask_type>();
	constexpr bits_type alternating_bits = []() constexpr noexcept
	{
		bits_type result = 0;
		for (std::size_t index = 0; index < mask_type::lane_count; index += 2)
			result |= bits_type{1} << index;
		return result;
	}();

	std::array<element_t, register_type::lane_count> left{};
	std::array<element_t, register_type::lane_count> right{};
	for (std::size_t index = 0; index < left.size(); ++index)
	{
		left[index] = static_cast<element_t>((index % 2) == 0 ? 2 : 0);
		right[index] = static_cast<element_t>(1);
	}
	const register_type lhs = register_type::from_array(left);
	const register_type rhs = register_type::from_array(right);
	const auto alternating = lhs.compare_greater(rhs);
	const auto inverse = lhs.compare_less(rhs);
	const auto all_true = lhs.compare_equal(lhs);
	const mask_type rewrapped{alternating.native};

	REQUIRE(mask_type{}.bits() == 0);
	REQUIRE(mask_type{}.none());
	REQUIRE_FALSE(mask_type{}.any());
	REQUIRE_FALSE(mask_type{}.all());
	REQUIRE(alternating.bits() == alternating_bits);
	REQUIRE(rewrapped.bits() == alternating_bits);
	REQUIRE(alternating.any());
	REQUIRE_FALSE(alternating.all());
	REQUIRE(all_true.bits() == all_bits);
	REQUIRE(all_true.all());
	REQUIRE((alternating | inverse).bits() == all_bits);
	REQUIRE((alternating & inverse).none());
	REQUIRE((alternating ^ inverse).bits() == all_bits);
	REQUIRE((~alternating).bits() == (all_bits ^ alternating_bits));

	auto reassigned = alternating;
	reassigned = reassigned & all_true;
	REQUIRE(reassigned.bits() == alternating_bits);
	reassigned = reassigned | inverse;
	REQUIRE(reassigned.all());
	reassigned = reassigned ^ inverse;
	REQUIRE(reassigned.bits() == alternating_bits);

	std::array<element_t, register_type::lane_count> first_left{};
	std::array<element_t, register_type::lane_count> first_right{};
	first_left.front() = static_cast<element_t>(1);
	first_right.back() = static_cast<element_t>(1);
	const auto first_only = register_type::from_array(first_left).compare_greater(register_type::zero());
	const auto highest_only = register_type::from_array(first_right).compare_greater(register_type::zero());
	REQUIRE(first_only.bits() == bits_type{1});
	REQUIRE(highest_only.bits() == (bits_type{1} << (register_type::lane_count - 1)));
	REQUIRE((first_only | highest_only).bits() == (bits_type{1} | (bits_type{1} << (register_type::lane_count - 1))));
	REQUIRE(((first_only | highest_only).bits() & ~all_bits) == 0);

	const auto selected =
		alternating.select(register_type::broadcast(static_cast<element_t>(11)), register_type::broadcast(static_cast<element_t>(22))).to_array();
	for (std::size_t index = 0; index < selected.size(); ++index)
		REQUIRE(selected[index] == static_cast<element_t>((index % 2) == 0 ? 11 : 22));

	REQUIRE(lhs.compare_greater_equal(rhs).bits() == alternating_bits);
	REQUIRE(lhs.compare_less_equal(rhs).bits() == (all_bits ^ alternating_bits));
	REQUIRE((lhs == lhs));
	REQUIRE_FALSE(lhs != lhs);
	REQUIRE_FALSE(lhs == rhs);
	REQUIRE(lhs != rhs);

	const auto native_lanes = register_type::api_type::to_array(alternating.native);
	for (std::size_t lane = 0; lane < native_lanes.size(); ++lane)
	{
		const auto bytes = std::bit_cast<std::array<std::uint8_t, sizeof(element_t)>>(native_lanes[lane]);
		for (const auto byte : bytes)
			REQUIRE(byte == ((lane % 2) == 0 ? 0xFFU : 0x00U));
	}
}

/** @brief Verifies signed or unsigned high-bit ordering for one integer geometry. */
template <class element_t, std::size_t bits>
	requires std::is_integral_v<element_t>
void require_integer_ordering()
{
	using register_type = SimdLib::Register<element_t, bits>;
	const auto low = register_type::broadcast(std::numeric_limits<element_t>::lowest());
	const auto high = register_type::broadcast(std::numeric_limits<element_t>::max());
	REQUIRE(high.compare_greater(low).all());
	REQUIRE(low.compare_less(high).all());
}

/** @brief Verifies ordered floating comparison behavior for NaNs and signed zero. */
template <class element_t, std::size_t bits>
	requires std::is_floating_point_v<element_t>
void require_floating_comparison_edges()
{
	using register_type = SimdLib::Register<element_t, bits>;
	const auto nan = register_type::broadcast(std::numeric_limits<element_t>::quiet_NaN());
	const auto one = register_type::broadcast(static_cast<element_t>(1));
	REQUIRE(nan.compare_equal(nan).none());
	REQUIRE(nan.compare_greater(one).none());
	REQUIRE(nan.compare_greater_equal(one).none());
	REQUIRE(nan.compare_less(one).none());
	REQUIRE(nan.compare_less_equal(one).none());
	REQUIRE(nan != nan);
	const auto positive_zero = register_type::broadcast(static_cast<element_t>(0.0));
	const auto negative_zero = register_type::broadcast(static_cast<element_t>(-0.0));
	REQUIRE(positive_zero.compare_equal(negative_zero).all());
	REQUIRE(positive_zero == negative_zero);
}

/** @brief Runs all mask and comparison contracts for one scalar type. */
template <class element_t> void require_mask_type_contracts()
{
	require_mask_contracts<element_t, 128>();
	require_mask_contracts<element_t, 256>();
	if constexpr (std::is_integral_v<element_t>)
	{
		require_integer_ordering<element_t, 128>();
		require_integer_ordering<element_t, 256>();
	}
	else
	{
		require_floating_comparison_edges<element_t, 128>();
		require_floating_comparison_edges<element_t, 256>();
	}
}

TEST_CASE("Register construction and exact-width transfers preserve every lane and surrounding canaries", "[simdlib][register][avx2][transfer]")
{
	require_type_contracts<std::int8_t>();
	require_type_contracts<std::uint8_t>();
	require_type_contracts<std::int16_t>();
	require_type_contracts<std::uint16_t>();
	require_type_contracts<std::int32_t>();
	require_type_contracts<std::uint32_t>();
	require_type_contracts<std::int64_t>();
	require_type_contracts<std::uint64_t>();
	require_type_contracts<float>();
	require_type_contracts<double>();
}

TEST_CASE("RegisterMask comparisons, reductions, combinations, and selection preserve lane semantics", "[simdlib][register][mask][comparison][avx2]")
{
	require_mask_type_contracts<std::int8_t>();
	require_mask_type_contracts<std::uint8_t>();
	require_mask_type_contracts<std::int16_t>();
	require_mask_type_contracts<std::uint16_t>();
	require_mask_type_contracts<std::int32_t>();
	require_mask_type_contracts<std::uint32_t>();
	require_mask_type_contracts<std::int64_t>();
	require_mask_type_contracts<std::uint64_t>();
	require_mask_type_contracts<float>();
	require_mask_type_contracts<double>();
}

} // namespace
