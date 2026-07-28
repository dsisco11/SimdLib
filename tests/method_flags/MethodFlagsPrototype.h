#pragma once

// Isolated preprocessing prototype. Production integration belongs to the
// public-macro implementation work after this grammar has been qualified.

#define SIMDLIB_DETAIL_FLAGS_CAT_RAW(left, right) left##right
#define SIMDLIB_DETAIL_FLAGS_CAT(left, right) SIMDLIB_DETAIL_FLAGS_CAT_RAW(left, right)

#define SIMDLIB_DETAIL_FLAGS_PROBE() ~, 1
#define SIMDLIB_DETAIL_FLAGS_IS_PROBE_IMPL(_ignored, value, ...) value
#define SIMDLIB_DETAIL_FLAGS_IS_PROBE_EXPAND(arguments) SIMDLIB_DETAIL_FLAGS_IS_PROBE_IMPL arguments
#define SIMDLIB_DETAIL_FLAGS_IS_PROBE(...) SIMDLIB_DETAIL_FLAGS_IS_PROBE_EXPAND((__VA_ARGS__, 0))

#define SIMDLIB_DETAIL_FLAGS_IF_0(when_true, when_false) when_false
#define SIMDLIB_DETAIL_FLAGS_IF_1(when_true, when_false) when_true
#define SIMDLIB_DETAIL_FLAGS_IF(condition) SIMDLIB_DETAIL_FLAGS_CAT(SIMDLIB_DETAIL_FLAGS_IF_, condition)

#define SIMDLIB_DETAIL_FLAGS_OR_VALUE_00 0
#define SIMDLIB_DETAIL_FLAGS_OR_VALUE_01 1
#define SIMDLIB_DETAIL_FLAGS_OR_VALUE_10 1
#define SIMDLIB_DETAIL_FLAGS_OR_VALUE_11 1
#define SIMDLIB_DETAIL_FLAGS_OR_IMPL(lhs, rhs) SIMDLIB_DETAIL_FLAGS_OR_VALUE_##lhs##rhs
#define SIMDLIB_DETAIL_FLAGS_OR_EXPAND(lhs, rhs) SIMDLIB_DETAIL_FLAGS_OR_IMPL(lhs, rhs)
#define SIMDLIB_DETAIL_FLAGS_OR(lhs, rhs) SIMDLIB_DETAIL_FLAGS_OR_EXPAND(lhs, rhs)
#define SIMDLIB_DETAIL_FLAGS_OR_2(a, b) SIMDLIB_DETAIL_FLAGS_OR(a, b)
#define SIMDLIB_DETAIL_FLAGS_OR_3(a, b, c) SIMDLIB_DETAIL_FLAGS_OR(a, SIMDLIB_DETAIL_FLAGS_OR_2(b, c))
#define SIMDLIB_DETAIL_FLAGS_OR_4(a, b, c, d) SIMDLIB_DETAIL_FLAGS_OR(a, SIMDLIB_DETAIL_FLAGS_OR_3(b, c, d))
#define SIMDLIB_DETAIL_FLAGS_OR_5(a, b, c, d, e) SIMDLIB_DETAIL_FLAGS_OR(a, SIMDLIB_DETAIL_FLAGS_OR_4(b, c, d, e))
#define SIMDLIB_DETAIL_FLAGS_OR_6(a, b, c, d, e, f) SIMDLIB_DETAIL_FLAGS_OR(a, SIMDLIB_DETAIL_FLAGS_OR_5(b, c, d, e, f))
#define SIMDLIB_DETAIL_FLAGS_OR_10(a, b, c, d, e, f, g, h, i, j)                                                                                               \
	SIMDLIB_DETAIL_FLAGS_OR(                                                                                                                                   \
		a, SIMDLIB_DETAIL_FLAGS_OR(                                                                                                                            \
			   b, SIMDLIB_DETAIL_FLAGS_OR(                                                                                                                     \
					  c, SIMDLIB_DETAIL_FLAGS_OR(                                                                                                              \
							 d, SIMDLIB_DETAIL_FLAGS_OR(                                                                                                       \
									e, SIMDLIB_DETAIL_FLAGS_OR(f, SIMDLIB_DETAIL_FLAGS_OR(g, SIMDLIB_DETAIL_FLAGS_OR(h, SIMDLIB_DETAIL_FLAGS_OR(i, j)))))))))

