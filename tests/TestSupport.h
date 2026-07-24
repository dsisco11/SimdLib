#pragma once

#include "constexpr/ApiConstexprContracts.h"
#include <SimdLib/Api.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <type_traits>

namespace SimdLib::Tests
{
template <std::size_t Width, class Element> void require_addition_parity()
{
	using simd = Api<Width, Element>;
	std::array<Element, simd::element_count> lhs{};
	std::array<Element, simd::element_count> rhs{};
	std::array<Element, simd::element_count> expected{};
	for (std::size_t index = 0; index < simd::element_count; ++index)
	{
		lhs[index] = static_cast<Element>(index + 1);
		rhs[index] = static_cast<Element>(2);
		expected[index] = static_cast<Element>(index + 3);
	}
	REQUIRE(simd::to_array(simd::add(simd::load(lhs), simd::load(rhs))) == expected);
}

template <std::size_t Width> void require_supported_addition_matrix()
{
	require_addition_parity<Width, std::int8_t>();
	require_addition_parity<Width, std::uint8_t>();
	require_addition_parity<Width, std::int16_t>();
	require_addition_parity<Width, std::uint16_t>();
	require_addition_parity<Width, std::int32_t>();
	require_addition_parity<Width, std::uint32_t>();
	require_addition_parity<Width, std::int64_t>();
	require_addition_parity<Width, std::uint64_t>();
	require_addition_parity<Width, float>();
	require_addition_parity<Width, double>();
}

template <std::size_t Width, class Element> void require_transfer_contracts()
{
	using simd = Api<Width, Element>;
	alignas(Width / 8) std::array<Element, simd::element_count> aligned{};
	for (std::size_t index = 0; index < aligned.size(); ++index)
		aligned[index] = static_cast<Element>(index + 1);

	const auto aligned_register = simd::load_aligned(aligned);
	alignas(Width / 8) std::array<Element, simd::element_count> aligned_output{};
	simd::store_aligned(aligned_register, aligned_output);
	REQUIRE(aligned_output == aligned);

	alignas(64) std::array<Element, simd::element_count + 1> offset_storage{};
	std::copy(aligned.begin(), aligned.end(), offset_storage.begin() + 1);
	const std::span<const Element, simd::element_count> unaligned_input{offset_storage.data() + 1, simd::element_count};
	const auto unaligned_register = simd::load_unaligned(unaligned_input);

	alignas(64) std::array<Element, simd::element_count + 1> offset_output{};
	std::span<Element, simd::element_count> unaligned_output{offset_output.data() + 1, simd::element_count};
	simd::store_unaligned(unaligned_register, unaligned_output);
	REQUIRE(std::equal(aligned.begin(), aligned.end(), unaligned_output.begin()));

	std::array<std::byte, simd::byte_count> bytes{};
	simd::store(unaligned_register, std::span<std::byte>{bytes});
	REQUIRE(bytes.size() == simd::byte_count);
	const auto byte_loaded = simd::load(std::span<const std::byte, simd::byte_count>{bytes});
	std::array<std::byte, simd::byte_count> exact_bytes{};
	simd::store(byte_loaded, std::span<std::byte, simd::byte_count>{exact_bytes});
	for (std::size_t index = 0; index < bytes.size(); ++index)
		REQUIRE(std::to_integer<unsigned int>(exact_bytes[index]) == std::to_integer<unsigned int>(bytes[index]));
	REQUIRE(simd::to_array(byte_loaded) == aligned);

	std::array<std::byte, simd::byte_count + 8> oversized_bytes{};
	simd::store(unaligned_register, std::span<std::byte>{oversized_bytes});
	std::array<Element, simd::element_count> recovered{};
	std::memcpy(recovered.data(), oversized_bytes.data(), simd::byte_count);
	REQUIRE(recovered == aligned);
}

template <std::size_t Width> void require_supported_transfer_matrix()
{
	require_transfer_contracts<Width, std::int8_t>();
	require_transfer_contracts<Width, std::uint8_t>();
	require_transfer_contracts<Width, std::int16_t>();
	require_transfer_contracts<Width, std::uint16_t>();
	require_transfer_contracts<Width, std::int32_t>();
	require_transfer_contracts<Width, std::uint32_t>();
	require_transfer_contracts<Width, std::int64_t>();
	require_transfer_contracts<Width, std::uint64_t>();
	require_transfer_contracts<Width, float>();
	require_transfer_contracts<Width, double>();
}

template <std::size_t Width, class Element> void require_partial_transfer_contracts()
{
	using simd = Api<Width, Element>;
	alignas(64) std::array<Element, simd::element_count + 1> storage{};
	for (std::size_t index = 0; index < simd::element_count; ++index)
		storage[index + 1] = static_cast<Element>(index + 1);

	const std::span<const Element> unaligned{storage.data() + 1, simd::element_count};
	const auto none = simd::template load_partial<0>(unaligned);
	REQUIRE(simd::to_array(none) == std::array<Element, simd::element_count>{});

	const auto one = simd::template load_partial<1>(unaligned);
	auto expected_one = std::array<Element, simd::element_count>{};
	expected_one[0] = storage[1];
	REQUIRE(simd::to_array(one) == expected_one);

	const auto almost_full = simd::template load_partial<simd::element_count - 1>(unaligned);
	auto expected_almost_full = std::array<Element, simd::element_count>{};
	std::copy_n(storage.begin() + 1, simd::element_count - 1, expected_almost_full.begin());
	REQUIRE(simd::to_array(almost_full) == expected_almost_full);

	const auto full = simd::template load_partial<simd::element_count>(unaligned);
	std::array<Element, simd::element_count> expected_full{};
	std::copy_n(storage.begin() + 1, simd::element_count, expected_full.begin());
	REQUIRE(simd::to_array(full) == expected_full);
}

template <std::size_t Width> void require_supported_partial_transfer_matrix()
{
	require_partial_transfer_contracts<Width, std::int8_t>();
	require_partial_transfer_contracts<Width, std::uint16_t>();
	require_partial_transfer_contracts<Width, std::int32_t>();
	require_partial_transfer_contracts<Width, std::uint64_t>();
	require_partial_transfer_contracts<Width, float>();
}

template <std::size_t Width, class Element>
	requires std::is_arithmetic_v<Element>
void require_comparison_contract()
{
	using simd = Api<Width, Element>;
	std::array<Element, simd::element_count> lhs{};
	std::array<Element, simd::element_count> rhs{};
	for (std::size_t index = 0; index < simd::element_count; ++index)
	{
		switch (index % 4)
		{
		case 0:
			lhs[index] = Element{0};
			rhs[index] = Element{0};
			break;
		case 1:
			lhs[index] = Element{1};
			rhs[index] = Element{2};
			break;
		case 2:
			lhs[index] = Element{3};
			rhs[index] = Element{2};
			break;
		default:
			lhs[index] = std::numeric_limits<Element>::max();
			rhs[index] = std::numeric_limits<Element>::lowest();
			break;
		}
	}

	typename simd::mask_t eq = 0;
	typename simd::mask_t gt = 0;
	typename simd::mask_t ge = 0;
	typename simd::mask_t lt = 0;
	typename simd::mask_t le = 0;
	typename simd::mask_t eqSlim = 0;
	typename simd::mask_t gtSlim = 0;
	typename simd::mask_t geSlim = 0;
	typename simd::mask_t ltSlim = 0;
	typename simd::mask_t leSlim = 0;
	std::array<Element, simd::element_count> selected{};
	constexpr typename simd::mask_t lane_mask = static_cast<typename simd::mask_t>((typename simd::mask_t{1} << sizeof(Element)) - 1);
	for (std::size_t index = 0; index < simd::element_count; ++index)
	{
		const auto mask = static_cast<typename simd::mask_t>(lane_mask << (index * sizeof(Element)));
		if (lhs[index] == rhs[index])
		{
			eq |= mask;
			eqSlim |= typename simd::mask_t{1} << index;
		}
		if (lhs[index] > rhs[index])
		{
			gt |= mask;
			gtSlim |= typename simd::mask_t{1} << index;
		}
		if (lhs[index] >= rhs[index])
		{
			ge |= mask;
			geSlim |= typename simd::mask_t{1} << index;
		}
		if (lhs[index] < rhs[index])
		{
			lt |= mask;
			ltSlim |= typename simd::mask_t{1} << index;
		}
		if (lhs[index] <= rhs[index])
		{
			le |= mask;
			leSlim |= typename simd::mask_t{1} << index;
		}
		selected[index] = lhs[index] == rhs[index] ? lhs[index] : rhs[index];
	}

	const auto left = simd::construct(lhs);
	const auto right = simd::construct(rhs);
	REQUIRE(simd::cmp_eq_mask(left, right) == eq);
	REQUIRE(simd::cmp_gt_mask(left, right) == gt);
	REQUIRE(simd::cmp_ge_mask(left, right) == ge);
	REQUIRE(simd::cmp_lt_mask(left, right) == lt);
	REQUIRE(simd::cmp_le_mask(left, right) == le);
	REQUIRE(simd::cmp_eq_slim(left, right) == eqSlim);
	REQUIRE(simd::cmp_gt_slim(left, right) == gtSlim);
	REQUIRE(simd::cmp_ge_slim(left, right) == geSlim);
	REQUIRE(simd::cmp_lt_slim(left, right) == ltSlim);
	REQUIRE(simd::cmp_le_slim(left, right) == leSlim);
	REQUIRE(simd::to_array(simd::select(simd::compare_equal(left, right), left, right)) == selected);
}

template <std::size_t Width> void require_supported_comparison_matrix()
{
	require_comparison_contract<Width, std::int8_t>();
	require_comparison_contract<Width, std::uint8_t>();
	require_comparison_contract<Width, std::int16_t>();
	require_comparison_contract<Width, std::uint16_t>();
	require_comparison_contract<Width, std::int32_t>();
	require_comparison_contract<Width, std::uint32_t>();
	require_comparison_contract<Width, std::int64_t>();
	require_comparison_contract<Width, std::uint64_t>();
}

template <std::size_t Width, std::integral Element, std::size_t Count> void require_transform_pack_mask_contract()
{
	using simd = Api<Width, Element>;
	using write_t = typename simd::template packed_element_t<1>;
	constexpr std::size_t output_count = simd::template packed_element_count<1, Count>;

	std::array<Element, Count> input{};
	for (std::size_t index = 0; index < Count; ++index)
		input[index] = index % 3 == 1 ? Element{0} : static_cast<Element>(index + 1);

	std::array<write_t, output_count + 2> guarded{};
	guarded.fill(static_cast<write_t>(0xA5));
	std::span<write_t, output_count> output{guarded.data() + 1, output_count};
	const auto predicate = simd::set1(Element{0});
	simd::template transform_pack<1>(std::span<const Element, Count>{input}, output,
									 [&predicate](const typename simd::vector_t value) noexcept { return simd::movemask_slim(simd::cmpeq(value, predicate)); });

	std::array<write_t, output_count> expected{};
	for (std::size_t index = 0; index < Count; ++index)
		if (input[index] == 0)
			expected[index / 8] |= static_cast<write_t>(write_t{1} << (index % 8));

	REQUIRE(std::equal(output.begin(), output.end(), expected.begin()));
	REQUIRE(guarded.front() == static_cast<write_t>(0xA5));
	REQUIRE(guarded.back() == static_cast<write_t>(0xA5));
}

template <std::size_t Width, std::unsigned_integral Element, std::size_t Count, std::size_t ResultBitWidth> void require_transform_pack_width_contract()
{
	static_assert(ResultBitWidth > 0 && ResultBitWidth <= 64);
	using simd = Api<Width, Element>;
	using write_t = typename simd::template packed_element_t<ResultBitWidth>;
	constexpr std::size_t output_count = simd::template packed_element_count<ResultBitWidth, Count>;
	constexpr std::size_t write_element_width = std::numeric_limits<write_t>::digits;
	constexpr std::uint64_t result_mask = ResultBitWidth == 64 ? std::numeric_limits<std::uint64_t>::max() : (std::uint64_t{1} << ResultBitWidth) - 1;

	std::array<Element, Count> input{};
	for (std::size_t index = 0; index < Count; ++index)
		input[index] = static_cast<Element>(index * 5 + 3);

	std::array<write_t, output_count + 2> guarded{};
	guarded.fill(static_cast<write_t>(0xA5));
	std::span<write_t, output_count> output{guarded.data() + 1, output_count};
	simd::template transform_pack<ResultBitWidth>(std::span<const Element, Count>{input}, output,
												  [](const typename simd::vector_t value) noexcept
												  {
													  const auto lanes = simd::to_array(value);
													  std::uint64_t packed = 0;
													  for (std::size_t lane = 0; lane < lanes.size(); ++lane)
														  packed |= (static_cast<std::uint64_t>(lanes[lane]) & result_mask) << (lane * ResultBitWidth);
													  return packed;
												  });

	std::array<write_t, output_count> expected{};
	for (std::size_t index = 0; index < Count; ++index)
	{
		const std::uint64_t result = static_cast<std::uint64_t>(input[index]) & result_mask;
		for (std::size_t bit = 0; bit < ResultBitWidth; ++bit)
		{
			const std::size_t output_bit = index * ResultBitWidth + bit;
			if ((result & (std::uint64_t{1} << bit)) != 0)
				expected[output_bit / write_element_width] |= static_cast<write_t>(write_t{1} << (output_bit % write_element_width));
		}
	}

	REQUIRE(std::equal(output.begin(), output.end(), expected.begin()));
	REQUIRE(guarded.front() == static_cast<write_t>(0xA5));
	REQUIRE(guarded.back() == static_cast<write_t>(0xA5));
}

/**
 * @brief Verifies a packed transform whose one-register result fills exactly one native output word.
 * @tparam Width SIMD register width in bits.
 *
 * This is intentionally separate from the general width matrix: it documents the no-shift-by-64
 * boundary and requires the accumulator flush that writes a complete native word.
 */
template <std::size_t Width> void require_transform_pack_full_native_word_contract()
{
	using simd = Api<Width, std::uint64_t>;
	constexpr std::size_t resultBitWidth = 64 / simd::element_count;
	require_transform_pack_width_contract<Width, std::uint64_t, simd::element_count, resultBitWidth>();
}

/**
 * @brief Adds a fixed scalar amount to every lane of a 32-bit SIMD register.
 * @tparam Width SIMD register width in bits.
 */
template <std::size_t Width> struct Add17Transform
{
	using simd = Api<Width, std::uint32_t>;

	/** @brief Applies the transform to one register. */
	[[nodiscard]] typename simd::vector_t operator()(const typename simd::vector_t value) const noexcept
	{
		return simd::add(value, simd::set1(17));
	}
};

/**
 * @brief Subtracts a fixed scalar amount from every lane of a 32-bit SIMD register.
 * @tparam Width SIMD register width in bits.
 */
template <std::size_t Width> struct Subtract13Transform
{
	using simd = Api<Width, std::uint32_t>;

	/** @brief Applies the transform to one register. */
	[[nodiscard]] typename simd::vector_t operator()(const typename simd::vector_t value) const noexcept
	{
		return simd::subtract(value, simd::set1(13));
	}
};

/**
 * @brief Subtracts corresponding lanes of two 32-bit SIMD registers.
 * @tparam Width SIMD register width in bits.
 */
template <std::size_t Width> struct SubtractTransform
{
	using simd = Api<Width, std::uint32_t>;

	/** @brief Applies the transform to two registers. */
	[[nodiscard]] typename simd::vector_t operator()(const typename simd::vector_t lhs, const typename simd::vector_t rhs) const noexcept
	{
		return simd::subtract(lhs, rhs);
	}
};

/**
 * @brief Verifies all public unary and binary transform overloads for one span extent.
 * @tparam Width SIMD register width in bits.
 * @tparam Count Number of logical elements in each source and destination span.
 */
template <std::size_t Width, std::size_t Count> void require_transform_overload_case()
{
	using simd = Api<Width, std::uint32_t>;
	constexpr std::uint32_t guard = 0xDEADBEEFU;
	constexpr std::size_t storageCount = Count + 2 > simd::element_count + 1 ? Count + 2 : simd::element_count + 1;
	std::array<std::uint32_t, storageCount> unaryStorage{};
	std::array<std::uint32_t, storageCount> leftStorage{};
	std::array<std::uint32_t, storageCount> rightStorage{};
	std::array<std::uint32_t, storageCount> outputStorage{};
	unaryStorage.fill(guard);
	leftStorage.fill(guard);
	rightStorage.fill(guard);
	outputStorage.fill(guard);

	auto unary = std::span<std::uint32_t>(unaryStorage).subspan(1, Count);
	auto left = std::span<std::uint32_t>(leftStorage).subspan(1, Count);
	auto right = std::span<std::uint32_t>(rightStorage).subspan(1, Count);
	auto output = std::span<std::uint32_t>(outputStorage).subspan(1, Count);
	for (std::size_t index = 0; index < Count; ++index)
	{
		unary[index] = static_cast<std::uint32_t>(index * 7 + 5);
		left[index] = static_cast<std::uint32_t>(index * 7 + 50);
		right[index] = static_cast<std::uint32_t>(index + 3);
	}

	simd::transform(unary, Add17Transform<Width>{});

	for (std::size_t index = 0; index < Count; ++index)
		REQUIRE(unary[index] == static_cast<std::uint32_t>(index * 7 + 22));
	REQUIRE(unaryStorage.front() == guard);
	REQUIRE(unaryStorage[Count + 1] == guard);

	simd::transform(std::span<const std::uint32_t>(left), output, Subtract13Transform<Width>{});

	for (std::size_t index = 0; index < Count; ++index)
		REQUIRE(output[index] == static_cast<std::uint32_t>(index * 7 + 37));
	REQUIRE(outputStorage.front() == guard);
	REQUIRE(outputStorage[Count + 1] == guard);

	std::fill(output.begin(), output.end(), guard);
	simd::transform(std::span<const std::uint32_t>(left), std::span<const std::uint32_t>(right), output, SubtractTransform<Width>{});

	for (std::size_t index = 0; index < Count; ++index)
		REQUIRE(output[index] == static_cast<std::uint32_t>(index * 6 + 47));
	REQUIRE(outputStorage.front() == guard);
	REQUIRE(outputStorage[Count + 1] == guard);
}

/**
 * @brief Verifies public transform overloads across empty, tail, full-register, and multi-register extents.
 * @tparam Width SIMD register width in bits.
 */
template <std::size_t Width> void require_transform_overload_contract()
{
	constexpr std::size_t laneCount = Api<Width, std::uint32_t>::element_count;
	require_transform_overload_case<Width, 0>();
	require_transform_overload_case<Width, 1>();
	require_transform_overload_case<Width, laneCount>();
	require_transform_overload_case<Width, laneCount + 1>();
	require_transform_overload_case<Width, laneCount * 2>();
}

template <std::size_t Width, class Element> constexpr auto movemask_test_bytes()
{
	std::array<std::uint8_t, Width / 8> bytes{};
	for (std::size_t index = 0; index < bytes.size(); ++index)
		bytes[index] = static_cast<std::uint8_t>((index * 19u) | (index % 3u == 1u ? 0u : 0x80u));
	return bytes;
}

template <std::size_t Width, class Element> constexpr auto movemask_test_values()
{
	using simd = Api<Width, Element>;
	constexpr auto bytes = movemask_test_bytes<Width, Element>();
	static_assert(sizeof(bytes) == sizeof(std::array<Element, simd::element_count>));
	return std::bit_cast<std::array<Element, simd::element_count>>(bytes);
}

template <std::size_t Width, class Element> constexpr auto expected_byte_movemask()
{
	using simd = Api<Width, Element>;
	constexpr auto bytes = movemask_test_bytes<Width, Element>();
	typename simd::mask_t result = 0;
	for (std::size_t index = 0; index < bytes.size(); ++index)
		result |= static_cast<typename simd::mask_t>((bytes[index] >> 7) & 1u) << index;
	return result;
}

template <std::size_t Width, class Element> constexpr auto expected_slim_movemask()
{
	using simd = Api<Width, Element>;
	constexpr auto bytes = movemask_test_bytes<Width, Element>();
	typename simd::mask_t result = 0;
	for (std::size_t index = 0; index < simd::element_count; ++index)
	{
		const std::size_t sign_byte = (index + 1) * sizeof(Element) - 1;
		result |= static_cast<typename simd::mask_t>((bytes[sign_byte] >> 7) & 1u) << index;
	}
	return result;
}

template <std::size_t Width, class Element> void require_movemask_contract()
{
	using simd = Api<Width, Element>;
	const auto value = simd::construct(movemask_test_values<Width, Element>());
	REQUIRE(simd::movemask(value) == expected_byte_movemask<Width, Element>());
	REQUIRE(simd::movemask_slim(value) == expected_slim_movemask<Width, Element>());
}

template <std::size_t Width> void require_supported_movemask_matrix()
{
	require_movemask_contract<Width, std::int8_t>();
	require_movemask_contract<Width, std::uint8_t>();
	require_movemask_contract<Width, std::int16_t>();
	require_movemask_contract<Width, std::uint16_t>();
	require_movemask_contract<Width, std::int32_t>();
	require_movemask_contract<Width, std::uint32_t>();
	require_movemask_contract<Width, std::int64_t>();
	require_movemask_contract<Width, std::uint64_t>();
	require_movemask_contract<Width, float>();
	require_movemask_contract<Width, double>();
}

/**
 * @brief Compares constant evaluation with optimized runtime dispatch using volatile-derived inputs.
 * @tparam Width SIMD register width in bits.
 */
template <std::size_t Width> void require_constexpr_runtime_parity()
{
	using simd = Api<Width, std::int32_t>;
	constexpr auto lhsConstant = Constexpr::lane_values<Width, std::int32_t>();
	constexpr auto rhsConstant = Constexpr::comparison_values<Width, std::int32_t>(1);
	constexpr auto expected = Constexpr::evaluate_api_contract<Width>(lhsConstant, rhsConstant);
	std::array<std::int32_t, simd::element_count> lhsRuntime{};
	std::array<std::int32_t, simd::element_count> rhsRuntime{};
	for (std::size_t index = 0; index < simd::element_count; ++index)
	{
		volatile std::int32_t lhsValue = lhsConstant[index];
		volatile std::int32_t rhsValue = rhsConstant[index];
		lhsRuntime[index] = lhsValue;
		rhsRuntime[index] = rhsValue;
	}
	REQUIRE(Constexpr::evaluate_api_contract<Width>(lhsRuntime, rhsRuntime) == expected);

	constexpr auto shiftSource = simd::set1(-8);
	constexpr int laneWidth = static_cast<int>(simd::element_width);
	constexpr auto expectedLogicalLeft = simd::to_array(simd::shift_left(shiftSource, laneWidth));
	constexpr auto expectedLogicalRight = simd::to_array(simd::shift_right(shiftSource, laneWidth));
	constexpr auto expectedArithmeticRight = simd::to_array(simd::shift_right_arithmetic(shiftSource, laneWidth));
	volatile int runtimeShift = laneWidth;
	REQUIRE(simd::to_array(simd::shift_left(shiftSource, runtimeShift)) == expectedLogicalLeft);
	REQUIRE(simd::to_array(simd::shift_right(shiftSource, runtimeShift)) == expectedLogicalRight);
	REQUIRE(simd::to_array(simd::shift_right_arithmetic(shiftSource, runtimeShift)) == expectedArithmeticRight);
}
/**
 * @brief Verifies public extrema values and first-position tie semantics for an integer Api specialization.
 *
 * @tparam Width The Api register width.
 * @tparam Element The signed or unsigned integer lane type.
 */
template <std::size_t Width, std::integral Element> void require_extrema_position_contract()
{
	using simd = Api<Width, Element>;
	std::array<Element, simd::element_count> values{};
	std::array<Element, simd::element_count> other{};
	for (std::size_t index = 0; index < values.size(); ++index)
	{
		values[index] = static_cast<Element>(index + 3);
		other[index] = static_cast<Element>(index + 1);
	}
	values[0] = std::numeric_limits<Element>::max();
	if constexpr (simd::element_count > 1)
		values[1] = std::numeric_limits<Element>::lowest();
	if constexpr (simd::element_count > 2)
		values[2] = std::numeric_limits<Element>::lowest();
	if constexpr (simd::element_count > 3)
		values[3] = std::numeric_limits<Element>::max();

	std::array<Element, simd::element_count> expected_min{};
	std::array<Element, simd::element_count> expected_max{};
	for (std::size_t index = 0; index < values.size(); ++index)
	{
		expected_min[index] = std::min(values[index], other[index]);
		expected_max[index] = std::max(values[index], other[index]);
	}
	const auto value = simd::construct(values);
	REQUIRE(simd::to_array(simd::min(value, simd::construct(other))) == expected_min);
	REQUIRE(simd::to_array(simd::max(value, simd::construct(other))) == expected_max);
	REQUIRE(simd::min_position(value) == 1);
	REQUIRE(simd::max_position(value) == 0);
	std::array<Element, simd::element_count> reversed{};
	reversed.fill(static_cast<Element>(1));
	reversed.front() = std::numeric_limits<Element>::lowest();
	reversed.back() = std::numeric_limits<Element>::max();
	const auto reversed_value = simd::construct(reversed);
	REQUIRE(simd::min_position(reversed_value) == 0);
	REQUIRE(simd::max_position(reversed_value) == simd::element_count - 1);
	REQUIRE(simd::min_position(simd::set1(std::numeric_limits<Element>::lowest())) == 0);
	REQUIRE(simd::max_position(simd::set1(std::numeric_limits<Element>::max())) == 0);
}

/**
 * @brief Exercises extrema contracts for every supported public integer lane family.
 *
 * @tparam Width The Api register width.
 */
template <std::size_t Width> void require_integer_extrema_position_matrix()
{
	require_extrema_position_contract<Width, std::int8_t>();
	require_extrema_position_contract<Width, std::uint8_t>();
	require_extrema_position_contract<Width, std::int16_t>();
	require_extrema_position_contract<Width, std::uint16_t>();
	require_extrema_position_contract<Width, std::int32_t>();
	require_extrema_position_contract<Width, std::uint32_t>();
	require_extrema_position_contract<Width, std::int64_t>();
	require_extrema_position_contract<Width, std::uint64_t>();
}
/**
 * @brief Verifies public 64-bit signed and unsigned arithmetic behavior.
 *
 * @tparam Width The Api register width.
 */
template <std::size_t Width> void require_64bit_arithmetic_contract()
{
	using signed_simd = Api<Width, std::int64_t>;
	const auto signed_value = signed_simd::set1(-9);
	const auto signed_divisor = signed_simd::set1(2);
	REQUIRE(signed_simd::to_array(signed_simd::divide(signed_value, signed_divisor))[0] == -4);
	REQUIRE(signed_simd::to_array(signed_simd::modulus(signed_value, signed_divisor))[0] == -1);
	REQUIRE(signed_simd::to_array(signed_simd::multiply(signed_value, signed_divisor))[0] == -18);
	REQUIRE(signed_simd::to_array(signed_simd::absolute(signed_value))[0] == 9);
	REQUIRE(signed_simd::to_array(signed_simd::shift_right_arithmetic(signed_value, 1))[0] == -5);
	REQUIRE(signed_simd::to_array(signed_simd::min(signed_value, signed_divisor))[0] == -9);
	REQUIRE(signed_simd::to_array(signed_simd::max(signed_value, signed_divisor))[0] == 2);

	using unsigned_simd = Api<Width, std::uint64_t>;
	const auto unsigned_value = unsigned_simd::set1(0x8000'0000'0000'0003ULL);
	const auto unsigned_divisor = unsigned_simd::set1(3);
	REQUIRE(unsigned_simd::to_array(unsigned_simd::divide(unsigned_value, unsigned_divisor))[0] == 0x2AAA'AAAA'AAAA'AAABL);
	REQUIRE(unsigned_simd::to_array(unsigned_simd::modulus(unsigned_value, unsigned_divisor))[0] == 2);
	REQUIRE(unsigned_simd::to_array(unsigned_simd::multiply(unsigned_value, unsigned_divisor))[0] == 0x8000'0000'0000'0009ULL);
	REQUIRE(unsigned_simd::to_array(unsigned_simd::absolute(unsigned_value))[0] == 0x8000'0000'0000'0003ULL);
	REQUIRE(unsigned_simd::to_array(unsigned_simd::shift_right(unsigned_value, 1))[0] == 0x4000'0000'0000'0001ULL);
	REQUIRE(unsigned_simd::to_array(unsigned_simd::min(unsigned_value, unsigned_divisor))[0] == 3);
	REQUIRE(unsigned_simd::to_array(unsigned_simd::max(unsigned_value, unsigned_divisor))[0] == 0x8000'0000'0000'0003ULL);
}

/**
 * @brief Verifies arithmetic, bitwise, lane-access, and shift behavior for one integer Api specialization.
 *
 * @tparam Width The Api register width.
 * @tparam Element The signed or unsigned integer lane type.
 */
template <std::size_t Width, std::integral Element> void require_integer_operation_contract()
{
	using simd = Api<Width, Element>;
	using unsigned_t = std::make_unsigned_t<Element>;
	std::array<Element, simd::element_count> lhs{};
	std::array<Element, simd::element_count> rhs{};
	std::array<Element, simd::element_count> sum{};
	std::array<Element, simd::element_count> difference{};
	std::array<Element, simd::element_count> product{};
	std::array<Element, simd::element_count> quotient{};
	std::array<Element, simd::element_count> remainder{};
	std::array<Element, simd::element_count> bit_and{};
	std::array<Element, simd::element_count> bit_or{};
	std::array<Element, simd::element_count> bit_xor{};
	std::array<Element, simd::element_count> bit_andnot{};
	std::array<Element, simd::element_count> bit_not{};
	std::array<Element, simd::element_count> shifted_left{};
	std::array<Element, simd::element_count> shifted_right{};
	for (std::size_t index = 0; index < simd::element_count; ++index)
	{
		lhs[index] = static_cast<Element>(index + 6);
		rhs[index] = static_cast<Element>(index % 3 + 1);
		sum[index] = static_cast<Element>(lhs[index] + rhs[index]);
		difference[index] = static_cast<Element>(lhs[index] - rhs[index]);
		product[index] = static_cast<Element>(lhs[index] * rhs[index]);
		quotient[index] = static_cast<Element>(lhs[index] / rhs[index]);
		remainder[index] = static_cast<Element>(lhs[index] % rhs[index]);
		const auto lhs_bits = static_cast<unsigned_t>(lhs[index]);
		const auto rhs_bits = static_cast<unsigned_t>(rhs[index]);
		bit_and[index] = static_cast<Element>(lhs_bits & rhs_bits);
		bit_or[index] = static_cast<Element>(lhs_bits | rhs_bits);
		bit_xor[index] = static_cast<Element>(lhs_bits ^ rhs_bits);
		bit_andnot[index] = static_cast<Element>((~lhs_bits) & rhs_bits);
		bit_not[index] = static_cast<Element>(~lhs_bits);
		shifted_left[index] = static_cast<Element>(lhs_bits << 1);
		shifted_right[index] = static_cast<Element>(lhs_bits >> 1);
	}

	const auto left = simd::construct(lhs);
	const auto right = simd::construct(rhs);
	REQUIRE(simd::to_array(simd::add(left, right)) == sum);
	REQUIRE(simd::to_array(simd::subtract(left, right)) == difference);
	REQUIRE(simd::to_array(simd::multiply(left, right)) == product);
	REQUIRE(simd::to_array(simd::divide(left, right)) == quotient);
	REQUIRE(simd::to_array(simd::modulus(left, right)) == remainder);
	REQUIRE(simd::to_array(simd::bitwise_and(left, right)) == bit_and);
	REQUIRE(simd::to_array(simd::bitwise_or(left, right)) == bit_or);
	REQUIRE(simd::to_array(simd::bitwise_xor(left, right)) == bit_xor);
	REQUIRE(simd::to_array(simd::bitwise_andnot(left, right)) == bit_andnot);
	REQUIRE(simd::to_array(simd::bitwise_not(left)) == bit_not);
	REQUIRE(simd::to_array(simd::shift_left(left, 1)) == shifted_left);
	REQUIRE(simd::to_array(simd::shift_right(left, 1)) == shifted_right);
	REQUIRE(simd::to_array(simd::min(left, right)) == rhs);
	REQUIRE(simd::to_array(simd::max(left, right)) == lhs);

	const auto absolute_source = simd::set1(static_cast<Element>(std::is_signed_v<Element> ? -7 : 7));
	REQUIRE(simd::to_array(simd::absolute(absolute_source)) == simd::to_array(simd::set1(7)));
	if constexpr (std::is_signed_v<Element>)
		REQUIRE(simd::to_array(simd::shift_right_arithmetic(absolute_source, 1)) == simd::to_array(simd::set1(-4)));

	REQUIRE(simd::get_element(left, 0) == lhs[0]);
	const auto replacement = static_cast<Element>(42);
	const auto replaced = simd::set_element(left, static_cast<int>(simd::element_count - 1), replacement);
	auto expected_replaced = lhs;
	expected_replaced.back() = replacement;
	REQUIRE(simd::to_array(replaced) == expected_replaced);
}

/**
 * @brief Exercises the public integer operation contract for every supported lane type.
 *
 * @tparam Width The Api register width.
 */
template <std::size_t Width> void require_integer_operation_matrix()
{
	require_integer_operation_contract<Width, std::int8_t>();
	require_integer_operation_contract<Width, std::uint8_t>();
	require_integer_operation_contract<Width, std::int16_t>();
	require_integer_operation_contract<Width, std::uint16_t>();
	require_integer_operation_contract<Width, std::int32_t>();
	require_integer_operation_contract<Width, std::uint32_t>();
	require_integer_operation_contract<Width, std::int64_t>();
	require_integer_operation_contract<Width, std::uint64_t>();
}

/**
 * @brief Verifies public floating arithmetic, comparison, bitwise, and lane-access behavior.
 *
 * @tparam Width The Api register width.
 * @tparam Element The floating-point lane type.
 */
template <std::size_t Width, std::floating_point Element> void require_floating_operation_contract()
{
	using simd = Api<Width, Element>;
	using bits_t = std::conditional_t<sizeof(Element) == 4, std::uint32_t, std::uint64_t>;
	std::array<Element, simd::element_count> lhs{};
	std::array<Element, simd::element_count> rhs{};
	std::array<Element, simd::element_count> sum{};
	std::array<Element, simd::element_count> difference{};
	std::array<Element, simd::element_count> product{};
	std::array<Element, simd::element_count> quotient{};
	std::array<Element, simd::element_count> minimum{};
	std::array<Element, simd::element_count> maximum{};
	std::array<Element, simd::element_count> absolute{};
	std::array<Element, simd::element_count> negated{};
	for (std::size_t index = 0; index < simd::element_count; ++index)
	{
		lhs[index] = index == 0 ? static_cast<Element>(-3.5) : static_cast<Element>(index + 2);
		rhs[index] = static_cast<Element>(2);
		sum[index] = lhs[index] + rhs[index];
		difference[index] = lhs[index] - rhs[index];
		product[index] = lhs[index] * rhs[index];
		quotient[index] = lhs[index] / rhs[index];
		minimum[index] = std::min(lhs[index], rhs[index]);
		maximum[index] = std::max(lhs[index], rhs[index]);
		absolute[index] = lhs[index] < Element{0} ? -lhs[index] : lhs[index];
		negated[index] = -lhs[index];
	}

	const auto left = simd::construct(lhs);
	const auto right = simd::construct(rhs);
	std::array<Element, simd::element_count> broadcast{};
	broadcast.fill(static_cast<Element>(2.5));
	REQUIRE(simd::to_array(simd::set1(static_cast<Element>(2.5))) == broadcast);
	REQUIRE(simd::to_array(simd::add(left, right)) == sum);
	REQUIRE(simd::to_array(simd::subtract(left, right)) == difference);
	REQUIRE(simd::to_array(simd::multiply(left, right)) == product);
	REQUIRE(simd::to_array(simd::divide(left, right)) == quotient);
	REQUIRE(simd::to_array(simd::min(left, right)) == minimum);
	REQUIRE(simd::to_array(simd::max(left, right)) == maximum);
	REQUIRE(simd::to_array(simd::absolute(left)) == absolute);
	REQUIRE(simd::to_array(simd::negate(left)) == negated);
	REQUIRE(simd::get_element(left, 0) == lhs[0]);
	const auto replaced = simd::set_element(left, static_cast<int>(simd::element_count - 1), static_cast<Element>(-9.25));
	auto expected_replaced = lhs;
	expected_replaced.back() = static_cast<Element>(-9.25);
	REQUIRE(simd::to_array(replaced) == expected_replaced);

	std::array<bits_t, simd::element_count> lhs_bits{};
	std::array<bits_t, simd::element_count> rhs_bits{};
	std::array<bits_t, simd::element_count> expected_and{};
	std::array<bits_t, simd::element_count> expected_or{};
	std::array<bits_t, simd::element_count> expected_xor{};
	std::array<bits_t, simd::element_count> expected_andnot{};
	std::array<bits_t, simd::element_count> expected_not{};
	for (std::size_t index = 0; index < simd::element_count; ++index)
	{
		lhs_bits[index] = static_cast<bits_t>(bits_t{0x55} << (index % sizeof(bits_t)));
		rhs_bits[index] = static_cast<bits_t>(~bits_t{0}) ^ static_cast<bits_t>(bits_t{0x11} << (index % sizeof(bits_t)));
		expected_and[index] = lhs_bits[index] & rhs_bits[index];
		expected_or[index] = lhs_bits[index] | rhs_bits[index];
		expected_xor[index] = lhs_bits[index] ^ rhs_bits[index];
		expected_andnot[index] = (~lhs_bits[index]) & rhs_bits[index];
		expected_not[index] = ~lhs_bits[index];
	}
	const auto bit_left = simd::construct(std::bit_cast<std::array<Element, simd::element_count>>(lhs_bits));
	const auto bit_right = simd::construct(std::bit_cast<std::array<Element, simd::element_count>>(rhs_bits));
	REQUIRE(std::bit_cast<std::array<bits_t, simd::element_count>>(simd::to_array(simd::bitwise_and(bit_left, bit_right))) == expected_and);
	REQUIRE(std::bit_cast<std::array<bits_t, simd::element_count>>(simd::to_array(simd::bitwise_or(bit_left, bit_right))) == expected_or);
	REQUIRE(std::bit_cast<std::array<bits_t, simd::element_count>>(simd::to_array(simd::bitwise_xor(bit_left, bit_right))) == expected_xor);
	REQUIRE(std::bit_cast<std::array<bits_t, simd::element_count>>(simd::to_array(simd::bitwise_andnot(bit_left, bit_right))) == expected_andnot);
	REQUIRE(std::bit_cast<std::array<bits_t, simd::element_count>>(simd::to_array(simd::bitwise_not(bit_left))) == expected_not);
}

/**
 * @brief Exercises public floating operations for both supported lane types.
 *
 * @tparam Width The Api register width.
 */
template <std::size_t Width> void require_floating_operation_matrix()
{
	require_floating_operation_contract<Width, float>();
	require_floating_operation_contract<Width, double>();
	require_comparison_contract<Width, float>();
	require_comparison_contract<Width, double>();
}

/**
 * @brief Verifies unsigned 32-bit conversion, division, and remainder boundary behavior.
 *
 * @tparam Width The Api register width.
 */
template <std::size_t Width> void require_unsigned_32bit_contract()
{
	using integers = Api<Width, std::uint32_t>;
	using floats = Api<Width, float>;
	constexpr std::array<std::uint32_t, 8> numerators{0, 1, 7, 0x7FFF'FFFFU, 0x8000'0000U, 0xFFFF'FFFFU, 4'000'000'001U, 10};
	constexpr std::array<std::uint32_t, 8> divisors{1, 1, 3, 7, 2, 65'535, 3, 4};
	std::array<std::uint32_t, integers::element_count> lhs{};
	std::array<std::uint32_t, integers::element_count> rhs{};
	std::array<std::uint32_t, integers::element_count> quotient{};
	std::array<std::uint32_t, integers::element_count> remainder{};
	std::array<float, integers::element_count> converted{};
	for (std::size_t index = 0; index < integers::element_count; ++index)
	{
		lhs[index] = numerators[index];
		rhs[index] = divisors[index];
		quotient[index] = lhs[index] / rhs[index];
		remainder[index] = lhs[index] % rhs[index];
		converted[index] = static_cast<float>(lhs[index]);
	}
	const auto left = integers::construct(lhs);
	const auto right = integers::construct(rhs);
	REQUIRE(integers::to_array(integers::divide(left, right)) == quotient);
	REQUIRE(integers::to_array(integers::modulus(left, right)) == remainder);
	REQUIRE(floats::to_array(integers::convert_to_float(left)) == converted);
}

/**
 * @brief Verifies uint64_t adjacent multiply-add ordering and modulo-2^64 overflow.
 *
 * @tparam Width The Api register width.
 */
template <std::size_t Width> void require_uint64_multiply_add_adjacent_contract()
{
	using simd = Api<Width, std::uint64_t>;
	std::array<std::uint64_t, simd::element_count> lhs{};
	std::array<std::uint64_t, simd::element_count> rhs{};
	for (std::size_t index = 0; index < simd::element_count; index += 2)
	{
		lhs[index] = std::numeric_limits<std::uint64_t>::max();
		lhs[index + 1] = static_cast<std::uint64_t>(index + 2);
		rhs[index] = 2;
		rhs[index + 1] = 3;
	}
	std::array<std::uint64_t, simd::element_count> expected{};
	for (std::size_t index = 0; index < simd::element_count; index += 2)
		expected[index] = lhs[index] * rhs[index] + lhs[index + 1] * rhs[index + 1];
	REQUIRE(simd::to_array(simd::multiply_add_adjacent(simd::construct(lhs), simd::construct(rhs))) == expected);
}

/**
 * @brief Verifies signed 32-bit conversion in both directions.
 *
 * @tparam Width The Api register width.
 */
template <std::size_t Width> void require_signed_32bit_conversion_contract()
{
	using integers = Api<Width, std::int32_t>;
	using floats = Api<Width, float>;
	constexpr std::array<std::int32_t, 8> source_values{-7, 0, 42, 1'000'000, -1024, 16'777'216, 9, -3};
	std::array<std::int32_t, integers::element_count> integers_source{};
	std::array<float, integers::element_count> floats_expected{};
	std::array<float, integers::element_count> floats_source{};
	std::array<std::int32_t, integers::element_count> integers_expected{};
	for (std::size_t index = 0; index < integers::element_count; ++index)
	{
		integers_source[index] = source_values[index];
		floats_expected[index] = static_cast<float>(source_values[index]);
		integers_expected[index] = static_cast<std::int32_t>(index) - 3;
		floats_source[index] = static_cast<float>(integers_expected[index]);
	}
	REQUIRE(floats::to_array(integers::convert_to_float(integers::construct(integers_source))) == floats_expected);
	REQUIRE(integers::to_array(floats::convert_to_int(floats::construct(floats_source))) == integers_expected);
}

/**
 * @brief Exercises transform_pack through every supported integer Api specialization.
 *
 * @tparam Width The Api register width.
 */
template <std::size_t Width> void require_transform_pack_type_matrix()
{
	require_transform_pack_mask_contract<Width, std::int8_t, Api<Width, std::int8_t>::element_count + 3>();
	require_transform_pack_mask_contract<Width, std::uint8_t, Api<Width, std::uint8_t>::element_count + 3>();
	require_transform_pack_mask_contract<Width, std::int16_t, Api<Width, std::int16_t>::element_count + 3>();
	require_transform_pack_mask_contract<Width, std::uint16_t, Api<Width, std::uint16_t>::element_count + 3>();
	require_transform_pack_mask_contract<Width, std::int32_t, Api<Width, std::int32_t>::element_count + 3>();
	require_transform_pack_mask_contract<Width, std::uint32_t, Api<Width, std::uint32_t>::element_count + 3>();
	require_transform_pack_mask_contract<Width, std::int64_t, Api<Width, std::int64_t>::element_count + 3>();
	require_transform_pack_mask_contract<Width, std::uint64_t, Api<Width, std::uint64_t>::element_count + 3>();
}

/**
 * @brief Verifies every lane produced by a documentation example.
 * @tparam Simd Api facade used to decode the raw register.
 * @tparam Vector Raw SIMD register type.
 * @tparam Expected Fixed-size container holding the documented values.
 * @param value Raw register produced by the documented invocation.
 * @param expected Values shown in the documentation.
 */
template <class Simd, class Vector, class Expected> void require_documented_register(const Vector value, const Expected &expected)
{
	const auto actual = Simd::to_array(value);
	STATIC_REQUIRE(std::tuple_size_v<decltype(actual)> == std::tuple_size_v<Expected>);
	for (std::size_t index = 0; index < actual.size(); ++index)
		REQUIRE(actual[index] == static_cast<typename Simd::element_type>(expected[index]));
}

} // namespace SimdLib::Tests
