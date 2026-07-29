include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "Coverage.cmake is available only to top-level SimdLib builds")
endif()
if(NOT TARGET SimdLib OR NOT TARGET SimdLibRegister)
    message(FATAL_ERROR "Coverage.cmake requires the production SimdLib targets")
endif()

block(SCOPE_FOR VARIABLES)

if(SIMDLIB_ENABLE_COVERAGE)
    get_filename_component(simdlib_compiler_directory "${CMAKE_CXX_COMPILER}" DIRECTORY)
    find_program(SIMDLIB_LLVM_PROFDATA
        NAMES llvm-profdata
        HINTS "${simdlib_compiler_directory}"
        REQUIRED)
    find_program(SIMDLIB_LLVM_COV
        NAMES llvm-cov
        HINTS "${simdlib_compiler_directory}"
        REQUIRED)
    find_program(SIMDLIB_LLVM_READOBJ
        NAMES llvm-readobj
        HINTS "${simdlib_compiler_directory}"
        REQUIRED)

    get_property(simdlib_coverage_targets GLOBAL PROPERTY SIMDLIB_COVERAGE_TARGETS)
    list(REMOVE_DUPLICATES simdlib_coverage_targets)
    if(NOT simdlib_coverage_targets)
        message(FATAL_ERROR "SIMDLIB_ENABLE_COVERAGE requires at least one executable target")
    endif()

    set(simdlib_coverage_manifest "")
    foreach(coverage_target IN LISTS simdlib_coverage_targets)
        get_target_property(coverage_profile_prefix ${coverage_target}
            SIMDLIB_COVERAGE_PROFILE_PREFIX)
        if(NOT coverage_profile_prefix)
            message(FATAL_ERROR
                "Coverage target ${coverage_target} has no CTest profile prefix")
        endif()
        string(APPEND simdlib_coverage_manifest
            "${coverage_target}|$<TARGET_FILE:${coverage_target}>|${coverage_profile_prefix}\n")
    endforeach()
    set(simdlib_coverage_manifest_file
        "${CMAKE_CURRENT_BINARY_DIR}/coverage-targets-$<CONFIG>.txt")
    file(GENERATE
        OUTPUT "${simdlib_coverage_manifest_file}"
        CONTENT "${simdlib_coverage_manifest}")

    add_custom_target(CoverageReset
        COMMAND ${CMAKE_COMMAND}
            -DBINARY_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ResetCoverage.cmake
        COMMENT "Removing previous SimdLib coverage data"
        VERBATIM)
    simdlib_register_development_target(CoverageReset COVERAGE_SUPPORT)

    add_custom_target(CoverageReport
        COMMAND ${CMAKE_COMMAND}
            -DBINARY_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}
            -DSOURCE_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}
            -DCOVERAGE_MANIFEST=${simdlib_coverage_manifest_file}
            -DLLVM_PROFDATA=${SIMDLIB_LLVM_PROFDATA}
            -DLLVM_COV=${SIMDLIB_LLVM_COV}
            -DLLVM_READOBJ=${SIMDLIB_LLVM_READOBJ}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateCoverageReport.cmake
        DEPENDS ${simdlib_coverage_targets}
        COMMENT "Generating SimdLib LCOV coverage report"
        VERBATIM)
    simdlib_register_development_target(CoverageReport COVERAGE_SUPPORT)
endif()

endblock()