#define SIMDLIB_DETAIL_FLAGS_AND_VALUE_00 0
#define SIMDLIB_DETAIL_FLAGS_AND_VALUE_01 0
#define SIMDLIB_DETAIL_FLAGS_AND_VALUE_10 0
#define SIMDLIB_DETAIL_FLAGS_AND_VALUE_11 1
#define SIMDLIB_DETAIL_FLAGS_AND_IMPL(lhs, rhs) SIMDLIB_DETAIL_FLAGS_AND_VALUE_##lhs##rhs
#define SIMDLIB_DETAIL_FLAGS_AND_EXPAND(lhs, rhs) SIMDLIB_DETAIL_FLAGS_AND_IMPL(lhs, rhs)
#define SIMDLIB_DETAIL_FLAGS_AND(lhs, rhs) SIMDLIB_DETAIL_FLAGS_AND_EXPAND(lhs, rhs)
#define SIMDLIB_DETAIL_FLAGS_AND_2(a, b) SIMDLIB_DETAIL_FLAGS_AND(a, b)
#define SIMDLIB_DETAIL_FLAGS_AND_3(a, b, c) SIMDLIB_DETAIL_FLAGS_AND(a, SIMDLIB_DETAIL_FLAGS_AND_2(b, c))
#define SIMDLIB_DETAIL_FLAGS_AND_4(a, b, c, d) SIMDLIB_DETAIL_FLAGS_AND(a, SIMDLIB_DETAIL_FLAGS_AND_3(b, c, d))
#define SIMDLIB_DETAIL_FLAGS_AND_5(a, b, c, d, e) SIMDLIB_DETAIL_FLAGS_AND(a, SIMDLIB_DETAIL_FLAGS_AND_4(b, c, d, e))

#define SIMDLIB_DETAIL_FLAGS_VALID_In SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_VALID_Out SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_VALID_RegisterOnly SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_VALID_ForceInline SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_VALID_Flatten SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_IS_VALID_IMPL(flag) SIMDLIB_DETAIL_FLAGS_IS_PROBE(SIMDLIB_DETAIL_FLAGS_VALID_##flag)
#define SIMDLIB_DETAIL_FLAGS_IS_VALID(flag) SIMDLIB_DETAIL_FLAGS_IS_VALID_IMPL(flag)

#define SIMDLIB_DETAIL_FLAGS_EMPTY_ SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_IS_EMPTY_IMPL(flag) SIMDLIB_DETAIL_FLAGS_IS_PROBE(SIMDLIB_DETAIL_FLAGS_EMPTY_##flag)
#define SIMDLIB_DETAIL_FLAGS_IS_EMPTY(flag) SIMDLIB_DETAIL_FLAGS_IS_EMPTY_IMPL(flag)

#define SIMDLIB_DETAIL_FLAGS_SAME_In_In SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_SAME_Out_Out SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_SAME_RegisterOnly_RegisterOnly SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_SAME_ForceInline_ForceInline SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_SAME_Flatten_Flatten SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_IS_SAME_IMPL(lhs, rhs) SIMDLIB_DETAIL_FLAGS_IS_PROBE(SIMDLIB_DETAIL_FLAGS_SAME_##lhs##_##rhs)
#define SIMDLIB_DETAIL_FLAGS_IS_SAME(lhs, rhs) SIMDLIB_DETAIL_FLAGS_IS_SAME_IMPL(lhs, rhs)

