include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "ArtifactAggregates.cmake is available only to top-level SimdLib builds")
endif()

block(SCOPE_FOR VARIABLES)

get_property(simdlib_development_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
list(REMOVE_DUPLICATES simdlib_development_targets)
list(FILTER simdlib_development_targets EXCLUDE
    REGEX "^(Continuous|Experimental|Nightly)")
list(SORT simdlib_development_targets)

set(simdlib_non_exhaustive_targets
    Benchmarks
    CoverageReset
    CoverageReport)
set(simdlib_exhaustive_dependencies "")
foreach(simdlib_development_target IN LISTS simdlib_development_targets)
    get_target_property(simdlib_development_target_type
        ${simdlib_development_target} TYPE)
    if(NOT simdlib_development_target_type STREQUAL "INTERFACE_LIBRARY"
            AND NOT simdlib_development_target IN_LIST simdlib_non_exhaustive_targets)
        list(APPEND simdlib_exhaustive_dependencies ${simdlib_development_target})
    endif()
endforeach()

add_custom_target(ExhaustiveArtifacts)
if(simdlib_exhaustive_dependencies)
    add_dependencies(ExhaustiveArtifacts ${simdlib_exhaustive_dependencies})
endif()

add_custom_target(BenchmarkArtifacts)
if(TARGET Benchmarks)
    add_dependencies(BenchmarkArtifacts Benchmarks)
endif()

list(APPEND simdlib_development_targets ExhaustiveArtifacts BenchmarkArtifacts)
list(REMOVE_DUPLICATES simdlib_development_targets)
list(SORT simdlib_development_targets)
string(REPLACE ";" "\n" simdlib_development_target_inventory
    "${simdlib_development_targets}")
file(WRITE "${CMAKE_BINARY_DIR}/development-targets.txt"
    "${simdlib_development_target_inventory}\n")

set(simdlib_external_consumer_targets CoreConsumerSmoke)
if(SIMDLIB_REGISTER_COMPILER_SUPPORTED)
    list(APPEND simdlib_external_consumer_targets RegisterConsumerSmoke)
endif()
string(REPLACE ";" "\n" simdlib_external_consumer_inventory
    "${simdlib_external_consumer_targets}")
file(WRITE "${CMAKE_BINARY_DIR}/external-consumer-targets.txt"
    "${simdlib_external_consumer_inventory}\n")

if(SIMDLIB_VALIDATE_EXHAUSTIVE_TARGETS)
    set(simdlib_required_exhaustive_options
        SIMDLIB_BUILD_SMOKE_TESTS
        SIMDLIB_BUILD_RUNTIME_TESTS
        SIMDLIB_BUILD_API_SSE42_TESTS
        SIMDLIB_BUILD_API_AVX2_TESTS
        SIMDLIB_BUILD_FMA_TESTS
        SIMDLIB_BUILD_BMI_TESTS
        SIMDLIB_BUILD_VECTOR_ALGORITHM_TESTS
        SIMDLIB_BUILD_BENCHMARKS
        SIMDLIB_BUILD_EXAMPLES
        SIMDLIB_BUILD_CONFIGURATION_PROBES
        SIMDLIB_BUILD_HEADER_PROBES)
    foreach(simdlib_required_exhaustive_option IN LISTS simdlib_required_exhaustive_options)
        if(NOT ${simdlib_required_exhaustive_option})
            message(FATAL_ERROR
                "Exhaustive profile requires ${simdlib_required_exhaustive_option}=ON")
        endif()
    endforeach()

    set(simdlib_required_exhaustive_targets
        ApiSse42Tests ApiAvx2Tests FmaEnabledTests FmaDisabledTests
        BmiPortableTests Bmi1Tests Bmi2Tests Bmi1Bmi2Tests
        VectorAlgorithmsTests ResampleScalarTests ApiExamples Benchmarks
        PublicHeaderAssertionAudit ConstexprProbes)
    if(SIMDLIB_REGISTER_COMPILER_SUPPORTED)
        list(APPEND simdlib_required_exhaustive_targets
            RegisterSse42Tests RegisterAvx2Tests RegisterExamples)
        if(SIMDLIB_BUILD_REGISTER_CODEGEN_GATES)
            list(APPEND simdlib_required_exhaustive_targets RegisterCodegen)
        endif()
    endif()
    foreach(simdlib_required_exhaustive_target IN LISTS simdlib_required_exhaustive_targets)
        if(NOT TARGET ${simdlib_required_exhaustive_target})
            message(FATAL_ERROR
                "Exhaustive target inventory is missing ${simdlib_required_exhaustive_target}")
        endif()
    endforeach()
endif()

endblock()
