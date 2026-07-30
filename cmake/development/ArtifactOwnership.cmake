include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "ArtifactOwnership.cmake is available only to top-level SimdLib builds")
endif()

set(SIMDLIB_VALIDATION_CATEGORIES
    COMPILER_CONTRACT
    CONSTEXPR_CONTRACT
    RUNTIME_VALIDATION
    CHECKS_VALIDATION
    SMOKE_VALIDATION
    OPTIMIZED_CODEGEN
    DEBUG_DIAGNOSTIC
    COVERAGE_SUPPORT
    BENCHMARK)

# @brief Assigns one development target to its sole validation category.
# @param target Existing project-owned development target.
# @param category One value from SIMDLIB_VALIDATION_CATEGORIES.
function(simdlib_register_development_target target category)
    if(NOT TARGET ${target})
        message(FATAL_ERROR
            "Cannot assign validation ownership before target ${target} exists")
    endif()
    if(NOT category IN_LIST SIMDLIB_VALIDATION_CATEGORIES)
        message(FATAL_ERROR
            "Target ${target} uses unknown validation category ${category}")
    endif()

    get_target_property(existing_category ${target} SIMDLIB_VALIDATION_CATEGORY)
    if(existing_category)
        message(FATAL_ERROR
            "Target ${target} has multiple validation owners: "
            "${existing_category} and ${category}")
    endif()

    set_property(TARGET ${target} PROPERTY
        SIMDLIB_VALIDATION_CATEGORY ${category})
endfunction()
# @brief Assigns one configured CTest test to its sole validation owner.
# @param test Existing CTest test name.
# @param owner Validation target category or the PROFILE_AUDIT test-only owner.
function(simdlib_register_development_test test owner)
    get_property(configured_tests DIRECTORY PROPERTY TESTS)
    if(NOT test IN_LIST configured_tests)
        message(FATAL_ERROR
            "Cannot assign validation ownership before test ${test} exists")
    endif()
    if(NOT owner IN_LIST SIMDLIB_VALIDATION_CATEGORIES AND
            NOT owner STREQUAL "PROFILE_AUDIT")
        message(FATAL_ERROR
            "Test ${test} uses unknown validation owner ${owner}")
    endif()

    get_property(existing_labels TEST "${test}" PROPERTY LABELS)
    set(existing_owner_labels ${existing_labels})
    list(FILTER existing_owner_labels INCLUDE
        REGEX "^SIMDLIB_OWNER_")
    if(existing_owner_labels)
        message(FATAL_ERROR
            "Test ${test} has multiple validation owners: "
            "${existing_owner_labels} and ${owner}")
    endif()

    list(APPEND existing_labels "SIMDLIB_OWNER_${owner}")
    list(REMOVE_DUPLICATES existing_labels)
    set_property(TEST "${test}" PROPERTY LABELS "${existing_labels}")
endfunction()
