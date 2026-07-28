cmake_minimum_required(VERSION 3.25)

foreach(required_variable IN ITEMS
    SIMDLIB_METHOD_FLAGS_COMPILER
    SIMDLIB_METHOD_FLAGS_COMPILER_ID
    SIMDLIB_METHOD_FLAGS_MSVC_STYLE
    SIMDLIB_METHOD_FLAGS_SOURCE_DIR
    SIMDLIB_METHOD_FLAGS_BINARY_DIR)
    if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} is required")
    endif()
endforeach()

set(prototype_header
    "${SIMDLIB_METHOD_FLAGS_SOURCE_DIR}/tests/method_flags/MethodFlagsPrototype.h")
if(NOT EXISTS "${prototype_header}")
    message(FATAL_ERROR "Method-flags prototype header is missing: ${prototype_header}")
endif()

file(READ "${prototype_header}" prototype_source)
if(prototype_source MATCHES "#[ \t]*include")
    message(FATAL_ERROR "Method-flags preprocessing prototype must remain dependency-free")
endif()

string(REGEX MATCHALL "#define[ \t]+[A-Za-z_][A-Za-z0-9_]*" prototype_definitions
    "${prototype_source}")
foreach(definition IN LISTS prototype_definitions)
    string(REGEX REPLACE "^#define[ \t]+" "" macro_name "${definition}")
    if(NOT macro_name STREQUAL "SIMD_FLAGS"
        AND NOT macro_name MATCHES "^SIMDLIB_DETAIL_")
        message(FATAL_ERROR
            "Method-flags prototype leaks a non-detail helper macro: ${macro_name}")
    endif()
endforeach()

set(probe_directory "${SIMDLIB_METHOD_FLAGS_BINARY_DIR}/method-flags-preprocessor")
file(MAKE_DIRECTORY "${probe_directory}")
set(probe_source "${probe_directory}/MethodFlagsPreprocessorProbe.cpp")
set(expected_output "${probe_directory}/MethodFlagsPreprocessorExpected.txt")
set(actual_output "${probe_directory}/MethodFlagsPreprocessorActual.txt")

set_property(GLOBAL PROPERTY SIMDLIB_METHOD_FLAGS_CASE_COUNT 0)
set_property(GLOBAL PROPERTY SIMDLIB_METHOD_FLAGS_PROBE_LINES "")
set_property(GLOBAL PROPERTY SIMDLIB_METHOD_FLAGS_EXPECTED_LINES "")

# Adds one canonical boundary-and-modifier expansion to the generated fixture.
function(simdlib_add_method_flags_case boundary)
    set(case_modifiers ${ARGN})
    set(case_flags ${boundary} ${case_modifiers})
    get_property(case_count GLOBAL PROPERTY SIMDLIB_METHOD_FLAGS_CASE_COUNT)
    math(EXPR case_count "${case_count} + 1")
    set_property(GLOBAL PROPERTY SIMDLIB_METHOD_FLAGS_CASE_COUNT "${case_count}")

    list(JOIN case_flags ", " invocation)
    set(case_name "SIMDLIB_PP_CASE_${case_count}")
    set(probe_line "${case_name} SIMD_FLAGS(${invocation})")
    set(expected_line "${case_name}")

    list(FIND case_modifiers Flatten flatten_index)
    if(NOT flatten_index EQUAL -1)
        string(APPEND expected_line " SIMDLIB_PP_FLATTEN")
    endif()
    list(FIND case_modifiers ForceInline force_inline_index)
    if(NOT force_inline_index EQUAL -1)
        string(APPEND expected_line " SIMDLIB_PP_FORCE_INLINE")
    endif()
    list(FIND case_modifiers RegisterOnly register_only_index)
    if(NOT register_only_index EQUAL -1)
        string(APPEND expected_line " SIMDLIB_PP_REGISTER_ONLY")
    endif()
    if(NOT boundary STREQUAL "Neither")
        string(APPEND expected_line " SIMDLIB_PP_VECTORCALL")
    endif()

    set_property(GLOBAL APPEND PROPERTY SIMDLIB_METHOD_FLAGS_PROBE_LINES "${probe_line}")
    set_property(GLOBAL APPEND PROPERTY SIMDLIB_METHOD_FLAGS_EXPECTED_LINES "${expected_line}")
endfunction()

set(boundary_modes Neither In Out InOut)
foreach(boundary IN LISTS boundary_modes)
    simdlib_add_method_flags_case(${boundary})
    simdlib_add_method_flags_case(${boundary} RegisterOnly)
    simdlib_add_method_flags_case(${boundary} ForceInline)
    simdlib_add_method_flags_case(${boundary} Flatten)
    simdlib_add_method_flags_case(${boundary} RegisterOnly ForceInline)
    simdlib_add_method_flags_case(${boundary} RegisterOnly Flatten)
    simdlib_add_method_flags_case(${boundary} ForceInline Flatten)
    simdlib_add_method_flags_case(${boundary} RegisterOnly ForceInline Flatten)
