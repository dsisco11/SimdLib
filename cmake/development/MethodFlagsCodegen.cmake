include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
	message(FATAL_ERROR "MethodFlagsCodegen.cmake is available only to top-level SimdLib builds")
endif()
if(NOT TARGET SimdLib)
	message(FATAL_ERROR "MethodFlagsCodegen.cmake requires the production SimdLib target")
endif()

block(SCOPE_FOR VARIABLES)

if(SIMDLIB_BUILD_CONFIGURATION_PROBES
	AND SIMDLIB_BUILD_METHOD_FLAGS_CODEGEN_GATES
	AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|amd64|x86_64|i[3-6]86)$")
	if(NOT CMAKE_OBJDUMP)
		find_program(CMAKE_OBJDUMP NAMES llvm-objdump llvm-objdump.exe objdump)
	endif()
	if(NOT CMAKE_OBJDUMP)
		message(FATAL_ERROR "Method-flags generated-code gates require an objdump-compatible disassembler")
	endif()

	add_library(MethodFlagsCodegenRaw OBJECT
		tests/method_flags/codegen/MethodFlagsRaw.cpp)
	add_library(MethodFlagsCodegenFlagged OBJECT
		tests/method_flags/codegen/MethodFlagsFlagged.cpp)
	foreach(method_flags_target IN ITEMS MethodFlagsCodegenRaw MethodFlagsCodegenFlagged)
		simdlib_register_development_target(${method_flags_target}
			OPTIMIZED_CODEGEN)
		target_link_libraries(${method_flags_target} PRIVATE SimdLib::SimdLib)
		simdlib_enable_development_warnings(${method_flags_target})
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			set_property(TARGET ${method_flags_target} PROPERTY MSVC_RUNTIME_CHECKS "")
			target_compile_options(${method_flags_target} PRIVATE /O2 /Ob2 /GS)
		else()
			target_compile_options(${method_flags_target} PRIVATE
				-O2 -msse4.2 -fstack-protector-strong)
		endif()
	endforeach()

	set(method_flags_vectorcall_enabled 0)
	if(WIN32 AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"
		OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
		set(method_flags_vectorcall_enabled 1)
	endif()
	if(SIMDLIB_MSVC_STYLE_DRIVER)
		set(method_flags_stack_protector_mode "msvc-gs")
	else()
		set(method_flags_stack_protector_mode "strong")
	endif()

	set(method_flags_artifact_directory
		"${CMAKE_CURRENT_BINARY_DIR}/method-flags-codegen")
	set(method_flags_record
		"${method_flags_artifact_directory}/comparison.record.json")
	set(method_flags_verification
		"${method_flags_artifact_directory}/verification.txt")
	add_custom_command(
		OUTPUT "${method_flags_record}" "${method_flags_verification}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${method_flags_artifact_directory}"
		COMMAND ${CMAKE_COMMAND} -E rm -f "${method_flags_verification}"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:MethodFlagsCodegenFlagged>
			-DRAW_OBJECT=$<TARGET_OBJECTS:MethodFlagsCodegenRaw>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${method_flags_artifact_directory}
			-DRECORD_FILE=${method_flags_record}
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=128
			-DISA_PROFILE=SSE42
			-DVECTORCALL_ENABLED=${method_flags_vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${method_flags_stack_protector_mode}
			-DCODEGEN_PROFILE=method-flags
			-DSYMBOL_PATTERN=simdlib_method_flags_codegen_
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		COMMAND ${CMAKE_COMMAND}
			-DFLAGGED_OBJECT=$<TARGET_OBJECTS:MethodFlagsCodegenFlagged>
			-DRAW_OBJECT=$<TARGET_OBJECTS:MethodFlagsCodegenRaw>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DSTACK_PROTECTOR_MODE=${method_flags_stack_protector_mode}
			-DOUTPUT_FILE=${method_flags_verification}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyMethodFlagsCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:MethodFlagsCodegenFlagged>
			$<TARGET_OBJECTS:MethodFlagsCodegenRaw>
			cmake/CompareRegisterCodegen.cmake
			cmake/VerifyMethodFlagsCodegen.cmake
		COMMENT "Verifying method-flags generated code and stack contract"
		VERBATIM)
	add_custom_target(MethodFlagsCodegen ALL
		DEPENDS "${method_flags_record}" "${method_flags_verification}")
	simdlib_register_development_target(MethodFlagsCodegen OPTIMIZED_CODEGEN)
	add_dependencies(MethodFlagsCodegen
		MethodFlagsCodegenRaw
		MethodFlagsCodegenFlagged)

	set(method_flags_record_index
		"${method_flags_artifact_directory}/all-records.txt")
	file(GENERATE OUTPUT "${method_flags_record_index}"
		CONTENT "${method_flags_record}\n")
	add_test(NAME MethodFlagsCodegen
		COMMAND ${CMAKE_COMMAND}
			-DRECORD_INDEX=${method_flags_record_index}
			-DVERIFICATION_FILE=${method_flags_verification}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyMethodFlagsCodegenRecords.cmake)
	set_tests_properties(MethodFlagsCodegen PROPERTIES
		LABELS "CONFIGURATION;METHOD_FLAGS;CODEGEN;ABI;STACK")
	simdlib_register_development_test(MethodFlagsCodegen OPTIMIZED_CODEGEN)
endif()

endblock()
