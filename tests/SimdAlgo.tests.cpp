#include <SimdLib/SimdAlgo.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace
{
/**
 * @brief Verifies packed comparison parity and destination boundaries for one static extent.
 * @tparam ReadWidth Source element width in bits.
 * @tparam Count Static source element count.
 */
template <std::size_t ReadWidth, std::size_t Count>
void require_compare_tail_contract()
{
	using Algo = SimdLib::SimdAlgo<ReadWidth, 1>;
	using read_t = typename Algo::read_t;
	using write_t = typename Algo::write_t;
	static_assert(Count % 8 == 0);

	std::array<read_t, Count> input{};
	constexpr read_t predicate = static_cast<read_t>(7);
	for (std::size_t index = 0; index < Count; ++index)
		input[index] = index % 3 == 1 ? predicate : static_cast<read_t>(index + 20);

	std::array<write_t, Count / 8 + 2> guarded{};
	guarded.fill(static_cast<write_t>(0xA5));
	std::span<write_t, Count / 8> output{guarded.data() + 1, Count / 8};
	Algo::Compare(std::span<const read_t, Count>{input}, output, predicate);

	std::array<write_t, Count / 8> expected{};
	for (std::size_t index = 0; index < Count; ++index)
		if (input[index] == predicate)
			expected[index / 8] |= static_cast<write_t>(write_t{1} << (index % 8));

	REQUIRE(std::equal(output.begin(), output.end(), expected.begin()));
	REQUIRE(guarded.front() == static_cast<write_t>(0xA5));
	REQUIRE(guarded.back() == static_cast<write_t>(0xA5));
}

/**
 * @brief Verifies every full-register and tail outcome of AnyEqual.
 * @tparam ReadWidth Source element width in bits.
 */
template <std::size_t ReadWidth>
void require_any_equal_outcome_contract()
{
	using Algo = SimdLib::SimdAlgo<ReadWidth, 1>;
	using read_t = typename Algo::read_t;
	using simd = SimdLib::Api<256, read_t>;
	constexpr std::size_t lanes = simd::element_count;
	constexpr std::size_t full_count = lanes * 3;
	constexpr std::size_t tail_count = full_count + 3;
	constexpr read_t predicate = static_cast<read_t>(7);
	constexpr read_t other = static_cast<read_t>(3);

	std::array<read_t, full_count> full{};
	full.fill(other);
	REQUIRE_FALSE(Algo::AnyEqual(std::span<const read_t, full_count>{full}, predicate));
	REQUIRE(Algo::AnyEqual(std::span<const read_t, full_count>{full}, predicate) ==
		std::ranges::any_of(full, [](const read_t value) { return value == predicate; }));

	full.front() = predicate;
	REQUIRE(Algo::AnyEqual(std::span<const read_t, full_count>{full}, predicate));
	full.front() = other;
	full[lanes + lanes / 2] = predicate;
	REQUIRE(Algo::AnyEqual(std::span<const read_t, full_count>{full}, predicate));
	full[lanes + lanes / 2] = other;
	full.back() = predicate;
	REQUIRE(Algo::AnyEqual(std::span<const read_t, full_count>{full}, predicate));

	std::array<read_t, tail_count> tail{};
	tail.fill(other);
	REQUIRE_FALSE(Algo::AnyEqual(std::span<const read_t, tail_count>{tail}, predicate));
	tail.back() = predicate;
	REQUIRE(Algo::AnyEqual(std::span<const read_t, tail_count>{tail}, predicate));
	REQUIRE(Algo::AnyEqual(std::span<const read_t, tail_count>{tail}, predicate) ==
		std::ranges::any_of(tail, [](const read_t value) { return value == predicate; }));
}

/**
 * @brief Verifies every full-register and tail outcome of AllEqual.
 * @tparam ReadWidth Source element width in bits.
 */
template <std::size_t ReadWidth>
void require_all_equal_outcome_contract()
{
	using Algo = SimdLib::SimdAlgo<ReadWidth, 1>;
	using read_t = typename Algo::read_t;
	using simd = SimdLib::Api<256, read_t>;
	constexpr std::size_t lanes = simd::element_count;
	constexpr std::size_t full_count = lanes * 3;
	constexpr std::size_t tail_count = full_count + 3;
	constexpr read_t predicate = static_cast<read_t>(7);
	constexpr read_t other = static_cast<read_t>(3);

	std::array<read_t, full_count> full{};
	full.fill(predicate);
	REQUIRE(Algo::AllEqual(std::span<const read_t, full_count>{full}, predicate));
	REQUIRE(Algo::AllEqual(std::span<const read_t, full_count>{full}, predicate) ==
		std::ranges::all_of(full, [](const read_t value) { return value == predicate; }));

	full.front() = other;
	REQUIRE_FALSE(Algo::AllEqual(std::span<const read_t, full_count>{full}, predicate));
	full.front() = predicate;
	full[lanes + lanes / 2] = other;
	REQUIRE_FALSE(Algo::AllEqual(std::span<const read_t, full_count>{full}, predicate));
	full[lanes + lanes / 2] = predicate;
	full.back() = other;
	REQUIRE_FALSE(Algo::AllEqual(std::span<const read_t, full_count>{full}, predicate));

	std::array<read_t, tail_count> tail{};
	tail.fill(predicate);
	REQUIRE(Algo::AllEqual(std::span<const read_t, tail_count>{tail}, predicate));
	tail.back() = other;
	REQUIRE_FALSE(Algo::AllEqual(std::span<const read_t, tail_count>{tail}, predicate));
	REQUIRE(Algo::AllEqual(std::span<const read_t, tail_count>{tail}, predicate) ==
		std::ranges::all_of(tail, [](const read_t value) { return value == predicate; }));
}

/**
 * @brief Verifies empty, single-element, multi-element, exact-register, and tail static extents.
 * @tparam ReadWidth Source element width in bits.
 */
template <std::size_t ReadWidth>
void require_search_extent_contract()
{
	using Algo = SimdLib::SimdAlgo<ReadWidth, 1>;
	using read_t = typename Algo::read_t;
	using simd = SimdLib::Api<128, read_t>;
	constexpr read_t predicate = static_cast<read_t>(7);
	constexpr read_t other = static_cast<read_t>(3);

	const std::array<read_t, 0> empty{};
	REQUIRE_FALSE(Algo::AnyEqual(std::span<const read_t, 0>{empty}, predicate));
	REQUIRE(Algo::AllEqual(std::span<const read_t, 0>{empty}, predicate));

	const std::array<read_t, 1> single{predicate};
	REQUIRE(Algo::AnyEqual(std::span<const read_t, 1>{single}, predicate));
	REQUIRE(Algo::AllEqual(std::span<const read_t, 1>{single}, predicate));

	const std::array<read_t, 3> multiple{other, predicate, other};
	REQUIRE(Algo::AnyEqual(std::span<const read_t, 3>{multiple}, predicate));
	REQUIRE_FALSE(Algo::AllEqual(std::span<const read_t, 3>{multiple}, predicate));

	std::array<read_t, simd::element_count> exact{};
	exact.fill(predicate);
	REQUIRE(Algo::AnyEqual(std::span<const read_t, simd::element_count>{exact}, predicate));
	REQUIRE(Algo::AllEqual(std::span<const read_t, simd::element_count>{exact}, predicate));

	std::array<read_t, simd::element_count + 1> tail{};
	tail.fill(other);
	tail.back() = predicate;
	REQUIRE(Algo::AnyEqual(std::span<const read_t, simd::element_count + 1>{tail}, predicate));
	REQUIRE_FALSE(Algo::AllEqual(std::span<const read_t, simd::element_count + 1>{tail}, predicate));
}

} // namespace