endforeach()

# A function-like macro is not expanded when its name is passed as a bare flag.
# This case proves that only object-like collisions impose a caller restriction.
set_property(GLOBAL APPEND PROPERTY SIMDLIB_METHOD_FLAGS_PROBE_LINES
    "#define InOut(...) downstream_function_macro"
    "SIMDLIB_PP_CASE_FUNCTION_MACRO SIMD_FLAGS(InOut, Flatten)"
    "#undef InOut")
set_property(GLOBAL APPEND PROPERTY SIMDLIB_METHOD_FLAGS_EXPECTED_LINES
    "SIMDLIB_PP_CASE_FUNCTION_MACRO SIMDLIB_PP_FLATTEN SIMDLIB_PP_VECTORCALL")

get_property(case_count GLOBAL PROPERTY SIMDLIB_METHOD_FLAGS_CASE_COUNT)
if(NOT case_count EQUAL 32)
    message(FATAL_ERROR
        "Expected 32 canonical boundary-and-modifier cases, generated ${case_count}")
endif()

get_property(probe_lines GLOBAL PROPERTY SIMDLIB_METHOD_FLAGS_PROBE_LINES)
get_property(expected_lines GLOBAL PROPERTY SIMDLIB_METHOD_FLAGS_EXPECTED_LINES)
list(JOIN probe_lines "\n" probe_body)
list(JOIN expected_lines "\n" expected_body)

file(WRITE "${probe_source}"
    "#define SIMDLIB_DETAIL_FLAGS_VECTORCALL SIMDLIB_PP_VECTORCALL\n"
    "#define SIMDLIB_DETAIL_FLAGS_REGISTER_ONLY SIMDLIB_PP_REGISTER_ONLY\n"
    "#define SIMDLIB_DETAIL_FLAGS_FORCE_INLINE SIMDLIB_PP_FORCE_INLINE\n"
    "#define SIMDLIB_DETAIL_FLAGS_FLATTEN SIMDLIB_PP_FLATTEN\n"
    "#include \"MethodFlagsPrototype.h\"\n"
    "#if defined(Neither) || defined(In) || defined(Out) || defined(InOut) || defined(RegisterOnly) || defined(ForceInline) || defined(Flatten)\n"
    "#error SIMDLIB_FLAGS_SHORT_MACRO_LEAK\n"
    "#endif\n"
    "${probe_body}\n")
file(WRITE "${expected_output}" "${expected_body}\n")

if(SIMDLIB_METHOD_FLAGS_MSVC_STYLE)
    set(preprocess_arguments
        /nologo
        /std:c++20
        ${SIMDLIB_METHOD_FLAGS_COMPILER_OPTIONS}
        /EP
        /TP
        "/I${SIMDLIB_METHOD_FLAGS_SOURCE_DIR}/tests/method_flags"
        "${probe_source}")
else()
    set(preprocess_arguments
        -std=c++20
        ${SIMDLIB_METHOD_FLAGS_COMPILER_OPTIONS}
        -E
        -P
        -x c++
        "-I${SIMDLIB_METHOD_FLAGS_SOURCE_DIR}/tests/method_flags"
        "${probe_source}")
endif()

execute_process(
    COMMAND "${SIMDLIB_METHOD_FLAGS_COMPILER}" ${preprocess_arguments}
    RESULT_VARIABLE preprocess_result
    OUTPUT_VARIABLE preprocess_stdout
    ERROR_VARIABLE preprocess_stderr)
file(WRITE "${actual_output}" "${preprocess_stdout}")
if(NOT preprocess_result EQUAL 0)
    message(FATAL_ERROR
        "${SIMDLIB_METHOD_FLAGS_COMPILER_ID} preprocessing failed:\n${preprocess_stderr}")
endif()

string(REPLACE "\r\n" "\n" preprocess_stdout "${preprocess_stdout}")
string(REPLACE "\r" "\n" preprocess_stdout "${preprocess_stdout}")
string(REGEX MATCHALL "SIMDLIB_PP_CASE_[A-Za-z0-9_]+[^\n]*" actual_lines
    "${preprocess_stdout}")
list(LENGTH expected_lines expected_count)
list(LENGTH actual_lines actual_count)
if(NOT actual_count EQUAL expected_count)
    message(FATAL_ERROR
        "${SIMDLIB_METHOD_FLAGS_COMPILER_ID} produced ${actual_count} marker lines; "
        "expected ${expected_count}. See ${actual_output}")
endif()

math(EXPR final_index "${expected_count} - 1")
foreach(index RANGE 0 ${final_index})
    list(GET expected_lines ${index} expected_line)
    list(GET actual_lines ${index} actual_line)
    string(STRIP "${actual_line}" actual_line)
    string(REGEX REPLACE "[ \t]+" " " actual_line "${actual_line}")
    if(NOT actual_line STREQUAL expected_line)
        message(FATAL_ERROR
            "${SIMDLIB_METHOD_FLAGS_COMPILER_ID} expansion mismatch at case ${index}:\n"
            "  expected: ${expected_line}\n"
            "  actual:   ${actual_line}\n"
            "See ${actual_output}")
    endif()
