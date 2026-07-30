cmake_minimum_required(VERSION 4.4)

project(SimdLibArtifactAggregateFixture LANGUAGES NONE)

if(NOT DEFINED SIMDLIB_SOURCE_DIRECTORY)
    message(FATAL_ERROR "SIMDLIB_SOURCE_DIRECTORY is required")
endif()
if(NOT DEFINED SIMDLIB_ARTIFACT_FAILURE_CASE)
    message(FATAL_ERROR "SIMDLIB_ARTIFACT_FAILURE_CASE is required")
endif()

set(SIMDLIB_VALIDATION_PROFILE CUSTOM)
set(SIMDLIB_REGISTER_COMPILER_SUPPORTED OFF)
set(SIMDLIB_BUILD_REGISTER_CODEGEN_GATES OFF)
set(SIMDLIB_REGISTER_CODEGEN_MODE OFF)
include("${SIMDLIB_SOURCE_DIRECTORY}/cmake/development/ArtifactOwnership.cmake")

if(SIMDLIB_ARTIFACT_FAILURE_CASE STREQUAL "UNOWNED")
    add_custom_target(UnownedFixture)
elseif(SIMDLIB_ARTIFACT_FAILURE_CASE STREQUAL "MULTIPLE")
    add_custom_target(MultipleFixture)
    simdlib_register_development_target(MultipleFixture RUNTIME_VALIDATION)
    simdlib_register_development_target(MultipleFixture CHECKS_VALIDATION)
elseif(SIMDLIB_ARTIFACT_FAILURE_CASE STREQUAL "EXCLUDED")
    set(SIMDLIB_VALIDATION_PROFILE SANITIZER)
    add_custom_target(ExcludedFixture)
    simdlib_register_development_target(ExcludedFixture COMPILER_CONTRACT)
else()
    message(FATAL_ERROR
        "Unsupported fixture case ${SIMDLIB_ARTIFACT_FAILURE_CASE}")
endif()

include("${SIMDLIB_SOURCE_DIRECTORY}/cmake/development/ArtifactAggregates.cmake")
