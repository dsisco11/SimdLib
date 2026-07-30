#pragma once
#include <SimdLib/Api.h>
#include <SimdLib/TemplateTools.h>
#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <span>

namespace SimdLib
{
/// <summary>
/// SIMD bulk comparison operations
/// </summary>
template <std::size_t ReadWidth, std::size_t WriteWidth> struct SimdAlgo final
{
  private:
	[[nodiscard]] constexpr static std::uint32_t LowBits(const std::uint32_t value, const std::size_t count) noexcept
	{
		if (count == 0)
			return 0;
		if (count >= 32)
			return value;
		return value & ((std::uint32_t{1} << count) - 1);
	}

  public:
	using read_t = select_unsigned_integer_t<ReadWidth>;
	using write_t = select_unsigned_integer_t<WriteWidth>;

	constexpr static inline std::size_t read_width = ReadWidth;
	constexpr static inline std::size_t write_width = WriteWidth;
	constexpr static inline std::size_t read_data_size = std::numeric_limits<read_t>::digits;
	constexpr static inline std::size_t write_data_size = std::numeric_limits<write_t>::digits;

	template <std::size_t count> using SimdImpl = Api<count * read_data_size >= 256 ? 256 : 128, read_t>;

	/**
	 * @brief Reports whether any input element equals a scalar predicate.
	 * @tparam count Fixed input element count.
	 * @param read Input
	 * elements to inspect.
	 * @param predicate Scalar value to find.
	 * @return `true` when at least one element equals `predicate`.
	 */
	template <std::size_t count> [[nodiscard]] constexpr static inline bool AnyEqual(std::span<const read_t, count> read, const read_t predicate) noexcept
	{
		using simd = SimdImpl<count>;
		const auto predicateVector = simd::set1(predicate);

		// When the input spans less than a SIMD register, avoid out-of-bounds reads by staging into a
		// zero-padded scratch register.
		if constexpr (count < simd::element_count)
		{
			std::array<read_t, simd::element_count> temp{};
			std::memcpy(temp.data(), read.data(), count * sizeof(read_t));
			const auto v = simd::load(std::span<const read_t, simd::element_count>(temp));
			const auto mask = simd::movemask_slim(simd::cmpeq(v, predicateVector));
			return (LowBits(static_cast<std::uint32_t>(mask), count) != 0);
		}
		else
		{
			constexpr size_t lanes = simd::element_count;
			size_t i = 0;
			for (; i + lanes <= count; i += lanes)
			{
				const auto v = simd::load_unsafe(read.subspan(i, lanes));
				const auto mask = simd::movemask_slim(simd::cmpeq(v, predicateVector));
				if (mask != 0)
				{
					return true;
				}
			}

			if constexpr (count % lanes != 0)
			{
				std::array<read_t, lanes> temp{};
				std::memcpy(temp.data(), read.data() + i, (count - i) * sizeof(read_t));
				const auto v = simd::load(std::span<const read_t, lanes>(temp));
				const auto mask = simd::movemask_slim(simd::cmpeq(v, predicateVector));
				return (mask & ((typename simd::mask_t{1} << (count - i)) - 1)) != 0;
			}
			else
			{
				return false;
			}
		}
	}

	/**
	 * @brief Reports whether every input element equals a scalar predicate.
	 * @tparam count Fixed input element count.
	 * @param read Input
	 * elements to inspect.
	 * @param predicate Scalar value required in every element.
	 * @return `true` when every element equals `predicate`.
	 */
	template <std::size_t count> [[nodiscard]] constexpr static inline bool AllEqual(std::span<const read_t, count> read, const read_t predicate) noexcept
	{
		using simd = SimdImpl<count>;
		const auto predicateVector = simd::set1(predicate);
		constexpr size_t lanes = simd::element_count;
		constexpr typename simd::mask_t fullMask = []
		{
			if constexpr (lanes >= std::numeric_limits<typename simd::mask_t>::digits)
				return std::numeric_limits<typename simd::mask_t>::max();
			else
				return static_cast<typename simd::mask_t>((typename simd::mask_t{1} << lanes) - 1);
		}();

		if constexpr (count < lanes)
		{
			std::array<read_t, lanes> temp{};
			std::memcpy(temp.data(), read.data(), count * sizeof(read_t));
			const auto v = simd::load(std::span<const read_t, lanes>(temp));
			const auto mask = simd::movemask_slim(simd::cmpeq(v, predicateVector));
			const auto needed = (typename simd::mask_t{1} << count) - 1;
			return (mask & needed) == needed;
		}
		else
		{
			size_t i = 0;
			for (; i + lanes <= count; i += lanes)
			{
				const auto v = simd::load_unsafe(read.subspan(i, lanes));
				const auto mask = simd::movemask_slim(simd::cmpeq(v, predicateVector));
				if (mask != fullMask)
				{
					return false;
				}
			}

			if constexpr (count % lanes != 0)
			{
				std::array<read_t, lanes> temp{};
				std::memcpy(temp.data(), read.data() + i, (count - i) * sizeof(read_t));
				const auto v = simd::load(std::span<const read_t, lanes>(temp));
				const auto mask = simd::movemask_slim(simd::cmpeq(v, predicateVector));
				const auto needed = (typename simd::mask_t{1} << (count - i)) - 1;
				return (mask & needed) == needed;
			}
			else
			{
				return true;
			}
		}
	}

	template <std::size_t count>
	constexpr static inline void Compare(std::span<const read_t, count> read, std::span<write_t, count / write_data_size> write,
										 const read_t predicate) noexcept
	{
		using simd = SimdImpl<count>;
		constexpr bool legacy_eight_byte_comparison = ReadWidth == 8 && WriteWidth == 8 && count == 8;
		static_assert(WriteWidth == 1 || legacy_eight_byte_comparison, "SimdAlgo comparison output is a packed one-bit mask");
		static_assert(count % write_data_size == 0, "Packed comparison output requires a whole number of destination elements");
		const auto predicateVector = simd::set1(predicate);
		simd::template transform_pack<1>(read, write, [&predicateVector](const typename simd::vector_t value) noexcept
										 { return simd::movemask_slim(simd::cmpeq(value, predicateVector)); });
	}

#pragma region Bitwise Operations (constrained)

	template <std::size_t count>
	constexpr static inline void BitwiseAnd(std::span<const read_t, count> lhs, std::span<const read_t, count> rhs, std::span<read_t, count> write) noexcept
	{
		using simd = SimdImpl<count>;
		simd::transform(lhs, rhs, write, simd::bitwise_and);
	}

	template <std::size_t count>
	constexpr static inline void BitwiseOr(std::span<const read_t, count> lhs, std::span<const read_t, count> rhs, std::span<read_t, count> write) noexcept
	{
		using simd = SimdImpl<count>;
		simd::transform(lhs, rhs, write, simd::bitwise_or);
	}

	template <std::size_t count>
	constexpr static inline void BitwiseXor(std::span<const read_t, count> lhs, std::span<const read_t, count> rhs, std::span<read_t, count> write) noexcept
	{
		using simd = SimdImpl<count>;
		simd::transform(lhs, rhs, write, simd::bitwise_xor);
	}

	template <std::size_t count> constexpr static inline void BitwiseNot(std::span<const read_t, count> lhs, std::span<read_t, count> write) noexcept
	{
		using simd = SimdImpl<count>;
		simd::transform(lhs, write, simd::bitwise_not);
	}

	template <std::size_t count>
	constexpr static inline void BitwiseAndNot(std::span<const read_t, count> lhs, std::span<const read_t, count> rhs, std::span<read_t, count> write) noexcept
	{
		using simd = SimdImpl<count>;
		simd::transform(lhs, rhs, write, simd::bitwise_andnot);
	}

#pragma endregion

#pragma region Bitwise Operations (non-constrained)

	static inline void BitwiseAnd(std::span<const read_t> lhs, std::span<const read_t> rhs, std::span<read_t> write) noexcept
	{
		SIMDLIB_PRECONDITION(lhs.size() == rhs.size() && rhs.size() == write.size(), "lhs, rhs and write must have the same size");
		const auto count = lhs.size();

		ChooseSimd(
			count,
			[&](simd_128_tag)
			{
				using simd = Api<128, read_t>;
				simd::transform(lhs, rhs, write, simd::bitwise_and);
			},
			[&](simd_256_tag)
			{
				using simd = Api<256, read_t>;
				simd::transform(lhs, rhs, write, simd::bitwise_and);
			});
	}

	static inline void BitwiseOr(std::span<const read_t> lhs, std::span<const read_t> rhs, std::span<read_t> write) noexcept
	{
		SIMDLIB_PRECONDITION(lhs.size() == rhs.size() && rhs.size() == write.size(), "lhs, rhs and write must have the same size");
		const auto count = lhs.size();

		ChooseSimd(
			count,
			[&](simd_128_tag)
			{
				using simd = Api<128, read_t>;
				simd::transform(lhs, rhs, write, simd::bitwise_or);
			},
			[&](simd_256_tag)
			{
				using simd = Api<256, read_t>;
				simd::transform(lhs, rhs, write, simd::bitwise_or);
			});
	}

	static inline void BitwiseXor(std::span<const read_t> lhs, std::span<const read_t> rhs, std::span<read_t> write) noexcept
	{
		SIMDLIB_PRECONDITION(lhs.size() == rhs.size() && rhs.size() == write.size(), "lhs, rhs and write must have the same size");
		const auto count = lhs.size();

		ChooseSimd(
			count,
			[&](simd_128_tag)
			{
				using simd = Api<128, read_t>;
				simd::transform(lhs, rhs, write, simd::bitwise_xor);
			},
			[&](simd_256_tag)
			{
				using simd = Api<256, read_t>;
				simd::transform(lhs, rhs, write, simd::bitwise_xor);
			});
	}

	static inline void BitwiseNot(std::span<const read_t> lhs, std::span<read_t> write) noexcept
	{
		SIMDLIB_PRECONDITION(lhs.size() == write.size(), "lhs and write must have the same size");
		const auto count = lhs.size();

		ChooseSimd(
			count,
			[&](simd_128_tag)
			{
				using simd = Api<128, read_t>;
				simd::transform(lhs, write, simd::bitwise_not);
			},
			[&](simd_256_tag)
			{
				using simd = Api<256, read_t>;
				simd::transform(lhs, write, simd::bitwise_not);
			});
	}

	static inline void BitwiseAndNot(std::span<const read_t> lhs, std::span<const read_t> rhs, std::span<read_t> write) noexcept
	{
		SIMDLIB_PRECONDITION(lhs.size() == rhs.size() && rhs.size() == write.size(), "lhs, rhs and write must have the same size");
		const auto count = lhs.size();

		ChooseSimd(
			count,
			[&](simd_128_tag)
			{
				using simd = Api<128, read_t>;
				simd::transform(lhs, rhs, write, simd::bitwise_andnot);
			},
			[&](simd_256_tag)
			{
				using simd = Api<256, read_t>;
				simd::transform(lhs, rhs, write, simd::bitwise_andnot);
			});
	}

#pragma endregion

#pragma region Type Conversion Casting
	/// <summary>
	/// Convert the data in the span from one type to another, storing the converted data in the same span.
	/// </summary>
	/// <typeparam name="SrcType"></typeparam>
	/// <typeparam name="DstType"></typeparam>
	/// <param name="data"></param>
	/*template <typename InType, typename OutType>
		requires std::is_arithmetic_v<InType> && std::is_arithmetic_v<OutType>
	static inline std::span<OutType> ConvertInplace(std::span<InType> data) noexcept
	{
		const auto count = data.size();
		ChooseSimd(
			count,
			[&](simd_128_tag) {
				using simd = Simd<128, InType>;
				simd::transform_unsafe(std::as_writable_bytes(data), simd::convert);
			},
			[&](simd_256_tag) {
				using simd = Simd<256, InType>;
				simd::transform_unsafe(std::as_writable_bytes(data), simd::convert);
			});
		return std::span<OutType>(std::launder(reinterpret_cast<OutType*>(data.data())), count);
	}*/
#pragma endregion

  protected:
#pragma region Simd Chooser
	// tags to denote the type of the function to be called in the ChooseSimd
	struct simd_128_tag
	{
	};

	struct simd_256_tag
	{
	};

	template <std::invocable<simd_128_tag> Select128, std::invocable<simd_256_tag> Select256>
	constexpr static void SIMD_FLAGS(Neither, ForceInline, Flatten) ChooseSimd(std::size_t element_count, Select128 &&select128, Select256 &&select256) noexcept
	{
		if (element_count * read_data_size >= 256)
			std::invoke(select256, simd_256_tag{});
		else
			std::invoke(select128, simd_128_tag{});
	}

#pragma endregion
};

} // namespace SimdLib
