#include <SimdLib/SimdVector.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <bit>
#include <cstdint>
#include <functional>
#include <limits>
#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>

namespace
{
/** @brief Verifies the logical lanes of a SimdVector result.
 *  @tparam Vector SimdVector type under test.
 *  @tparam Element Expected lane element type.
 *  @tparam Count Expected logical lane count.
 *  @param value Vector produced by the documented invocation.
 *  @param expected Documented logical lane values.
 */
template <class Vector, class Element, std::size_t Count>
	requires requires { typename Vector::simd; }
void require_lanes(const Vector &value, const std::array<Element, Count> &expected)
{
	const auto actual = value.toArray();
	for (std::size_t index = 0; index < Count; ++index)
		REQUIRE(actual[index] == static_cast<typename Vector::simd::element_type>(expected[index]));
	for (std::size_t index = Count; index < actual.size(); ++index)
		REQUIRE(actual[index] == 0);
}

/**
 * @brief Verifies a raw register result by wrapping it in the documented logical vector type.
 * @tparam Register Raw SIMD register type.
 * @tparam Element Logical vector element type.
 * @tparam Count Logical vector element count.
 * @param value Raw register produced by the documented invocation.
 * @param expected Documented logical lane values.
 */
template <class Register, class Element, std::size_t Count>
	requires(!requires { typename Register::simd; })
void require_lanes(const Register value, const std::array<Element, Count> &expected)
{
	require_lanes(SimdLib::SimdVector<Element, static_cast<int>(Count)>{value}, expected);
}
} // namespace

namespace
{
/** @brief Verifies a floating-point vector's scalar dot product against an exact expected value.
 *  @tparam Element Floating-point lane type.
 *  @tparam Count Logical vector lane count.
 *  @param lhs Left-hand logical lanes.
 *  @param rhs Right-hand logical lanes.
 *  @param expected Expected scalar dot product.
 */
template <class Element, std::size_t Count>
void require_dot_product(const std::array<Element, Count> &lhs, const std::array<Element, Count> &rhs, const Element expected)
{
	using Vector = SimdLib::SimdVector<Element, static_cast<int>(Count)>;
	const Vector lhs_vector(lhs);
	const Vector rhs_vector(rhs);
	REQUIRE(lhs_vector.dot_product(rhs_vector.getRegister()) == expected);
}
} // namespace

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

TEST_CASE("SimdVector partial division and modulus neutralize inactive divisors", "[simdlib][vector][partial][division]")
{
	using Vector = SimdLib::SimdVector<std::int32_t, 3>;
	using Simd = Vector::simd;
	const Vector lhs(20, -21, 22);
	const auto rhs = Simd::setr(4, 5, 6, 0);

	require_lanes(Vector(lhs / rhs), std::array<std::int32_t, 3>{5, -4, 3});
	require_lanes(Vector(lhs % rhs), std::array<std::int32_t, 3>{0, -1, 4});

	Vector quotient = lhs;
	quotient /= rhs;
	require_lanes(quotient, std::array<std::int32_t, 3>{5, -4, 3});

	Vector remainder = lhs;
	remainder %= rhs;
	require_lanes(remainder, std::array<std::int32_t, 3>{0, -1, 4});

	using Full = SimdLib::SimdVector<std::int32_t, 4>;
	const Full full_lhs(20, -21, 22, 24);
	const Full full_rhs(4, 5, 6, 8);
	require_lanes(Full(full_lhs / full_rhs.getRegister()), std::array<std::int32_t, 4>{5, -4, 3, 3});
	require_lanes(Full(full_lhs % full_rhs.getRegister()), std::array<std::int32_t, 4>{0, -1, 4, 0});
}

