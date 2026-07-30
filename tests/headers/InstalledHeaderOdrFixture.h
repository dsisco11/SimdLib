#pragma once

#include <SimdLib/Config.h>

/**
 * @brief Declares a cross-translation-unit function through the copied headers.
 * @param value Scalar value transformed by the definition translation unit.
 * @return The transformed scalar value.
 */
int SIMD_FLAGS(Neither) installed_header_odr_value(int value) noexcept;
