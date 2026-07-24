#define SIMDLIB_HAS_SSE42 1
#include <SimdLib/Register.h>

#include <cstdint>

using register_type = SimdLib::Register<std::int16_t, 128>;

/** @brief Reports whether any immediate-controlled rearrangement accepts a negative control. */
template <class value_t>
concept accepts_negative_immediate = requires(value_t value) {
	value.template shuffle_low<-1>();
	value.template shuffle_high<-1>();
	value.template blend<-1>(value);
};

/** @brief Reports whether any immediate-controlled rearrangement accepts a control above one byte. */
template <class value_t>
concept accepts_oversized_immediate = requires(value_t value) {
	value.template shuffle_low<256>();
	value.template shuffle_high<256>();
	value.template blend<256>(value);
};

static_assert(accepts_negative_immediate<register_type> || accepts_oversized_immediate<register_type>,
			  "SIMDLIB_REGISTER_REJECTS_INVALID_REARRANGEMENT_IMMEDIATE");
