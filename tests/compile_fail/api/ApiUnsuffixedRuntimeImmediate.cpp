#define SIMDLIB_HAS_SSE42 1
#define SIMDLIB_HAS_AVX2 1
#include <SimdLib/Api.h>

#include <cstdint>

using byte_api = SimdLib::Api<128, std::uint8_t>;
using half_api = SimdLib::Api<128, std::uint16_t>;
using word_api = SimdLib::Api<128, std::uint32_t>;
using float_api = SimdLib::Api<128, float>;
using byte_impl = SimdLib::Detail::SimdMappings<128, std::uint8_t>;
using half_impl = SimdLib::Detail::SimdMappings<128, std::uint16_t>;
using word_impl = SimdLib::Detail::SimdMappings<128, std::uint32_t>;
using float_impl = SimdLib::Detail::SimdMappings<128, float>;

/** @brief Reports whether unsuffixed Api extraction accepts a runtime lane index. */
template <class api_t>
concept api_accepts_runtime_extract = requires(typename api_t::vector_t value, int control) { api_t::extract(value, control); };
/** @brief Reports whether unsuffixed Api insertion accepts a runtime lane index. */
template <class api_t>
concept api_accepts_runtime_insert =
	requires(typename api_t::vector_t value, typename api_t::element_type lane, int control) { api_t::insert(value, lane, control); };
/** @brief Reports whether unsuffixed Api blend accepts a runtime immediate mask. */
template <class api_t>
concept api_accepts_runtime_blend = requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs, int control) { api_t::blend(lhs, rhs, control); };
/** @brief Reports whether unsuffixed Api floating shuffle accepts a runtime immediate mask. */
template <class api_t>
concept api_accepts_runtime_shuffle = requires(typename api_t::vector_t lhs, typename api_t::vector_t rhs, int control) { api_t::shuffle(lhs, rhs, control); };
/** @brief Reports whether an unsuffixed native register-selector shuffle accepts a scalar selector. */
template <class api_t>
concept api_accepts_scalar_shuffle_selector = requires(typename api_t::vector_t value, int control) { api_t::shuffle(value, control); };
/** @brief Reports whether unsuffixed Api low-half shuffle accepts a runtime immediate mask. */
template <class api_t>
concept api_accepts_runtime_shuffle_low = requires(typename api_t::vector_t value, int control) { api_t::shuffle_lo(value, control); };
/** @brief Reports whether unsuffixed Api high-half shuffle accepts a runtime immediate mask. */
template <class api_t>
concept api_accepts_runtime_shuffle_high = requires(typename api_t::vector_t value, int control) { api_t::shuffle_hi(value, control); };
/** @brief Reports whether unsuffixed Api 32-bit shuffle accepts a runtime immediate mask. */
template <class api_t>
concept api_accepts_runtime_shuffle_32 = requires(typename api_t::int_vector_t value, int control) { api_t::shuffle_32(value, control); };
/** @brief Reports whether unsuffixed Api byte shift accepts a runtime count. */
template <class api_t>
concept api_accepts_runtime_byte_shift = requires(typename api_t::int_vector_t value, int control) {
	api_t::byte_shift_left(value, control);
	api_t::byte_shift_right(value, control);
};
/** @brief Reports whether unsuffixed Api complete-register bit shift accepts a runtime count. */
template <class api_t>
concept api_accepts_runtime_bit_shift = requires(typename api_t::int_vector_t value, int control) {
	api_t::bit_shift_left(value, control);
	api_t::bit_shift_right(value, control);
};

