if(NOT DEFINED BINARY_DIRECTORY)
    message(FATAL_ERROR "BINARY_DIRECTORY is required")
endif()

# CTest 4.4 clears profiles for tests selected in its current invocation. Clear
# the entire build tree as well so a partial run cannot inherit unrelated data.
file(GLOB_RECURSE coverage_profiles LIST_DIRECTORIES FALSE
    "${BINARY_DIRECTORY}/*.profraw"
    "${BINARY_DIRECTORY}/*.profdata")
if(coverage_profiles)
    file(REMOVE ${coverage_profiles})
endif()

file(REMOVE
    "${BINARY_DIRECTORY}/coverage.info"
    "${BINARY_DIRECTORY}/coverage-profiles.rsp")