#define SIMDLIB_DETAIL_FLAGS_IS_In_In SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_IS_Out_Out SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_IS_RegisterOnly_RegisterOnly SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_IS_ForceInline_ForceInline SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_IS_Flatten_Flatten SIMDLIB_DETAIL_FLAGS_PROBE()
#define SIMDLIB_DETAIL_FLAGS_IS_In_IMPL(flag) SIMDLIB_DETAIL_FLAGS_IS_PROBE(SIMDLIB_DETAIL_FLAGS_IS_In_##flag)
#define SIMDLIB_DETAIL_FLAGS_IS_In(flag) SIMDLIB_DETAIL_FLAGS_IS_In_IMPL(flag)
#define SIMDLIB_DETAIL_FLAGS_IS_Out_IMPL(flag) SIMDLIB_DETAIL_FLAGS_IS_PROBE(SIMDLIB_DETAIL_FLAGS_IS_Out_##flag)
#define SIMDLIB_DETAIL_FLAGS_IS_Out(flag) SIMDLIB_DETAIL_FLAGS_IS_Out_IMPL(flag)
#define SIMDLIB_DETAIL_FLAGS_IS_RegisterOnly_IMPL(flag) SIMDLIB_DETAIL_FLAGS_IS_PROBE(SIMDLIB_DETAIL_FLAGS_IS_RegisterOnly_##flag)
#define SIMDLIB_DETAIL_FLAGS_IS_RegisterOnly(flag) SIMDLIB_DETAIL_FLAGS_IS_RegisterOnly_IMPL(flag)
#define SIMDLIB_DETAIL_FLAGS_IS_ForceInline_IMPL(flag) SIMDLIB_DETAIL_FLAGS_IS_PROBE(SIMDLIB_DETAIL_FLAGS_IS_ForceInline_##flag)
#define SIMDLIB_DETAIL_FLAGS_IS_ForceInline(flag) SIMDLIB_DETAIL_FLAGS_IS_ForceInline_IMPL(flag)
#define SIMDLIB_DETAIL_FLAGS_IS_Flatten_IMPL(flag) SIMDLIB_DETAIL_FLAGS_IS_PROBE(SIMDLIB_DETAIL_FLAGS_IS_Flatten_##flag)
#define SIMDLIB_DETAIL_FLAGS_IS_Flatten(flag) SIMDLIB_DETAIL_FLAGS_IS_Flatten_IMPL(flag)
#define SIMDLIB_DETAIL_FLAGS_IS_VECTOR_BOUNDARY(flag) SIMDLIB_DETAIL_FLAGS_OR(SIMDLIB_DETAIL_FLAGS_IS_In(flag), SIMDLIB_DETAIL_FLAGS_IS_Out(flag))

#define SIMDLIB_DETAIL_FLAGS_ALL_VALID_1(a) SIMDLIB_DETAIL_FLAGS_IS_VALID(a)
#define SIMDLIB_DETAIL_FLAGS_ALL_VALID_2(a, b) SIMDLIB_DETAIL_FLAGS_AND_2(SIMDLIB_DETAIL_FLAGS_IS_VALID(a), SIMDLIB_DETAIL_FLAGS_IS_VALID(b))
#define SIMDLIB_DETAIL_FLAGS_ALL_VALID_3(a, b, c)                                                                                                              \
	SIMDLIB_DETAIL_FLAGS_AND_3(SIMDLIB_DETAIL_FLAGS_IS_VALID(a), SIMDLIB_DETAIL_FLAGS_IS_VALID(b), SIMDLIB_DETAIL_FLAGS_IS_VALID(c))
#define SIMDLIB_DETAIL_FLAGS_ALL_VALID_4(a, b, c, d)                                                                                                           \
	SIMDLIB_DETAIL_FLAGS_AND_4(SIMDLIB_DETAIL_FLAGS_IS_VALID(a), SIMDLIB_DETAIL_FLAGS_IS_VALID(b), SIMDLIB_DETAIL_FLAGS_IS_VALID(c),                           \
							   SIMDLIB_DETAIL_FLAGS_IS_VALID(d))
#define SIMDLIB_DETAIL_FLAGS_ALL_VALID_5(a, b, c, d, e)                                                                                                        \
	SIMDLIB_DETAIL_FLAGS_AND_5(SIMDLIB_DETAIL_FLAGS_IS_VALID(a), SIMDLIB_DETAIL_FLAGS_IS_VALID(b), SIMDLIB_DETAIL_FLAGS_IS_VALID(c),                           \
							   SIMDLIB_DETAIL_FLAGS_IS_VALID(d), SIMDLIB_DETAIL_FLAGS_IS_VALID(e))