TEST_CASE("SimdVector partial clamp excludes inactive bound lanes", "[simdlib][vector][partial][clamp]")
{
	using Vector = SimdLib::SimdVector<std::int32_t, 3>;
	using Simd = Vector::simd;
	const Vector value(-10, 5, 50);
	const auto lower = Simd::setr(-5, 10, 0, 100);
	const auto upper = Simd::setr(0, 20, 40, -100);

	require_lanes(Vector(value.clamp(lower, upper)), std::array<std::int32_t, 3>{-5, 10, 40});
	using Full = SimdLib::SimdVector<std::int32_t, 4>;
	require_lanes(Full(Full(-10, 5, 50, 7).clamp(-5, 40)), std::array<std::int32_t, 4>{-5, 5, 40, 7});
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
	const auto otherHash = std::hash<SimdLib::SimdVector<std::int32_t, 3>>{}(SimdLib::SimdVector<std::int32_t, 3>(-4, 7, 301));
	REQUIRE(sameHash != otherHash);
}

TEST_CASE("SimdVector signed partial masks preserve every active bit", "[simdlib][vector][partial][regression]")
{
	using Vector = SimdLib::SimdVector<std::int32_t, 3>;
	require_lanes(Vector(~Vector(0, 1, -1)), std::array<std::int32_t, 3>{-1, -2, 0});
	REQUIRE(Vector(-2, 3, 4).area() == -24);
}

TEST_CASE("SimdVector integer area covers full partial odd and cross-lane extents", "[simdlib][vector][partial][area]")
{
	REQUIRE(SimdLib::SimdVector<std::int8_t, 5>(-2, 3, 4, 1, 2).area() == -48);
	REQUIRE(SimdLib::SimdVector<std::uint16_t, 5>(2, 3, 4, 5, 6).area() == 720u);
	REQUIRE(SimdLib::SimdVector<std::int16_t, 9>(2, 2, 2, 2, 2, 2, 2, 2, 2).area() == 512);
	REQUIRE(SimdLib::SimdVector<std::uint8_t, 17>(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2).area() == 131072u);
	REQUIRE(SimdLib::SimdVector<std::int64_t, 2>(-2, 3).area() == -6);
	REQUIRE(SimdLib::SimdVector<std::int64_t, 3>(2, 3, 4).area() == 24);
	REQUIRE(SimdLib::SimdVector<std::uint32_t, 5>(2, 3, 4, 5, 6).area() == 720u);
	REQUIRE(SimdLib::SimdVector<std::int32_t, 4>(2, 3, 4, 5).area() == 120);
	REQUIRE(SimdLib::SimdVector<std::int32_t, 2>(std::numeric_limits<std::int32_t>::max(), 2).area() == -2);
}

TEST_CASE("SimdVector integer magnitude preserves sparse per-128-bit-group results", "[simdlib][vector][partial][magnitude]")
{
	using Signed = SimdLib::SimdVector<std::int16_t, 9>;
	const Signed signedValue(3, 4, 0, 0, 0, 0, 0, 0, 6);
	const auto signedMagnitude = Signed::simd::to_array(signedValue.magnitude());
	const auto signedChecked = Signed::simd::to_array(signedValue.magnitude_checked());
	REQUIRE(signedMagnitude[0] == 5);
	REQUIRE(signedMagnitude[8] == 6);
	REQUIRE(signedChecked[0] == 5);
	REQUIRE(signedChecked[1] == 0);
	REQUIRE(signedChecked[8] == 6);
	REQUIRE(signedChecked[9] == 0);

	using Unsigned = SimdLib::SimdVector<std::uint8_t, 17>;
	const Unsigned unsignedValue(6, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9);
	const auto unsignedMagnitude = Unsigned::simd::to_array(unsignedValue.magnitude());
	const auto unsignedChecked = Unsigned::simd::to_array(unsignedValue.magnitude_checked());
	REQUIRE(unsignedMagnitude[0] == 10);
	REQUIRE(unsignedMagnitude[16] == 9);
	REQUIRE(unsignedChecked[0] == 10);
	REQUIRE(unsignedChecked[1] == 0);
	REQUIRE(unsignedChecked[16] == 9);
	REQUIRE(unsignedChecked[17] == 0);
}
TEST_CASE("SimdVector partial positions ignore inactive zero-filled lanes", "[simdlib][vector][partial][position]")
{
	using Unsigned = SimdLib::SimdVector<std::uint16_t, 3>;
	const Unsigned positive(8, 4, 7);
	REQUIRE(positive.min_position() == 1);
	REQUIRE(positive.max_position() == 0);
	const SimdLib::SimdVector<std::uint16_t, 8> full(8, 4, 7, 9, 6, 5, 3, 10);
	REQUIRE(full.min_position() == 6);
	REQUIRE(full.max_position() == 7);
}

