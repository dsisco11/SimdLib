cmake_minimum_required(VERSION 3.20)

project(SimdLib VERSION 0.2.0 LANGUAGES CXX)

option(SIMDLIB_BUILD_SMOKE_TESTS "Build header-only ODR smoke tests" ON)
option(SIMDLIB_BUILD_TESTS "Build SimdLib Catch2 tests" OFF)
option(SIMDLIB_BUILD_TESTS_128 "Build 128-bit SSE4.2 tests" ON)
option(SIMDLIB_BUILD_TESTS_256 "Build 256-bit AVX2 tests" ON)
option(SIMDLIB_BUILD_TESTS_FMA "Build FMA tests" ON)
option(SIMDLIB_BUILD_TESTS_OPTIONAL "Build optional BMI-family tests" OFF)
option(SIMDLIB_BUILD_VECTOR_ALGORITHM_TESTS "Build SimdVector, SimdAlgo, and resampling parity tests" ON)
option(SIMDLIB_BUILD_BENCHMARKS "Build SimdLib Catch2 benchmarks" OFF)
option(SIMDLIB_BUILD_EXAMPLES "Build the executable API example" OFF)
option(SIMDLIB_BUILD_CONFIGURATION_TESTS "Build compile-only configuration probes" ON)
option(SIMDLIB_BUILD_HEADER_TESTS "Build first-and-only public-header probes" ON)
option(SIMDLIB_FETCH_TEST_DEPENDENCIES "Fetch missing test-only dependencies" ON)
option(SIMDLIB_STRICT_WARNINGS "Treat warnings in SimdLib-owned targets as errors" OFF)
option(SIMDLIB_ENABLE_COVERAGE "Instrument SimdLib-owned targets for source coverage" OFF)

add_library(SimdLib INTERFACE)
add_library(SimdLib::SimdLib ALIAS SimdLib)

set(SIMDLIB_MSVC_STYLE_DRIVER ${MSVC})
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
	set(SIMDLIB_MSVC_STYLE_DRIVER OFF)
endif()

