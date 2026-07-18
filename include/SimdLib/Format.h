#pragma once

#include <SimdLib/SimdVector.h>
#include <SimdLib/UInt128.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <limits>
#include <string>

namespace SimdLib::Detail
{
struct container_format_spec
{
	char fill = ' ';
	char alignment = 0;
	std::size_t width = 0;
};

struct uint128_format_spec : container_format_spec
{
	char sign = '-';
	char presentation = 'd';
	bool alternate = false;
	bool zero_pad = false;
};

[[nodiscard]] constexpr bool is_alignment(const char value) noexcept
{
	return value == '<' || value == '>' || value == '^';
}

template <class ParseContext> constexpr void validate_container_format_spec(ParseContext &context)
{
	auto current = context.begin();
	const auto end = context.end();
	if (current == end || *current == '}')
	{
		return;
	}

	auto next = current;
	++next;
	if (next != end && is_alignment(*next))
	{
		if (*current == '{' || *current == '}')
		{
			throw std::format_error("invalid SimdLib format fill character");
		}
		current = ++next;
	}
	else if (is_alignment(*current))
	{
		++current;
	}

	while (current != end && *current >= '0' && *current <= '9')
	{
		++current;
	}
	if (current != end && *current != '}')
	{
		throw std::format_error("SimdVector supports only fill, alignment, and width");
	}
}

template <class OutputIterator> OutputIterator write_fill(OutputIterator output, const char fill, std::size_t count)
{
	while (count-- != 0)
	{
		*output++ = fill;
	}
	return output;
}

template <class OutputIterator>
OutputIterator write_padded(OutputIterator output, const char *data, const std::size_t size, const container_format_spec &spec, const char default_alignment)
{
	const std::size_t padding = spec.width > size ? spec.width - size : 0;
	const char alignment = spec.alignment == 0 ? default_alignment : spec.alignment;
	const std::size_t left_padding = alignment == '>' ? padding : alignment == '^' ? padding / 2 : 0;
	const std::size_t right_padding = padding - left_padding;

	output = write_fill(output, spec.fill, left_padding);
	for (std::size_t index = 0; index < size; ++index)
	{
		*output++ = data[index];
	}
	return write_fill(output, spec.fill, right_padding);
}

struct uint128_digits
{
	std::array<char, 128> storage{};
	std::size_t begin = storage.size();
};

[[nodiscard]] constexpr std::uint32_t divide_small(uint128_t &value, const std::uint32_t divisor) noexcept
{
	std::uint32_t remainder = 0;
	std::uint64_t quotient_low = 0;
	std::uint64_t quotient_high = 0;
	for (int bit_index = 127; bit_index >= 0; --bit_index)
	{
		const std::uint64_t word = bit_index >= 64 ? value.high() : value.low();
		const unsigned word_bit = static_cast<unsigned>(bit_index >= 64 ? bit_index - 64 : bit_index);
		remainder = static_cast<std::uint32_t>(remainder * 2 + ((word >> word_bit) & 1));
		if (remainder >= divisor)
		{
			remainder -= divisor;
			if (bit_index >= 64)
			{
				quotient_high |= std::uint64_t{1} << word_bit;
			}
			else
			{
				quotient_low |= std::uint64_t{1} << word_bit;
			}
		}
	}
	value = uint128_t{quotient_low, quotient_high};
	return remainder;
}

[[nodiscard]] constexpr uint128_digits make_digits(const uint128_t &source, const std::uint32_t base, const bool uppercase) noexcept
{
	uint128_digits result;
	uint128_t value{source.low(), source.high()};
	const char *alphabet = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
	do
	{
		result.storage[--result.begin] = alphabet[divide_small(value, base)];
	} while (static_cast<bool>(value));
	return result;
}
} // namespace SimdLib::Detail

