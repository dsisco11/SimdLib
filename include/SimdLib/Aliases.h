#pragma once

#include <SimdLib/PartialRegister.h>

#include <cstddef>
#include <cstdint>

namespace SimdLib
{

#if SIMDLIB_HAS_SSE42
#pragma region Partial Register Type Aliases (Unsigned)

/** @brief 128-bit unsigned-byte PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::uint8_t, 128, active_lane_count>
using partial_uint8x16 = PartialRegister<std::uint8_t, 128, active_lane_count>;

/** @brief 128-bit unsigned-word PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::uint16_t, 128, active_lane_count>
using partial_uint16x8 = PartialRegister<std::uint16_t, 128, active_lane_count>;

/** @brief 128-bit unsigned-doubleword PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::uint32_t, 128, active_lane_count>
using partial_uint32x4 = PartialRegister<std::uint32_t, 128, active_lane_count>;

/** @brief 128-bit unsigned-quadword PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::uint64_t, 128, active_lane_count>
using partial_uint64x2 = PartialRegister<std::uint64_t, 128, active_lane_count>;

#pragma endregion

#pragma region Partial Register Type Aliases (Signed)

/** @brief 128-bit signed-byte PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::int8_t, 128, active_lane_count>
using partial_int8x16 = PartialRegister<std::int8_t, 128, active_lane_count>;

/** @brief 128-bit signed-word PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::int16_t, 128, active_lane_count>
using partial_int16x8 = PartialRegister<std::int16_t, 128, active_lane_count>;

/** @brief 128-bit signed-doubleword PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::int32_t, 128, active_lane_count>
using partial_int32x4 = PartialRegister<std::int32_t, 128, active_lane_count>;

/** @brief 128-bit signed-quadword PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::int64_t, 128, active_lane_count>
using partial_int64x2 = PartialRegister<std::int64_t, 128, active_lane_count>;

#pragma endregion

#if SIMDLIB_HAS_AVX2
#pragma region Partial Register Type Aliases (Unsigned AVX2)

/** @brief 256-bit unsigned-byte PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::uint8_t, 256, active_lane_count>
using partial_uint8x32 = PartialRegister<std::uint8_t, 256, active_lane_count>;

/** @brief 256-bit unsigned-word PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::uint16_t, 256, active_lane_count>
using partial_uint16x16 = PartialRegister<std::uint16_t, 256, active_lane_count>;

/** @brief 256-bit unsigned-doubleword PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::uint32_t, 256, active_lane_count>
using partial_uint32x8 = PartialRegister<std::uint32_t, 256, active_lane_count>;

/** @brief 256-bit unsigned-quadword PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::uint64_t, 256, active_lane_count>
using partial_uint64x4 = PartialRegister<std::uint64_t, 256, active_lane_count>;

#pragma endregion

#pragma region Partial Register Type Aliases (Signed AVX2)

/** @brief 256-bit signed-byte PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::int8_t, 256, active_lane_count>
using partial_int8x32 = PartialRegister<std::int8_t, 256, active_lane_count>;

/** @brief 256-bit signed-word PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::int16_t, 256, active_lane_count>
using partial_int16x16 = PartialRegister<std::int16_t, 256, active_lane_count>;

/** @brief 256-bit signed-doubleword PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::int32_t, 256, active_lane_count>
using partial_int32x8 = PartialRegister<std::int32_t, 256, active_lane_count>;

/** @brief 256-bit signed-quadword PartialRegister with a caller-selected logical prefix. */
template <std::size_t active_lane_count>
	requires PartialRegisterAvailable<std::int64_t, 256, active_lane_count>
using partial_int64x4 = PartialRegister<std::int64_t, 256, active_lane_count>;

#pragma endregion
#endif

#pragma region Vector Types

// TODO: Remove these "Vector..." aliases in favor of the more descriptive "int8x16" style aliases below.
using VectorInt8 = Register<std::int8_t, 128>;
using VectorUInt8 = Register<std::uint8_t, 128>;

using VectorInt16 = Register<std::int16_t, 128>;
using VectorUInt16 = Register<std::uint16_t, 128>;

using VectorInt32 = Register<std::int32_t, 128>;
using VectorUInt32 = Register<std::uint32_t, 128>;

#if SIMDLIB_HAS_AVX2
using VectorInt64 = Register<std::int64_t, 256>;
using VectorUInt64 = Register<std::uint64_t, 256>;
#endif

#pragma endregion

#pragma region Type Aliases (Unsigned)

using uint8x16 = Register<std::uint8_t, 128>;
using uint16x8 = Register<std::uint16_t, 128>;
using uint32x4 = Register<std::uint32_t, 128>;
using uint64x2 = Register<std::uint64_t, 128>;
#if SIMDLIB_HAS_AVX2
using uint8x32 = Register<std::uint8_t, 256>;
using uint16x16 = Register<std::uint16_t, 256>;
using uint32x8 = Register<std::uint32_t, 256>;
using uint64x4 = Register<std::uint64_t, 256>;
#endif

#pragma endregion

#pragma region Type Aliases (Signed)

using int8x16 = Register<std::int8_t, 128>;
using int16x8 = Register<std::int16_t, 128>;
using int32x4 = Register<std::int32_t, 128>;
using int64x2 = Register<std::int64_t, 128>;
#if SIMDLIB_HAS_AVX2
using int8x32 = Register<std::int8_t, 256>;
using int16x16 = Register<std::int16_t, 256>;
using int32x8 = Register<std::int32_t, 256>;
using int64x4 = Register<std::int64_t, 256>;
#endif

#pragma endregion

#endif
} // namespace SimdLib
