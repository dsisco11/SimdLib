cmake_minimum_required(VERSION 3.31)

foreach(required_variable IN ITEMS PROPERTY_FILE DEFAULT_CHECKS_PROBE)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "Missing required variable ${required_variable}")
    endif()
endforeach()
if(NOT EXISTS "${PROPERTY_FILE}")
    message(FATAL_ERROR
        "Checks-contract property inventory does not exist: ${PROPERTY_FILE}")
endif()

file(STRINGS "${PROPERTY_FILE}" property_rows)
list(POP_FRONT property_rows property_header)
if(NOT property_header STREQUAL "target\tcompile_definitions\tsources")
    message(FATAL_ERROR "Checks-contract property inventory has an invalid header")
endif()

set(default_checks_target_count 0)
foreach(property_row IN LISTS property_rows)
    if(NOT property_row MATCHES "^([^\t]+)\t([^\t]*)\t(.+)$")
        message(FATAL_ERROR "Malformed checks-contract property row: ${property_row}")
    endif()
    set(target "${CMAKE_MATCH_1}")
    set(compile_definitions "${CMAKE_MATCH_2}")
    set(sources "${CMAKE_MATCH_3}")

    if(target STREQUAL "ConfigDefaultChecksDebugProbe")
        math(EXPR default_checks_target_count "${default_checks_target_count} + 1")
        if(NOT compile_definitions MATCHES
                "(^|,)SIMDLIB_EXPECT_DEFAULT_CHECKS=1(,|$)")
            message(FATAL_ERROR
                "Debug default-checks probe has an invalid contract: ${property_row}")
        endif()
        string(REPLACE "," ";" source_list "${sources}")
        list(GET source_list 0 source)
        file(READ "${source}" source_text)
        if(NOT source_text MATCHES
                "SIMDLIB_EXPECT_DEFAULT_CHECKS && defined\\(NDEBUG\\)")
            message(FATAL_ERROR
                "Debug default-checks probe does not reject NDEBUG: ${source}")
        endif()
    elseif(target MATCHES "^(VectorChecksTests|PreconditionTests)$")
        if(NOT compile_definitions MATCHES
                "(^|,)SIMDLIB_ENABLE_CHECKS=1(,|$)")
            message(FATAL_ERROR
                "Checks target ${target} does not explicitly enable checks")
        endif()
        if(target STREQUAL "PreconditionTests")
            string(REPLACE "," ";" source_list "${sources}")
            list(GET source_list 0 source)
            file(READ "${source}" source_text)
            string(FIND "${source_text}"
                "#define SIMDLIB_PRECONDITION" precondition_definition_position)
            string(FIND "${source_text}"
                "#include <SimdLib/Api.h>" api_include_position)
            if(precondition_definition_position LESS 0 OR
                    api_include_position LESS 0 OR
                    NOT precondition_definition_position LESS api_include_position)
                message(FATAL_ERROR
                    "PreconditionTests does not install its explicit failure hook before Api.h")
            endif()
        endif()
    elseif(target STREQUAL "RegisterPreconditionTests")
        string(REPLACE "," ";" source_list "${sources}")
        list(GET source_list 0 source)
        file(READ "${source}" source_text)
        string(FIND "${source_text}"
            "#define SIMDLIB_PRECONDITION" precondition_definition_position)
        string(FIND "${source_text}"
            "#include <SimdLib/Register.h>" register_include_position)
        if(precondition_definition_position LESS 0 OR
                register_include_position LESS 0 OR
                NOT precondition_definition_position LESS register_include_position)
            message(FATAL_ERROR
                "RegisterPreconditionTests does not install its explicit failure hook before Register.h")
        endif()
    endif()
endforeach()

if(DEFAULT_CHECKS_PROBE STREQUAL "DEBUG")
    if(NOT default_checks_target_count EQUAL 1)
        message(FATAL_ERROR
            "The checks-enabled Debug profile requires exactly one default-checks probe")
    endif()
elseif(NOT default_checks_target_count EQUAL 0)
    message(FATAL_ERROR
        "A Debug default-checks target exists while its profile is ${DEFAULT_CHECKS_PROBE}")
endif()

message(STATUS "Validated explicit checks and precondition configuration")
