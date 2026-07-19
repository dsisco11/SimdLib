#include <SimdLib/Format.h>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <format>
#include <limits>
#include <string>
#include <string_view>

namespace
{
/** @brief Stores the expected digits for one unsigned 128-bit value. */
struct format_case
{
	SimdLib::uint128_t value;
	std::string_view decimal;
	std::string hexadecimal;
	std::string binary;
	std::string octal;
};

/** @brief Describes one runtime formatter-parse rejection. */
struct invalid_format_case
{
	std::string_view category;
	std::string_view specification;
};

/** @brief Describes one alternate-octal rendering and its exact scalar result. */
struct octal_format_case
{
	std::uint64_t value;
	std::string_view format;
	std::string_view expected;
};

/**
 * @brief Creates a string containing one repeated character.
 * @param value Character to repeat.
 * @param count Number of repetitions.
 * @return The completed string.
 */
[[nodiscard]] std::string repeated(const char value, const std::size_t count)
{
	return std::string(count, value);
}

/**
 * @brief Verifies that a formatter rejects a raw format specification at runtime.
 * @tparam Formatted Formatted value type whose parser is exercised.
 * @param test Rejection category and raw specification, including its closing brace.
 */
template <class Formatted> void require_parse_rejected(const invalid_format_case &test)
{
	CAPTURE(std::string(test.category), std::string(test.specification));
	std::formatter<Formatted, char> formatter;
	std::format_parse_context context(test.specification);
	CHECK_THROWS_AS(formatter.parse(context), std::format_error);
}
} // namespace

TEST_CASE("uint128_t formats full-width boundary values in every supported base", "[format][uint128]")
{
	const std::uint64_t max64 = std::numeric_limits<std::uint64_t>::max();
	const std::array cases{
		format_case{{0, 0}, "0", "0", "0", "0"},
		format_case{{1, 0}, "1", "1", "1", "1"},
		format_case{{max64, 0}, "18446744073709551615", repeated('f', 16), repeated('1', 64), "1" + repeated('7', 21)},
		format_case{{0, 1}, "18446744073709551616", "1" + repeated('0', 16), "1" + repeated('0', 64), "2" + repeated('0', 21)},
		format_case{
			{0, std::uint64_t{1} << 63}, "170141183460469231731687303715884105728", "8" + repeated('0', 31), "1" + repeated('0', 127), "2" + repeated('0', 42)},
		format_case{{max64, max64}, "340282366920938463463374607431768211455", repeated('f', 32), repeated('1', 128), "3" + repeated('7', 42)},
	};

	for (const auto &test : cases)
	{
		CAPTURE(std::string(test.decimal), test.hexadecimal, test.binary, test.octal);
		CHECK(std::format("{}", test.value) == std::string(test.decimal));
		CHECK(std::format("{:d}", test.value) == std::string(test.decimal));
		CHECK(std::format("{:x}", test.value) == test.hexadecimal);
		CHECK(std::format("{:X}", test.value) ==
			  [&]
			  {
				  auto result = test.hexadecimal;
				  for (char &character : result)
				  {
					  if (character >= 'a' && character <= 'f')
					  {
						  character = static_cast<char>(character - 'a' + 'A');
					  }
				  }
				  return result;
			  }());
		CHECK(std::format("{:b}", test.value) == test.binary);
		CHECK(std::format("{:B}", test.value) == test.binary);
		CHECK(std::format("{:o}", test.value) == test.octal);
	}
}

