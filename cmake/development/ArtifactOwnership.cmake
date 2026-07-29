include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "ArtifactOwnership.cmake is available only to top-level SimdLib builds")
endif()

set(SIMDLIB_VALIDATION_CATEGORIES
    REPOSITORY_AUDIT
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
