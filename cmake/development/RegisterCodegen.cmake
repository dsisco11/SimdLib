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
	set(specialized_fma_enabled_wrapper_target RegisterSpecializedFmaEnabledWrapper${target_suffix})
	set(specialized_fma_enabled_raw_target RegisterSpecializedFmaEnabledRaw${target_suffix})
	set(specialized_fma_disabled_wrapper_target RegisterSpecializedFmaDisabledWrapper${target_suffix})
	set(specialized_fma_disabled_raw_target RegisterSpecializedFmaDisabledRaw${target_suffix})
	set(rearrangement_wrapper_target RegisterRearrangementWrapper${target_suffix})
	set(rearrangement_raw_target RegisterRearrangementRaw${target_suffix})
	set(logical_shuffle_intrinsic_target LogicalShuffleIntrinsic${target_suffix})
	set(type_matrix_wrapper_target RegisterTypeMatrixWrapper${target_suffix})
	set(type_matrix_raw_target RegisterTypeMatrixRaw${target_suffix})
	add_library(${wrapper_target} OBJECT tests/codegen/RegisterCodegen.cpp)
	add_library(${raw_target} OBJECT tests/codegen/RegisterCodegenRaw.cpp)
	add_library(${default_wrapper_target} OBJECT tests/codegen/RegisterDefaultAbi.cpp)
	add_library(${default_raw_target} OBJECT tests/codegen/RegisterDefaultAbiRaw.cpp)
	add_library(${abi_wrapper_target} OBJECT tests/codegen/RegisterAbi.cpp)
	add_library(${abi_raw_target} OBJECT tests/codegen/RegisterAbiRaw.cpp)
	if(isa_profile STREQUAL "AVX2")
		add_library(${specialized_fma_enabled_wrapper_target} OBJECT tests/codegen/RegisterSpecializedCodegen.cpp)
		add_library(${specialized_fma_enabled_raw_target} OBJECT tests/codegen/RegisterSpecializedCodegenRaw.cpp)
	endif()
	add_library(${specialized_fma_disabled_wrapper_target} OBJECT tests/codegen/RegisterSpecializedCodegen.cpp)
	add_library(${specialized_fma_disabled_raw_target} OBJECT tests/codegen/RegisterSpecializedCodegenRaw.cpp)
	add_library(${rearrangement_wrapper_target} OBJECT tests/codegen/RegisterRearrangementCodegen.cpp)
	add_library(${rearrangement_raw_target} OBJECT tests/codegen/RegisterRearrangementCodegenRaw.cpp)
	add_library(${logical_shuffle_intrinsic_target} OBJECT tests/codegen/LogicalShuffleCodegenRaw.cpp)
	add_library(${type_matrix_wrapper_target} OBJECT tests/codegen/RegisterTypeMatrixCodegen.cpp)
	add_library(${type_matrix_raw_target} OBJECT tests/codegen/RegisterTypeMatrixCodegenRaw.cpp)
	set(codegen_object_targets
		${wrapper_target} ${raw_target} ${default_wrapper_target} ${default_raw_target}
		${abi_wrapper_target} ${abi_raw_target}
		${specialized_fma_disabled_wrapper_target} ${specialized_fma_disabled_raw_target}
		${rearrangement_wrapper_target} ${rearrangement_raw_target} ${logical_shuffle_intrinsic_target}
		${type_matrix_wrapper_target} ${type_matrix_raw_target})
	if(isa_profile STREQUAL "AVX2")
		list(APPEND codegen_object_targets
			${specialized_fma_enabled_wrapper_target} ${specialized_fma_enabled_raw_target})
	endif()
	foreach(target IN LISTS codegen_object_targets)
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
		foreach(target IN ITEMS ${specialized_fma_enabled_wrapper_target} ${specialized_fma_enabled_raw_target})
			target_compile_definitions(${target} PRIVATE SIMDLIB_HAS_FMA=1)
			if(NOT SIMDLIB_MSVC_STYLE_DRIVER)
				target_compile_options(${target} PRIVATE -mfma)
			endif()
		endforeach()
	endif()
	foreach(target IN ITEMS ${specialized_fma_disabled_wrapper_target} ${specialized_fma_disabled_raw_target})
		target_compile_definitions(${target} PRIVATE SIMDLIB_HAS_FMA=0)
		if(NOT SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_options(${target} PRIVATE -mno-fma)
		endif()
	endforeach()

	set(artifact_directory "${CMAKE_CURRENT_BINARY_DIR}/register-codegen/${artifact_profile}/${register_width}")
	set(stamp_file "${artifact_directory}/comparison.record.json")
	set(register_only_stamp_file "${artifact_directory}/register-only/comparison.record.json")
	set(reassignment_stamp_file "${artifact_directory}/reassignment/comparison.record.json")
	set(lane_stamp_file "${artifact_directory}/lanes/comparison.record.json")
	set(default_abi_stamp_file "${artifact_directory}/default-abi.record.json")
	set(abi_stamp_file "${artifact_directory}/abi/comparison.record.json")
	set(consumer_abi_stamp_file "${artifact_directory}/consumer-abi/comparison.record.json")
	set(specialized_fma_enabled_stamp_file "${artifact_directory}/specialized/fma-enabled/comparison.record.json")
	set(specialized_fma_disabled_stamp_file "${artifact_directory}/specialized/fma-disabled/comparison.record.json")
	set(rearrangement_stamp_file "${artifact_directory}/rearrangement-conversion/comparison.record.json")
	set(logical_shuffle_intrinsic_stamp_file "${artifact_directory}/logical-shuffle-intrinsic/comparison.record.json")
	set(type_matrix_stamp_file "${artifact_directory}/type-matrix/comparison.record.json")
	add_custom_command(
		OUTPUT "${stamp_file}"
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
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DRECORD_ONLY=${codegen_comparison_record_only}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${wrapper_target}>
			$<TARGET_OBJECTS:${raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit Register and raw generated code"
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
			"-DSYMBOL_PATTERN=simdlib_codegen_(unary|binary|ternary|scalar|mask|native|zero|broadcast_reuse|from_array|lane_|with_lane_last|special_members|pressure|basic_)"
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${wrapper_target}>
			$<TARGET_OBJECTS:${raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit register-only wrapper and raw generated code"
		VERBATIM)
	if(isa_profile STREQUAL "AVX2")
		add_custom_command(
			OUTPUT "${specialized_fma_enabled_stamp_file}"
			COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/specialized/fma-enabled"
			COMMAND ${CMAKE_COMMAND}
				-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${specialized_fma_enabled_wrapper_target}>
				-DRAW_OBJECT=$<TARGET_OBJECTS:${specialized_fma_enabled_raw_target}>
				-DOBJDUMP=${CMAKE_OBJDUMP}
				-DARTIFACT_DIRECTORY=${artifact_directory}/specialized/fma-enabled
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
				-DCODEGEN_PROFILE=specialized-fma-enabled
				-DFMA_EXPECTATION=enabled
				-DSYMBOL_PATTERN=simdlib_specialized_codegen_
				-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
			DEPENDS
				$<TARGET_OBJECTS:${specialized_fma_enabled_wrapper_target}>
				$<TARGET_OBJECTS:${specialized_fma_enabled_raw_target}>
				cmake/CompareRegisterCodegen.cmake
			COMMENT "Comparing ${register_width}-bit specialized Register code with FMA enabled"
			VERBATIM)
	endif()
	add_custom_command(
		OUTPUT "${specialized_fma_disabled_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/specialized/fma-disabled"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${specialized_fma_disabled_wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${specialized_fma_disabled_raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/specialized/fma-disabled
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
			-DCODEGEN_PROFILE=specialized-fma-disabled
			-DFMA_EXPECTATION=disabled
			-DSYMBOL_PATTERN=simdlib_specialized_codegen_
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${specialized_fma_disabled_wrapper_target}>
			$<TARGET_OBJECTS:${specialized_fma_disabled_raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit specialized Register code with FMA disabled"
		VERBATIM)
	add_custom_command(
		OUTPUT "${lane_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/lanes"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/lanes
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
			-DSYMBOL_PATTERN=simdlib_codegen_lane_
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${wrapper_target}>
			$<TARGET_OBJECTS:${raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit Register and raw constant-index lane extraction"
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
		OUTPUT "${logical_shuffle_intrinsic_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/logical-shuffle-intrinsic"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${rearrangement_raw_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${logical_shuffle_intrinsic_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/logical-shuffle-intrinsic
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
			-DCODEGEN_PROFILE=logical-shuffle-intrinsic
			-DSYMBOL_PATTERN=simdlib_rearrangement_codegen_logical_shuffle_
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${rearrangement_raw_target}>
			$<TARGET_OBJECTS:${logical_shuffle_intrinsic_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit Api logical shuffles against direct intrinsics"
		VERBATIM)
	add_custom_command(
		OUTPUT "${type_matrix_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/type-matrix"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${type_matrix_wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${type_matrix_raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/type-matrix
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
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS
			$<TARGET_OBJECTS:${type_matrix_wrapper_target}>
			$<TARGET_OBJECTS:${type_matrix_raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit common operations across every Register element type"
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
		"${register_only_stamp_file}" "${reassignment_stamp_file}" "${lane_stamp_file}"
		"${specialized_fma_disabled_stamp_file}"
		"${rearrangement_stamp_file}" "${logical_shuffle_intrinsic_stamp_file}" "${type_matrix_stamp_file}")
	if(isa_profile STREQUAL "AVX2")
		list(APPEND expression_codegen_gate_outputs "${specialized_fma_enabled_stamp_file}")
	endif()
	if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		list(APPEND expression_codegen_gate_outputs "${stamp_file}")
	endif()
	add_custom_target(LogicalShuffleCodegen${target_suffix}
		DEPENDS "${rearrangement_stamp_file}" "${logical_shuffle_intrinsic_stamp_file}")
	add_dependencies(LogicalShuffleCodegen${target_suffix}
		${rearrangement_wrapper_target} ${rearrangement_raw_target} ${logical_shuffle_intrinsic_target})
	add_custom_target(RegisterExpressionCodegen${target_suffix}
		DEPENDS ${expression_codegen_gate_outputs})
	add_dependencies(RegisterExpressionCodegen${target_suffix} ${codegen_object_targets})
	set(expression_record_index "${artifact_directory}/expression-records.txt")
	file(GENERATE OUTPUT "${expression_record_index}"
		CONTENT "$<JOIN:${expression_codegen_gate_outputs},\n>\n")
	add_test(NAME RegisterExpressionCodegen.${target_suffix}
		COMMAND ${CMAKE_COMMAND}
			-DRECORD_INDEX=${expression_record_index}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ValidateCodegenRecords.cmake)
	set_tests_properties(RegisterExpressionCodegen.${target_suffix} PROPERTIES
		LABELS "REGISTER;CODEGEN;${isa_profile}" RUN_SERIAL TRUE)
	add_custom_target(RegisterConsumerAbi${target_suffix}
		DEPENDS "${consumer_abi_stamp_file}")
	add_dependencies(RegisterConsumerAbi${target_suffix}
		${abi_wrapper_target} ${abi_raw_target})
	set(consumer_abi_record_index "${artifact_directory}/consumer-abi-record.txt")
	file(GENERATE OUTPUT "${consumer_abi_record_index}"
		CONTENT "${consumer_abi_stamp_file}\n")
	add_test(NAME RegisterConsumerAbi.${target_suffix}
		COMMAND ${CMAKE_COMMAND}
			-DRECORD_INDEX=${consumer_abi_record_index}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ValidateCodegenRecords.cmake)
	set_tests_properties(RegisterConsumerAbi.${target_suffix} PROPERTIES
		LABELS "REGISTER;CODEGEN;ABI;${isa_profile}" RUN_SERIAL TRUE)
	set(codegen_gate_outputs
		${expression_codegen_gate_outputs} "${consumer_abi_stamp_file}" "${abi_stamp_file}" "${default_abi_stamp_file}")
	add_custom_target(RegisterCodegen${target_suffix} ALL
		DEPENDS "${abi_stamp_file}" "${default_abi_stamp_file}")
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
	add_custom_target(LogicalShuffleCodegen DEPENDS
		LogicalShuffleCodegen128Sse42
		LogicalShuffleCodegen128Avx2
		LogicalShuffleCodegen256Avx2)
	add_custom_target(RegisterCodegen DEPENDS
		RegisterCodegen128Sse42
		RegisterCodegen128Avx2
		RegisterCodegen256Avx2)
endif()

endblock()
