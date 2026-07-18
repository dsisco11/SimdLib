#pragma once
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

namespace SimdLib
{

#pragma region Disambiguation Tags
/* Use until we get access to C++23 */
struct sorted_unique_t final
{
	constexpr explicit sorted_unique_t() = default;
};
/* Use until we get access to C++23 */
constexpr static inline sorted_unique_t sorted_unique{};
#pragma endregion

#pragma region Type Traits

// helper for selecting an integer type based on the number of bits
template <std::size_t bit_width>
using select_unsigned_integer_t =
	typename std::conditional_t<(bit_width <= 8), uint8_t,
								std::conditional_t<(bit_width <= 16), uint16_t, std::conditional_t<(bit_width <= 32), uint32_t, uint64_t>>>;

// helper for selecting a signed integer type based on the number of bit_width
template <std::size_t bit_width>
using select_signed_integer_t =
	typename std::conditional_t<(bit_width <= 8), int8_t,
								std::conditional_t<(bit_width <= 16), int16_t, std::conditional_t<(bit_width <= 32), int32_t, int64_t>>>;

#pragma endregion

#pragma region Concepts

template <class T>
concept integer_like = std::numeric_limits<T>::is_specialized && std::numeric_limits<T>::is_integer &&
	!std::same_as<std::remove_cv_t<T>, bool>;

#pragma endregion

consteval auto force_consteval(auto &&x)
{
	return x;
}

#pragma region Compile-Time Utilities

template <class... Args, class F> constexpr void constexpr_for_each(F &&func, Args &&...args)
{
	(func(std::forward<Args>(args)), ...);
}

/// Unrolls a loop from start to end, calling the function with the current index
template <auto start, auto end, auto increment, class F> constexpr void constexpr_for(F &&func)
{
	if constexpr (start < end)
	{
		func.template operator()<start>();
		constexpr_for<start + increment, end, increment>(func);
	}
}

template <class Tuple, class F> constexpr void constexpr_for_tuple(Tuple &&tuple, F &&func)
{
	constexpr size_t cnt = std::tuple_size_v<std::decay_t<Tuple>>;

	constexpr_for<size_t(0), cnt, size_t(1)>([&]<auto i>() { func(i, std::get<i>(tuple)); });
}

#pragma endregion
} // namespace SimdLib
