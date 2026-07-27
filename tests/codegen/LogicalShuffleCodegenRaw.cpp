#include <SimdLib/Api.h>

#include <cstddef>
#include <cstdint>

#if SIMDLIB_COMPILER_MSVC
#define SIMDLIB_LOGICAL_SHUFFLE_CODEGEN_NOINLINE __declspec(noinline)
#else
#define SIMDLIB_LOGICAL_SHUFFLE_CODEGEN_NOINLINE __attribute__((noinline))
#endif

namespace SimdLibLogicalShuffleCodegen
{

/** @brief Native register type for one direct-intrinsic logical-shuffle fixture. */
template <class element_t> using native_t = typename SimdLib::Api<SIMDLIB_REGISTER_TEST_WIDTH, element_t>::vector_t;

} // namespace SimdLibLogicalShuffleCodegen

#define SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(token, type, expression)                                                                                            \
	/** @brief Emits the direct-intrinsic reference for one logical shuffle cell. */                                                                           \
	SIMDLIB_REGISTER_ONLY SIMDLIB_LOGICAL_SHUFFLE_CODEGEN_NOINLINE SimdLibLogicalShuffleCodegen::native_t<type> VECTORCALL                                     \
	simdlib_rearrangement_codegen_logical_shuffle_##token(SimdLibLogicalShuffleCodegen::native_t<type> value) noexcept                                         \
	{                                                                                                                                                          \
		return expression;                                                                                                                                     \
	}

#if SIMDLIB_REGISTER_TEST_WIDTH == 128
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(i8, std::int8_t, _mm_shuffle_epi8(value, _mm_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(u8, std::uint8_t, _mm_shuffle_epi8(value, _mm_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(i16, std::int16_t, _mm_shuffle_epi8(value, _mm_setr_epi8(14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1)))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(u16, std::uint16_t, _mm_shuffle_epi8(value, _mm_setr_epi8(14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1)))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(i32, std::int32_t, _mm_shuffle_epi32(value, 0x1B))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(u32, std::uint32_t, _mm_shuffle_epi32(value, 0x1B))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(i64, std::int64_t, _mm_shuffle_epi32(value, 0x4E))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(u64, std::uint64_t, _mm_shuffle_epi32(value, 0x4E))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(f32, float, _mm_shuffle_ps(value, value, 0x1B))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(f64, double, _mm_shuffle_pd(value, value, 0x1))
#else
#define SIMDLIB_LOGICAL_SHUFFLE_LOCAL_BYTES                                                                                                                    \
	_mm256_setr_epi8(0, -128, 2, -128, 4, -128, 6, -128, 8, -128, 10, -128, 12, -128, 14, -128, 0, -128, 2, -128, 4, -128, 6, -128, 8, -128, 10, -128, 12,     \
					 -128, 14, -128)
#define SIMDLIB_LOGICAL_SHUFFLE_CROSS_BYTES                                                                                                                    \
	_mm256_setr_epi8(-128, 1, -128, 3, -128, 5, -128, 7, -128, 9, -128, 11, -128, 13, -128, 15, -128, 1, -128, 3, -128, 5, -128, 7, -128, 9, -128, 11, -128,   \
					 13, -128, 15)
#define SIMDLIB_LOGICAL_SHUFFLE_LOCAL_WORDS                                                                                                                    \
	_mm256_setr_epi8(0, 1, -128, -128, 4, 5, -128, -128, 8, 9, -128, -128, 12, 13, -128, -128, 0, 1, -128, -128, 4, 5, -128, -128, 8, 9, -128, -128, 12, 13,   \
					 -128, -128)
#define SIMDLIB_LOGICAL_SHUFFLE_CROSS_WORDS                                                                                                                    \
	_mm256_setr_epi8(-128, -128, 2, 3, -128, -128, 6, 7, -128, -128, 10, 11, -128, -128, 14, 15, -128, -128, 2, 3, -128, -128, 6, 7, -128, -128, 10, 11, -128, \
					 -128, 14, 15)
#define SIMDLIB_RAW_MIXED_BYTE_SHUFFLE(value, local_control, cross_control)                                                                                    \
	_mm256_or_si256(_mm256_shuffle_epi8(value, local_control), _mm256_shuffle_epi8(_mm256_permute2x128_si256(value, value, 0x01), cross_control))

SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(i8, std::int8_t,
								   SIMDLIB_RAW_MIXED_BYTE_SHUFFLE(value, SIMDLIB_LOGICAL_SHUFFLE_LOCAL_BYTES, SIMDLIB_LOGICAL_SHUFFLE_CROSS_BYTES))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(u8, std::uint8_t,
								   SIMDLIB_RAW_MIXED_BYTE_SHUFFLE(value, SIMDLIB_LOGICAL_SHUFFLE_LOCAL_BYTES, SIMDLIB_LOGICAL_SHUFFLE_CROSS_BYTES))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(i16, std::int16_t,
								   SIMDLIB_RAW_MIXED_BYTE_SHUFFLE(value, SIMDLIB_LOGICAL_SHUFFLE_LOCAL_WORDS, SIMDLIB_LOGICAL_SHUFFLE_CROSS_WORDS))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(u16, std::uint16_t,
								   SIMDLIB_RAW_MIXED_BYTE_SHUFFLE(value, SIMDLIB_LOGICAL_SHUFFLE_LOCAL_WORDS, SIMDLIB_LOGICAL_SHUFFLE_CROSS_WORDS))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(i32, std::int32_t, _mm256_permutevar8x32_epi32(value, _mm256_setr_epi32(7, 6, 5, 4, 3, 2, 1, 0)))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(u32, std::uint32_t, _mm256_permutevar8x32_epi32(value, _mm256_setr_epi32(7, 6, 5, 4, 3, 2, 1, 0)))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(i64, std::int64_t, _mm256_permute4x64_epi64(value, 0x1B))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(u64, std::uint64_t, _mm256_permute4x64_epi64(value, 0x1B))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(f32, float, _mm256_permutevar8x32_ps(value, _mm256_setr_epi32(7, 6, 5, 4, 3, 2, 1, 0)))
SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE(f64, double, _mm256_permute4x64_pd(value, 0x1B))

#undef SIMDLIB_RAW_MIXED_BYTE_SHUFFLE
#undef SIMDLIB_LOGICAL_SHUFFLE_CROSS_WORDS
#undef SIMDLIB_LOGICAL_SHUFFLE_LOCAL_WORDS
#undef SIMDLIB_LOGICAL_SHUFFLE_CROSS_BYTES
#undef SIMDLIB_LOGICAL_SHUFFLE_LOCAL_BYTES
#endif

#undef SIMDLIB_DEFINE_RAW_LOGICAL_SHUFFLE
#undef SIMDLIB_LOGICAL_SHUFFLE_CODEGEN_NOINLINE