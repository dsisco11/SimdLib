#pragma once

#include <SimdLib/Register.h>

#include <cstdint>

namespace SimdLib
{

#if SIMDLIB_HAS_SSE42
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
