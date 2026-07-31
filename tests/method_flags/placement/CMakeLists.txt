cmake_minimum_required(VERSION 4.4)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	project(SimdLibMethodFlagsPlacement LANGUAGES CXX)
	set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
	include(CTest)
	set(SIMDLIB_METHOD_FLAGS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
	add_library(SimdLibMethodFlagsHeaders INTERFACE)
	target_include_directories(SimdLibMethodFlagsHeaders INTERFACE
		"${SIMDLIB_METHOD_FLAGS_ROOT}/include"
		"${SIMDLIB_METHOD_FLAGS_ROOT}/tests/method_flags")
	set(simdlib_method_flags_dependency SimdLibMethodFlagsHeaders)
	set(simdlib_method_flags_enable_cxx23 ON)
else()
	set(SIMDLIB_METHOD_FLAGS_ROOT "${CMAKE_SOURCE_DIR}")
	set(simdlib_method_flags_dependency SimdLib::SimdLib)
	set(simdlib_method_flags_enable_cxx23 ${SIMDLIB_REGISTER_COMPILER_SUPPORTED})
endif()

# Adds strict warnings to one compiler-placement target.
function(simdlib_configure_method_flags_target target)
	if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
		target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
	else()
		target_compile_options(${target} PRIVATE
			-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror)
	endif()
endfunction()

add_library(MethodFlagsPlacementCxx20 OBJECT MethodFlagsPlacementCxx20.cpp)
if(COMMAND simdlib_register_development_target)
	simdlib_register_development_target(MethodFlagsPlacementCxx20
		COMPILER_CONTRACT)
endif()
target_link_libraries(MethodFlagsPlacementCxx20 PRIVATE
	${simdlib_method_flags_dependency})
target_compile_features(MethodFlagsPlacementCxx20 PRIVATE cxx_std_20)
set_target_properties(MethodFlagsPlacementCxx20 PROPERTIES
	CXX_EXTENSIONS OFF)
simdlib_configure_method_flags_target(MethodFlagsPlacementCxx20)

if(simdlib_method_flags_enable_cxx23)
	add_library(MethodFlagsPlacementCxx23 OBJECT MethodFlagsPlacementCxx23.cpp)
	if(COMMAND simdlib_register_development_target)
		simdlib_register_development_target(MethodFlagsPlacementCxx23
			COMPILER_CONTRACT)
	endif()
	target_link_libraries(MethodFlagsPlacementCxx23 PRIVATE
		${simdlib_method_flags_dependency})
	target_compile_features(MethodFlagsPlacementCxx23 PRIVATE cxx_std_23)
	set_target_properties(MethodFlagsPlacementCxx23 PROPERTIES
		CXX_EXTENSIONS OFF)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		target_compile_options(MethodFlagsPlacementCxx23 PRIVATE /std:c++latest)
	endif()
	simdlib_configure_method_flags_target(MethodFlagsPlacementCxx23)
endif()

add_executable(MethodFlagsPlacementAbi
	MethodFlagsPlacementAbiDefinition.cpp
	MethodFlagsPlacementAbiConsumer.cpp)
if(COMMAND simdlib_register_development_target)
	simdlib_register_development_target(MethodFlagsPlacementAbi
		COMPILER_CONTRACT)
endif()
target_link_libraries(MethodFlagsPlacementAbi PRIVATE
	${simdlib_method_flags_dependency})
target_compile_features(MethodFlagsPlacementAbi PRIVATE cxx_std_20)
set_target_properties(MethodFlagsPlacementAbi PROPERTIES
	CXX_EXTENSIONS OFF)
simdlib_configure_method_flags_target(MethodFlagsPlacementAbi)

add_custom_target(MethodFlagsPlacement)
if(COMMAND simdlib_register_development_target)
	simdlib_register_development_target(MethodFlagsPlacement COMPILER_CONTRACT)
endif()
add_dependencies(MethodFlagsPlacement
	MethodFlagsPlacementCxx20
	MethodFlagsPlacementAbi)
if(TARGET MethodFlagsPlacementCxx23)
	add_dependencies(MethodFlagsPlacement MethodFlagsPlacementCxx23)
endif()

if(BUILD_TESTING)
	add_test(NAME MethodFlagsPlacementAbi
		COMMAND MethodFlagsPlacementAbi)
	set_tests_properties(MethodFlagsPlacementAbi PROPERTIES
		LABELS "CONFIGURATION;METHOD_FLAGS;ABI")
	simdlib_register_development_test(MethodFlagsPlacementAbi COMPILER_CONTRACT)
endif()
