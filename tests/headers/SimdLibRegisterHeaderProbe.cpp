#include <SimdLib/SimdLib.h>

#include <cstdint>

static_assert(SIMDLIB_REGISTER_INTERFACE_AVAILABLE == 1);
static_assert(SIMDLIB_REQUIRE_REGISTER_INTERFACE == 1);

using UmbrellaRegister = SimdLib::Register<std::uint32_t, 128>;
using UmbrellaNativeRegister = SimdLib::NativeRegister<std::uint32_t>;
using UmbrellaRegisterMask = typename UmbrellaRegister::mask_type;

static_assert(SimdLib::IRegister::Type<UmbrellaRegister>);
static_assert(SimdLib::IRegister::Type<UmbrellaNativeRegister>);
static_assert(SimdLib::IRegisterMask::Type<UmbrellaRegisterMask>);
