include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "PartialRegisterCodegen.cmake is available only to top-level SimdLib builds")
endif()

block(SCOPE_FOR VARIABLES)

# @brief Adds one wrapper/raw partial-register invariant-boundary generated-code comparison.
# @param register_width Native register width selected for the fixture.
# @param isa_profile ISA profile used to compile both fixture sides.
function(simdlib_add_partial_mask_codegen_gate register_width isa_profile)
	set(predicate_record_only OFF)
	set(predicate_difference_reason "")
	set(value_record_only OFF)
	set(value_difference_reason "")
	set(stack_protector_mode "default")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		set(predicate_record_only ON)
		set(predicate_difference_reason "msvc-gs-predicate-composition-cookie")
		set(value_record_only ON)
		set(value_difference_reason "msvc-equivalent-unaligned-move-and-division-register-allocation")
		set(stack_protector_mode "msvc-gs")
	endif()
	if(isa_profile STREQUAL "SSE42")
		set(target_suffix "${register_width}Sse42")
		set(artifact_profile sse42)
	else()
		set(target_suffix "${register_width}Avx2")
		set(artifact_profile avx2)
	endif()
	set(wrapper_target PartialRegisterMaskCodegenWrapper${target_suffix})
	set(raw_target PartialRegisterMaskCodegenRaw${target_suffix})
	add_library(${wrapper_target} OBJECT tests/codegen/PartialRegisterMaskCodegen.cpp)
	add_library(${raw_target} OBJECT tests/codegen/PartialRegisterMaskCodegenRaw.cpp)
	foreach(target IN ITEMS ${wrapper_target} ${raw_target})
		simdlib_register_development_target(${target} OPTIMIZED_CODEGEN)
		target_link_libraries(${target} PRIVATE SimdLib::Register)
		target_compile_definitions(${target} PRIVATE SIMDLIB_PARTIAL_MASK_CODEGEN_WIDTH=${register_width})
		simdlib_enable_development_warnings(${target})
		if(isa_profile STREQUAL "SSE42")
			simdlib_enable_register_sse42(${target})
		else()
			simdlib_enable_register_avx2(${target})
		endif()
	endforeach()
	set(artifact_directory "${CMAKE_CURRENT_BINARY_DIR}/partial-register-codegen/${artifact_profile}/${register_width}")
	set(record_file "${artifact_directory}/predicate-composition/comparison.record.json")
	set(value_record_file "${artifact_directory}/value-operations/comparison.record.json")
	add_custom_command(
		OUTPUT "${record_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/predicate-composition"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/predicate-composition
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DISA_PROFILE=${isa_profile}
			-DVECTORCALL_ENABLED=0
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DRECORD_ONLY=${predicate_record_only}
			-DRECORDED_DIFFERENCE_REASON=${predicate_difference_reason}
			-DCODEGEN_PROFILE=partial-register-invariant-boundaries
			"-DSYMBOL_PATTERN=simdlib_partial_mask_codegen_compose"
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS ${wrapper_target} ${raw_target} $<TARGET_OBJECTS:${wrapper_target}> $<TARGET_OBJECTS:${raw_target}> cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit partial-register invariant-boundary code"
		VERBATIM)
	add_custom_command(
		OUTPUT "${value_record_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/value-operations"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/value-operations
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DISA_PROFILE=${isa_profile}
			-DVECTORCALL_ENABLED=0
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DRECORD_ONLY=${value_record_only}
			-DRECORDED_DIFFERENCE_REASON=${value_difference_reason}
			-DCODEGEN_PROFILE=partial-register-value-operations
			"-DSYMBOL_PATTERN=simdlib_partial_register_codegen_(add|divide)"
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS ${wrapper_target} ${raw_target} $<TARGET_OBJECTS:${wrapper_target}> $<TARGET_OBJECTS:${raw_target}> cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit partial-register value-operation code"
		VERBATIM)
	add_custom_target(PartialRegisterMaskCodegen${target_suffix} DEPENDS "${record_file}" "${value_record_file}")
	add_dependencies(PartialRegisterMaskCodegen${target_suffix} ${wrapper_target} ${raw_target})
	simdlib_register_development_target(PartialRegisterMaskCodegen${target_suffix} OPTIMIZED_CODEGEN)
	add_test(NAME PartialRegisterMaskCodegen.${target_suffix}
		COMMAND ${CMAKE_COMMAND} -E compare_files "${record_file}" "${record_file}")
	set_tests_properties(PartialRegisterMaskCodegen.${target_suffix} PROPERTIES LABELS "PARTIAL_REGISTER;CODEGEN;${isa_profile}" RUN_SERIAL TRUE)
	simdlib_register_development_test(PartialRegisterMaskCodegen.${target_suffix} OPTIMIZED_CODEGEN)
