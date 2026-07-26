cmake_minimum_required(VERSION 3.20)

project(SimdLibParentSmoke LANGUAGES CXX)

if(NOT DEFINED SIMDLIB_SOURCE_DIR)
	get_filename_component(SIMDLIB_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

include(CTest)
if(NOT BUILD_TESTING)
	message(FATAL_ERROR "The parent-consumer fixture requires its own tests to be enabled")
endif()
add_test(NAME ParentConsumer.OwnTest COMMAND "${CMAKE_COMMAND}" -E true)

get_cmake_property(parent_cache_before CACHE_VARIABLES)
add_subdirectory("${SIMDLIB_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/simdlib" EXCLUDE_FROM_ALL)
get_cmake_property(parent_cache_after CACHE_VARIABLES)

foreach(cache_variable IN LISTS parent_cache_after)
	if(cache_variable MATCHES "^SIMDLIB_" AND NOT cache_variable IN_LIST parent_cache_before)
		message(FATAL_ERROR
			"add_subdirectory introduced development cache option ${cache_variable}")
	endif()
endforeach()

get_property(simdlib_nested_tests
	DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/simdlib" PROPERTY TESTS)
if(simdlib_nested_tests)
	message(FATAL_ERROR
		"add_subdirectory registered development tests: ${simdlib_nested_tests}")
endif()
get_property(parent_tests DIRECTORY PROPERTY TESTS)
if(NOT parent_tests STREQUAL "ParentConsumer.OwnTest")
	message(FATAL_ERROR
		"SimdLib altered the parent CTest inventory: ${parent_tests}")
endif()

foreach(required_target IN ITEMS SimdLib SimdLib::SimdLib SimdLibRegister SimdLib::Register)
	if(NOT TARGET ${required_target})
		message(FATAL_ERROR "Required production target is missing: ${required_target}")
	endif()
endforeach()
foreach(forbidden_target IN ITEMS
	ExhaustiveArtifacts BenchmarkArtifacts Benchmarks DevelopmentWarnings
	ApiExamples RegisterExamples CoverageReset CoverageReport Catch2 Catch2WithMain)
	if(TARGET ${forbidden_target})
		message(FATAL_ERROR
			"add_subdirectory introduced development target ${forbidden_target}")
	endif()
endforeach()
