#define SIMDLIB_HAS_SSE42 1
#define SIMDLIB_HAS_AVX2 1
#include <SimdLib/Api.h>
#include <SimdLib/IImpl.h>
#include <SimdLib/Register.h>

#include <cstdint>

namespace
{

/** @brief Reports whether an Api accepts an unsuffixed runtime byte count. */
template <class api_t>
concept api_accepts_runtime_byte_shift = requires(typename api_t::int_vector_t value, int count) {
	api_t::shift_bytes_left(value, count);
	api_t::shift_bytes_right(value, count);
};

/** @brief Reports whether an implementation accepts an unsuffixed runtime byte count. */
template <class implementation_t>
concept implementation_accepts_runtime_byte_shift = requires(typename implementation_t::int_vector_t value, int count) {
	implementation_t::shift_bytes_left(value, count);
	implementation_t::shift_bytes_right(value, count);
};

/** @brief Reports whether a Register accepts an unsuffixed runtime byte count. */
template <class register_t>
concept register_accepts_runtime_byte_shift = requires(register_t value, int count) {
	value.shift_bytes_left(count);
	value.shift_bytes_right(count);
};

/** @brief Verifies immediate byte-shift availability for one integral lane type. */
template <class element_t> consteval bool integral_availability_contract()
{
	using api128 = SimdLib::Api<128, element_t>;
	using api256 = SimdLib::Api<256, element_t>;
	using implementation128 = SimdLib::Detail::SimdMappings<128, element_t>;
	using implementation256 = SimdLib::Detail::SimdMappings<256, element_t>;
	using register128 = SimdLib::Register<element_t, 128>;
	using register256 = SimdLib::Register<element_t, 256>;
	return SimdLib::IApi::ShiftBytesLeft<api128, 1> && SimdLib::IApi::ShiftBytesRight<api128, 1> && SimdLib::IApi::ShiftBytesLeft<api256, 17> &&
		   SimdLib::IApi::ShiftBytesRight<api256, 17> && SimdLib::IImpl::ShiftBytesLeft<implementation128, 1> &&
		   SimdLib::IImpl::ShiftBytesRight<implementation128, 1> && SimdLib::IImpl::ShiftBytesLeft<implementation256, 17> &&
		   SimdLib::IImpl::ShiftBytesRight<implementation256, 17> && SimdLib::IRegister::ShiftBytesLeft<register128, 1> &&
		   SimdLib::IRegister::ShiftBytesRight<register128, 1> && SimdLib::IRegister::ShiftBytesLeft<register256, 17> &&
		   SimdLib::IRegister::ShiftBytesRight<register256, 17> && !api_accepts_runtime_byte_shift<api128> && !api_accepts_runtime_byte_shift<api256> &&
		   !implementation_accepts_runtime_byte_shift<implementation128> && !implementation_accepts_runtime_byte_shift<implementation256> &&
		   !register_accepts_runtime_byte_shift<register128> && !register_accepts_runtime_byte_shift<register256>;
}

} // namespace

static_assert(integral_availability_contract<std::int8_t>());
static_assert(integral_availability_contract<std::uint8_t>());
static_assert(integral_availability_contract<std::int16_t>());
static_assert(integral_availability_contract<std::uint16_t>());
static_assert(integral_availability_contract<std::int32_t>());
static_assert(integral_availability_contract<std::uint32_t>());
static_assert(integral_availability_contract<std::int64_t>());
static_assert(integral_availability_contract<std::uint64_t>());
static_assert(!SimdLib::IApi::ShiftBytesLeft<SimdLib::Api<128, float>, 1>);
static_assert(!SimdLib::IApi::ShiftBytesRight<SimdLib::Api<256, double>, 1>);
static_assert(!SimdLib::IRegister::ShiftBytesLeft<SimdLib::Register<float, 128>, 1>);
static_assert(!SimdLib::IRegister::ShiftBytesRight<SimdLib::Register<double, 256>, 1>);
static_assert(!SimdLib::IApi::ShiftBytesLeft<SimdLib::Api<128, std::uint8_t>, -1>);
static_assert(!SimdLib::IRegister::ShiftBytesRight<SimdLib::Register<std::uint8_t, 256>, -1>);