#define SIMDLIB_DETAIL_FLAGS_HAS_DUPLICATE_2(a, b) SIMDLIB_DETAIL_FLAGS_IS_SAME(a, b)
#define SIMDLIB_DETAIL_FLAGS_HAS_DUPLICATE_3(a, b, c)                                                                                                          \
	SIMDLIB_DETAIL_FLAGS_OR_3(SIMDLIB_DETAIL_FLAGS_IS_SAME(a, b), SIMDLIB_DETAIL_FLAGS_IS_SAME(a, c), SIMDLIB_DETAIL_FLAGS_IS_SAME(b, c))
#define SIMDLIB_DETAIL_FLAGS_HAS_DUPLICATE_4(a, b, c, d)                                                                                                       \
	SIMDLIB_DETAIL_FLAGS_OR_6(SIMDLIB_DETAIL_FLAGS_IS_SAME(a, b), SIMDLIB_DETAIL_FLAGS_IS_SAME(a, c), SIMDLIB_DETAIL_FLAGS_IS_SAME(a, d),                      \
							  SIMDLIB_DETAIL_FLAGS_IS_SAME(b, c), SIMDLIB_DETAIL_FLAGS_IS_SAME(b, d), SIMDLIB_DETAIL_FLAGS_IS_SAME(c, d))
#define SIMDLIB_DETAIL_FLAGS_HAS_DUPLICATE_5(a, b, c, d, e)                                                                                                    \
	SIMDLIB_DETAIL_FLAGS_OR_10(SIMDLIB_DETAIL_FLAGS_IS_SAME(a, b), SIMDLIB_DETAIL_FLAGS_IS_SAME(a, c), SIMDLIB_DETAIL_FLAGS_IS_SAME(a, d),                     \
							   SIMDLIB_DETAIL_FLAGS_IS_SAME(a, e), SIMDLIB_DETAIL_FLAGS_IS_SAME(b, c), SIMDLIB_DETAIL_FLAGS_IS_SAME(b, d),                     \
							   SIMDLIB_DETAIL_FLAGS_IS_SAME(b, e), SIMDLIB_DETAIL_FLAGS_IS_SAME(c, d), SIMDLIB_DETAIL_FLAGS_IS_SAME(c, e),                     \
							   SIMDLIB_DETAIL_FLAGS_IS_SAME(d, e))

#define SIMDLIB_DETAIL_FLAGS_ANY_1(predicate, a) predicate(a)
#define SIMDLIB_DETAIL_FLAGS_ANY_2(predicate, a, b) SIMDLIB_DETAIL_FLAGS_OR_2(predicate(a), predicate(b))
#define SIMDLIB_DETAIL_FLAGS_ANY_3(predicate, a, b, c) SIMDLIB_DETAIL_FLAGS_OR_3(predicate(a), predicate(b), predicate(c))
#define SIMDLIB_DETAIL_FLAGS_ANY_4(predicate, a, b, c, d) SIMDLIB_DETAIL_FLAGS_OR_4(predicate(a), predicate(b), predicate(c), predicate(d))
#define SIMDLIB_DETAIL_FLAGS_ANY_5(predicate, a, b, c, d, e) SIMDLIB_DETAIL_FLAGS_OR_5(predicate(a), predicate(b), predicate(c), predicate(d), predicate(e))

#define SIMDLIB_DETAIL_FLAGS_EMIT_IF_0(...)
#define SIMDLIB_DETAIL_FLAGS_EMIT_IF_1(...) __VA_ARGS__
#define SIMDLIB_DETAIL_FLAGS_EMIT_IF(condition) SIMDLIB_DETAIL_FLAGS_CAT(SIMDLIB_DETAIL_FLAGS_EMIT_IF_, condition)