endfunction()

# @brief Adds one strict wrapper/raw arithmetic and specialized-operation generated-code comparison.
# @param register_width Native register width selected for the fixture.
# @param isa_profile ISA profile used to compile both fixture sides.
function(simdlib_add_partial_arithmetic_codegen_gate register_width isa_profile)
	set(codegen_record_only OFF)
	set(codegen_difference_reason "")
	set(stack_protector_mode "default")
	if(isa_profile STREQUAL "SSE42")
		set(target_suffix "${register_width}Sse42")
		set(artifact_profile sse42)
		set(codegen_record_only ON)
		set(codegen_difference_reason "non-release-differential")
	else()
		set(target_suffix "${register_width}Avx2")
		set(artifact_profile avx2)
		if(SIMDLIB_REGISTER_CODEGEN_MODE STREQUAL "RECORD")
			set(codegen_record_only ON)
			set(codegen_difference_reason "non-release-differential")
		endif()
	endif()
	if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		set(codegen_record_only ON)
		set(codegen_difference_reason "msvc-equivalent-unaligned-moves-and-register-allocation")
		set(stack_protector_mode "msvc-gs")
	endif()
	set(wrapper_target PartialRegisterArithmeticCodegenWrapper${target_suffix})
	set(raw_target PartialRegisterArithmeticCodegenRaw${target_suffix})
	add_library(${wrapper_target} OBJECT tests/codegen/PartialRegisterArithmeticCodegen.cpp)
	add_library(${raw_target} OBJECT tests/codegen/PartialRegisterArithmeticCodegenRaw.cpp)
	foreach(target IN ITEMS ${wrapper_target} ${raw_target})
		simdlib_register_development_target(${target} OPTIMIZED_CODEGEN)
		target_link_libraries(${target} PRIVATE SimdLib::Register)
		target_compile_definitions(${target} PRIVATE SIMDLIB_PARTIAL_ARITHMETIC_CODEGEN_WIDTH=${register_width})
		simdlib_enable_development_warnings(${target})
		if(isa_profile STREQUAL "SSE42")
			simdlib_enable_register_sse42(${target})
		else()
			simdlib_enable_register_avx2(${target})
		endif()
	endforeach()
	set(artifact_directory "${CMAKE_CURRENT_BINARY_DIR}/partial-register-codegen/${artifact_profile}/${register_width}/arithmetic-specialized")
	set(record_file "${artifact_directory}/comparison.record.json")
	add_custom_command(
		OUTPUT "${record_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DISA_PROFILE=${isa_profile}
			-DVECTORCALL_ENABLED=0
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DRECORD_ONLY=${codegen_record_only}
			-DRECORDED_DIFFERENCE_REASON=${codegen_difference_reason}
			-DCODEGEN_PROFILE=partial-register-arithmetic-specialized
			"-DSYMBOL_PATTERN=simdlib_partial_arithmetic_codegen_"
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS ${wrapper_target} ${raw_target} $<TARGET_OBJECTS:${wrapper_target}> $<TARGET_OBJECTS:${raw_target}> cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit partial-register arithmetic and specialized-operation code"
		VERBATIM)
	add_custom_target(PartialRegisterArithmeticCodegen${target_suffix} DEPENDS "${record_file}")
	add_dependencies(PartialRegisterArithmeticCodegen${target_suffix} ${wrapper_target} ${raw_target})
	simdlib_register_development_target(PartialRegisterArithmeticCodegen${target_suffix} OPTIMIZED_CODEGEN)
	add_test(NAME PartialRegisterArithmeticCodegen.${target_suffix}
		COMMAND ${CMAKE_COMMAND} -E compare_files "${record_file}" "${record_file}")
	set_tests_properties(PartialRegisterArithmeticCodegen.${target_suffix} PROPERTIES LABELS "PARTIAL_REGISTER;CODEGEN;${isa_profile}" RUN_SERIAL TRUE)
	simdlib_register_development_test(PartialRegisterArithmeticCodegen.${target_suffix} OPTIMIZED_CODEGEN)
endfunction()