TEST_CASE("SimdAlgo fixed spans preserve equality and packed comparison semantics", "[simdlib][algo]")
{
	using Algo = SimdLib::SimdAlgo<8, 8>;
	const std::array<std::uint8_t, 8> eight{3, 7, 3, 0, 3, 9, 3, 3};
	REQUIRE(Algo::AnyEqual(std::span<const std::uint8_t, 8>(eight), std::uint8_t{7}));
	REQUIRE_FALSE(Algo::AnyEqual(std::span<const std::uint8_t, 8>(eight), std::uint8_t{8}));
	REQUIRE_FALSE(Algo::AllEqual(std::span<const std::uint8_t, 8>(eight), std::uint8_t{3}));

	std::array<std::uint8_t, 1> packed{};
	Algo::Compare(std::span<const std::uint8_t, 8>(eight), std::span<std::uint8_t, 1>(packed), std::uint8_t{3});
	REQUIRE(packed[0] == 0b11010101);

	const std::array<std::uint8_t, 32> uniform = []
	{
		std::array<std::uint8_t, 32> value{};
		value.fill(42);
		return value;
	}();
	REQUIRE(Algo::AnyEqual(std::span<const std::uint8_t, 32>(uniform), std::uint8_t{42}));
	REQUIRE(Algo::AllEqual(std::span<const std::uint8_t, 32>(uniform), std::uint8_t{42}));
}