endforeach()

set(negative_sources
    InvalidEmpty.cpp
    InvalidUnknown.cpp
    InvalidDuplicate.cpp
    InvalidTooMany.cpp
    InvalidObjectMacroCollision.cpp
    InvalidMissingBoundary.cpp
    InvalidModifierOrder.cpp)
set(negative_expansions
    SIMDLIB_FLAGS_ERROR_EMPTY
    SIMDLIB_DETAIL_FLAGS_MODIFIERS_1_Unknown
    SIMDLIB_DETAIL_FLAGS_MODIFIERS_2_RegisterOnly_RegisterOnly
    SIMDLIB_FLAGS_ERROR_TOO_MANY
    SIMDLIB_DETAIL_FLAGS_BOUNDARY_downstream_object_macro
    SIMDLIB_DETAIL_FLAGS_BOUNDARY_RegisterOnly
    SIMDLIB_DETAIL_FLAGS_MODIFIERS_2_Flatten_ForceInline)

list(LENGTH negative_sources negative_count)
math(EXPR negative_final_index "${negative_count} - 1")
foreach(index RANGE 0 ${negative_final_index})
    list(GET negative_sources ${index} negative_source_name)
    list(GET negative_expansions ${index} expected_expansion)
    set(negative_source
        "${SIMDLIB_METHOD_FLAGS_SOURCE_DIR}/tests/method_flags/${negative_source_name}")
    set(negative_log "${probe_directory}/${negative_source_name}.log")
    set(negative_object "${probe_directory}/${negative_source_name}.obj")

    if(SIMDLIB_METHOD_FLAGS_MSVC_STYLE)
        set(negative_preprocess_arguments
            /nologo
            /std:c++20
            ${SIMDLIB_METHOD_FLAGS_COMPILER_OPTIONS}
            /EP
            /TP
            "/I${SIMDLIB_METHOD_FLAGS_SOURCE_DIR}/tests/method_flags"
            "${negative_source}")
        set(negative_arguments
            /nologo
            /std:c++20
            ${SIMDLIB_METHOD_FLAGS_COMPILER_OPTIONS}
            /TP
            /c
            "/I${SIMDLIB_METHOD_FLAGS_SOURCE_DIR}/tests/method_flags"
            "/Fo${negative_object}"
            "${negative_source}")
    else()
        set(negative_preprocess_arguments
            -std=c++20
            ${SIMDLIB_METHOD_FLAGS_COMPILER_OPTIONS}
            -E
            -P
            -x c++
            "-I${SIMDLIB_METHOD_FLAGS_SOURCE_DIR}/tests/method_flags"
            "${negative_source}")
        set(negative_arguments
            -std=c++20
            ${SIMDLIB_METHOD_FLAGS_COMPILER_OPTIONS}
            -fsyntax-only
            -x c++
            "-I${SIMDLIB_METHOD_FLAGS_SOURCE_DIR}/tests/method_flags"
            "${negative_source}")
    endif()

    execute_process(
        COMMAND "${SIMDLIB_METHOD_FLAGS_COMPILER}" ${negative_preprocess_arguments}
        RESULT_VARIABLE negative_preprocess_result
        OUTPUT_VARIABLE negative_preprocess_stdout
        ERROR_VARIABLE negative_preprocess_stderr)
    if(NOT negative_preprocess_result EQUAL 0)
        message(FATAL_ERROR
            "${SIMDLIB_METHOD_FLAGS_COMPILER_ID} could not preprocess "
            "${negative_source_name}:\n${negative_preprocess_stderr}")
    endif()
    if(NOT negative_preprocess_stdout MATCHES "${expected_expansion}")
        message(FATAL_ERROR
            "${SIMDLIB_METHOD_FLAGS_COMPILER_ID} did not preserve "
            "${expected_expansion} in ${negative_source_name}")
    endif()

    execute_process(
        COMMAND "${SIMDLIB_METHOD_FLAGS_COMPILER}" ${negative_arguments}
        RESULT_VARIABLE negative_result
        OUTPUT_VARIABLE negative_stdout
        ERROR_VARIABLE negative_stderr)
    set(negative_output "${negative_stdout}\n${negative_stderr}")
    file(WRITE "${negative_log}" "${negative_output}")
    if(negative_result EQUAL 0)
        message(FATAL_ERROR
            "${SIMDLIB_METHOD_FLAGS_COMPILER_ID} unexpectedly accepted ${negative_source_name}")
    endif()
endforeach()

message(STATUS
    "${SIMDLIB_METHOD_FLAGS_COMPILER_ID}: verified ${expected_count} canonical "
    "expansions and ${negative_count} focused failures")
