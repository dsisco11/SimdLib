cmake_minimum_required(VERSION 3.31)

if(NOT DEFINED PORTABLE_EXECUTABLE OR NOT DEFINED ENABLED_EXECUTABLE)
    message(FATAL_ERROR "Both BMI test executable paths are required")
endif()

execute_process(
    COMMAND "${PORTABLE_EXECUTABLE}" "[simdlib][bmi][digest]"
    RESULT_VARIABLE portable_result
    OUTPUT_VARIABLE portable_output
    ERROR_VARIABLE portable_error)
execute_process(
    COMMAND "${ENABLED_EXECUTABLE}" "[simdlib][bmi][digest]"
    RESULT_VARIABLE enabled_result
    OUTPUT_VARIABLE enabled_output
    ERROR_VARIABLE enabled_error)

if(NOT portable_result EQUAL 0)
    message(FATAL_ERROR "BMI-disabled digest test failed: ${portable_error}\n${portable_output}")
endif()
if(NOT enabled_result EQUAL 0)
    message(FATAL_ERROR "BMI-enabled digest test failed: ${enabled_error}\n${enabled_output}")
endif()

string(REGEX MATCH "SIMDLIB_BMI_RESULT_DIGEST=[0-9a-fA-F]+" portable_digest "${portable_output}")
string(REGEX MATCH "SIMDLIB_BMI_RESULT_DIGEST=[0-9a-fA-F]+" enabled_digest "${enabled_output}")
if(portable_digest STREQUAL "" OR enabled_digest STREQUAL "")
    message(FATAL_ERROR "A BMI test executable did not emit its deterministic result digest")
endif()
if(NOT portable_digest STREQUAL enabled_digest)
    message(FATAL_ERROR "BMI result sets differ: ${portable_digest} versus ${enabled_digest}")
endif()

message(STATUS "BMI enabled/disabled result sets are identical: ${portable_digest}")