# @brief Adds one strict Api partial-transfer versus intrinsic-baseline codegen comparison.
# @param register_width Native register width selected for the fixture.
# @param isa_profile ISA profile used to compile both fixture sides.
function(simdlib_add_api_partial_transfer_codegen_gate register_width isa_profile)
	if(isa_profile STREQUAL "SSE42")
		set(target_suffix "${register_width}Sse42")
		set(artifact_profile sse42)
	else()
		set(target_suffix "${register_width}Avx2")
		set(artifact_profile avx2)
	endif()
	set(wrapper_target ApiPartialTransferCodegenWrapper${target_suffix})
	set(raw_target ApiPartialTransferCodegenRaw${target_suffix})
	add_library(${wrapper_target} OBJECT tests/codegen/ApiPartialTransferCodegen.cpp)
	add_library(${raw_target} OBJECT tests/codegen/ApiPartialTransferCodegenRaw.cpp)
	foreach(target IN ITEMS ${wrapper_target} ${raw_target})
		simdlib_register_development_target(${target} OPTIMIZED_CODEGEN)
		target_link_libraries(${target} PRIVATE SimdLib::SimdLib)
		target_compile_definitions(${target} PRIVATE SIMDLIB_API_PARTIAL_CODEGEN_WIDTH=${register_width})
		simdlib_enable_development_warnings(${target})
		if(isa_profile STREQUAL "SSE42")
			simdlib_enable_register_sse42(${target})
		else()
			simdlib_enable_register_avx2(${target})
		endif()
	endforeach()
	set(artifact_directory "${CMAKE_CURRENT_BINARY_DIR}/api-partial-transfer-codegen/${artifact_profile}/${register_width}")
	set(record_file "${artifact_directory}/comparison.record.json")
	add_custom_command(
		OUTPUT "${record_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DISA_PROFILE=${isa_profile}
			-DVECTORCALL_ENABLED=0
			-DSTACK_PROTECTOR_MODE=default
			-DRECORD_ONLY=OFF
			-DCODEGEN_PROFILE=api-partial-transfer
			"-DSYMBOL_PATTERN=simdlib_api_partial_codegen_"
			"-DEXCLUDE_SYMBOL_PATTERN=simdlib_api_partial_codegen_to_array"
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS ${wrapper_target} ${raw_target} $<TARGET_OBJECTS:${wrapper_target}> $<TARGET_OBJECTS:${raw_target}> cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit Api partial-transfer code against intrinsic baselines"
		VERBATIM)
	add_custom_target(ApiPartialTransferCodegen${target_suffix} DEPENDS "${record_file}")
	add_dependencies(ApiPartialTransferCodegen${target_suffix} ${wrapper_target} ${raw_target})
	simdlib_register_development_target(ApiPartialTransferCodegen${target_suffix} OPTIMIZED_CODEGEN)
	add_test(NAME ApiPartialTransferCodegen.${target_suffix}
		COMMAND ${CMAKE_COMMAND} -E compare_files "${record_file}" "${record_file}")
	set_tests_properties(ApiPartialTransferCodegen.${target_suffix} PROPERTIES LABELS "API;PARTIAL_TRANSFER;CODEGEN;${isa_profile}" RUN_SERIAL TRUE)
	simdlib_register_development_test(ApiPartialTransferCodegen.${target_suffix} OPTIMIZED_CODEGEN)
endfunction()

if(SIMDLIB_BUILD_REGISTER_CODEGEN_GATES AND SIMDLIB_REGISTER_COMPILER_SUPPORTED)
	if(NOT CMAKE_OBJDUMP)
		find_program(CMAKE_OBJDUMP NAMES llvm-objdump llvm-objdump.exe)
	endif()
	if(NOT CMAKE_OBJDUMP)
		message(FATAL_ERROR "PartialRegisterMask codegen gates require llvm-objdump")
	endif()
	simdlib_add_partial_mask_codegen_gate(128 SSE42)
	simdlib_add_partial_mask_codegen_gate(256 AVX2)
	simdlib_add_partial_arithmetic_codegen_gate(128 SSE42)
	simdlib_add_partial_arithmetic_codegen_gate(256 AVX2)
	simdlib_add_api_partial_transfer_codegen_gate(128 SSE42)
	simdlib_add_api_partial_transfer_codegen_gate(256 AVX2)
	add_custom_target(PartialRegisterMaskCodegen DEPENDS PartialRegisterMaskCodegen128Sse42 PartialRegisterMaskCodegen256Avx2)
	simdlib_register_development_target(PartialRegisterMaskCodegen OPTIMIZED_CODEGEN)
	add_dependencies(RegisterCodegen PartialRegisterMaskCodegen)
	add_custom_target(PartialRegisterArithmeticCodegen DEPENDS PartialRegisterArithmeticCodegen128Sse42 PartialRegisterArithmeticCodegen256Avx2)
	simdlib_register_development_target(PartialRegisterArithmeticCodegen OPTIMIZED_CODEGEN)
	add_dependencies(RegisterCodegen PartialRegisterArithmeticCodegen)
	add_custom_target(ApiPartialTransferCodegen DEPENDS ApiPartialTransferCodegen128Sse42 ApiPartialTransferCodegen256Avx2)
	simdlib_register_development_target(ApiPartialTransferCodegen OPTIMIZED_CODEGEN)
	add_dependencies(RegisterCodegen ApiPartialTransferCodegen)
endif()

endblock()