namespace std
{
/**
 * @brief Formats the logical lanes of a SimdLib vector as `{a, b, c}`.
 *
 * The format specification controls only the completed container text and accepts
 * `[[fill]align][width]`. Element-specific format specifications are intentionally
 * not forwarded; every element uses its default `std::formatter` presentation.
 */
template <class element_t, int element_count> struct formatter<SimdLib::SimdVector<element_t, element_count>, char> : formatter<std::string_view, char>
{
	using base_formatter = formatter<std::string_view, char>;

	template <class ParseContext> constexpr auto parse(ParseContext &context) -> typename ParseContext::iterator
	{
		SimdLib::Detail::validate_container_format_spec(context);
		return base_formatter::parse(context);
	}

	template <class FormatContext>
	auto format(const SimdLib::SimdVector<element_t, element_count> &value, FormatContext &context) const -> typename FormatContext::iterator
	{
		std::string rendered;
		rendered.push_back('{');
		for (int index = 0; index < element_count; ++index)
		{
			if (index != 0)
			{
				rendered.append(", ");
			}
			std::format_to(std::back_inserter(rendered), "{}", value[index]);
		}
		rendered.push_back('}');
		return base_formatter::format(std::string_view{rendered}, context);
	}
};

/**
 * @brief Formats SimdLib's unsigned 128-bit integer without compiler-native 128-bit arithmetic.
 *
 * Supported grammar is `[[fill]align][sign][#][0][width][type]`, where type is
 * `d`, `x`, `X`, `b`, `B`, or `o`. The `-` sign is accepted as the unsigned
 * default; `+` and space emit their corresponding positive sign. Alternate form,
 * numeric zero padding, fill, and left/right/center alignment follow the ordinary
 * unsigned-integer conventions. Precision, locale, dynamic width, and other
 * presentation types are rejected with `std::format_error`.
 */
template <> struct formatter<SimdLib::uint128_t, char>
{
	SimdLib::Detail::uint128_format_spec spec{};

	template <class ParseContext> constexpr auto parse(ParseContext &context) -> typename ParseContext::iterator
	{
		auto current = context.begin();
		const auto end = context.end();

		if (current != end && *current != '}')
		{
			auto next = current;
			++next;
			if (next != end && SimdLib::Detail::is_alignment(*next))
			{
				if (*current == '{' || *current == '}')
				{
					throw format_error("invalid uint128_t format fill character");
				}
				spec.fill = *current;
				spec.alignment = *next;
				current = ++next;
			}
			else if (SimdLib::Detail::is_alignment(*current))
			{
				spec.alignment = *current++;
			}
		}

		if (current != end && (*current == '+' || *current == '-' || *current == ' '))
		{
			spec.sign = *current++;
		}
		if (current != end && *current == '#')
		{
			spec.alternate = true;
			++current;
		}
		if (current != end && *current == '0')
		{
			spec.zero_pad = true;
			++current;
		}
		while (current != end && *current >= '0' && *current <= '9')
		{
			const std::size_t digit = static_cast<std::size_t>(*current - '0');
			if (spec.width > (std::numeric_limits<std::size_t>::max() - digit) / 10)
			{
				throw format_error("uint128_t format width is too large");
			}
			spec.width = spec.width * 10 + digit;
			++current;
		}
		if (current != end && *current != '}')
		{
			spec.presentation = *current++;
			if (spec.presentation != 'd' && spec.presentation != 'x' && spec.presentation != 'X' && spec.presentation != 'b' && spec.presentation != 'B' &&
				spec.presentation != 'o')
			{
				throw format_error("unsupported uint128_t presentation type");
			}
		}
		if (current != end && *current != '}')
		{
			throw format_error("invalid uint128_t format specification");
		}
		return current;
	}

	template <class FormatContext> auto format(const SimdLib::uint128_t &value, FormatContext &context) const -> typename FormatContext::iterator
	{
		const bool uppercase = spec.presentation == 'X' || spec.presentation == 'B';
		const std::uint32_t base = spec.presentation == 'x' || spec.presentation == 'X'	  ? 16
								   : spec.presentation == 'b' || spec.presentation == 'B' ? 2
								   : spec.presentation == 'o'							  ? 8
																						  : 10;
		const auto digits = SimdLib::Detail::make_digits(value, base, uppercase);
		const std::size_t digit_count = digits.storage.size() - digits.begin;

		std::array<char, 3> prefix{};
		std::size_t prefix_size = 0;
		if (spec.sign == '+' || spec.sign == ' ')
		{
			prefix[prefix_size++] = spec.sign;
		}
		if (spec.alternate)
		{
			if (base == 16)
			{
				prefix[prefix_size++] = '0';
				prefix[prefix_size++] = uppercase ? 'X' : 'x';
			}
			else if (base == 2)
			{
				prefix[prefix_size++] = '0';
				prefix[prefix_size++] = uppercase ? 'B' : 'b';
			}
			else if (base == 8 && !(digit_count == 1 && digits.storage[digits.begin] == '0'))
			{
				prefix[prefix_size++] = '0';
			}
		}

		auto output = context.out();
		const std::size_t content_size = prefix_size + digit_count;
		if (spec.zero_pad && spec.alignment == 0 && spec.width > content_size)
		{
			for (std::size_t index = 0; index < prefix_size; ++index)
			{
				*output++ = prefix[index];
			}
			output = SimdLib::Detail::write_fill(output, '0', spec.width - content_size);
			for (std::size_t index = digits.begin; index < digits.storage.size(); ++index)
			{
				*output++ = digits.storage[index];
			}
			return output;
		}

		std::array<char, 131> rendered{};
		std::size_t rendered_size = 0;
		for (std::size_t index = 0; index < prefix_size; ++index)
		{
			rendered[rendered_size++] = prefix[index];
		}
		for (std::size_t index = digits.begin; index < digits.storage.size(); ++index)
		{
			rendered[rendered_size++] = digits.storage[index];
		}
		return SimdLib::Detail::write_padded(output, rendered.data(), rendered_size, spec, '>');
	}
};
} // namespace std
