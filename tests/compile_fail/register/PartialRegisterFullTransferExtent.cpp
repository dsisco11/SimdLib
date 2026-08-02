#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/PartialRegister.h>

#include <cstdint>
#include <span>

using partial_register_type = SimdLib::PartialRegister<std::int32_t, 128, 3>;

/** @brief Reports whether a full-native-width element load bypasses the active transfer extent. */
template <class value_t>
concept accepts_full_element_load = requires(std::span<const typename value_t::element_type, value_t::native_lane_count> source) {
	value_t::load(source);
};

/** @brief Reports whether a full-native-width aligned element load bypasses the active transfer extent. */
template <class value_t>
concept accepts_full_aligned_element_load = requires(std::span<const typename value_t::element_type, value_t::native_lane_count> source) {
	value_t::load_aligned(source);
};

/** @brief Reports whether a full-native-width byte load bypasses the active byte extent. */
template <class value_t>
concept accepts_full_byte_load = requires(std::span<const std::byte, value_t::byte_count> source) { value_t::load_bytes(source); };

/** @brief Reports whether a full-native-width element store bypasses the active transfer extent. */
template <class value_t>
concept accepts_full_element_store = requires(value_t value, std::span<typename value_t::element_type, value_t::native_lane_count> destination) {
	value.store(destination);
};

/** @brief Reports whether a full-native-width aligned element store bypasses the active transfer extent. */
template <class value_t>
concept accepts_full_aligned_element_store = requires(value_t value, std::span<typename value_t::element_type, value_t::native_lane_count> destination) {
	value.store_aligned(destination);
};

/** @brief Reports whether a full-native-width byte store bypasses the active byte extent. */
template <class value_t>
concept accepts_full_byte_store = requires(value_t value, std::span<std::byte, value_t::byte_count> destination) { value.store_bytes(destination); };

static_assert(accepts_full_element_load<partial_register_type> || accepts_full_aligned_element_load<partial_register_type> ||
				  accepts_full_byte_load<partial_register_type> || accepts_full_element_store<partial_register_type> ||
				  accepts_full_aligned_element_store<partial_register_type> || accepts_full_byte_store<partial_register_type>,
	"SIMDLIB_PARTIAL_REGISTER_REJECTS_FULL_TRANSFER_EXTENT");
