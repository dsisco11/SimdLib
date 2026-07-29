include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "Options.cmake is available only to top-level SimdLib builds")
endif()

block(SCOPE_FOR VARIABLES)

set(simdlib_retired_options
    SIMDLIB_BUILD_TESTS
    SIMDLIB_BUILD_TESTS_128
    SIMDLIB_BUILD_TESTS_256
    SIMDLIB_BUILD_TESTS_FMA
    SIMDLIB_BUILD_TESTS_OPTIONAL
    SIMDLIB_BUILD_CONFIGURATION_TESTS
    SIMDLIB_BUILD_HEADER_TESTS
    SIMDLIB_BUILD_REGISTER_CODEGEN
    SIMDLIB_REGISTER_CODEGEN_RECORD_ONLY)
set(simdlib_retired_replacements
    SIMDLIB_BUILD_RUNTIME_TESTS
    SIMDLIB_BUILD_API_SSE42_TESTS
    SIMDLIB_BUILD_API_AVX2_TESTS
    SIMDLIB_BUILD_FMA_TESTS
    SIMDLIB_BUILD_BMI_TESTS
    SIMDLIB_BUILD_CONFIGURATION_PROBES
    SIMDLIB_BUILD_HEADER_PROBES
    SIMDLIB_BUILD_REGISTER_CODEGEN_GATES
    SIMDLIB_REGISTER_CODEGEN_MODE)
list(LENGTH simdlib_retired_options simdlib_retired_option_count)
math(EXPR simdlib_retired_option_last "${simdlib_retired_option_count} - 1")
foreach(simdlib_retired_option_index RANGE ${simdlib_retired_option_last})
    list(GET simdlib_retired_options ${simdlib_retired_option_index} simdlib_retired_option)
    if(DEFINED CACHE{${simdlib_retired_option}})
        list(GET simdlib_retired_replacements ${simdlib_retired_option_index}
            simdlib_retired_replacement)
        message(FATAL_ERROR
            "Retired CMake option ${simdlib_retired_option} was supplied. "
            "Use ${simdlib_retired_replacement}; compatibility aliases are intentionally unavailable.")
    endif()
endforeach()

option(SIMDLIB_BUILD_SMOKE_TESTS "Build header-only ODR smoke tests" ON)
option(SIMDLIB_BUILD_RUNTIME_TESTS "Build Catch2 runtime tests" OFF)
option(SIMDLIB_BUILD_API_SSE42_TESTS "Build Api SSE4.2 tests" ON)
option(SIMDLIB_BUILD_API_AVX2_TESTS "Build Api AVX2 tests" ON)
option(SIMDLIB_BUILD_FMA_TESTS "Build FMA tests" ON)
option(SIMDLIB_BUILD_BMI_TESTS "Build BMI profile tests" OFF)
option(SIMDLIB_BUILD_VECTOR_ALGORITHM_TESTS
    "Build SimdVector, SimdAlgo, and resampling parity tests" ON)
option(SIMDLIB_BUILD_BENCHMARKS "Build Catch2 benchmarks" OFF)
option(SIMDLIB_BUILD_EXAMPLES "Build executable API examples" OFF)
option(SIMDLIB_BUILD_CONFIGURATION_PROBES
    "Build compile-only configuration probes" ON)
option(SIMDLIB_BUILD_CONSTEXPR_PROBES
    "Build compile-only constant-evaluation contract probes" ON)
option(SIMDLIB_BUILD_HEADER_PROBES
    "Build first-and-only public-header probes" ON)
option(SIMDLIB_FETCH_TEST_DEPENDENCIES
    "Fetch missing development-only dependencies" ON)
option(SIMDLIB_STRICT_WARNINGS
    "Treat warnings in SimdLib-owned development targets as errors" OFF)
option(SIMDLIB_ENABLE_COVERAGE
    "Instrument SimdLib-owned development targets for source coverage" OFF)
option(SIMDLIB_BUILD_REGISTER_CODEGEN_GATES
    "Build Register generated-code comparisons" OFF)
option(SIMDLIB_VALIDATE_EXHAUSTIVE_TARGETS
    "Require every category selected by the validation profile to contain owned targets" OFF)

set(SIMDLIB_VALIDATION_PROFILE "CUSTOM" CACHE STRING
    "Validation ownership profile: CUSTOM, RELEASE, DEBUG, SANITIZER, COVERAGE, or COMPILER_CONTRACTS")
set_property(CACHE SIMDLIB_VALIDATION_PROFILE PROPERTY STRINGS
    CUSTOM RELEASE DEBUG SANITIZER COVERAGE COMPILER_CONTRACTS)
if(NOT SIMDLIB_VALIDATION_PROFILE MATCHES
        "^(CUSTOM|RELEASE|DEBUG|SANITIZER|COVERAGE|COMPILER_CONTRACTS)$")
    message(FATAL_ERROR
        "SIMDLIB_VALIDATION_PROFILE has unsupported value "
        "'${SIMDLIB_VALIDATION_PROFILE}'")
endif()

set(SIMDLIB_REGISTER_CODEGEN_MODE "ENFORCE" CACHE STRING
    "Register generated-code policy: ENFORCE or RECORD")
set_property(CACHE SIMDLIB_REGISTER_CODEGEN_MODE PROPERTY STRINGS ENFORCE RECORD)
if(NOT SIMDLIB_REGISTER_CODEGEN_MODE MATCHES "^(ENFORCE|RECORD)$")
    message(FATAL_ERROR
        "SIMDLIB_REGISTER_CODEGEN_MODE must be ENFORCE or RECORD; got '${SIMDLIB_REGISTER_CODEGEN_MODE}'")
endif()

if(SIMDLIB_ENABLE_COVERAGE)
    set(CTEST_TEST_COVERAGE_TOOL "LLVM-COV" PARENT_SCOPE)
endif()

endblock()
