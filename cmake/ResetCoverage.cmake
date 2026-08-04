cmake_minimum_required(VERSION 3.31)

if(NOT DEFINED BINARY_DIRECTORY)
    message(FATAL_ERROR "BINARY_DIRECTORY is required")
endif()

# Clear the entire build tree so a partial run cannot inherit profiles from
# unrelated tests or a previous coverage invocation.
file(GLOB_RECURSE coverage_profiles LIST_DIRECTORIES FALSE
    "${BINARY_DIRECTORY}/*.profraw"
    "${BINARY_DIRECTORY}/*.profdata")
if(coverage_profiles)
    file(REMOVE ${coverage_profiles})
endif()

file(REMOVE
    "${BINARY_DIRECTORY}/coverage.info"
    "${BINARY_DIRECTORY}/coverage-profiles.rsp")
file(REMOVE_RECURSE "${BINARY_DIRECTORY}/coverage-work")
