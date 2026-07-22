#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/Register.h>

#include <cstdint>
#include <span>

using register_type = SimdLib::Register<std::int32_t, 128>;

/** @brief Reports whether a dynamic-extent load bypasses the exact-width contract. */
template <class value_t>
concept accepts_dynamic_load = requires(std::span<const typename value_t::element_type> source) {
	value_t::load(source);
};

/** @brief Reports whether a partial-load escape hatch is exposed. */
template <class value_t>
concept has_partial_load = requires(std::span<const typename value_t::element_type> source) {
	value_t::template load_partial<1>(source);
};

/** @brief Reports whether an unsafe dynamic-load escape hatch is exposed. */
template <class value_t>
concept has_unsafe_load = requires(std::span<const typename value_t::element_type> source) {
	value_t::load_unsafe(source);
};

/** @brief Reports whether a partial-store escape hatch is exposed. */
template <class value_t>
concept has_partial_store = requires(value_t value, std::span<typename value_t::element_type> destination) {
	value.template store_partial<1>(destination);
};

/** @brief Reports whether an unsafe dynamic-store escape hatch is exposed. */
template <class value_t>
concept has_unsafe_store = requires(value_t value, std::span<typename value_t::element_type> destination) {
	value.store_unsafe(destination);
};

static_assert(accepts_dynamic_load<register_type> || has_partial_load<register_type> ||
		has_unsafe_load<register_type> || has_partial_store<register_type> || has_unsafe_store<register_type>,
	"SIMDLIB_REGISTER_REJECTS_DYNAMIC_TRANSFER");
