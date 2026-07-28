cmake_minimum_required(VERSION 3.20)

project(SimdLibConsumerSmoke LANGUAGES CXX)

if(NOT DEFINED SIMDLIB_SOURCE_DIR)
    get_filename_component(SIMDLIB_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

add_subdirectory("${SIMDLIB_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/simdlib" EXCLUDE_FROM_ALL)

if(DEFINED CACHE{BUILD_TESTING})
	message(FATAL_ERROR
		"add_subdirectory introduced CTest's BUILD_TESTING cache option")
endif()
get_property(simdlib_nested_tests
	DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/simdlib" PROPERTY TESTS)
if(simdlib_nested_tests)
	message(FATAL_ERROR
		"add_subdirectory registered development tests: ${simdlib_nested_tests}")
endif()

set(simdlib_forbidden_development_options
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
	SIMDLIB_BUILD_CONSTEXPR_PROBES
	SIMDLIB_BUILD_HEADER_PROBES
	SIMDLIB_FETCH_TEST_DEPENDENCIES
	SIMDLIB_STRICT_WARNINGS
	SIMDLIB_ENABLE_COVERAGE
	SIMDLIB_BUILD_REGISTER_CODEGEN_GATES
	SIMDLIB_REGISTER_CODEGEN_MODE
	SIMDLIB_VALIDATE_EXHAUSTIVE_TARGETS)
foreach(simdlib_forbidden_development_option IN LISTS simdlib_forbidden_development_options)
	if(DEFINED CACHE{${simdlib_forbidden_development_option}})
		message(FATAL_ERROR
			"add_subdirectory introduced development option ${simdlib_forbidden_development_option}")
	endif()
endforeach()

set(simdlib_forbidden_development_targets
	ExhaustiveArtifacts BenchmarkArtifacts Benchmarks DevelopmentWarnings
	ApiExamples RegisterExamples CoverageReset CoverageReport Catch2 Catch2WithMain)
foreach(simdlib_forbidden_development_target IN LISTS simdlib_forbidden_development_targets)
	if(TARGET ${simdlib_forbidden_development_target})
		message(FATAL_ERROR
			"add_subdirectory introduced development target ${simdlib_forbidden_development_target}")
	endif()
endforeach()

get_target_property(simdlib_target_type SimdLib TYPE)
if(NOT simdlib_target_type STREQUAL "INTERFACE_LIBRARY")
    message(FATAL_ERROR "SimdLib must remain header-only; target type is ${simdlib_target_type}")
endif()

get_target_property(simdlib_core_features SimdLib INTERFACE_COMPILE_FEATURES)
if(NOT "cxx_std_20" IN_LIST simdlib_core_features OR "cxx_std_23" IN_LIST simdlib_core_features)
	message(FATAL_ERROR "SimdLib::SimdLib must require C++20 without inheriting the Register language requirement")
endif()

get_target_property(simdlib_register_features SimdLibRegister INTERFACE_COMPILE_FEATURES)
get_target_property(simdlib_register_definitions SimdLibRegister INTERFACE_COMPILE_DEFINITIONS)
get_target_property(simdlib_register_links SimdLibRegister INTERFACE_LINK_LIBRARIES)
if(NOT "cxx_std_23" IN_LIST simdlib_register_features)
	message(FATAL_ERROR "SimdLib::Register must request C++23")
endif()
if(NOT "SIMDLIB_REQUIRE_REGISTER_INTERFACE=1" IN_LIST simdlib_register_definitions)
	message(FATAL_ERROR "SimdLib::Register must publish the Register requirement signal")
endif()
if(NOT "SimdLib::SimdLib" IN_LIST simdlib_register_links)
	message(FATAL_ERROR "SimdLib::Register must link the core SimdLib target")
endif()

add_executable(CoreConsumerSmoke main.cpp)
target_link_libraries(CoreConsumerSmoke PRIVATE SimdLib::SimdLib)
set_target_properties(CoreConsumerSmoke PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF)
if(MSVC)
	target_compile_definitions(CoreConsumerSmoke PRIVATE
		SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
	target_compile_options(CoreConsumerSmoke PRIVATE /arch:AVX2)
else()
	target_compile_options(CoreConsumerSmoke PRIVATE -msse4.2)
endif()

enable_testing()
add_test(NAME CoreConsumerSmoke COMMAND CoreConsumerSmoke)

get_target_property(simdlib_register_compiler_supported SimdLibRegister
	SIMDLIB_REGISTER_COMPILER_SUPPORTED)
option(SIMDLIB_BUILD_REGISTER_CONSUMER
	"Build the opt-in C++23 Register consumer smoke test"
	${simdlib_register_compiler_supported})
if(SIMDLIB_BUILD_REGISTER_CONSUMER)
	add_executable(RegisterConsumerSmoke
		register.cpp
		register_api.cpp
		register_api.h)
	target_link_libraries(RegisterConsumerSmoke PRIVATE SimdLib::Register)
	target_compile_definitions(RegisterConsumerSmoke PRIVATE
		SIMDLIB_HAS_SSE=1 SIMDLIB_HAS_SSE2=1 SIMDLIB_HAS_SSE3=1
		SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1
		SIMDLIB_HAS_AVX=0 SIMDLIB_HAS_AVX2=0 SIMDLIB_HAS_FMA=0)
	if(MSVC)
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			target_compile_options(RegisterConsumerSmoke PRIVATE
				/clang:-msse4.2 /clang:-mno-avx /clang:-mno-avx2 /clang:-mno-fma)
		endif()
	else()
		target_compile_options(RegisterConsumerSmoke PRIVATE
			-msse4.2 -mno-avx -mno-avx2 -mno-fma)
	endif()
	add_test(NAME RegisterConsumerSmoke COMMAND RegisterConsumerSmoke)
endif()
