cmake_minimum_required(VERSION 3.20)

project(SimdLibConsumerSmoke LANGUAGES CXX)

if(NOT DEFINED SIMDLIB_SOURCE_DIR)
    get_filename_component(SIMDLIB_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

set(SIMDLIB_BUILD_SMOKE_TESTS OFF CACHE BOOL "" FORCE)
set(SIMDLIB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SIMDLIB_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(SIMDLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SIMDLIB_BUILD_CONFIGURATION_TESTS OFF CACHE BOOL "" FORCE)
set(SIMDLIB_BUILD_HEADER_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory("${SIMDLIB_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/simdlib" EXCLUDE_FROM_ALL)

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

add_executable(SimdLibConsumerSmoke main.cpp)
target_link_libraries(SimdLibConsumerSmoke PRIVATE SimdLib::SimdLib)
set_target_properties(SimdLibConsumerSmoke PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF)
if(MSVC)
	target_compile_definitions(SimdLibConsumerSmoke PRIVATE
		SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
	target_compile_options(SimdLibConsumerSmoke PRIVATE /arch:AVX2)
else()
	target_compile_options(SimdLibConsumerSmoke PRIVATE -msse4.2)
endif()

enable_testing()
add_test(NAME SimdLib.ConsumerSmoke COMMAND SimdLibConsumerSmoke)

get_target_property(simdlib_register_compiler_supported SimdLibRegister
	SIMDLIB_REGISTER_COMPILER_SUPPORTED)
option(SIMDLIB_BUILD_REGISTER_CONSUMER
	"Build the opt-in C++23 Register consumer smoke test"
	${simdlib_register_compiler_supported})
if(SIMDLIB_BUILD_REGISTER_CONSUMER)
	add_executable(SimdLibRegisterConsumerSmoke register.cpp)
	target_link_libraries(SimdLibRegisterConsumerSmoke PRIVATE SimdLib::Register)
	add_test(NAME SimdLib.RegisterConsumerSmoke COMMAND SimdLibRegisterConsumerSmoke)
endif()