#define SIMDLIB_DETAIL_FLAGS_EMIT_1(a)                                                                                                                         \
	SIMDLIB_DETAIL_FLAGS_EMIT(                                                                                                                                 \
		SIMDLIB_DETAIL_FLAGS_ANY_1(SIMDLIB_DETAIL_FLAGS_IS_VECTOR_BOUNDARY, a), SIMDLIB_DETAIL_FLAGS_ANY_1(SIMDLIB_DETAIL_FLAGS_IS_RegisterOnly, a),           \
		SIMDLIB_DETAIL_FLAGS_ANY_1(SIMDLIB_DETAIL_FLAGS_IS_ForceInline, a), SIMDLIB_DETAIL_FLAGS_ANY_1(SIMDLIB_DETAIL_FLAGS_IS_Flatten, a))
#define SIMDLIB_DETAIL_FLAGS_EMIT_2(a, b)                                                                                                                      \
	SIMDLIB_DETAIL_FLAGS_EMIT(                                                                                                                                 \
		SIMDLIB_DETAIL_FLAGS_ANY_2(SIMDLIB_DETAIL_FLAGS_IS_VECTOR_BOUNDARY, a, b), SIMDLIB_DETAIL_FLAGS_ANY_2(SIMDLIB_DETAIL_FLAGS_IS_RegisterOnly, a, b),     \
		SIMDLIB_DETAIL_FLAGS_ANY_2(SIMDLIB_DETAIL_FLAGS_IS_ForceInline, a, b), SIMDLIB_DETAIL_FLAGS_ANY_2(SIMDLIB_DETAIL_FLAGS_IS_Flatten, a, b))
#define SIMDLIB_DETAIL_FLAGS_EMIT_3(a, b, c)                                                                                                                   \
	SIMDLIB_DETAIL_FLAGS_EMIT(SIMDLIB_DETAIL_FLAGS_ANY_3(SIMDLIB_DETAIL_FLAGS_IS_VECTOR_BOUNDARY, a, b, c),                                                    \
							  SIMDLIB_DETAIL_FLAGS_ANY_3(SIMDLIB_DETAIL_FLAGS_IS_RegisterOnly, a, b, c),                                                       \
							  SIMDLIB_DETAIL_FLAGS_ANY_3(SIMDLIB_DETAIL_FLAGS_IS_ForceInline, a, b, c),                                                        \
							  SIMDLIB_DETAIL_FLAGS_ANY_3(SIMDLIB_DETAIL_FLAGS_IS_Flatten, a, b, c))
#define SIMDLIB_DETAIL_FLAGS_EMIT_4(a, b, c, d)                                                                                                                \
	SIMDLIB_DETAIL_FLAGS_EMIT(SIMDLIB_DETAIL_FLAGS_ANY_4(SIMDLIB_DETAIL_FLAGS_IS_VECTOR_BOUNDARY, a, b, c, d),                                                 \
							  SIMDLIB_DETAIL_FLAGS_ANY_4(SIMDLIB_DETAIL_FLAGS_IS_RegisterOnly, a, b, c, d),                                                    \
							  SIMDLIB_DETAIL_FLAGS_ANY_4(SIMDLIB_DETAIL_FLAGS_IS_ForceInline, a, b, c, d),                                                     \
							  SIMDLIB_DETAIL_FLAGS_ANY_4(SIMDLIB_DETAIL_FLAGS_IS_Flatten, a, b, c, d))
#define SIMDLIB_DETAIL_FLAGS_EMIT_5(a, b, c, d, e)                                                                                                             \
	SIMDLIB_DETAIL_FLAGS_EMIT(SIMDLIB_DETAIL_FLAGS_ANY_5(SIMDLIB_DETAIL_FLAGS_IS_VECTOR_BOUNDARY, a, b, c, d, e),                                              \
							  SIMDLIB_DETAIL_FLAGS_ANY_5(SIMDLIB_DETAIL_FLAGS_IS_RegisterOnly, a, b, c, d, e),                                                 \
							  SIMDLIB_DETAIL_FLAGS_ANY_5(SIMDLIB_DETAIL_FLAGS_IS_ForceInline, a, b, c, d, e),                                                  \
							  SIMDLIB_DETAIL_FLAGS_ANY_5(SIMDLIB_DETAIL_FLAGS_IS_Flatten, a, b, c, d, e))

