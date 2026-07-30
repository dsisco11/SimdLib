cmake_minimum_required(VERSION 3.20)

project(SimdLibConsumerSmoke LANGUAGES CXX)

if(NOT DEFINED SIMDLIB_SOURCE_DIR)
    get_filename_component(SIMDLIB_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

get_cmake_property(consumer_cache_before CACHE_VARIABLES)
add_subdirectory("${SIMDLIB_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/simdlib" EXCLUDE_FROM_ALL)
get_cmake_property(consumer_cache_after CACHE_VARIABLES)

if(DEFINED CACHE{BUILD_TESTING})
    message(FATAL_ERROR
        "add_subdirectory introduced CTest's BUILD_TESTING cache option")
endif()

foreach(cache_variable IN LISTS consumer_cache_after)
    if(cache_variable MATCHES "^SIMDLIB_" AND
            NOT cache_variable IN_LIST consumer_cache_before)
        message(FATAL_ERROR
            "add_subdirectory introduced development cache option ${cache_variable}")
    endif()
endforeach()

get_property(simdlib_nested_targets
    DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/simdlib" PROPERTY BUILDSYSTEM_TARGETS)
list(SORT simdlib_nested_targets)
if(NOT simdlib_nested_targets STREQUAL "SimdLib;SimdLibRegister")
    message(FATAL_ERROR
        "add_subdirectory introduced unexpected targets: ${simdlib_nested_targets}")
endif()
get_property(simdlib_nested_tests
	DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/simdlib" PROPERTY TESTS)
if(simdlib_nested_tests)
	message(FATAL_ERROR
		"add_subdirectory registered development tests: ${simdlib_nested_tests}")
endif()

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
