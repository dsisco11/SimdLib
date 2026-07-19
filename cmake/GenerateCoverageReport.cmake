foreach(required_variable IN ITEMS
    BINARY_DIRECTORY
    SOURCE_DIRECTORY
    OBJECTS_FILE
    LLVM_PROFDATA
    LLVM_COV)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} is required")
    endif()
endforeach()

file(GLOB_RECURSE coverage_profiles LIST_DIRECTORIES FALSE
    "${BINARY_DIRECTORY}/*.profraw"
    "${BINARY_DIRECTORY}/*.profdata")
list(FILTER coverage_profiles EXCLUDE REGEX "[/\\\\]coverage\\.profdata$")
# CTest creates a tiny probe profile while validating llvm-profdata itself;
# it is tool-detection data rather than coverage from a registered test.
list(FILTER coverage_profiles EXCLUDE REGEX "[/\\\\]probe-[0-9]+\\.profraw$")
if(NOT coverage_profiles)
    message(FATAL_ERROR
        "No LLVM profiles were found. Run CTest with LLVM coverage enabled first.")
endif()

# CTest 4.4 normally merges each raw profile into a per-test .profdata file.
# Any raw files it could not merge are retained, so both formats are inputs.
# A response file avoids Windows command-line limits for the discovered cases.
set(profile_response_file "${BINARY_DIRECTORY}/coverage-profiles.rsp")
file(WRITE "${profile_response_file}" "")
foreach(coverage_profile IN LISTS coverage_profiles)
    file(APPEND "${profile_response_file}" "\"${coverage_profile}\"\n")
endforeach()

set(merged_profile "${BINARY_DIRECTORY}/coverage.profdata")
execute_process(
    COMMAND "${LLVM_PROFDATA}" merge -sparse "@${profile_response_file}"
        -o "${merged_profile}"
    COMMAND_ERROR_IS_FATAL ANY)

if(NOT EXISTS "${OBJECTS_FILE}")
    message(FATAL_ERROR "Coverage object list does not exist: ${OBJECTS_FILE}")
endif()
file(STRINGS "${OBJECTS_FILE}" coverage_objects)
list(FILTER coverage_objects EXCLUDE REGEX "^$")
if(NOT coverage_objects)
    message(FATAL_ERROR "Coverage object list is empty: ${OBJECTS_FILE}")
endif()

list(POP_FRONT coverage_objects primary_object)
set(object_arguments "")
foreach(coverage_object IN LISTS coverage_objects)
    if(EXISTS "${coverage_object}")
        list(APPEND object_arguments -object "${coverage_object}")
    endif()
endforeach()

# Restrict the report to the library's public headers. Catch2, test sources,
# and other fetched dependencies are execution machinery, not SimdLib surface.
file(GLOB_RECURSE coverage_sources LIST_DIRECTORIES FALSE
    "${SOURCE_DIRECTORY}/include/SimdLib/*.h")
if(NOT coverage_sources)
    message(FATAL_ERROR "No SimdLib public headers were found for coverage")
endif()

execute_process(
    COMMAND "${LLVM_COV}" export
        "${primary_object}"
        ${object_arguments}
        "-instr-profile=${merged_profile}"
        -format=lcov
        -sources ${coverage_sources}
    OUTPUT_FILE "${BINARY_DIRECTORY}/coverage.info"
    COMMAND_ERROR_IS_FATAL ANY)

list(LENGTH coverage_profiles profile_count)
message(STATUS
    "Generated ${BINARY_DIRECTORY}/coverage.info from ${profile_count} profiles")
