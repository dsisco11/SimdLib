cmake_minimum_required(VERSION 4.4)

foreach(required_variable IN ITEMS
    CASE SOURCE_DIRECTORY BINARY_DIRECTORY GENERATOR MAKE_PROGRAM)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "Missing required variable ${required_variable}")
    endif()
endforeach()

if(CASE STREQUAL "UNOWNED")
    set(expected_diagnostic "has no validation-category owner")
elseif(CASE STREQUAL "MULTIPLE")
    set(expected_diagnostic "has multiple validation owners")
elseif(CASE STREQUAL "EXCLUDED")
    set(expected_diagnostic "excluded by validation")
else()
    message(FATAL_ERROR "Unsupported artifact-aggregate failure case ${CASE}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        --fresh
        -G "${GENERATOR}"
        -S "${SOURCE_DIRECTORY}/tests/cmake/artifact_aggregates"
        -B "${BINARY_DIRECTORY}"
        "-DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}"
        "-DSIMDLIB_SOURCE_DIRECTORY=${SOURCE_DIRECTORY}"
        "-DSIMDLIB_ARTIFACT_FAILURE_CASE=${CASE}"
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_stdout
    ERROR_VARIABLE configure_stderr)
set(configure_output "${configure_stdout}${configure_stderr}")
if(configure_result EQUAL 0)
    message(FATAL_ERROR
        "Artifact-aggregate case ${CASE} unexpectedly configured successfully")
endif()
if(NOT configure_output MATCHES "${expected_diagnostic}")
    message(FATAL_ERROR
        "Artifact-aggregate case ${CASE} did not emit ${expected_diagnostic}:\n"
        "${configure_output}")
endif()

message(STATUS "Artifact-aggregate case ${CASE} failed as required")
