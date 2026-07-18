#include <SimdLib/SimdVector.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <type_traits>

namespace
{
template <class Vector, std::size_t Count>
void require_lanes(const Vector& value, const std::array<typename Vector::simd::element_type, Count>& expected)
{
	const auto actual = value.toArray();
	for (std::size_t index = 0; index < Count; ++index)
		REQUIRE(actual[index] == expected[index]);
	for (std::size_t index = Count; index < actual.size(); ++index)
		REQUIRE(actual[index] == 0);
}
}

TEST_CASE("SimdVector exposes the complete aliases and storage facade", "[simdlib][vector]")
{
	static_assert(std::same_as<SimdLib::uint8x16, SimdLib::SimdVector<std::uint8_t, 16>>);
	static_assert(std::same_as<SimdLib::uint64x4, SimdLib::SimdVector<std::uint64_t, 4>>);
	static_assert(std::same_as<SimdLib::int32x8, SimdLib::SimdVector<std::int32_t, 8>>);

	SimdLib::SimdVector<std::int32_t, 3> value(4, -7, 11);
	require_lanes(value, std::array<std::int32_t, 3>{4, -7, 11});
	value.x() = 9;
	value.z() = -3;
	REQUIRE(value[0] == 9);
	REQUIRE(value[1] == -7);
	REQUIRE(value[2] == -3);
	REQUIRE(value.getSpan().size() == 4);
	REQUIRE(static_cast<std::array<std::int32_t, 4>>(value)[3] == 0);
}

TEST_CASE("SimdVector arithmetic scalar and operator facade preserves inactive lanes", "[simdlib][vector][partial]")
{
	using Vector = SimdLib::SimdVector<std::int32_t, 3>;
	const Vector lhs(8, -12, 21);
	const Vector rhs(2, 3, -7);

	require_lanes(Vector(lhs + rhs.getRegister()), std::array<std::int32_t, 3>{10, -9, 14});
	require_lanes(Vector(lhs - rhs.getRegister()), std::array<std::int32_t, 3>{6, -15, 28});
	require_lanes(Vector(lhs * rhs.getRegister()), std::array<std::int32_t, 3>{16, -36, -147});
	require_lanes(Vector(lhs / rhs.getRegister()), std::array<std::int32_t, 3>{4, -4, -3});
	require_lanes(Vector(lhs % rhs.getRegister()), std::array<std::int32_t, 3>{0, 0, 0});
	require_lanes(Vector(lhs + 5), std::array<std::int32_t, 3>{13, -7, 26});
	require_lanes(Vector(lhs << 1), std::array<std::int32_t, 3>{16, -24, 42});

	Vector accumulated = lhs;
	accumulated += rhs.getRegister();
	accumulated -= 1;
	REQUIRE(accumulated == Vector(9, -10, 13).getRegister());
	REQUIRE(accumulated.any_greater(Vector(8, -11, 13).getRegister()));
	REQUIRE_FALSE(accumulated.all_greater(Vector(8, -11, 13).getRegister()));
	REQUIRE(accumulated.all_less_equal(Vector(9, -9, 13).getRegister()));
}

TEST_CASE("SimdVector bitwise saturation widening and hash match logical lanes", "[simdlib][vector][partial]")
{
	using Bytes = SimdLib::SimdVector<std::uint8_t, 3>;
	const Bytes lhs(0xF0, 250, 3);
	const Bytes rhs(0x0F, 10, 5);
	require_lanes(Bytes(lhs & rhs.getRegister()), std::array<std::uint8_t, 3>{0, 10, 1});
	require_lanes(Bytes(lhs | rhs.getRegister()), std::array<std::uint8_t, 3>{0xFF, 250, 7});
	require_lanes(Bytes(lhs ^ rhs.getRegister()), std::array<std::uint8_t, 3>{0xFF, 240, 6});
	require_lanes(Bytes(lhs.add_saturated(rhs.getRegister())), std::array<std::uint8_t, 3>{255, 255, 8});
	require_lanes(Bytes(rhs.subtract_saturated(lhs.getRegister())), std::array<std::uint8_t, 3>{0, 0, 2});

	const SimdLib::SimdVector<std::int16_t, 3> narrow(-4, 7, 300);
	const SimdLib::SimdVector<std::int32_t, 3> wide(narrow);
	require_lanes(wide, std::array<std::int32_t, 3>{-4, 7, 300});

	const auto sameHash = std::hash<SimdLib::SimdVector<std::int32_t, 3>>{}(wide);
	const auto otherHash = std::hash<SimdLib::SimdVector<std::int32_t, 3>>{}(
	    SimdLib::SimdVector<std::int32_t, 3>(-4, 7, 301));
	REQUIRE(sameHash != otherHash);
}

TEST_CASE("SimdVector floating convenience operations retain scalar semantics", "[simdlib][vector][float]")
{
	using Vector = SimdLib::SimdVector<float, 3>;
	const Vector lhs(1.0f, 2.0f, 3.0f);
	const Vector rhs(4.0f, -5.0f, 6.0f);
	require_lanes(lhs, std::array<float, 3>{1.0f, 2.0f, 3.0f});
	REQUIRE(lhs.dot_product(rhs.getRegister()) == 12.0f);
	require_lanes(Vector(lhs.clamp(1.5f, 2.5f)), std::array<float, 3>{1.5f, 2.0f, 2.5f});

	using WideVector = SimdLib::SimdVector<float, 5>;
	const WideVector wide(-2.0f, 0.5f, 2.0f, 4.0f, 8.0f);
	require_lanes(WideVector(wide.clamp(0.0f, 4.0f)), std::array<float, 5>{0.0f, 0.5f, 2.0f, 4.0f, 4.0f});

	using DoubleVector = SimdLib::SimdVector<double, 1>;
	const DoubleVector precise(12.0);
	require_lanes(DoubleVector(precise.clamp(2.0, 8.0)), std::array<double, 1>{8.0});
}