TEST_CASE("uint128_t formatting supports documented integer presentation controls", "[format][uint128]")
{
	const SimdLib::uint128_t value{0x23, 1};

	CHECK(std::format("{:+}", value) == "+18446744073709551651");
	CHECK(std::format("{: }", value) == " 18446744073709551651");
	CHECK(std::format("{:-}", value) == "18446744073709551651");
	CHECK(std::format("{:#x}", value) == "0x10000000000000023");
	CHECK(std::format("{:#X}", value) == "0X10000000000000023");
	CHECK(std::format("{:#b}", SimdLib::uint128_t{5}) == "0b101");
	CHECK(std::format("{:#B}", SimdLib::uint128_t{5}) == "0B101");
	CHECK(std::format("{:#o}", SimdLib::uint128_t{9}) == "011");
	CHECK(std::format("{:#o}", SimdLib::uint128_t{0}) == "0");

	CHECK(std::format("{:>24x}", value) == "       10000000000000023");
	CHECK(std::format("{:*<24x}", value) == "10000000000000023*******");
	CHECK(std::format("{:*^24x}", value) == "***10000000000000023****");
	CHECK(std::format("{:#024x}", value) == "0x0000010000000000000023");
	CHECK(std::format("{:+024x}", value) == "+" + repeated('0', 6) + "10000000000000023");
	CHECK(std::format("{:0>24x}", value) == "000000010000000000000023");
}

TEST_CASE("uint128_t alternate octal formatting covers alignment padding and width branches", "[format][uint128][octal][parity]")
{
	const std::array cases{
		octal_format_case{0, "{:#o}", "0"},
		octal_format_case{9, "{:#o}", "011"},
		octal_format_case{0, "{:#5o}", "    0"},
		octal_format_case{9, "{:#5o}", "  011"},
		octal_format_case{0, "{:>#5o}", "    0"},
		octal_format_case{9, "{:>#5o}", "  011"},
		octal_format_case{0, "{:<#5o}", "0    "},
		octal_format_case{9, "{:<#5o}", "011  "},
		octal_format_case{0, "{:#05o}", "00000"},
		octal_format_case{9, "{:#05o}", "00011"},
		octal_format_case{0, "{:>#05o}", "    0"},
		octal_format_case{9, "{:>#05o}", "  011"},
		octal_format_case{0, "{:#1o}", "0"},
		octal_format_case{9, "{:#2o}", "011"},
		octal_format_case{0, "{:#01o}", "0"},
		octal_format_case{9, "{:#02o}", "011"},
	};

	for (const auto &test : cases)
	{
		CAPTURE(test.value, std::string(test.format), std::string(test.expected));
		const SimdLib::uint128_t wide{test.value};
		CHECK(std::vformat(test.format, std::make_format_args(wide)) == std::string(test.expected));
		CHECK(std::vformat(test.format, std::make_format_args(test.value)) == std::string(test.expected));
	}
}

