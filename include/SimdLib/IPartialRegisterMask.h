#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace SimdLib::IPartialRegisterMask
{

/** @brief Identifies an independent predicate type associated with a PartialRegister geometry. */
template <class mask_t>
concept Type =
	std::is_final_v<mask_t> && std::is_aggregate_v<mask_t> && std::is_standard_layout_v<mask_t> && requires(mask_t value, typename mask_t::native_type native) {
		typename mask_t::element_type;
		typename mask_t::api_type;
		typename mask_t::native_type;
		typename mask_t::register_type;
		typename mask_t::bits_type;
		{ mask_t::register_width } -> std::convertible_to<const std::size_t &>;
		{ mask_t::byte_count } -> std::convertible_to<const std::size_t &>;
		{ mask_t::native_lane_count } -> std::convertible_to<const std::size_t &>;
		{ mask_t::lane_count } -> std::convertible_to<const std::size_t &>;
		{ value.native } -> std::same_as<typename mask_t::native_type &>;
		{ mask_t::from_native(native) } -> std::same_as<mask_t>;
		{ value.to_native() } -> std::same_as<typename mask_t::native_type>;
	};

/** @brief Identifies an independent partial predicate that can reduce its logical lanes. */
template <class mask_t>
concept Reductions = Type<mask_t> && requires(mask_t value) {
	{ value.any() } -> std::same_as<bool>;
	{ value.all() } -> std::same_as<bool>;
	{ value.none() } -> std::same_as<bool>;
	{ value.bits() } -> std::same_as<typename mask_t::bits_type>;
};

/** @brief Identifies an independent partial predicate that supports immutable Boolean composition. */
template <class mask_t>
concept Composition = Type<mask_t> && requires(mask_t lhs, mask_t rhs) {
	{ lhs & rhs } -> std::same_as<mask_t>;
	{ lhs | rhs } -> std::same_as<mask_t>;
	{ lhs ^ rhs } -> std::same_as<mask_t>;
	{ ~lhs } -> std::same_as<mask_t>;
};

/** @brief Identifies an independent partial predicate that selects between matching partial values. */
template <class mask_t>
concept Select = Type<mask_t> && requires(mask_t condition, typename mask_t::register_type when_true, typename mask_t::register_type when_false) {
	{ condition.select(when_true, when_false) } -> std::same_as<typename mask_t::register_type>;
};

} // namespace SimdLib::IPartialRegisterMask