target_compile_features(SimdLib INTERFACE cxx_std_20)
target_include_directories(SimdLib INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_sources(SimdLib INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/visualizers/SimdLib.natvis>)

# Consumer-facing examples and probes may use focused public headers, but must
# never depend on implementation-only Detail declarations or include paths.
file(GLOB_RECURSE SIMDLIB_PUBLIC_CONSUMER_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/tests/consumer/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/tests/headers/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke/*.cpp")
foreach(consumer_source IN LISTS SIMDLIB_PUBLIC_CONSUMER_SOURCES)
	file(READ "${consumer_source}" consumer_source_text)
	if(consumer_source_text MATCHES "SimdLib::Detail|<SimdLib/Detail/")
		message(FATAL_ERROR "Public consumer surface names implementation detail: ${consumer_source}")
	endif()
endforeach()

add_library(SimdLibDevelopmentWarnings INTERFACE)
if(SIMDLIB_ENABLE_COVERAGE)
	if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR SIMDLIB_MSVC_STYLE_DRIVER)
		message(FATAL_ERROR "SIMDLIB_ENABLE_COVERAGE currently requires Clang with its GNU-like command-line driver")
	endif()
	target_compile_options(SimdLibDevelopmentWarnings INTERFACE
		-fprofile-instr-generate -fcoverage-mapping)
	target_link_options(SimdLibDevelopmentWarnings INTERFACE
		-fprofile-instr-generate)
endif()
if(SIMDLIB_STRICT_WARNINGS)
	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_options(SimdLibDevelopmentWarnings INTERFACE
				/W4 /WX /permissive-
				-Wno-unknown-attributes -Wno-ignored-attributes -Wno-c2y-extensions)
		else()
			target_compile_options(SimdLibDevelopmentWarnings INTERFACE
				-Wall -Wextra -Wpedantic -Werror
				-Wno-unknown-attributes -Wno-ignored-attributes -Wno-c2y-extensions)
		endif()
	elseif(SIMDLIB_MSVC_STYLE_DRIVER)
		target_compile_options(SimdLibDevelopmentWarnings INTERFACE /W4 /WX /permissive-)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(SimdLibDevelopmentWarnings INTERFACE
            -Wall -Wextra -Wpedantic -Werror
            -Wno-attributes -Wno-ignored-attributes)
    endif()
endif()

function(simdlib_enable_development_warnings target)
    target_link_libraries(${target} PRIVATE SimdLibDevelopmentWarnings)
endfunction()

if(SIMDLIB_BUILD_CONFIGURATION_TESTS)
    foreach(config_probe IN ITEMS
        ConfigDefaultProbe
        ConfigOverrideVectorcallProbe
        ConfigOverrideForceInlineProbe
        ConfigOverridePreconditionProbe
        ConfigDisabledInstructionsProbe
        ConfigDisabledPublicHeadersProbe
        ConfigClangUnsupportedTargetProbe
        ConfigVendorAttributeProbe
        ConstexprProbe)
        add_library(SimdLib${config_probe} OBJECT tests/config/${config_probe}.cpp)
        target_link_libraries(SimdLib${config_probe} PRIVATE SimdLib::SimdLib)
        simdlib_enable_development_warnings(SimdLib${config_probe})
    endforeach()
endif()

if(SIMDLIB_BUILD_HEADER_TESTS)
    foreach(header_probe IN ITEMS
        Config
        TemplateTools
        Api
        SimdApi
        SimdVector
        SimdAlgo
        SimdResample
        Bmi
        UInt128
        Format
        SimdLib
        PublicSurface)
        add_library(SimdLibHeader${header_probe}Probe OBJECT tests/headers/${header_probe}HeaderProbe.cpp)
        target_link_libraries(SimdLibHeader${header_probe}Probe PRIVATE SimdLib::SimdLib)
        simdlib_enable_development_warnings(SimdLibHeader${header_probe}Probe)
    endforeach()
endif()

add_library(SimdLibAvailabilityDisabledProbe OBJECT tests/availability/ApiDisabledProbe.cpp)
target_link_libraries(SimdLibAvailabilityDisabledProbe PRIVATE SimdLib::SimdLib)
simdlib_enable_development_warnings(SimdLibAvailabilityDisabledProbe)

add_library(SimdLibAvailabilityEnabledProbe OBJECT tests/availability/ApiEnabledProbe.cpp)
target_link_libraries(SimdLibAvailabilityEnabledProbe PRIVATE SimdLib::SimdLib)
simdlib_enable_development_warnings(SimdLibAvailabilityEnabledProbe)
if(SIMDLIB_MSVC_STYLE_DRIVER)
    target_compile_options(SimdLibAvailabilityEnabledProbe PRIVATE /arch:AVX2)
else()
    target_compile_options(SimdLibAvailabilityEnabledProbe PRIVATE -mavx2)
endif()

if(SIMDLIB_BUILD_SMOKE_TESTS)
    enable_testing()
    add_executable(SimdLibHeaderOnlySmoke
        tests/smoke/main.cpp
        tests/smoke/second_translation_unit.cpp)
    target_link_libraries(SimdLibHeaderOnlySmoke PRIVATE SimdLib::SimdLib)
    simdlib_enable_development_warnings(SimdLibHeaderOnlySmoke)
    add_test(NAME SimdLib.HeaderOnlySmoke COMMAND SimdLibHeaderOnlySmoke)
endif()

if(SIMDLIB_BUILD_TESTS)
    enable_testing()
    find_package(Catch2 3 CONFIG QUIET)
    if(NOT Catch2_FOUND AND SIMDLIB_FETCH_TEST_DEPENDENCIES)
        include(FetchContent)
        FetchContent_Declare(Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.8.1
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(Catch2)
    endif()
    if(NOT TARGET Catch2::Catch2WithMain)
        message(FATAL_ERROR "Catch2 3 is required; install it or enable SIMDLIB_FETCH_TEST_DEPENDENCIES")
    endif()
    function(simdlib_add_catch_test target source test_name)
        add_executable(${target} ${source})
        target_link_libraries(${target} PRIVATE SimdLib::SimdLib Catch2::Catch2WithMain)
        simdlib_enable_development_warnings(${target})
        add_test(NAME ${test_name} COMMAND ${target})
    endfunction()

    simdlib_add_catch_test(SimdLibTestsBmiPortable tests/Bmi.tests.cpp SimdLib.Tests.BmiPortable)
    target_compile_definitions(SimdLibTestsBmiPortable PRIVATE
        SIMDLIB_HAS_BMI1=0 SIMDLIB_HAS_BMI2=0
        SIMDLIB_BMI_EXPECT_BMI1=0 SIMDLIB_BMI_EXPECT_BMI2=0)
	if(NOT SIMDLIB_MSVC_STYLE_DRIVER)
        target_compile_options(SimdLibTestsBmiPortable PRIVATE -mno-bmi -mno-bmi2)
    endif()
    set_tests_properties(SimdLib.Tests.BmiPortable PROPERTIES LABELS "BMI;PORTABLE")

    simdlib_add_catch_test(SimdLibTestsFormat tests/Format.tests.cpp SimdLib.Tests.Format)
    set_tests_properties(SimdLib.Tests.Format PROPERTIES LABELS "FORMAT;SSE42")
	add_executable(SimdLibFormatOdr
		tests/format_odr/main.cpp
		tests/format_odr/second_translation_unit.cpp)
	target_link_libraries(SimdLibFormatOdr PRIVATE SimdLib::SimdLib)
	simdlib_enable_development_warnings(SimdLibFormatOdr)
	add_test(NAME SimdLib.FormatOdr COMMAND SimdLibFormatOdr)
	set_tests_properties(SimdLib.FormatOdr PROPERTIES LABELS "FORMAT;ODR")
	if(SIMDLIB_MSVC_STYLE_DRIVER)
		target_compile_definitions(SimdLibTestsFormat PRIVATE
			SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
		target_compile_definitions(SimdLibFormatOdr PRIVATE
			SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			target_compile_options(SimdLibTestsFormat PRIVATE /arch:AVX2)
			target_compile_options(SimdLibFormatOdr PRIVATE /arch:AVX2)
		endif()
    else()
        target_compile_options(SimdLibTestsFormat PRIVATE -msse4.2)
		target_compile_options(SimdLibFormatOdr PRIVATE -msse4.2)
    endif()

    if(SIMDLIB_BUILD_TESTS_128)
        simdlib_add_catch_test(SimdLibTests128 tests/Api128.tests.cpp SimdLib.Tests.SSE42)
        set_tests_properties(SimdLib.Tests.SSE42 PROPERTIES LABELS "SSE42")
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_definitions(SimdLibTests128 PRIVATE
				SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
			if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
				target_compile_options(SimdLibTests128 PRIVATE /arch:AVX2)
			endif()
        else()
            target_compile_options(SimdLibTests128 PRIVATE -msse4.2)
        endif()

        simdlib_add_catch_test(SimdLibTestsUInt128Optimized tests/UInt128.tests.cpp SimdLib.Tests.UInt128Optimized)
        simdlib_add_catch_test(SimdLibTestsUInt128Portable tests/UInt128.tests.cpp SimdLib.Tests.UInt128Portable)
        simdlib_add_catch_test(SimdLibTestsUInt128Scalar tests/UInt128.tests.cpp SimdLib.Tests.UInt128Scalar)
        target_compile_definitions(SimdLibTestsUInt128Portable PRIVATE
            SIMDLIB_USE_COMPILER_CARRY_INTRINSICS=0)
		target_compile_definitions(SimdLibTestsUInt128Scalar PRIVATE
			SIMDLIB_USE_COMPILER_CARRY_INTRINSICS=0
			SIMDLIB_HAS_SSE=0 SIMDLIB_HAS_SSE2=0 SIMDLIB_HAS_SSE3=0 SIMDLIB_HAS_SSSE3=0
			SIMDLIB_HAS_SSE41=0 SIMDLIB_HAS_SSE42=0 SIMDLIB_HAS_AVX=0 SIMDLIB_HAS_AVX2=0
			SIMDLIB_HAS_FMA=0 SIMDLIB_HAS_BMI1=0 SIMDLIB_HAS_BMI2=0)
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_definitions(SimdLibTestsUInt128Optimized PRIVATE
				SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
			target_compile_definitions(SimdLibTestsUInt128Portable PRIVATE
				SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
			if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
				target_compile_options(SimdLibTestsUInt128Optimized PRIVATE /arch:AVX2)
				target_compile_options(SimdLibTestsUInt128Portable PRIVATE /arch:AVX2)
			endif()
        else()
            target_compile_options(SimdLibTestsUInt128Optimized PRIVATE -msse4.2)
            target_compile_options(SimdLibTestsUInt128Portable PRIVATE -msse4.2)
        endif()
        set_tests_properties(SimdLib.Tests.UInt128Optimized PROPERTIES LABELS "UINT128;OPTIMIZED;SSE42")
        set_tests_properties(SimdLib.Tests.UInt128Portable PROPERTIES LABELS "UINT128;PORTABLE;SSE42")
		set_tests_properties(SimdLib.Tests.UInt128Scalar PROPERTIES LABELS "UINT128;PORTABLE;SCALAR")

        add_test(NAME SimdLib.Tests.UInt128ResultSetEquivalence
            COMMAND ${CMAKE_COMMAND}
                -DPORTABLE_EXECUTABLE=$<TARGET_FILE:SimdLibTestsUInt128Portable>
                -DOPTIMIZED_EXECUTABLE=$<TARGET_FILE:SimdLibTestsUInt128Optimized>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareUInt128ResultSets.cmake)
        set_tests_properties(SimdLib.Tests.UInt128ResultSetEquivalence PROPERTIES LABELS "UINT128;EQUIVALENCE;SSE42")

		add_test(NAME SimdLib.Tests.UInt128ScalarResultSetEquivalence
			COMMAND ${CMAKE_COMMAND}
				-DPORTABLE_EXECUTABLE=$<TARGET_FILE:SimdLibTestsUInt128Scalar>
				-DOPTIMIZED_EXECUTABLE=$<TARGET_FILE:SimdLibTestsUInt128Optimized>
				-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareUInt128ResultSets.cmake)
		set_tests_properties(SimdLib.Tests.UInt128ScalarResultSetEquivalence PROPERTIES LABELS "UINT128;EQUIVALENCE;SCALAR")
    endif()

    if(SIMDLIB_BUILD_TESTS_256)
        simdlib_add_catch_test(SimdLibTests256 tests/Api256.tests.cpp SimdLib.Tests.AVX2)
        set_tests_properties(SimdLib.Tests.AVX2 PROPERTIES LABELS "AVX2")
		if(SIMDLIB_MSVC_STYLE_DRIVER)
            target_compile_options(SimdLibTests256 PRIVATE /arch:AVX2)
        else()
            target_compile_options(SimdLibTests256 PRIVATE -mavx2)
        endif()
    endif()

    if(SIMDLIB_BUILD_TESTS_FMA)
        simdlib_add_catch_test(SimdLibTestsFmaEnabled tests/SimdFma.tests.cpp SimdLib.Tests.FMA.Enabled)
        simdlib_add_catch_test(SimdLibTestsFmaDisabled tests/SimdFma.tests.cpp SimdLib.Tests.FMA.Disabled)
		target_compile_definitions(SimdLibTestsFmaEnabled PRIVATE SIMDLIB_HAS_FMA=1 SIMDLIB_EXPECT_FMA=1)
        target_compile_definitions(SimdLibTestsFmaDisabled PRIVATE SIMDLIB_HAS_FMA=0 SIMDLIB_EXPECT_FMA=0)
        set_tests_properties(SimdLib.Tests.FMA.Enabled PROPERTIES LABELS "FMA;ENABLED")
        set_tests_properties(SimdLib.Tests.FMA.Disabled PROPERTIES LABELS "FMA;DISABLED")
		if(SIMDLIB_MSVC_STYLE_DRIVER)
            target_compile_options(SimdLibTestsFmaEnabled PRIVATE /arch:AVX2)
            target_compile_options(SimdLibTestsFmaDisabled PRIVATE /arch:AVX2)
        else()
            target_compile_options(SimdLibTestsFmaEnabled PRIVATE -mavx2 -mfma)
            target_compile_options(SimdLibTestsFmaDisabled PRIVATE -mavx2 -mno-fma)
        endif()
    endif()

    if(SIMDLIB_BUILD_TESTS_OPTIONAL)
        function(simdlib_add_bmi_profile profile_name bmi1 bmi2)
            set(target SimdLibTestsBmi${profile_name})
            set(test_name SimdLib.Tests.Bmi.${profile_name})
            simdlib_add_catch_test(${target} tests/Bmi.tests.cpp ${test_name})
            target_compile_definitions(${target} PRIVATE
                SIMDLIB_HAS_BMI1=${bmi1} SIMDLIB_HAS_BMI2=${bmi2}
                SIMDLIB_BMI_EXPECT_BMI1=${bmi1} SIMDLIB_BMI_EXPECT_BMI2=${bmi2})
			if(SIMDLIB_MSVC_STYLE_DRIVER)
                target_compile_options(${target} PRIVATE /arch:AVX2)
            else()
                target_compile_options(${target} PRIVATE -mno-bmi -mno-bmi2)
                if(bmi1)
                    target_compile_options(${target} PRIVATE -mbmi)
                endif()
                if(bmi2)
                    target_compile_options(${target} PRIVATE -mbmi2)
                endif()
            endif()
            set_tests_properties(${test_name} PROPERTIES LABELS "BMI;${profile_name};OPTIONAL")

            set(equivalence_name SimdLib.Tests.Bmi.${profile_name}.Equivalence)
            add_test(NAME ${equivalence_name}
                COMMAND ${CMAKE_COMMAND}
                    -DPORTABLE_EXECUTABLE=$<TARGET_FILE:SimdLibTestsBmiPortable>
                    -DENABLED_EXECUTABLE=$<TARGET_FILE:${target}>
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareBmiResultSets.cmake)
            set_tests_properties(${equivalence_name} PROPERTIES LABELS "BMI;EQUIVALENCE;${profile_name};OPTIONAL")
        endfunction()

        simdlib_add_bmi_profile(Bmi1Only 1 0)
        simdlib_add_bmi_profile(Bmi2Only 0 1)
        simdlib_add_bmi_profile(Bmi1AndBmi2 1 1)
    endif()

    if(SIMDLIB_BUILD_VECTOR_ALGORITHM_TESTS)
        add_executable(SimdLibTestsVectorAlgorithms
            tests/SimdVector.tests.cpp
            tests/SimdAlgo.tests.cpp
            tests/SimdResample.tests.cpp)
        target_link_libraries(SimdLibTestsVectorAlgorithms PRIVATE SimdLib::SimdLib Catch2::Catch2WithMain)
        simdlib_enable_development_warnings(SimdLibTestsVectorAlgorithms)
        add_test(NAME SimdLib.Tests.VectorAlgorithms COMMAND SimdLibTestsVectorAlgorithms)
        set_tests_properties(SimdLib.Tests.VectorAlgorithms PROPERTIES LABELS "VECTOR_ALGORITHMS;AVX2")
		if(SIMDLIB_MSVC_STYLE_DRIVER)
            target_compile_options(SimdLibTestsVectorAlgorithms PRIVATE /arch:AVX2)
        else()
            target_compile_options(SimdLibTestsVectorAlgorithms PRIVATE -mavx2 -mfma)
        endif()

        add_executable(SimdLibTestsResampleScalar tests/SimdResample.tests.cpp)
        target_link_libraries(SimdLibTestsResampleScalar PRIVATE SimdLib::SimdLib Catch2::Catch2WithMain)
        simdlib_enable_development_warnings(SimdLibTestsResampleScalar)
        target_compile_definitions(SimdLibTestsResampleScalar PRIVATE
            SIMDLIB_HAS_SSE3=0 SIMDLIB_HAS_SSSE3=0 SIMDLIB_HAS_SSE41=0 SIMDLIB_HAS_SSE42=0
            SIMDLIB_HAS_AVX=0 SIMDLIB_HAS_AVX2=0 SIMDLIB_HAS_FMA=0)
        add_test(NAME SimdLib.Tests.ResampleScalar COMMAND SimdLibTestsResampleScalar)
        set_tests_properties(SimdLib.Tests.ResampleScalar PROPERTIES LABELS "VECTOR_ALGORITHMS;SCALAR")
    endif()
endif()

if(SIMDLIB_BUILD_BENCHMARKS)
    if(NOT TARGET Catch2::Catch2WithMain)
        find_package(Catch2 3 CONFIG QUIET)
    endif()
    if(NOT Catch2_FOUND AND NOT TARGET Catch2::Catch2WithMain AND SIMDLIB_FETCH_TEST_DEPENDENCIES)
        include(FetchContent)
        FetchContent_Declare(Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.8.1
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(Catch2)
    endif()
    if(NOT TARGET Catch2::Catch2WithMain)
        message(FATAL_ERROR "Catch2 3 is required; install it or enable SIMDLIB_FETCH_TEST_DEPENDENCIES")
    endif()
    add_executable(SimdLibBenchmarks benchmarks/SimdLib.benchmarks.cpp)
    target_link_libraries(SimdLibBenchmarks PRIVATE SimdLib::SimdLib Catch2::Catch2WithMain)
    simdlib_enable_development_warnings(SimdLibBenchmarks)
    target_compile_definitions(SimdLibBenchmarks PRIVATE SIMDLIB_HAS_BMI1=1 SIMDLIB_HAS_BMI2=1)
	if(SIMDLIB_MSVC_STYLE_DRIVER)
        target_compile_options(SimdLibBenchmarks PRIVATE /arch:AVX2)
    else()
        target_compile_options(SimdLibBenchmarks PRIVATE -mavx2 -mfma -mbmi -mbmi2)
    endif()
endif()

if(SIMDLIB_BUILD_EXAMPLES)
    enable_testing()
    add_executable(SimdLibApiExamples examples/ApiExamples.cpp)
    target_link_libraries(SimdLibApiExamples PRIVATE SimdLib::SimdLib)
    simdlib_enable_development_warnings(SimdLibApiExamples)
	if(SIMDLIB_MSVC_STYLE_DRIVER)
        target_compile_options(SimdLibApiExamples PRIVATE /arch:AVX2)
    else()
        target_compile_options(SimdLibApiExamples PRIVATE -mavx2 -mfma -mbmi -mbmi2)
    endif()
    add_test(NAME SimdLib.ApiExamples COMMAND SimdLibApiExamples)
    set_tests_properties(SimdLib.ApiExamples PROPERTIES LABELS "EXAMPLES;AVX2;FMA;BMI")
endif()
