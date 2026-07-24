#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/Register.h>

#include <cstddef>
#include <cstdint>

using register_type = SimdLib::Register<std::int32_t, 128>;

/** @brief Reports whether runtime-selected lane extraction leaks into the preferred Register surface. */
template <class value_t>
concept has_runtime_extract = requires(value_t value, std::size_t index) { value.extract(index); };

/** @brief Reports whether an implementation-specific generic shuffle leaks into the preferred Register surface. */
template <class value_t>
concept has_generic_shuffle = requires(value_t value) { value.shuffle(value); };

/** @brief Reports whether an ambiguous expansion operation leaks into the preferred Register surface. */
template <class value_t>
concept has_expand = requires(value_t value) { value.expand(value); };

/** @brief Reports whether an ambiguous compression operation leaks into the preferred Register surface. */
template <class value_t>
concept has_compress = requires(value_t value) { value.compress(value); };

static_assert(has_runtime_extract<register_type> || has_generic_shuffle<register_type> || has_expand<register_type> || has_compress<register_type>,
			  "SIMDLIB_REGISTER_REJECTS_COMPATIBILITY_REARRANGEMENT");