TEST_CASE("SimdVector hashes respect floating equality for signed zero", "[simdlib][vector][hash][float]")
{
	const SimdLib::SimdVector<float, 3> positive_zero(0.0f, 2.0f, 0.0f);
	const SimdLib::SimdVector<float, 3> negative_zero(-0.0f, 2.0f, -0.0f);
	REQUIRE(positive_zero == negative_zero.getRegister());
	REQUIRE(std::hash<SimdLib::SimdVector<float, 3>>{}(positive_zero) == std::hash<SimdLib::SimdVector<float, 3>>{}(negative_zero));

	const SimdLib::SimdVector<double, 1> positive_double_zero(0.0);
	const SimdLib::SimdVector<double, 1> negative_double_zero(-0.0);
	REQUIRE(positive_double_zero == negative_double_zero.getRegister());
	REQUIRE(std::hash<SimdLib::SimdVector<double, 1>>{}(positive_double_zero) == std::hash<SimdLib::SimdVector<double, 1>>{}(negative_double_zero));
}

TEST_CASE("SimdVector floating hashes cover nonzero infinities and NaNs", "[simdlib][vector][hash][float]")
{
	using FloatVector = SimdLib::SimdVector<float, 5>;
	using DoubleVector = SimdLib::SimdVector<double, 3>;
	const std::hash<FloatVector> float_hash;
	const std::hash<DoubleVector> double_hash;

	const FloatVector float_value(1.25f, -2.5f, 3.75f, 4.5f, -6.0f);
	const FloatVector float_copy = float_value;
	const FloatVector float_distinct(1.25f, -2.5f, 3.75f, 4.5f, -7.0f);
	REQUIRE(float_hash(float_value) != 0u);
	REQUIRE(float_hash(float_value) == float_hash(float_copy));
	REQUIRE(float_hash(float_value) != float_hash(float_distinct));

	const DoubleVector double_value(1.25, -2.5, 3.75);
	const DoubleVector double_copy = double_value;
	const DoubleVector double_distinct(1.25, -2.5, 4.75);
	REQUIRE(double_hash(double_value) != 0u);
	REQUIRE(double_hash(double_value) == double_hash(double_copy));
	REQUIRE(double_hash(double_value) != double_hash(double_distinct));

	const FloatVector float_infinities(std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), 1.0f, 2.0f, 3.0f);
	REQUIRE(float_hash(float_infinities) == float_hash(FloatVector(float_infinities)));
	const DoubleVector double_infinities(std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), 1.0);
	REQUIRE(double_hash(double_infinities) == double_hash(DoubleVector(double_infinities)));

	const float alternate_float_nan = std::bit_cast<float>(std::uint32_t{0x7FC00001u});
	const FloatVector float_nans(std::numeric_limits<float>::quiet_NaN(), alternate_float_nan, 1.0f, 2.0f, 3.0f);
	REQUIRE(float_hash(float_nans) == float_hash(FloatVector(float_nans)));
	const double alternate_double_nan = std::bit_cast<double>(std::uint64_t{0x7FF8000000000001ull});
	const DoubleVector double_nans(std::numeric_limits<double>::quiet_NaN(), alternate_double_nan, 1.0);
	REQUIRE(double_hash(double_nans) == double_hash(DoubleVector(double_nans)));
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

