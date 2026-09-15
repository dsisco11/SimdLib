include_guard(GLOBAL)

# @brief Reuses the project Catch2 dependency or an explicitly supplied local source.
function(simdlib_codegen_catch2_dependency)
    if(NOT TARGET Catch2::Catch2WithMain)
        find_package(Catch2 3 CONFIG QUIET)
        if(NOT TARGET Catch2::Catch2WithMain)
            set(SIMDLIB_CATCH2_SOURCE "" CACHE PATH "Existing Catch2 source for standalone instruction tests")
            if(NOT EXISTS "${SIMDLIB_CATCH2_SOURCE}/CMakeLists.txt")
                message(FATAL_ERROR "Supply SIMDLIB_CATCH2_SOURCE or an installed Catch2 3 package")
            endif()
            add_subdirectory("${SIMDLIB_CATCH2_SOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/catch2" EXCLUDE_FROM_ALL)
        endif()
    endif()
    if(SIMDLIB_CATCH2_SOURCE)
        list(APPEND CMAKE_MODULE_PATH "${SIMDLIB_CATCH2_SOURCE}/extras")
    elseif(catch2_SOURCE_DIR)
        list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
    endif()
    set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
endfunction()

# @brief Applies list-valued labels after Catch2 discovery, matching runtime-test integration.
function(simdlib_label_codegen_tests test_list labels)
    set(path "${CMAKE_CURRENT_BINARY_DIR}/${test_list}-labels.cmake")
    file(GENERATE OUTPUT "${path}" CONTENT
        "foreach(test IN LISTS ${test_list})\n set_tests_properties(\"\${test}\" PROPERTIES LABELS \"${labels};SIMDLIB_OWNER_OPTIMIZED_CODEGEN\" TIMEOUT 180)\nendforeach()\n")
    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${path}")
endfunction()

# @brief Discovers Catch2 tests independently of whether runtime suites are enabled.
function(simdlib_discover_codegen_tests target prefix)
    include(Catch)
    catch_discover_tests(${target} TEST_PREFIX "${prefix}"
        TEST_LIST ${target}_DISCOVERED_TESTS
        DISCOVERY_MODE POST_BUILD)
    simdlib_label_codegen_tests(${target}_DISCOVERED_TESTS CODEGEN_CONTRACT)
    include("${SIMDLIB_SOURCE_ROOT}/tests/codegen/pilot/ExpectedCases.cmake")
    set(expected "")
    foreach(case IN LISTS SIMDLIB_EXPECTED_CODEGEN_CASES)
        list(APPEND expected "${prefix}${case}")
    endforeach()
    # Run after Catch2's generated include, even for filtered CTest invocations.
    set(audit "${CMAKE_CURRENT_BINARY_DIR}/${target}-coverage.cmake")
    file(GENERATE OUTPUT "${audit}" CONTENT
        "set(expected [==[${expected}]==])\nset(actual \${${target}_DISCOVERED_TESTS})\nlist(SORT expected)\nlist(SORT actual)\nif(NOT \"\${actual}\" STREQUAL \"\${expected}\")\n message(FATAL_ERROR \"Expected Catch2 discovery coverage mismatch: \${actual}\")\nendif()\n")
    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${audit}")
    if(COMMAND simdlib_register_development_target)
        simdlib_register_development_target(${target} OPTIMIZED_CODEGEN)
    endif()
endfunction()