/** @brief Reports whether unsuffixed implementation extraction accepts a runtime lane index. */
template <class impl_t>
concept impl_accepts_runtime_extract = requires(typename impl_t::vector_t value, int control) { impl_t::extract(value, control); };
/** @brief Reports whether unsuffixed implementation insertion accepts a runtime lane index. */
template <class impl_t>
concept impl_accepts_runtime_insert = requires(typename impl_t::vector_t value, std::uint32_t lane, int control) { impl_t::insert(value, lane, control); };
/** @brief Reports whether unsuffixed implementation blend accepts a runtime immediate mask. */
template <class impl_t>
concept impl_accepts_runtime_blend = requires(typename impl_t::vector_t lhs, typename impl_t::vector_t rhs, int control) { impl_t::blend(lhs, rhs, control); };
/** @brief Reports whether unsuffixed implementation floating shuffle accepts a runtime immediate mask. */
template <class impl_t>
concept impl_accepts_runtime_shuffle =
	requires(typename impl_t::vector_t lhs, typename impl_t::vector_t rhs, int control) { impl_t::shuffle(lhs, rhs, control); };
/** @brief Reports whether a native implementation shuffle accepts a scalar selector. */
template <class impl_t>
concept impl_accepts_scalar_shuffle_selector = requires(typename impl_t::vector_t value, int control) { impl_t::shuffle(value, control); };
/** @brief Reports whether unsuffixed implementation low-half shuffle accepts a runtime immediate mask. */
template <class impl_t>
concept impl_accepts_runtime_shuffle_low = requires(typename impl_t::vector_t value, int control) { impl_t::shuffle_lo(value, control); };
/** @brief Reports whether unsuffixed implementation high-half shuffle accepts a runtime immediate mask. */
template <class impl_t>
concept impl_accepts_runtime_shuffle_high = requires(typename impl_t::vector_t value, int control) { impl_t::shuffle_hi(value, control); };
/** @brief Reports whether unsuffixed implementation 32-bit shuffle accepts a runtime immediate mask. */
template <class impl_t>
concept impl_accepts_runtime_shuffle_32 = requires(typename impl_t::int_vector_t value, int control) { impl_t::shuffle_32(value, control); };
/** @brief Reports whether unsuffixed implementation byte shift accepts a runtime count. */
template <class impl_t>
concept impl_accepts_runtime_byte_shift = requires(typename impl_t::int_vector_t value, int control) {
	impl_t::byte_shift_left(value, control);
	impl_t::byte_shift_right(value, control);
};
/** @brief Reports whether unsuffixed implementation complete-register bit shift accepts a runtime count. */
template <class impl_t>
concept impl_accepts_runtime_bit_shift = requires(typename impl_t::int_vector_t value, int control) {
	impl_t::bit_shift_left(value, control);
	impl_t::bit_shift_right(value, control);
};

static_assert(api_accepts_runtime_extract<word_api> || api_accepts_runtime_insert<word_api> || api_accepts_runtime_blend<word_api> ||
				  api_accepts_runtime_blend<byte_api> || api_accepts_runtime_shuffle<float_api> || api_accepts_scalar_shuffle_selector<byte_api> ||
				  api_accepts_runtime_shuffle_low<half_api> || api_accepts_runtime_shuffle_high<half_api> || api_accepts_runtime_shuffle_32<word_api> ||
				  api_accepts_runtime_byte_shift<byte_api> || api_accepts_runtime_bit_shift<word_api> || impl_accepts_runtime_extract<word_impl> ||
				  impl_accepts_runtime_insert<word_impl> || impl_accepts_runtime_blend<word_impl> || impl_accepts_runtime_blend<byte_impl> ||
				  impl_accepts_runtime_shuffle<float_impl> || impl_accepts_scalar_shuffle_selector<byte_impl> || impl_accepts_runtime_shuffle_low<half_impl> ||
				  impl_accepts_runtime_shuffle_high<half_impl> || impl_accepts_runtime_shuffle_32<word_impl> || impl_accepts_runtime_byte_shift<byte_impl> ||
				  impl_accepts_runtime_bit_shift<word_impl>,
			  "SIMDLIB_REJECTS_UNSUFFIXED_RUNTIME_IMMEDIATE_CONTROLS");