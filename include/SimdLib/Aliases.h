#pragma once

#include <SimdLib/SimdVector.h>

#include <cstdint>

namespace SimdLib
{

#pragma region Vector Types

// TODO: Remove these "Vector..." aliases in favor of the more descriptive "int8x16" style aliases below.
using VectorInt8 = SimdVector<std::int8_t, 4>;
using VectorUInt8 = SimdVector<std::uint8_t, 4>;

using VectorInt16 = SimdVector<std::int16_t, 4>;
using VectorUInt16 = SimdVector<std::uint16_t, 4>;

using VectorInt32 = SimdVector<std::int32_t, 4>;
using VectorUInt32 = SimdVector<std::uint32_t, 4>;

using VectorInt64 = SimdVector<std::int64_t, 4>;
using VectorUInt64 = SimdVector<std::uint64_t, 4>;

#pragma endregion

// TODO: Redefine these aliases to use SimdRegister rather than SimdVector for better performance and clarity.

#pragma region Type Aliases (Unsigned)

using uint8x16 = SimdVector<std::uint8_t, 16>;
using uint8x32 = SimdVector<std::uint8_t, 32>;

using uint16x8 = SimdVector<std::uint16_t, 8>;
using uint16x16 = SimdVector<std::uint16_t, 16>;

using uint32x4 = SimdVector<std::uint32_t, 4>;
using uint32x8 = SimdVector<std::uint32_t, 8>;

using uint64x2 = SimdVector<std::uint64_t, 2>;
using uint64x4 = SimdVector<std::uint64_t, 4>;

#pragma endregion

#pragma region Type Aliases (Signed)

using int8x16 = SimdVector<std::int8_t, 16>;
using int8x32 = SimdVector<std::int8_t, 32>;

using int16x8 = SimdVector<std::int16_t, 8>;
using int16x16 = SimdVector<std::int16_t, 16>;

using int32x4 = SimdVector<std::int32_t, 4>;
using int32x8 = SimdVector<std::int32_t, 8>;

using int64x2 = SimdVector<std::int64_t, 2>;
using int64x4 = SimdVector<std::int64_t, 4>;

#pragma endregion

} // namespace SimdLib
