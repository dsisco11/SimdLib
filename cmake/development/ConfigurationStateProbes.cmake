include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "ConfigurationStateProbes.cmake is available only to top-level SimdLib builds")
endif()
if(NOT TARGET SimdLib)
    message(FATAL_ERROR "ConfigurationStateProbes.cmake requires the production SimdLib target")
endif()

block(SCOPE_FOR VARIABLES)

if(NOT SIMDLIB_DEFAULT_CHECKS_PROBE STREQUAL "NONE")
    if(SIMDLIB_DEFAULT_CHECKS_PROBE STREQUAL "RELEASE")
        set(default_checks_target ConfigDefaultChecksReleaseProbe)
        set(default_checks_expected 0)
    elseif(SIMDLIB_DEFAULT_CHECKS_PROBE STREQUAL "DEBUG")
        set(default_checks_target ConfigDefaultChecksDebugProbe)
        set(default_checks_expected 1)
    else()
        message(FATAL_ERROR
            "Unsupported default-checks probe ${SIMDLIB_DEFAULT_CHECKS_PROBE}")
    endif()

    add_library(${default_checks_target} OBJECT
        tests/config/ConfigDefaultChecksProbe.cpp)
    simdlib_register_development_target(${default_checks_target}
        COMPILER_CONTRACT)
    target_link_libraries(${default_checks_target} PRIVATE SimdLib::SimdLib)
    target_compile_definitions(${default_checks_target} PRIVATE
        SIMDLIB_EXPECT_DEFAULT_CHECKS=${default_checks_expected})
    simdlib_enable_development_warnings(${default_checks_target})
endif()

endblock()
