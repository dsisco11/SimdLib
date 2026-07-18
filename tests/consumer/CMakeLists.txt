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

add_executable(SimdLibConsumerSmoke main.cpp)
target_link_libraries(SimdLibConsumerSmoke PRIVATE SimdLib::SimdLib)
if(MSVC)
	target_compile_definitions(SimdLibConsumerSmoke PRIVATE
		SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
	target_compile_options(SimdLibConsumerSmoke PRIVATE /arch:AVX2)
else()
	target_compile_options(SimdLibConsumerSmoke PRIVATE -msse4.2)
endif()

enable_testing()
add_test(NAME SimdLib.ConsumerSmoke COMMAND SimdLibConsumerSmoke)
