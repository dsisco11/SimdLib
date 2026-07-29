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

# @brief Requires the source-contract audit to reject one prohibited form.
# @param probe_name Stable name used for the audit log.
# @param source_name Source file containing the prohibited declaration.
# @param expected_diagnostic Stable violation token required from the audit.
function(simdlib_expect_method_flags_audit_failure probe_name source_name expected_diagnostic)
	execute_process(
		COMMAND "${CMAKE_COMMAND}"
			"-DSOURCE_FILE=${CMAKE_CURRENT_LIST_DIR}/${source_name}"
			-P "${SIMDLIB_METHOD_FLAGS_ROOT}/cmake/VerifyMethodFlagsPlacementSource.cmake"
		RESULT_VARIABLE audit_result
		OUTPUT_VARIABLE audit_stdout
		ERROR_VARIABLE audit_stderr)
	file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${probe_name}.log"
		"${audit_stdout}${audit_stderr}")
	if(audit_result EQUAL 0)
		message(FATAL_ERROR "${probe_name} unexpectedly passed the source audit")
	endif()
	if(NOT "${audit_stdout}${audit_stderr}" MATCHES "${expected_diagnostic}")
		message(FATAL_ERROR
			"${probe_name} did not emit ${expected_diagnostic}; see ${CMAKE_CURRENT_BINARY_DIR}/${probe_name}.log")
	endif()
endfunction()

# @brief Requires one supported fixture to pass the declaration source audit.
# @param source_name Supported source file to audit.
function(simdlib_require_method_flags_audit_success source_name)
	execute_process(
		COMMAND "${CMAKE_COMMAND}"
			"-DSOURCE_FILE=${CMAKE_CURRENT_LIST_DIR}/${source_name}"
			-P "${SIMDLIB_METHOD_FLAGS_ROOT}/cmake/VerifyMethodFlagsPlacementSource.cmake"
		RESULT_VARIABLE audit_result
		OUTPUT_VARIABLE audit_stdout
		ERROR_VARIABLE audit_stderr)
	if(NOT audit_result EQUAL 0)
		message(FATAL_ERROR
			"${source_name} failed the method-flags source audit:\n${audit_stdout}${audit_stderr}")
	endif()
endfunction()

simdlib_expect_method_flags_audit_failure(
	MethodFlagsInvalidConstructor InvalidConstructor.cpp
	SIMDLIB_METHOD_FLAGS_PROHIBITED_CONSTRUCTOR)
simdlib_expect_method_flags_audit_failure(
	MethodFlagsInvalidConversionOperator InvalidConversionOperator.cpp
	SIMDLIB_METHOD_FLAGS_PROHIBITED_CONVERSION)
simdlib_expect_method_flags_audit_failure(
	MethodFlagsInvalidLambda InvalidLambda.cpp
	SIMDLIB_METHOD_FLAGS_PROHIBITED_LAMBDA)
simdlib_expect_method_flags_audit_failure(
	MethodFlagsInvalidConsteval InvalidConsteval.cpp
	SIMDLIB_METHOD_FLAGS_PROHIBITED_CONSTEVAL)
simdlib_expect_method_flags_audit_failure(
	MethodFlagsInvalidFunctionPointer InvalidFunctionPointer.cpp
	SIMDLIB_METHOD_FLAGS_PROHIBITED_FUNCTION_POINTER)
simdlib_expect_method_flags_audit_failure(
	MethodFlagsInvalidDestructor InvalidDestructor.cpp
	SIMDLIB_METHOD_FLAGS_PROHIBITED_DESTRUCTOR)
simdlib_expect_method_flags_audit_failure(
	MethodFlagsInvalidDeductionGuide InvalidDeductionGuide.cpp
	SIMDLIB_METHOD_FLAGS_PROHIBITED_DEDUCTION_GUIDE)
simdlib_expect_method_flags_audit_failure(
	MethodFlagsInvalidVirtual InvalidVirtual.cpp
	SIMDLIB_METHOD_FLAGS_PROHIBITED_VIRTUAL)
simdlib_expect_method_flags_audit_failure(
	MethodFlagsInvalidExternC InvalidExternC.cpp
	SIMDLIB_METHOD_FLAGS_PROHIBITED_EXTERN_C)
simdlib_expect_method_flags_audit_failure(
	MethodFlagsInvalidVariadic InvalidVariadic.cpp
	SIMDLIB_METHOD_FLAGS_PROHIBITED_VARIADIC)
simdlib_expect_method_flags_audit_failure(
	MethodFlagsInvalidAllocation InvalidAllocation.cpp
	SIMDLIB_METHOD_FLAGS_PROHIBITED_ALLOCATION)
simdlib_expect_method_flags_audit_failure(
	MethodFlagsInvalidDefaulted InvalidDefaulted.cpp
	SIMDLIB_METHOD_FLAGS_PROHIBITED_DEFAULTED_OR_DELETED)
simdlib_expect_method_flags_audit_failure(
	MethodFlagsInvalidCoroutine InvalidCoroutine.cpp
	SIMDLIB_METHOD_FLAGS_PROHIBITED_COROUTINE)
simdlib_require_method_flags_audit_success(MethodFlagsPlacementFixture.h)
simdlib_require_method_flags_audit_success(MethodFlagsPlacementCxx20.cpp)
simdlib_require_method_flags_audit_success(MethodFlagsPlacementCxx23.cpp)
simdlib_require_method_flags_audit_success(MethodFlagsPlacementAbiDefinition.cpp)
simdlib_require_method_flags_audit_success(MethodFlagsPlacementAbiConsumer.cpp)

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
endif()