TEST_CASE("SimdVector 256-bit float dot products include every active high lane", "[simdlib][vector][float][dot]")
{
	require_dot_product<float, 4>({1, 1, 1, 1}, {1, 2, 3, 4}, 10.0f);
	require_dot_product<float, 5>({1, 1, 1, 1, 1}, {1, 2, 3, 4, 5}, 15.0f);
	require_dot_product<float, 6>({1, 1, 1, 1, 1, 1}, {1, 2, 3, 4, 5, 6}, 21.0f);
	require_dot_product<float, 7>({1, 1, 1, 1, 1, 1, 1}, {1, 2, 3, 4, 5, 6, 7}, 28.0f);
	require_dot_product<float, 8>({1, 1, 1, 1, 1, 1, 1, 1}, {1, 2, 3, 4, 5, 6, 7, 8}, 36.0f);
}

TEST_CASE("SimdVector double dot products cover full and partial 128-bit and 256-bit vectors", "[simdlib][vector][double][dot]")
{
	require_dot_product<double, 1>({7.0}, {8.0}, 56.0);
	require_dot_product<double, 2>({3.0, -4.0}, {5.0, 6.0}, -9.0);
	require_dot_product<double, 3>({1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}, 14.0);
	require_dot_product<double, 4>({1.0, 2.0, 3.0, 4.0}, {1.0, 2.0, 3.0, 4.0}, 30.0);
}

