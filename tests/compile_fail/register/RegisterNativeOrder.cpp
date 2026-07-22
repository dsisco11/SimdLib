#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/Register.h>

#include <cstdint>

using register_type = SimdLib::Register<std::int32_t, 128>;

/** @brief Reports whether a native-order construction spelling is exposed. */
template <class value_t>
concept has_native_order_constructor = requires { value_t::from_native_order(4, 3, 2, 1); };

static_assert(has_native_order_constructor<register_type>, "SIMDLIB_REGISTER_REJECTS_NATIVE_ORDER_CONSTRUCTION");
