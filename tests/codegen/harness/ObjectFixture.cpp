#include <immintrin.h>

#if defined(_MSC_VER)
#define SIMDLIB_EXTRACTION_NOINLINE __declspec(noinline)
#else
#define SIMDLIB_EXTRACTION_NOINLINE __attribute__((noinline))
#endif

extern "C"
{
/** @brief Supplies a named relocation target whose implementation is intentionally opaque. */
int extraction_opaque(int value) noexcept;

/** @brief Exposes exact constant bytes for relocation-aware extraction checks. */
extern const double extraction_data = 3.141592653589793;

// region Scalar and control-flow fixtures
/** @brief Emits a straight-line scalar body. */
SIMDLIB_EXTRACTION_NOINLINE int extraction_straight(int value) noexcept
{
    return value + 7;
}

/** @brief Retains a conditional return and an opaque-call path in one function. */
SIMDLIB_EXTRACTION_NOINLINE int extraction_early(int value) noexcept
{
    if (value != 0)
        return value + 1;
    return extraction_opaque(value);
}

/** @brief Emits a valid minimal function without artificial work. */
SIMDLIB_EXTRACTION_NOINLINE void extraction_identity() noexcept
{
}

/** @brief Makes the named constant address observable without copying its algorithm. */
SIMDLIB_EXTRACTION_NOINLINE const double *extraction_constant() noexcept
{
    return &extraction_data;
}

// endregion
// region Vector-width fixtures
/** @brief Preserves distinct 128-bit operand and result roles in disassembly. */
SIMDLIB_EXTRACTION_NOINLINE __m128 extraction_vector128(__m128 lhs, __m128 rhs) noexcept
{
    return _mm_add_ps(lhs, rhs);
}

/** @brief Preserves the 256-bit register width independently of the 128-bit fixture. */
SIMDLIB_EXTRACTION_NOINLINE __m256 extraction_vector256(__m256 lhs, __m256 rhs) noexcept
{
    return _mm256_add_ps(lhs, rhs);
}
// endregion
}

#undef SIMDLIB_EXTRACTION_NOINLINE