TEST_CASE("SimdVector documentation examples produce their documented results", "[simdlib][vector][documentation]")
{
	using Vector3 = SimdLib::SimdVector<float, 3>;
	using U8x3 = SimdLib::SimdVector<std::uint8_t, 3>;
	using I16x4 = SimdLib::SimdVector<std::int16_t, 4>;
	using U16x3 = SimdLib::SimdVector<std::uint16_t, 3>;
	using I32x3 = SimdLib::SimdVector<std::int32_t, 3>;
	using U32x3 = SimdLib::SimdVector<std::uint32_t, 3>;
	require_lanes(Vector3{1.0F, 2.0F, 3.0F}, std::array{1.0F, 2.0F, 3.0F});
	{
		const Vector3 temporary{1.0F, 2.0F, 3.0F};
		REQUIRE(temporary.z() == 3.0F);
	}
	require_lanes(U16x3{5U, 7U, 9U}.size(U16x3{1U, 2U, 3U}.getRegister()), std::array<std::uint16_t, 3>{5U, 6U, 7U});
	require_lanes(SimdLib::SimdVector<std::uint32_t, 3>{U16x3{1U, 2U, 3U}}, std::array<std::uint32_t, 3>{1U, 2U, 3U});
	REQUIRE(Vector3{1.0F, 2.0F, 3.0F}.x() == 1.0F);
	REQUIRE(Vector3{1.0F, 2.0F, 3.0F}.y() == 2.0F);
	REQUIRE(Vector3{1.0F, 2.0F, 3.0F}.z() == 3.0F);
	REQUIRE(SimdLib::SimdVector<float, 4>{1.0F, 2.0F, 3.0F, 4.0F}.w() == 4.0F);
	REQUIRE(U16x3{2U, 3U, 4U}.area() == 24U);
	const auto magnitude = Vector3::simd::to_array(Vector3{3.0F, 4.0F, 0.0F}.magnitude());
	REQUIRE(magnitude == std::array{5.0F, 5.0F, 5.0F, 5.0F});
	const auto normalized = Vector3{Vector3{3.0F, 4.0F, 0.0F}.normalize()}.toArray();
	REQUIRE(normalized[0] > 0.599F);
	REQUIRE(normalized[0] < 0.601F);
	REQUIRE(normalized[1] > 0.799F);
	REQUIRE(normalized[1] < 0.801F);
	const auto dot = Vector3{1.0F, 2.0F, 3.0F}.dot_product(Vector3{4.0F, 5.0F, 6.0F});
	REQUIRE(dot == 32.0F);
	require_lanes(Vector3{-1.0F, 2.0F, -3.0F}.abs(), std::array{1.0F, 2.0F, 3.0F});
	require_lanes(Vector3{-1.0F, 0.0F, 3.0F}.sign(), std::array{-1.0F, 0.0F, 1.0F});
	require_lanes(Vector3{1.0F, 4.0F, 9.0F}.sqrt(), std::array{1.0F, 2.0F, 3.0F});
	require_lanes(Vector3{1.0F, 5.0F, 3.0F}.min(Vector3{2.0F, 4.0F, 6.0F}), std::array{1.0F, 4.0F, 3.0F});
	require_lanes(Vector3{1.0F, 5.0F, 3.0F}.max(Vector3{2.0F, 4.0F, 6.0F}), std::array{2.0F, 5.0F, 6.0F});
	require_lanes(Vector3{-2.0F, 5.0F, 12.0F}.clamp(Vector3{0.0F}, Vector3{10.0F}), std::array{0.0F, 5.0F, 10.0F});
	require_lanes(U8x3{2U, 4U, 6U}.avg(U8x3{4U, 6U, 8U}), std::array<std::uint8_t, 3>{3U, 5U, 7U});
	REQUIRE(U16x3{4U, 1U, 3U}.min_position() == 1);
	REQUIRE(U16x3{4U, 1U, 3U}.max_position() == 0);
	REQUIRE(Vector3{2.0F, 2.0F, 2.0F}.all_equal(Vector3{2.0F, 2.0F, 2.0F}));
	REQUIRE(Vector3{1.0F, 2.0F, 3.0F}.any_equal(Vector3{9.0F, 2.0F, 8.0F}));
	REQUIRE(Vector3{4.0F, 5.0F, 6.0F}.all_greater(Vector3{1.0F, 2.0F, 3.0F}));
	REQUIRE(Vector3{1.0F, 5.0F, 2.0F}.any_greater(Vector3{3.0F, 4.0F, 6.0F}));
	REQUIRE(Vector3{1.0F, 2.0F, 3.0F}.all_greater_equal(Vector3{1.0F, 1.0F, 3.0F}));
	REQUIRE(Vector3{1.0F, 2.0F, 3.0F}.any_greater_equal(Vector3{4.0F, 2.0F, 5.0F}));
	REQUIRE(Vector3{1.0F, 2.0F, 3.0F}.all_less(Vector3{4.0F, 5.0F, 6.0F}));
	REQUIRE(Vector3{1.0F, 5.0F, 6.0F}.any_less(Vector3{2.0F, 4.0F, 3.0F}));
	REQUIRE(Vector3{1.0F, 2.0F, 3.0F}.all_less_equal(Vector3{1.0F, 3.0F, 3.0F}));
	REQUIRE(Vector3{5.0F, 2.0F, 6.0F}.any_less_equal(Vector3{4.0F, 2.0F, 3.0F}));
	REQUIRE(Vector3{1.0F, 2.0F, 3.0F}[1] == 2.0F);
	require_lanes(Vector3{Vector3{1.0F, 2.0F, 3.0F} + Vector3{4.0F, 5.0F, 6.0F}}, std::array{5.0F, 7.0F, 9.0F});
	require_lanes(Vector3{Vector3{4.0F, 5.0F, 6.0F} - Vector3{1.0F, 2.0F, 3.0F}}, std::array{3.0F, 3.0F, 3.0F});
	require_lanes(Vector3{Vector3{1.0F, 2.0F, 3.0F} * 2.0F}, std::array{2.0F, 4.0F, 6.0F});
	require_lanes(Vector3{Vector3{2.0F, 4.0F, 6.0F} / 2.0F}, std::array{1.0F, 2.0F, 3.0F});
	require_lanes(I32x3{I32x3{7, 8, 9} % 4}, std::array{3, 0, 1});
	const bool equal = Vector3{1.0F, 2.0F, 3.0F} == Vector3{1.0F, 2.0F, 3.0F}.getRegister();
	const bool less = Vector3{1.0F, 2.0F, 3.0F} < Vector3{2.0F, 3.0F, 4.0F}.getRegister();
	const bool less_equal = Vector3{1.0F, 2.0F, 3.0F} <= Vector3{1.0F, 3.0F, 3.0F}.getRegister();
	const bool greater = Vector3{4.0F, 5.0F, 6.0F} > Vector3{1.0F, 2.0F, 3.0F}.getRegister();
	const bool greater_equal = Vector3{4.0F, 5.0F, 6.0F} >= Vector3{4.0F, 2.0F, 6.0F}.getRegister();
	REQUIRE(equal);
	REQUIRE(less);
	REQUIRE(less_equal);
	REQUIRE(greater);
	REQUIRE(greater_equal);
	REQUIRE(Vector3{1.0F, 2.0F, 3.0F}.toArray() == std::array{1.0F, 2.0F, 3.0F, 0.0F});
	const Vector3 tuple_source{1.0F, 2.0F, 3.0F};
	REQUIRE(std::ranges::equal(std::get<0>(tuple_source.getTuple()).first<3>(), std::array{1.0F, 2.0F, 3.0F}));
	Vector3 input{1.0F, 2.0F, 3.0F};
	REQUIRE(std::ranges::equal(input.getSpan().first<3>(), std::array{1.0F, 2.0F, 3.0F}));
	REQUIRE(Vector3::simd::to_array(input.getRegister()) == std::array{1.0F, 2.0F, 3.0F, 0.0F});
	REQUIRE(static_cast<std::array<float, Vector3::simd::element_count>>(input) == std::array{1.0F, 2.0F, 3.0F, 0.0F});
	const Vector3 const_input{1.0F, 2.0F, 3.0F};
	REQUIRE(std::ranges::equal(static_cast<std::span<const float, Vector3::simd::element_count>>(const_input).first<3>(), std::array{1.0F, 2.0F, 3.0F}));
	REQUIRE(std::ranges::equal(static_cast<std::span<float, Vector3::simd::element_count>>(input).first<3>(), std::array{1.0F, 2.0F, 3.0F}));
	REQUIRE(Vector3::simd::to_array(static_cast<Vector3::vector_t>(input)) == std::array{1.0F, 2.0F, 3.0F, 0.0F});
	require_lanes(U8x3{250, 10, 20}.add_saturated(U8x3{10, 20, 30}), std::array<std::uint8_t, 3>{255, 30, 50});
	require_lanes(U8x3{5, 20, 30}.subtract_saturated(U8x3{10, 7, 40}), std::array<std::uint8_t, 3>{0, 13, 0});
	require_lanes(U16x3{40000U, 5U, 2U}.multiply_saturated(U16x3{2U, 6U, 4U}), std::array<std::uint16_t, 3>{65535U, 30U, 8U});
	require_lanes(Vector3{Vector3{2.0F, 3.0F, 4.0F}.multiply_add(Vector3{5.0F, 6.0F, 7.0F}, Vector3{1.0F})}, std::array{11.0F, 19.0F, 29.0F});
	REQUIRE(Vector3::simd::to_array(Vector3{1.0F, 2.0F, 3.0F}.add_horizontal(Vector3{4.0F, 5.0F, 6.0F})) == std::array{3.0F, 3.0F, 9.0F, 6.0F});
	REQUIRE(Vector3::simd::to_array(Vector3{10.0F, 10.0F, 10.0F}.add_subtract(Vector3{1.0F, 2.0F, 3.0F})) == std::array{9.0F, 12.0F, 7.0F, 0.0F});
	REQUIRE(I16x4::simd::to_array(I16x4{30000, 10000, 200, 300}.add_horizontal_saturated(I16x4{1, 2, 3, 4})) ==
			std::array<std::int16_t, 8>{32767, 500, 0, 0, 3, 7, 0, 0});
	REQUIRE(Vector3::simd::to_array(Vector3{5.0F, 2.0F, 9.0F}.subtract_horizontal(Vector3{8.0F, 3.0F, 6.0F})) == std::array{3.0F, 9.0F, 5.0F, 6.0F});
	REQUIRE(I16x4::simd::to_array(I16x4{30000, -10000, -30000, 10000}.subtract_horizontal_saturated(I16x4{1, 2, 3, 4})) ==
			std::array<std::int16_t, 8>{32767, -32768, 0, 0, -1, -1, 0, 0});
	REQUIRE(SimdLib::Api<128, std::int32_t>::to_array(I16x4{1, 2, 3, 4}.multiply_add_adjacent(I16x4{5, 6, 7, 8})) == std::array{17, 53, 0, 0});
	using U8x16 = SimdLib::uint8x16;
	REQUIRE(SimdLib::Api<128, std::int16_t>::to_array(U8x16{2}.multiply_add_unsigned_signed_bytes(U8x16{3})) ==
			std::array<std::int16_t, 8>{12, 12, 12, 12, 12, 12, 12, 12});
	REQUIRE(SimdLib::Api<128, std::uint64_t>::to_array(U8x16{9}.sum_absolute_byte_differences(U8x16{4})) == std::array<std::uint64_t, 2>{40, 40});
	REQUIRE(SimdLib::Api<128, std::uint16_t>::to_array(U8x16{9}.multi_sum_absolute_byte_differences<0>(U8x16{4})) ==
			std::array<std::uint16_t, 8>{20, 20, 20, 20, 20, 20, 20, 20});
	Vector3 floats{};
	floats = Vector3{1.0F, 2.0F, 3.0F};
	require_lanes(floats, std::array{1.0F, 2.0F, 3.0F});
	floats = Vector3{1.0F, 2.0F, 3.0F};
	floats += Vector3{4.0F, 5.0F, 6.0F};
	require_lanes(floats, std::array{5.0F, 7.0F, 9.0F});
	floats = Vector3{7.0F, 8.0F, 9.0F};
	floats -= Vector3{1.0F, 2.0F, 3.0F};
	require_lanes(floats, std::array{6.0F, 6.0F, 6.0F});
	floats = Vector3{1.0F, 2.0F, 3.0F};
	floats *= Vector3{4.0F, 5.0F, 6.0F};
	require_lanes(floats, std::array{4.0F, 10.0F, 18.0F});
	floats = Vector3{8.0F, 12.0F, 18.0F};
	floats /= Vector3{2.0F, 3.0F, 6.0F};
	require_lanes(floats, std::array{4.0F, 4.0F, 3.0F});
	I32x3 ints{7, 8, 9};
	ints %= I32x3{4, 4, 4};
	require_lanes(ints, std::array{3, 0, 1});
	U32x3 bits{12U, 10U, 15U};
	bits &= U32x3{10U, 6U, 5U};
	require_lanes(bits, std::array{8U, 2U, 5U});
	bits = U32x3{12U, 10U, 15U};
	bits |= U32x3{10U, 6U, 5U};
	require_lanes(bits, std::array{14U, 14U, 15U});
	bits = U32x3{12U, 10U, 15U};
	bits ^= U32x3{10U, 6U, 5U};
	require_lanes(bits, std::array{6U, 12U, 10U});
	bits = U32x3{1U, 2U, 3U};
	bits <<= 1;
	require_lanes(bits, std::array{2U, 4U, 6U});
	bits = U32x3{2U, 4U, 6U};
	bits >>= 1;
	require_lanes(bits, std::array{1U, 2U, 3U});
	require_lanes(U32x3{~U32x3{0U}}, std::array{0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU});
	require_lanes(U32x3{U32x3{12U, 10U, 15U} & U32x3{10U, 6U, 5U}}, std::array{8U, 2U, 5U});
	require_lanes(U32x3{U32x3{12U, 10U, 15U} | U32x3{10U, 6U, 5U}}, std::array{14U, 14U, 15U});
	require_lanes(U32x3{U32x3{12U, 10U, 15U} ^ U32x3{10U, 6U, 5U}}, std::array{6U, 12U, 10U});
	require_lanes(U32x3{U32x3{1U, 2U, 3U} << 1}, std::array{2U, 4U, 6U});
	require_lanes(U32x3{U32x3{2U, 4U, 6U} >> 1}, std::array{1U, 2U, 3U});
}
