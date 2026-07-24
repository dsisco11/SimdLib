#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/Register.h>

#include <cstdint>
#include <span>

using register_type = SimdLib::Register<std::int32_t, 128>;

/** @brief Reports whether packed collection transformation leaks into the preferred Register surface. */
template <class value_t>
concept has_transform_pack = requires(value_t value, std::span<typename value_t::element_type> data) { value.transform_pack(data); };

/** @brief Reports whether unary collection transformation leaks into the preferred Register surface. */
template <class value_t>
concept has_unary_transform = requires(value_t value, std::span<typename value_t::element_type> data) { value.transform(data); };

/** @brief Reports whether binary collection transformation leaks into the preferred Register surface. */
template <class value_t>
concept has_binary_transform = requires(value_t value, std::span<typename value_t::element_type> data) { value.transform(data, data); };

static_assert(has_transform_pack<register_type> || has_unary_transform<register_type> || has_binary_transform<register_type>,
			  "SIMDLIB_REGISTER_REJECTS_COLLECTION_OPERATIONS");