TEST_CASE("SimdAlgo AnyEqual covers every full-register and tail outcome", "[simdlib][algo][search][tail]")
{
	require_any_equal_outcome_contract<8>();
	require_any_equal_outcome_contract<16>();
	require_any_equal_outcome_contract<32>();
	require_any_equal_outcome_contract<64>();
}

TEST_CASE("SimdAlgo AllEqual covers every full-register and tail outcome", "[simdlib][algo][search][tail]")
{
	require_all_equal_outcome_contract<8>();
	require_all_equal_outcome_contract<16>();
	require_all_equal_outcome_contract<32>();
	require_all_equal_outcome_contract<64>();
}

TEST_CASE("SimdAlgo fixed searches cover empty single exact-lane and non-lane-multiple extents", "[simdlib][algo][search]")
{
	require_search_extent_contract<8>();
	require_search_extent_contract<16>();
	require_search_extent_contract<32>();
	require_search_extent_contract<64>();
}

TEST_CASE("SimdAlgo packed comparisons overwrite exact tail output without overread or overwrite", "[simdlib][algo][compare][tail]")
{
	require_compare_tail_contract<8, 24>();
	require_compare_tail_contract<8, 40>();
	require_compare_tail_contract<16, 24>();
	require_compare_tail_contract<16, 40>();
	require_compare_tail_contract<32, 24>();
	require_compare_tail_contract<32, 40>();
	require_compare_tail_contract<64, 24>();
	require_compare_tail_contract<64, 40>();
}