#ifndef SIMDLIB_DETAIL_FLAGS_VECTORCALL
#define SIMDLIB_DETAIL_FLAGS_VECTORCALL SIMDLIB_PP_VECTORCALL
#endif
#ifndef SIMDLIB_DETAIL_FLAGS_REGISTER_ONLY
#define SIMDLIB_DETAIL_FLAGS_REGISTER_ONLY SIMDLIB_PP_REGISTER_ONLY
#endif
#ifndef SIMDLIB_DETAIL_FLAGS_FORCE_INLINE
#define SIMDLIB_DETAIL_FLAGS_FORCE_INLINE SIMDLIB_PP_FORCE_INLINE
#endif
#ifndef SIMDLIB_DETAIL_FLAGS_FLATTEN
#define SIMDLIB_DETAIL_FLAGS_FLATTEN SIMDLIB_PP_FLATTEN
#endif

#define SIMDLIB_DETAIL_FLAGS_EMIT(vector_boundary, register_only, force_inline, flatten)                                                                       \
	SIMDLIB_DETAIL_FLAGS_EMIT_IF(vector_boundary)(SIMDLIB_DETAIL_FLAGS_VECTORCALL)                                                                             \
		SIMDLIB_DETAIL_FLAGS_EMIT_IF(register_only)(SIMDLIB_DETAIL_FLAGS_REGISTER_ONLY)                                                                        \
			SIMDLIB_DETAIL_FLAGS_EMIT_IF(force_inline)(SIMDLIB_DETAIL_FLAGS_FORCE_INLINE) SIMDLIB_DETAIL_FLAGS_EMIT_IF(flatten)(SIMDLIB_DETAIL_FLAGS_FLATTEN)

#define SIMDLIB_DETAIL_FLAGS_ERROR_EMPTY(...) static_assert(false, "SIMDLIB_FLAGS_ERROR_EMPTY");
#define SIMDLIB_DETAIL_FLAGS_ERROR_UNKNOWN(...) static_assert(false, "SIMDLIB_FLAGS_ERROR_UNKNOWN");
#define SIMDLIB_DETAIL_FLAGS_ERROR_DUPLICATE(...) static_assert(false, "SIMDLIB_FLAGS_ERROR_DUPLICATE");
#define SIMDLIB_DETAIL_FLAGS_ERROR_TOO_MANY(...) static_assert(false, "SIMDLIB_FLAGS_ERROR_TOO_MANY");

#define SIMDLIB_DETAIL_FLAGS_CHECK_DUPLICATE_2(a, b)                                                                                                           \
	SIMDLIB_DETAIL_FLAGS_IF(SIMDLIB_DETAIL_FLAGS_HAS_DUPLICATE_2(a, b))(SIMDLIB_DETAIL_FLAGS_ERROR_DUPLICATE, SIMDLIB_DETAIL_FLAGS_EMIT_2)(a, b)
#define SIMDLIB_DETAIL_FLAGS_CHECK_DUPLICATE_3(a, b, c)                                                                                                        \
	SIMDLIB_DETAIL_FLAGS_IF(SIMDLIB_DETAIL_FLAGS_HAS_DUPLICATE_3(a, b, c))(SIMDLIB_DETAIL_FLAGS_ERROR_DUPLICATE, SIMDLIB_DETAIL_FLAGS_EMIT_3)(a, b, c)
#define SIMDLIB_DETAIL_FLAGS_CHECK_DUPLICATE_4(a, b, c, d)                                                                                                     \
	SIMDLIB_DETAIL_FLAGS_IF(SIMDLIB_DETAIL_FLAGS_HAS_DUPLICATE_4(a, b, c, d))(SIMDLIB_DETAIL_FLAGS_ERROR_DUPLICATE, SIMDLIB_DETAIL_FLAGS_EMIT_4)(a, b, c, d)
#define SIMDLIB_DETAIL_FLAGS_CHECK_DUPLICATE_5(a, b, c, d, e)                                                                                                  \
	SIMDLIB_DETAIL_FLAGS_IF(SIMDLIB_DETAIL_FLAGS_HAS_DUPLICATE_5(a, b, c, d, e))(SIMDLIB_DETAIL_FLAGS_ERROR_DUPLICATE, SIMDLIB_DETAIL_FLAGS_EMIT_5)(a, b, c,   \
																																					d, e)

