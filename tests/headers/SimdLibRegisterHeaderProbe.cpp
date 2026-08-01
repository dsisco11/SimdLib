#include <SimdLib/SimdLib.h>

#include <concepts>
#include <cstdint>

static_assert(SIMDLIB_REGISTER_INTERFACE_AVAILABLE == 1);
static_assert(SIMDLIB_REQUIRE_REGISTER_INTERFACE == 1);

using UmbrellaRegister = SimdLib::Register<std::uint32_t, 128>;
using UmbrellaNativeRegister = SimdLib::NativeRegister<std::uint32_t>;
using UmbrellaRegisterMask = typename UmbrellaRegister::mask_type;
using UmbrellaPartialRegister = SimdLib::PartialRegister<std::uint32_t, 128, 3>;

static_assert(SimdLib::IRegister::Type<UmbrellaRegister>);
static_assert(std::same_as<SimdLib::uint32x4, UmbrellaRegister>);
static_assert(SimdLib::IRegister::Type<UmbrellaNativeRegister>);
static_assert(SimdLib::IRegisterMask::Type<UmbrellaRegisterMask>);
static_assert(UmbrellaPartialRegister::inactive_lane_count == 1);
