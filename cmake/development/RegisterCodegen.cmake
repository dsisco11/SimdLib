include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "RegisterCodegen.cmake is available only to top-level SimdLib builds")
endif()
if(NOT TARGET SimdLib OR NOT TARGET SimdLibRegister)
    message(FATAL_ERROR "RegisterCodegen.cmake requires the production SimdLib targets")
endif()

block(SCOPE_FOR VARIABLES)

# @brief Adds paired wrapper/raw object fixtures and a mandatory disassembly comparison.
# @param register_width Width of the compared native and wrapped register values.
# @param isa_profile Instruction-set profile used to compile both sides of the comparison.
function(simdlib_add_register_codegen_gate register_width isa_profile)
	if(SIMDLIB_REGISTER_CODEGEN_MODE STREQUAL "ENFORCE")
		set(codegen_validation_category OPTIMIZED_CODEGEN)
	else()
		set(codegen_validation_category DEBUG_DIAGNOSTIC)
	endif()
	if(NOT isa_profile STREQUAL "SSE42" AND NOT isa_profile STREQUAL "AVX2")
		message(FATAL_ERROR "Unsupported Register codegen ISA profile: ${isa_profile}")
	endif()
	if(isa_profile STREQUAL "SSE42" AND NOT register_width EQUAL 128)
		message(FATAL_ERROR "The SSE4.2 Register codegen profile supports only 128-bit registers")
	endif()
	if(isa_profile STREQUAL "SSE42")
		set(target_suffix "${register_width}Sse42")
		set(artifact_profile "sse42")
		set(codegen_comparison_record_only ON)
	else()
		set(target_suffix "${register_width}Avx2")
		set(artifact_profile "avx2")
		if(SIMDLIB_REGISTER_CODEGEN_MODE STREQUAL "RECORD")
			set(codegen_comparison_record_only ON)
		else()
			set(codegen_comparison_record_only OFF)
		endif()
	endif()
	set(composition_record_only ${codegen_comparison_record_only})
	set(composition_difference_reason "non-release-differential")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		set(composition_record_only ON)
		set(composition_difference_reason "msvc-gs-composition-cookie")
	endif()
	set(modulus_record_only ${codegen_comparison_record_only})
	set(modulus_difference_reason "non-release-differential")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND isa_profile STREQUAL "AVX2" AND register_width EQUAL 256)
		set(modulus_record_only ON)
		set(modulus_difference_reason "msvc-scalar-remainder-scheduling")
	endif()
	set(vectorcall_enabled 0)
	set(stack_protector_mode "compiler-default")
	if(WIN32 AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|amd64|x86_64|i[3-6]86)$" AND
		(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
		set(vectorcall_enabled 1)
	endif()
	if(NOT SIMDLIB_MSVC_STYLE_DRIVER AND
		(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
		set(stack_protector_mode "strong")
	endif()
	set(wrapper_target RegisterCodegenWrapper${target_suffix})
	set(raw_target RegisterCodegenRaw${target_suffix})
	set(default_wrapper_target RegisterDefaultAbiWrapper${target_suffix})
	set(default_raw_target RegisterDefaultAbiRaw${target_suffix})
	set(abi_wrapper_target RegisterAbiWrapper${target_suffix})
	set(abi_raw_target RegisterAbiRaw${target_suffix})
	set(specialized_wrapper_target RegisterSpecializedWrapper${target_suffix})
	set(specialized_raw_target RegisterSpecializedRaw${target_suffix})
	set(fma_enabled_wrapper_target RegisterFmaEnabledWrapper${target_suffix})
	set(fma_enabled_raw_target RegisterFmaEnabledRaw${target_suffix})
	set(fma_disabled_wrapper_target RegisterFmaDisabledWrapper${target_suffix})
	set(fma_disabled_raw_target RegisterFmaDisabledRaw${target_suffix})
	set(rearrangement_wrapper_target RegisterRearrangementWrapper${target_suffix})
	set(rearrangement_raw_target RegisterRearrangementRaw${target_suffix})
	set(type_matrix_wrapper_target RegisterTypeMatrixWrapper${target_suffix})
	set(type_matrix_raw_target RegisterTypeMatrixRaw${target_suffix})
	add_library(${wrapper_target} OBJECT tests/codegen/RegisterCodegen.cpp)
	add_library(${raw_target} OBJECT tests/codegen/RegisterCodegenRaw.cpp)
	add_library(${default_wrapper_target} OBJECT tests/codegen/RegisterDefaultAbi.cpp)
	add_library(${default_raw_target} OBJECT tests/codegen/RegisterDefaultAbiRaw.cpp)
	add_library(${abi_wrapper_target} OBJECT tests/codegen/RegisterAbi.cpp)
	add_library(${abi_raw_target} OBJECT tests/codegen/RegisterAbiRaw.cpp)
	add_library(${specialized_wrapper_target} OBJECT tests/codegen/RegisterSpecializedCodegen.cpp)
	add_library(${specialized_raw_target} OBJECT tests/codegen/RegisterSpecializedCodegenRaw.cpp)
	if(isa_profile STREQUAL "AVX2")
		add_library(${fma_enabled_wrapper_target} OBJECT tests/codegen/RegisterFmaCodegen.cpp)
		add_library(${fma_enabled_raw_target} OBJECT tests/codegen/RegisterFmaCodegenRaw.cpp)
	endif()
	add_library(${fma_disabled_wrapper_target} OBJECT tests/codegen/RegisterFmaCodegen.cpp)
	add_library(${fma_disabled_raw_target} OBJECT tests/codegen/RegisterFmaCodegenRaw.cpp)
	add_library(${rearrangement_wrapper_target} OBJECT tests/codegen/RegisterRearrangementCodegen.cpp)
	add_library(${rearrangement_raw_target} OBJECT tests/codegen/RegisterRearrangementCodegenRaw.cpp)
	add_library(${type_matrix_wrapper_target} OBJECT tests/codegen/RegisterTypeMatrixCodegen.cpp)
	add_library(${type_matrix_raw_target} OBJECT tests/codegen/RegisterTypeMatrixCodegenRaw.cpp)
	set(codegen_object_targets
		${wrapper_target} ${raw_target} ${default_wrapper_target} ${default_raw_target}
		${abi_wrapper_target} ${abi_raw_target}
		${specialized_wrapper_target} ${specialized_raw_target}
		${fma_disabled_wrapper_target} ${fma_disabled_raw_target}
		${rearrangement_wrapper_target} ${rearrangement_raw_target}
		${type_matrix_wrapper_target} ${type_matrix_raw_target})
	if(isa_profile STREQUAL "AVX2")
		list(APPEND codegen_object_targets
			${fma_enabled_wrapper_target} ${fma_enabled_raw_target})
	endif()
	foreach(target IN LISTS codegen_object_targets)
		simdlib_register_development_target(${target}
			${codegen_validation_category})
		target_link_libraries(${target} PRIVATE SimdLib::Register)
		target_compile_definitions(${target} PRIVATE SIMDLIB_REGISTER_TEST_WIDTH=${register_width})
		simdlib_enable_development_warnings(${target})
		if(isa_profile STREQUAL "SSE42")
			simdlib_enable_register_sse42(${target})
		else()
			simdlib_enable_register_avx2(${target})
		endif()
		if(NOT SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_options(${target} PRIVATE -fstack-protector-strong)
		endif()
		if(SIMDLIB_REGISTER_CODEGEN_MODE STREQUAL "ENFORCE")
			if(SIMDLIB_MSVC_STYLE_DRIVER)
				target_compile_options(${target} PRIVATE /O2)
			else()
				target_compile_options(${target} PRIVATE -O2)
			endif()
		endif()
	endforeach()
	if(isa_profile STREQUAL "AVX2")
		foreach(target IN ITEMS ${fma_enabled_wrapper_target} ${fma_enabled_raw_target})
			target_compile_definitions(${target} PRIVATE SIMDLIB_HAS_FMA=1)
			if(NOT SIMDLIB_MSVC_STYLE_DRIVER)
				target_compile_options(${target} PRIVATE -mfma)
			endif()
		endforeach()
	endif()
	foreach(target IN ITEMS ${fma_disabled_wrapper_target} ${fma_disabled_raw_target})
		target_compile_definitions(${target} PRIVATE SIMDLIB_HAS_FMA=0)
		if(NOT SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_options(${target} PRIVATE -mno-fma)
		endif()
	endforeach()

	set(artifact_directory "${CMAKE_CURRENT_BINARY_DIR}/register-codegen/${artifact_profile}/${register_width}")
	set(composition_stamp_file "${artifact_directory}/primary-composition/comparison.record.json")
	set(register_only_stamp_file "${artifact_directory}/register-only/comparison.record.json")
	set(reassignment_stamp_file "${artifact_directory}/reassignment/comparison.record.json")
	set(default_abi_stamp_file "${artifact_directory}/default-abi.record.json")
	set(abi_stamp_file "${artifact_directory}/abi/comparison.record.json")
	set(consumer_abi_stamp_file "${artifact_directory}/consumer-abi/comparison.record.json")
	set(specialized_stamp_file "${artifact_directory}/specialized/comparison.record.json")
	set(fma_enabled_stamp_file "${artifact_directory}/fma/enabled/comparison.record.json")
	set(fma_disabled_stamp_file "${artifact_directory}/fma/disabled/comparison.record.json")
	set(rearrangement_stamp_file "${artifact_directory}/rearrangement-conversion/comparison.record.json")
	set(type_matrix_stamp_file "${artifact_directory}/type-matrix/common/comparison.record.json")
	set(type_matrix_modulus_stamp_file "${artifact_directory}/type-matrix/modulus/comparison.record.json")
	add_custom_command(
		OUTPUT "${composition_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/primary-composition"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/primary-composition
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DISA_PROFILE=${isa_profile}
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DRECORD_ONLY=${composition_record_only}
			-DRECORDED_DIFFERENCE_REASON=${composition_difference_reason}
			-DCODEGEN_PROFILE=primary-composition
			"-DSYMBOL_PATTERN=simdlib_codegen_(load_operate_store|aligned_transfer|byte_transfer|mutate|complete_shift_static|complete_shift_runtime|complete_byte_shift|opaque)"
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${wrapper_target}>
			$<TARGET_OBJECTS:${raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit composed and memory-capable Register code"
		VERBATIM)
	add_custom_command(
		OUTPUT "${register_only_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/register-only"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/register-only
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DISA_PROFILE=${isa_profile}
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DRECORD_ONLY=${codegen_comparison_record_only}
			"-DSYMBOL_PATTERN=simdlib_codegen_(ternary|mask_combine|mask_select|mask_bits|mask_any|mask_all|native|broadcast_reuse|lane_last|special_members|pressure|basic_bitwise|basic_broadcast_chain|basic_shift_left_immediate)"
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${wrapper_target}>
			$<TARGET_OBJECTS:${raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit register-only wrapper and raw generated code"
		VERBATIM)
	add_custom_command(
		OUTPUT "${specialized_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/specialized"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${specialized_wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${specialized_raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/specialized
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DISA_PROFILE=${isa_profile}
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DRECORD_ONLY=${codegen_comparison_record_only}
			-DCODEGEN_PROFILE=specialized
			-DSYMBOL_PATTERN=simdlib_specialized_codegen_
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${specialized_wrapper_target}>
			$<TARGET_OBJECTS:${specialized_raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit specialized Register code"
		VERBATIM)
	if(isa_profile STREQUAL "AVX2")
		add_custom_command(
			OUTPUT "${fma_enabled_stamp_file}"
			COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/fma/enabled"
			COMMAND ${CMAKE_COMMAND}
				-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${fma_enabled_wrapper_target}>
				-DRAW_OBJECT=$<TARGET_OBJECTS:${fma_enabled_raw_target}>
				-DOBJDUMP=${CMAKE_OBJDUMP}
				-DARTIFACT_DIRECTORY=${artifact_directory}/fma/enabled
				-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
				-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
				-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
				-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
				-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
				-DCONFIGURATION=$<CONFIG>
				-DREGISTER_WIDTH=${register_width}
				-DISA_PROFILE=${isa_profile}
				-DVECTORCALL_ENABLED=${vectorcall_enabled}
				-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
				-DRECORD_ONLY=${codegen_comparison_record_only}
				-DCODEGEN_PROFILE=fma-enabled
				-DFMA_EXPECTATION=enabled
				"-DSYMBOL_PATTERN=simdlib_fma_codegen_multiply_add_(f32|f64)"
				-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
			DEPENDS
				$<TARGET_OBJECTS:${fma_enabled_wrapper_target}>
				$<TARGET_OBJECTS:${fma_enabled_raw_target}>
				cmake/CompareRegisterCodegen.cmake
			COMMENT "Comparing ${register_width}-bit isolated Register multiply-add code with FMA enabled"
			VERBATIM)
	endif()
	add_custom_command(
		OUTPUT "${fma_disabled_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/fma/disabled"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${fma_disabled_wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${fma_disabled_raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/fma/disabled
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DISA_PROFILE=${isa_profile}
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DRECORD_ONLY=${codegen_comparison_record_only}
			-DCODEGEN_PROFILE=fma-disabled
			-DFMA_EXPECTATION=disabled
			"-DSYMBOL_PATTERN=simdlib_fma_codegen_multiply_add_(f32|f64)"
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${fma_disabled_wrapper_target}>
			$<TARGET_OBJECTS:${fma_disabled_raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit isolated Register multiply-add code with FMA disabled"
		VERBATIM)
	add_custom_command(
		OUTPUT "${rearrangement_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/rearrangement-conversion"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${rearrangement_wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${rearrangement_raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/rearrangement-conversion
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DISA_PROFILE=${isa_profile}
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DRECORD_ONLY=${codegen_comparison_record_only}
			-DCODEGEN_PROFILE=rearrangement-conversion
			-DSYMBOL_PATTERN=simdlib_rearrangement_codegen_
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${rearrangement_wrapper_target}>
			$<TARGET_OBJECTS:${rearrangement_raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit rearrangement and conversion wrapper and raw generated code"
		VERBATIM)
	add_custom_command(
		OUTPUT "${type_matrix_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/type-matrix/common"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${type_matrix_wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${type_matrix_raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/type-matrix/common
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DISA_PROFILE=${isa_profile}
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DRECORD_ONLY=${codegen_comparison_record_only}
			-DCODEGEN_PROFILE=common-type-matrix
			-DSYMBOL_PATTERN=simdlib_type_matrix_
			-DEXCLUDE_SYMBOL_PATTERN=simdlib_type_matrix_modulus_
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${type_matrix_wrapper_target}>
			$<TARGET_OBJECTS:${type_matrix_raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit common non-modulus operations across every Register element type"
		VERBATIM)
	add_custom_command(
		OUTPUT "${type_matrix_modulus_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/type-matrix/modulus"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${type_matrix_wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${type_matrix_raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/type-matrix/modulus
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DISA_PROFILE=${isa_profile}
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DRECORD_ONLY=${modulus_record_only}
			-DRECORDED_DIFFERENCE_REASON=${modulus_difference_reason}
			-DCODEGEN_PROFILE=modulus-type-matrix
			-DSYMBOL_PATTERN=simdlib_type_matrix_modulus_
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${type_matrix_wrapper_target}>
			$<TARGET_OBJECTS:${type_matrix_raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit modulus operations across every integer Register element type"
		VERBATIM)
	add_custom_command(
		OUTPUT "${reassignment_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/reassignment"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/reassignment
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DISA_PROFILE=${isa_profile}
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DRECORD_ONLY=${codegen_comparison_record_only}
			-DSYMBOL_PATTERN=simdlib_codegen_reassignment_arithmetic
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${wrapper_target}>
			$<TARGET_OBJECTS:${raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit reassignment wrapper and raw generated code"
		VERBATIM)
	add_custom_command(
		OUTPUT "${abi_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/abi"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${abi_wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${abi_raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/abi
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DISA_PROFILE=${isa_profile}
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DRECORD_ONLY=${codegen_comparison_record_only}
			-DSYMBOL_PATTERN=simdlib_abi_
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${abi_wrapper_target}>
			$<TARGET_OBJECTS:${abi_raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit explicit-object and raw ABI mirrors"
		VERBATIM)
	add_custom_command(
		OUTPUT "${default_abi_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${default_wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${default_raw_target}>
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
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RecordRegisterDefaultAbi.cmake
		DEPENDS
			$<TARGET_OBJECTS:${default_wrapper_target}>
			$<TARGET_OBJECTS:${default_raw_target}>
			cmake/RecordRegisterDefaultAbi.cmake
		COMMENT "Recording ${register_width}-bit platform-default Register ABI"
		VERBATIM)
	add_custom_command(
		OUTPUT "${consumer_abi_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/consumer-abi"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${abi_wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${abi_raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/consumer-abi
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DISA_PROFILE=${isa_profile}
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DRECORD_ONLY=${codegen_comparison_record_only}
			-DSYMBOL_PATTERN=simdlib_consumer_abi_
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${abi_wrapper_target}>
			$<TARGET_OBJECTS:${abi_raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit downstream Register wrappers and raw ABI boundaries"
		VERBATIM)
	set(expression_codegen_gate_outputs
		"${composition_stamp_file}" "${register_only_stamp_file}" "${reassignment_stamp_file}"
		"${specialized_stamp_file}" "${fma_disabled_stamp_file}"
		"${rearrangement_stamp_file}" "${type_matrix_stamp_file}" "${type_matrix_modulus_stamp_file}")
	if(isa_profile STREQUAL "AVX2")
		list(APPEND expression_codegen_gate_outputs "${fma_enabled_stamp_file}")
	endif()
	add_custom_target(RegisterExpressionCodegen${target_suffix}
		DEPENDS ${expression_codegen_gate_outputs})
	simdlib_register_development_target(
		RegisterExpressionCodegen${target_suffix}
		${codegen_validation_category})
	add_dependencies(RegisterExpressionCodegen${target_suffix} ${codegen_object_targets})
	add_custom_target(RegisterConsumerAbi${target_suffix}
		DEPENDS "${consumer_abi_stamp_file}")
	simdlib_register_development_target(RegisterConsumerAbi${target_suffix}
		${codegen_validation_category})
	add_dependencies(RegisterConsumerAbi${target_suffix}
		${abi_wrapper_target} ${abi_raw_target})
	set(codegen_gate_outputs
		${expression_codegen_gate_outputs} "${consumer_abi_stamp_file}" "${abi_stamp_file}" "${default_abi_stamp_file}")
	add_custom_target(RegisterCodegen${target_suffix} ALL
		DEPENDS "${abi_stamp_file}" "${default_abi_stamp_file}")
	simdlib_register_development_target(RegisterCodegen${target_suffix}
		${codegen_validation_category})
	add_dependencies(RegisterCodegen${target_suffix}
		RegisterExpressionCodegen${target_suffix}
		RegisterConsumerAbi${target_suffix})
	set(codegen_record_index "${artifact_directory}/all-records.txt")
	file(GENERATE OUTPUT "${codegen_record_index}"
		CONTENT "$<JOIN:${codegen_gate_outputs},\n>\n")
	add_test(NAME RegisterCodegen.${target_suffix}
		COMMAND ${CMAKE_COMMAND}
			-DRECORD_INDEX=${codegen_record_index}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ValidateCodegenRecords.cmake)
	set_tests_properties(RegisterCodegen.${target_suffix} PROPERTIES
		LABELS "REGISTER;CODEGEN;ABI;${isa_profile}" RUN_SERIAL TRUE)
endfunction()

if(SIMDLIB_BUILD_REGISTER_CODEGEN_GATES AND SIMDLIB_REGISTER_COMPILER_SUPPORTED)
	if(NOT CMAKE_OBJDUMP)
		find_program(CMAKE_OBJDUMP NAMES llvm-objdump llvm-objdump.exe)
	endif()
	if(NOT CMAKE_OBJDUMP)
		message(FATAL_ERROR "Register generated-code gates require an objdump-compatible disassembler")
	endif()
	simdlib_add_register_codegen_gate(128 SSE42)
	simdlib_add_register_codegen_gate(128 AVX2)
	simdlib_add_register_codegen_gate(256 AVX2)
	add_custom_target(RegisterCodegen DEPENDS
		RegisterCodegen128Sse42
		RegisterCodegen128Avx2
		RegisterCodegen256Avx2)
	if(SIMDLIB_REGISTER_CODEGEN_MODE STREQUAL "ENFORCE")
		simdlib_register_development_target(RegisterCodegen OPTIMIZED_CODEGEN)
	else()
		simdlib_register_development_target(RegisterCodegen DEBUG_DIAGNOSTIC)
	endif()
endif()

endblock()