#define SIMDLIB_DETAIL_FLAGS_NONEMPTY_1(a)                                                                                                                     \
	SIMDLIB_DETAIL_FLAGS_IF(SIMDLIB_DETAIL_FLAGS_ALL_VALID_1(a))(SIMDLIB_DETAIL_FLAGS_EMIT_1, SIMDLIB_DETAIL_FLAGS_ERROR_UNKNOWN)(a)
#define SIMDLIB_DETAIL_FLAGS_1(a)                                                                                                                              \
	SIMDLIB_DETAIL_FLAGS_IF(SIMDLIB_DETAIL_FLAGS_IS_EMPTY(a))(SIMDLIB_DETAIL_FLAGS_ERROR_EMPTY, SIMDLIB_DETAIL_FLAGS_NONEMPTY_1)(a)
#define SIMDLIB_DETAIL_FLAGS_2(a, b)                                                                                                                           \
	SIMDLIB_DETAIL_FLAGS_IF(SIMDLIB_DETAIL_FLAGS_ALL_VALID_2(a, b))(SIMDLIB_DETAIL_FLAGS_CHECK_DUPLICATE_2, SIMDLIB_DETAIL_FLAGS_ERROR_UNKNOWN)(a, b)
#define SIMDLIB_DETAIL_FLAGS_3(a, b, c)                                                                                                                        \
	SIMDLIB_DETAIL_FLAGS_IF(SIMDLIB_DETAIL_FLAGS_ALL_VALID_3(a, b, c))(SIMDLIB_DETAIL_FLAGS_CHECK_DUPLICATE_3, SIMDLIB_DETAIL_FLAGS_ERROR_UNKNOWN)(a, b, c)
#define SIMDLIB_DETAIL_FLAGS_4(a, b, c, d)                                                                                                                     \
	SIMDLIB_DETAIL_FLAGS_IF(SIMDLIB_DETAIL_FLAGS_ALL_VALID_4(a, b, c, d))(SIMDLIB_DETAIL_FLAGS_CHECK_DUPLICATE_4, SIMDLIB_DETAIL_FLAGS_ERROR_UNKNOWN)(a, b, c, \
																																					  d)
#define SIMDLIB_DETAIL_FLAGS_5(a, b, c, d, e)                                                                                                                  \
	SIMDLIB_DETAIL_FLAGS_IF(SIMDLIB_DETAIL_FLAGS_ALL_VALID_5(a, b, c, d, e))(SIMDLIB_DETAIL_FLAGS_CHECK_DUPLICATE_5,                                           \
																			 SIMDLIB_DETAIL_FLAGS_ERROR_UNKNOWN)(a, b, c, d, e)
#define SIMDLIB_DETAIL_FLAGS_6(...) SIMDLIB_DETAIL_FLAGS_ERROR_TOO_MANY(__VA_ARGS__)

// Arity 1 deliberately includes an empty invocation. SIMDLIB_DETAIL_FLAGS_1
// distinguishes that case without requiring __VA_OPT__ or a compiler extension.
#define SIMDLIB_DETAIL_FLAGS_ARITY_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, count, ...) count
#define SIMDLIB_DETAIL_FLAGS_ARITY_EXPAND(arguments) SIMDLIB_DETAIL_FLAGS_ARITY_IMPL arguments
#define SIMDLIB_DETAIL_FLAGS_ARITY(...) SIMDLIB_DETAIL_FLAGS_ARITY_EXPAND((__VA_ARGS__, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 4, 3, 2, 1))
#define SIMDLIB_DETAIL_FLAGS_DISPATCH(count) SIMDLIB_DETAIL_FLAGS_CAT(SIMDLIB_DETAIL_FLAGS_, count)
#define SIMDLIB_DETAIL_FLAGS_EXPAND(...) __VA_ARGS__

#define SIMD_FLAGS(...) SIMDLIB_DETAIL_FLAGS_EXPAND(SIMDLIB_DETAIL_FLAGS_DISPATCH(SIMDLIB_DETAIL_FLAGS_ARITY(__VA_ARGS__))(__VA_ARGS__))
