cmake_minimum_required(VERSION 4.4)

foreach(required_variable IN ITEMS PROPERTY_FILE SOURCE_FILE DEFAULT_CHECKS_PROBE)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "Missing required variable ${required_variable}")
    endif()
endforeach()
if(NOT EXISTS "${PROPERTY_FILE}")
    message(FATAL_ERROR
        "Compiler-contract property inventory does not exist: ${PROPERTY_FILE}")
endif()
if(NOT EXISTS "${SOURCE_FILE}")
    message(FATAL_ERROR
        "Compiler-contract source inventory does not exist: ${SOURCE_FILE}")
endif()

file(STRINGS "${PROPERTY_FILE}" property_rows)
list(POP_FRONT property_rows property_header)
if(NOT property_header STREQUAL
        "target\tcompile_definitions\tcompile_options\tlink_options\tcxx_standard")
    message(FATAL_ERROR "Compiler-contract property inventory has an invalid header")
endif()

set(forbidden_property_pattern
    "NDEBUG|SIMDLIB_ENABLE_CHECKS|fsanitize|sanitize=|fprofile|coverage|/RTC|\\$<CONFIG")
set(default_checks_target_count 0)
foreach(property_row IN LISTS property_rows)
    if(NOT property_row MATCHES "^([^\t]+)\t(.*)$")
        message(FATAL_ERROR "Malformed compiler-contract property row: ${property_row}")
    endif()
    set(target "${CMAKE_MATCH_1}")
    if(property_row MATCHES "${forbidden_property_pattern}")
        message(FATAL_ERROR
            "Compiler-contract target ${target} has configuration-dependent properties: ${property_row}")
    endif()

    if(target MATCHES "^ConfigDefaultChecks(Release|Debug)Probe$")
        math(EXPR default_checks_target_count "${default_checks_target_count} + 1")
        if(DEFAULT_CHECKS_PROBE STREQUAL "RELEASE")
            if(NOT property_row MATCHES
                    "^ConfigDefaultChecksReleaseProbe\tSIMDLIB_EXPECT_DEFAULT_CHECKS=0\t")
                message(FATAL_ERROR
                    "Release default-checks probe has an invalid contract: ${property_row}")
            endif()
        elseif(DEFAULT_CHECKS_PROBE STREQUAL "DEBUG")
            if(NOT property_row MATCHES
                    "^ConfigDefaultChecksDebugProbe\tSIMDLIB_EXPECT_DEFAULT_CHECKS=1\t")
                message(FATAL_ERROR
                    "Debug default-checks probe has an invalid contract: ${property_row}")
            endif()
        else()
            message(FATAL_ERROR
                "A default-checks target exists while its profile is NONE")
        endif()
    endif()
endforeach()

if(DEFAULT_CHECKS_PROBE STREQUAL "NONE")
    if(NOT default_checks_target_count EQUAL 0)
        message(FATAL_ERROR "The NONE profile created a default-checks target")
    endif()
elseif(NOT default_checks_target_count EQUAL 1)
    message(FATAL_ERROR
        "Profile ${DEFAULT_CHECKS_PROBE} requires exactly one default-checks target")
endif()

file(STRINGS "${SOURCE_FILE}" source_rows)
list(POP_FRONT source_rows source_header)
if(NOT source_header STREQUAL "target\tsource")
    message(FATAL_ERROR "Compiler-contract source inventory has an invalid header")
endif()
foreach(source_row IN LISTS source_rows)
    if(NOT source_row MATCHES "^([^\t]+)\t(.+)$")
        message(FATAL_ERROR "Malformed compiler-contract source row: ${source_row}")
    endif()
    set(target "${CMAKE_MATCH_1}")
    set(source "${CMAKE_MATCH_2}")
    if(NOT EXISTS "${source}")
        message(FATAL_ERROR
            "Compiler-contract source does not exist: ${target}: ${source}")
    endif()
    if(target MATCHES "^ConfigDefaultChecks(Release|Debug)Probe$")
        continue()
    endif()
    file(READ "${source}" source_text)
    if(source_text MATCHES
            "NDEBUG|SIMDLIB_ENABLE_CHECKS|__SANITIZE|fsanitize|fprofile|LLVM_PROFILE_FILE")
        message(FATAL_ERROR
            "Compiler-contract source ${source} depends on configuration or instrumentation state")
    endif()
endforeach()

message(STATUS
    "Validated configuration-independent compiler contracts in ${PROPERTY_FILE}")
