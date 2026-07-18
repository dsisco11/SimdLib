if(NOT DEFINED PORTABLE_EXECUTABLE OR NOT DEFINED OPTIMIZED_EXECUTABLE)
    message(FATAL_ERROR "Both uint128 test executable paths are required")
endif()

execute_process(
    COMMAND "${PORTABLE_EXECUTABLE}" "[simdlib][uint128][digest]"
    RESULT_VARIABLE portable_result
    OUTPUT_VARIABLE portable_output
    ERROR_VARIABLE portable_error)
execute_process(
    COMMAND "${OPTIMIZED_EXECUTABLE}" "[simdlib][uint128][digest]"
    RESULT_VARIABLE optimized_result
    OUTPUT_VARIABLE optimized_output
    ERROR_VARIABLE optimized_error)

if(NOT portable_result EQUAL 0)
    message(FATAL_ERROR "Portable uint128 digest test failed: ${portable_error}\n${portable_output}")
endif()
if(NOT optimized_result EQUAL 0)
    message(FATAL_ERROR "Optimized uint128 digest test failed: ${optimized_error}\n${optimized_output}")
endif()

string(REGEX MATCH "SIMDLIB_UINT128_RESULT_DIGEST=[0-9a-fA-F]+" portable_digest "${portable_output}")
string(REGEX MATCH "SIMDLIB_UINT128_RESULT_DIGEST=[0-9a-fA-F]+" optimized_digest "${optimized_output}")
if(portable_digest STREQUAL "" OR optimized_digest STREQUAL "")
    message(FATAL_ERROR "A uint128 test executable did not emit its deterministic result digest")
endif()
if(NOT portable_digest STREQUAL optimized_digest)
    message(FATAL_ERROR "uint128 result sets differ: ${portable_digest} versus ${optimized_digest}")
endif()

message(STATUS "uint128 optimized/portable result sets are identical: ${portable_digest}")
