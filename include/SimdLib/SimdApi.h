#pragma once

// Compatibility header. New code should include <SimdLib/Api.h>.
#include <SimdLib/Api.h>

namespace SimdLib
{
template <std::size_t register_width, class element_t>
inline constexpr bool is_simd_api_available_v [[deprecated("Use SimdLib::is_api_available_v")]] =
	is_api_available_v<register_width, element_t>;

template <std::size_t register_width, class element_t>
concept SimdApiAvailable [[deprecated("Use SimdLib::ApiAvailable")]] = ApiAvailable<register_width, element_t>;

template <std::size_t register_width, class element_t>
using SimdApi [[deprecated("Use SimdLib::Api")]] = Api<register_width, element_t>;
} // namespace SimdLib