TEST_CASE("SimdAlgo fixed bitwise operations match scalar references including tails", "[simdlib][algo]")
{
	using Algo = SimdLib::SimdAlgo<32, 8>;
	std::array<std::uint32_t, 9> lhs{};
	std::array<std::uint32_t, 9> rhs{};
	std::array<std::uint32_t, 11> guarded{};
	guarded.fill(0xA5A5'A5A5u);
	std::span<std::uint32_t, 9> output{guarded.data() + 1, 9};
	for (std::size_t index = 0; index < lhs.size(); ++index)
	{
		lhs[index] = 0xF0F00000u + static_cast<std::uint32_t>(index);
		rhs[index] = 0x0FF00FF0u ^ static_cast<std::uint32_t>(index * 17);
	}

	Algo::BitwiseAnd(std::span<const std::uint32_t, 9>(lhs), std::span<const std::uint32_t, 9>(rhs), std::span<std::uint32_t, 9>(output));
	for (std::size_t index = 0; index < lhs.size(); ++index)
		REQUIRE(output[index] == (lhs[index] & rhs[index]));
	REQUIRE(guarded.front() == 0xA5A5'A5A5u);
	REQUIRE(guarded.back() == 0xA5A5'A5A5u);
	Algo::BitwiseOr(std::span<const std::uint32_t, 9>(lhs), std::span<const std::uint32_t, 9>(rhs), std::span<std::uint32_t, 9>(output));
	for (std::size_t index = 0; index < lhs.size(); ++index)
		REQUIRE(output[index] == (lhs[index] | rhs[index]));
	REQUIRE(guarded.front() == 0xA5A5'A5A5u);
	REQUIRE(guarded.back() == 0xA5A5'A5A5u);
	Algo::BitwiseXor(std::span<const std::uint32_t, 9>(lhs), std::span<const std::uint32_t, 9>(rhs), std::span<std::uint32_t, 9>(output));
	for (std::size_t index = 0; index < lhs.size(); ++index)
		REQUIRE(output[index] == (lhs[index] ^ rhs[index]));
	REQUIRE(guarded.front() == 0xA5A5'A5A5u);
	REQUIRE(guarded.back() == 0xA5A5'A5A5u);
	Algo::BitwiseNot(std::span<const std::uint32_t, 9>(lhs), std::span<std::uint32_t, 9>(output));
	for (std::size_t index = 0; index < lhs.size(); ++index)
		REQUIRE(output[index] == ~lhs[index]);
	REQUIRE(guarded.front() == 0xA5A5'A5A5u);
	REQUIRE(guarded.back() == 0xA5A5'A5A5u);
	Algo::BitwiseAndNot(std::span<const std::uint32_t, 9>(lhs), std::span<const std::uint32_t, 9>(rhs), std::span<std::uint32_t, 9>(output));
	for (std::size_t index = 0; index < lhs.size(); ++index)
		REQUIRE(output[index] == ((~lhs[index]) & rhs[index]));
	REQUIRE(guarded.front() == 0xA5A5'A5A5u);
	REQUIRE(guarded.back() == 0xA5A5'A5A5u);
}

TEST_CASE("SimdAlgo dynamic spans select 128 and 256 bit execution without semantic drift", "[simdlib][algo]")
{
	using Algo = SimdLib::SimdAlgo<32, 8>;
	for (const std::size_t count : {std::size_t{3}, std::size_t{8}, std::size_t{9}, std::size_t{65}})
	{
		std::vector<std::uint32_t> lhs(count);
		std::vector<std::uint32_t> rhs(count);
		std::vector<std::uint32_t> output(count);
		for (std::size_t index = 0; index < count; ++index)
		{
			lhs[index] = static_cast<std::uint32_t>(index * 0x10203u);
			rhs[index] = static_cast<std::uint32_t>(0xFFFFFFFFu - index * 31u);
		}

		Algo::BitwiseAnd(std::span<const std::uint32_t>(lhs), std::span<const std::uint32_t>(rhs), std::span<std::uint32_t>(output));
		for (std::size_t index = 0; index < count; ++index)
			REQUIRE(output[index] == (lhs[index] & rhs[index]));
		Algo::BitwiseOr(std::span<const std::uint32_t>(lhs), std::span<const std::uint32_t>(rhs), std::span<std::uint32_t>(output));
		for (std::size_t index = 0; index < count; ++index)
			REQUIRE(output[index] == (lhs[index] | rhs[index]));
		Algo::BitwiseXor(std::span<const std::uint32_t>(lhs), std::span<const std::uint32_t>(rhs), std::span<std::uint32_t>(output));
		for (std::size_t index = 0; index < count; ++index)
			REQUIRE(output[index] == (lhs[index] ^ rhs[index]));
		Algo::BitwiseNot(std::span<const std::uint32_t>(lhs), std::span<std::uint32_t>(output));
		for (std::size_t index = 0; index < count; ++index)
			REQUIRE(output[index] == ~lhs[index]);
		Algo::BitwiseAndNot(std::span<const std::uint32_t>(lhs), std::span<const std::uint32_t>(rhs), std::span<std::uint32_t>(output));
		for (std::size_t index = 0; index < count; ++index)
			REQUIRE(output[index] == ((~lhs[index]) & rhs[index]));
	}
}
