#pragma once

#include <SimdLib/SimdLib.h>

#include <cstdint>
#include <immintrin.h>

namespace SimdLibConsumer
{
using Register = SimdLib::Register<std::uint32_t, 128>;
using RegisterMask = Register::mask_type;
using native_type = __m128i;

/**
 * @brief Increments every lane of a downstream Register value.
 * @param value Input register.
 * @return Input register increased by one in every lane.
 */
[[nodiscard]] Register SIMD_FLAGS(InOut, RegisterOnly) increment(Register value) noexcept;

/**
 * @brief Increments every lane of a downstream native SIMD value.
 * @param value Input native register.
 * @return Input register increased by one in every lane.
 */
[[nodiscard]] native_type SIMD_FLAGS(InOut, RegisterOnly) increment_native(native_type value) noexcept;
} // namespace SimdLibConsumer