TEST_CASE("uint128_t formatting matches the standard uint64 formatter within the scalar range", "[format][uint128][parity]")
{
	const std::array values{
		std::uint64_t{0}, std::uint64_t{1}, std::uint64_t{9}, std::uint64_t{42},
		std::uint64_t{0x1234'5678'9ABC'DEF0}, std::numeric_limits<std::uint64_t>::max()};
	const std::array<std::string_view, 24> formats{
		"{}", "{:d}", "{:x}", "{:X}", "{:b}", "{:B}", "{:o}",
		"{:+}", "{: }", "{:-}", "{:#d}", "{:#x}", "{:#X}", "{:#b}", "{:#B}", "{:#o}",
		"{:024x}", "{:*>30x}", "{:>24x}", "{:*<24x}", "{:*^24x}",
		"{:#024x}", "{:+024x}", "{:0>24x}"};

	for (std::uint64_t scalar : values)
	{
		const SimdLib::uint128_t wide{scalar};
		for (const std::string_view format : formats)
		{
			CAPTURE(scalar, std::string(format));
			CHECK(std::vformat(format, std::make_format_args(wide)) ==
				  std::vformat(format, std::make_format_args(scalar)));
		}
	}
}

TEST_CASE("uint128_t formatting rejects unsupported specifications", "[format][uint128]")
{
	const std::array cases{
		invalid_format_case{"opening brace fill", "{<5}"},
		invalid_format_case{"precision", ".2}"},
		invalid_format_case{"dynamic width", "{}"},
		invalid_format_case{"nested replacement field", ">{}"},
		invalid_format_case{"locale", "L}"},
		invalid_format_case{"unsupported presentation", "q}"},
		invalid_format_case{"trailing specification", "dx}"},
	};
	for (const auto &test : cases)
	{
		require_parse_rejected<SimdLib::uint128_t>(test);
	}

	const invalid_format_case overflow{
		"width overflow", "184467440737095516160}"};
	require_parse_rejected<SimdLib::uint128_t>(overflow);

	const SimdLib::uint128_t value{1};
	int dynamic_width = 4;
	CHECK_THROWS_AS(std::vformat("{:{<5}", std::make_format_args(value)), std::format_error);
	CHECK_THROWS_AS(std::vformat("{:}<5}", std::make_format_args(value)), std::format_error);
	CHECK_THROWS_AS(std::vformat("{:{}}", std::make_format_args(value, dynamic_width)), std::format_error);
	CHECK_THROWS_AS(std::vformat("{:>{}}", std::make_format_args(value, dynamic_width)), std::format_error);
	CHECK_THROWS_AS(std::vformat("{:dx}", std::make_format_args(value)), std::format_error);
}

TEST_CASE("SimdVector formatting preserves logical element order and container presentation", "[format][vector]")
{
	const SimdLib::SimdVector<int, 4> full{1, -2, 3, 40};
	const SimdLib::SimdVector<int, 3> partial{7, 8, 9};

	CHECK(std::format("{}", full) == "{1, -2, 3, 40}");
	CHECK(std::format("{}", partial) == "{7, 8, 9}");
	CHECK(std::format("{:14}", partial) == "{7, 8, 9}     ");
	CHECK(std::format("{:>20}", partial) == "           {7, 8, 9}");
	CHECK(std::format("{:>5}", partial) == "{7, 8, 9}");
	CHECK(std::format("{:*<14}", partial) == "{7, 8, 9}*****");
	CHECK(std::format("{:-^15}", partial) == "---{7, 8, 9}---");

	const std::array invalid_cases{
		invalid_format_case{"opening brace fill", "{<5}"},
		invalid_format_case{"precision", ".3}"},
		invalid_format_case{"dynamic width", "{}"},
		invalid_format_case{"nested replacement field", ">{}"},
		invalid_format_case{"locale", "L}"},
		invalid_format_case{"unsupported presentation", "x}"},
		invalid_format_case{"trailing input after width", "20x}"},
	};
	for (const auto &test : invalid_cases)
	{
		require_parse_rejected<SimdLib::SimdVector<int, 4>>(test);
	}

	int dynamic_width = 20;
	CHECK_THROWS_AS(std::vformat("{:{<5}", std::make_format_args(full)), std::format_error);
	CHECK_THROWS_AS(std::vformat("{:}<5}", std::make_format_args(full)), std::format_error);
	CHECK_THROWS_AS(std::vformat("{:{}}", std::make_format_args(full, dynamic_width)), std::format_error);
	CHECK_THROWS_AS(std::vformat("{:>{}}", std::make_format_args(full, dynamic_width)), std::format_error);
}

TEST_CASE("SimdVector formatting delegates element presentation across scalar families", "[format][vector][parity]")
{
	const SimdLib::SimdVector<std::uint64_t, 2> unsigned_values{0, std::numeric_limits<std::uint64_t>::max()};
	const SimdLib::SimdVector<float, 3> float_values{0.0f, -1.5f, std::numeric_limits<float>::infinity()};
	const SimdLib::SimdVector<double, 1> double_value{-0.0};

	CHECK(std::format("{}", unsigned_values) == "{0, 18446744073709551615}");
	CHECK(std::format("{}", float_values) == "{0, -1.5, inf}");
	CHECK(std::format("{}", double_value) == "{-0}");
}
