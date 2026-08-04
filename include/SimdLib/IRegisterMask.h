#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace SimdLib::IRegisterMask
{

/** @brief Identifies an aggregate RegisterMask-shaped type with public predicate metadata and native storage. */
template <class mask_t>
concept Type = std::is_aggregate_v<mask_t> && requires(mask_t value, typename mask_t::native_type native) {
	typename mask_t::element_type;
	typename mask_t::api_type;
	typename mask_t::native_type;
	typename mask_t::register_type;
	typename mask_t::bits_type;
	{ mask_t::register_width } -> std::convertible_to<const std::size_t &>;
	{ mask_t::byte_count } -> std::convertible_to<const std::size_t &>;
	{ mask_t::lane_count } -> std::convertible_to<const std::size_t &>;
	{ value.native } -> std::same_as<typename mask_t::native_type &>;
	{ mask_t{native} } -> std::same_as<mask_t>;
};

/** @brief Reports whether a RegisterMask type exposes an any-lane reduction. */
template <class mask_t>
concept Any = Type<mask_t> && requires(mask_t value) {
	{ value.any() } -> std::same_as<bool>;
};

/** @brief Reports whether a RegisterMask type exposes an all-lanes reduction. */
template <class mask_t>
concept All = Type<mask_t> && requires(mask_t value) {
	{ value.all() } -> std::same_as<bool>;
};

/** @brief Reports whether a RegisterMask type exposes a no-lanes reduction. */
template <class mask_t>
concept None = Type<mask_t> && requires(mask_t value) {
	{ value.none() } -> std::same_as<bool>;
};

/** @brief Reports whether a RegisterMask type exposes one compact bit per logical lane. */
template <class mask_t>
concept Bits = Type<mask_t> && requires(mask_t value) {
	{ value.bits() } -> std::same_as<typename mask_t::bits_type>;
};

/** @brief Reports whether a RegisterMask type can select corresponding lanes from two Registers. */
template <class mask_t>
concept Select = Type<mask_t> && requires(mask_t condition, typename mask_t::register_type when_true, typename mask_t::register_type when_false) {
	{ condition.select(when_true, when_false) } -> std::same_as<typename mask_t::register_type>;
};

/** @brief Reports whether a RegisterMask type exposes predicate intersection. */
template <class mask_t>
concept BitwiseAnd = Type<mask_t> && requires(mask_t lhs, mask_t rhs) {
	{ lhs & rhs } -> std::same_as<mask_t>;
};

/** @brief Reports whether a RegisterMask type exposes predicate union. */
template <class mask_t>
concept BitwiseOr = Type<mask_t> && requires(mask_t lhs, mask_t rhs) {
	{ lhs | rhs } -> std::same_as<mask_t>;
};

/** @brief Reports whether a RegisterMask type exposes predicate exclusive union. */
template <class mask_t>
concept BitwiseXor = Type<mask_t> && requires(mask_t lhs, mask_t rhs) {
	{ lhs ^ rhs } -> std::same_as<mask_t>;
};

/** @brief Reports whether a RegisterMask type exposes predicate complement. */
template <class mask_t>
concept BitwiseNot = Type<mask_t> && requires(mask_t value) {
	{ ~value } -> std::same_as<mask_t>;
};

} // namespace SimdLib::IRegisterMask
