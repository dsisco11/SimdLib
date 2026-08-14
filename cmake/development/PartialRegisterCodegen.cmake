include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "PartialRegisterCodegen.cmake is available only to top-level SimdLib builds")
endif()

block(SCOPE_FOR VARIABLES)

# @brief Selects the exact retained generated-code profiles for one qualified compiler cell.
# @param profile Qualified PartialRegister code-generation profile.
# @param register_width Native width selected for the fixture.
# @param isa_profile ISA profile selected for the fixture.
# @param reason_variable Output variable receiving the documented exception identifier.
# @param wrapper_hash_variable Output variable receiving the retained wrapper profile hash.
# @param raw_hash_variable Output variable receiving the retained raw profile hash.
function(simdlib_partial_retained_profiles profile register_width isa_profile reason_variable wrapper_hash_variable raw_hash_variable)
	set(reason "")
	set(wrapper_hash "")
	set(raw_hash "")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		if(profile STREQUAL "arithmetic" AND register_width EQUAL 128)
			set(reason "clang-equivalent-operand-selection-and-scalar-division-scheduling")
			set(wrapper_hash "895b587adb28f832780881dc106e6419dc5d55bcc8c68aec36a3e0caad2dd70c")
			set(raw_hash "ae9f72212abfe3ed49b456a04b547b43a1f388c52ede0391ffaa63ebe3e57d7a")
		elseif(profile STREQUAL "arithmetic" AND register_width EQUAL 256)
			set(reason "clang-equivalent-commutative-operand-and-register-selection")
			set(wrapper_hash "f6c808129c4d128a7c0b49bff6732f99bccc47d1a430462040d4cd840dbd853f")
			set(raw_hash "7534f844dae0255c78786edce1196290fc97d1f8f85b651cc77f8578eb5ad38e")
		elseif(profile STREQUAL "value" AND register_width EQUAL 128)
			set(reason "clang-equivalent-add-operand-selection-and-scalar-division-scheduling")
			set(wrapper_hash "caa6b7c5c57cc3754f495f85bb2f1593de64c0d425df56f2798aeebd2e0ae297")
			set(raw_hash "e0972c1f50b1a8e1448c63879488fb04416e1510e0396ea922822b42be0f3031")
		elseif(profile STREQUAL "value" AND register_width EQUAL 256)
			set(reason "clang-equivalent-commutative-add-operand-selection")
			set(wrapper_hash "cfd9f0deb4b440152aac87cb0d1cd2223b8c0023a81aa9666fc37c8491aa786f")
			set(raw_hash "947545f582ed36c65c4252daab6b7e406d49a764d2761b8f4e3e437a240ae14b")
		endif()
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		if(profile STREQUAL "predicate" AND register_width EQUAL 128)
			set(reason "msvc-gs-predicate-composition-cookie")
			set(wrapper_hash "40d3d2c14684c7cf4f7ec54e2745a71cc9c96ca77045f8e20e1d2cf2125f5d0f")
			set(raw_hash "274480001869ad12ae5da9e9bfc801eb9823aa555ef42c2dd70ca4d37fb61c2a")
		elseif(profile STREQUAL "predicate" AND register_width EQUAL 256)
			set(reason "msvc-gs-predicate-composition-cookie")
			set(wrapper_hash "d080a20d867fa7df30bb9ea1701ec5199900025e9629b4fbc25ee317d9e7a890")
			set(raw_hash "c93ce1d1b5f1089deb799e23ed0e17deb3049498e50c10c37d65905296dfa555")
		elseif(profile STREQUAL "value" AND register_width EQUAL 128)
			set(reason "msvc-equivalent-import-normalization-gs-and-value-operation-allocation")
			set(wrapper_hash "e500054cea61f6aabcebf97fadf9264b6de112e5d00519119b9ed093d70239dd")
			set(raw_hash "9c085b63a9217f371422e89c09f4984b04cd0b37c3e780d3cb9bc4356f9857d1")
		elseif(profile STREQUAL "value" AND register_width EQUAL 256)
			set(reason "msvc-equivalent-import-normalization-gs-and-value-operation-allocation")
			set(wrapper_hash "c6addf1f03722e3402215409c4f7c076ee16deecabcead203205e8e89d581ad8")
			set(raw_hash "94367252b5828ebb560c32fdbcc95014c0991337a8adac50a808404378fa0d5c")
		elseif(profile STREQUAL "arithmetic" AND register_width EQUAL 128)
			set(reason "msvc-equivalent-unaligned-moves-and-register-allocation")
			set(wrapper_hash "558ce994846ad0e4a9a44fff73b7e156eee94861041cb82ad2016d65af1b537c")
			set(raw_hash "1d135fce910efd2919e05b564bc41f3d9fdaa5a474c0900947a9594716c8132c")
		elseif(profile STREQUAL "arithmetic" AND register_width EQUAL 256)
			set(reason "msvc-equivalent-unaligned-moves-and-register-allocation")
			set(wrapper_hash "2eb906ffce8bf9931bcd1867dd50332fb06f4a17f288f98f7f7906e9c0f0067b")
			set(raw_hash "167f03d2b0e974c1db1250d84257fe0a0612cb2173a2cde65d65dc636c72e8f0")
		elseif(profile STREQUAL "general" AND register_width EQUAL 128)
			set(reason "msvc-equivalent-vector-moves-mask-materialization-register-allocation-and-gs-cookie")
			set(wrapper_hash "3e1b4ffebe93da71eec7360fb305c441d4255bea45d6af73e2fdc3a174d00075")
			set(raw_hash "6b764b03be59acd44ab6301df1db1b2997902d94c20e603865f3a9fadb240709")
		elseif(profile STREQUAL "general" AND register_width EQUAL 256)
			set(reason "msvc-equivalent-vector-moves-mask-materialization-register-allocation-and-gs-cookie")
			set(wrapper_hash "58abbe6e22f172fff40d7053103eb667cf4eb70a9c9b4a28eb40a998a8c12c26")
			set(raw_hash "98e2e877a335a6423a156a813f6bf17a87bd04fd11cb8210eb26d2cc5ba0673a")
		elseif(profile STREQUAL "abi" AND register_width EQUAL 128)
			set(reason "msvc-partial-invariant-boundary-normalization-and-gs-cookie")
			set(wrapper_hash "e2fc5990e063a65e7a77fd7bdcdd7469fed24bc3ccc3a4c927c2ab4fb5c528d1")
			set(raw_hash "f8c290188f776e5887e14f2b2254d7d43a9fa6cdc8edfd0b835331d0270befca")
		elseif(profile STREQUAL "abi" AND register_width EQUAL 256)
			set(reason "msvc-partial-invariant-boundary-normalization-and-gs-cookie")
			set(wrapper_hash "50201329cc0be4ebf40194ad0f789818634b0616c56bc690893023b111bf1ed4")
			set(raw_hash "1db3aea52362ed96c2c45709be8725a73cbdf97db50ac7d5ae2481b0bf803b13")
		endif()
	endif()
	set(${reason_variable} "${reason}" PARENT_SCOPE)
	set(${wrapper_hash_variable} "${wrapper_hash}" PARENT_SCOPE)
	set(${raw_hash_variable} "${raw_hash}" PARENT_SCOPE)
endfunction()

# @brief Adds one wrapper/raw partial-register invariant-boundary generated-code comparison.
# @param register_width Native register width selected for the fixture.
# @param isa_profile ISA profile used to compile both fixture sides.
function(simdlib_add_partial_mask_codegen_gate register_width isa_profile)
	set(stack_protector_mode "default")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		set(stack_protector_mode "msvc-gs")
	endif()
	simdlib_partial_retained_profiles(predicate ${register_width} ${isa_profile}
		predicate_difference_reason predicate_wrapper_hash predicate_raw_hash)
	simdlib_partial_retained_profiles(value ${register_width} ${isa_profile}
		value_difference_reason value_wrapper_hash value_raw_hash)
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
			-DRECORD_ONLY=OFF
			-DRECORDED_DIFFERENCE_REASON=${predicate_difference_reason}
			-DEXPECTED_WRAPPER_PROFILE_SHA256=${predicate_wrapper_hash}
			-DEXPECTED_RAW_PROFILE_SHA256=${predicate_raw_hash}
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
			-DRECORD_ONLY=OFF
			-DRECORDED_DIFFERENCE_REASON=${value_difference_reason}
			-DEXPECTED_WRAPPER_PROFILE_SHA256=${value_wrapper_hash}
			-DEXPECTED_RAW_PROFILE_SHA256=${value_raw_hash}
			-DCODEGEN_PROFILE=partial-register-value-operations
			"-DSYMBOL_PATTERN=simdlib_partial_register_codegen_(add|divide|import)"
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS ${wrapper_target} ${raw_target} $<TARGET_OBJECTS:${wrapper_target}> $<TARGET_OBJECTS:${raw_target}> cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit partial-register value-operation code"
		VERBATIM)
	add_custom_target(PartialRegisterMaskCodegen${target_suffix} DEPENDS "${record_file}" "${value_record_file}")
	add_dependencies(PartialRegisterMaskCodegen${target_suffix} ${wrapper_target} ${raw_target})
	simdlib_register_development_target(PartialRegisterMaskCodegen${target_suffix} OPTIMIZED_CODEGEN)
	add_test(NAME PartialRegisterMaskCodegen.${target_suffix}
		COMMAND ${CMAKE_COMMAND} -DRECORD_FILE=${record_file} -DEXPECTED_POLICY_MODE=ENFORCE -DEXPECTED_CONFIGURATION=$<CONFIG> -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ValidateCodegenRecords.cmake)
	set_tests_properties(PartialRegisterMaskCodegen.${target_suffix} PROPERTIES LABELS "PARTIAL_REGISTER;CODEGEN;${isa_profile}" RUN_SERIAL TRUE)
	simdlib_register_development_test(PartialRegisterMaskCodegen.${target_suffix} OPTIMIZED_CODEGEN)
	add_test(NAME PartialRegisterValueCodegen.${target_suffix}
		COMMAND ${CMAKE_COMMAND} -DRECORD_FILE=${value_record_file} -DEXPECTED_POLICY_MODE=ENFORCE -DEXPECTED_CONFIGURATION=$<CONFIG> -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ValidateCodegenRecords.cmake)
	set_tests_properties(PartialRegisterValueCodegen.${target_suffix} PROPERTIES LABELS "PARTIAL_REGISTER;CODEGEN;${isa_profile}" RUN_SERIAL TRUE)
	simdlib_register_development_test(PartialRegisterValueCodegen.${target_suffix} OPTIMIZED_CODEGEN)
endfunction()

# @brief Adds one strict wrapper/raw arithmetic and specialized-operation generated-code comparison.
# @param register_width Native register width selected for the fixture.
# @param isa_profile ISA profile used to compile both fixture sides.
function(simdlib_add_partial_arithmetic_codegen_gate register_width isa_profile)
	set(stack_protector_mode "default")
	if(isa_profile STREQUAL "SSE42")
		set(target_suffix "${register_width}Sse42")
		set(artifact_profile sse42)
	else()
		set(target_suffix "${register_width}Avx2")
		set(artifact_profile avx2)
	endif()
	if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		set(stack_protector_mode "msvc-gs")
	endif()
	simdlib_partial_retained_profiles(arithmetic ${register_width} ${isa_profile}
		codegen_difference_reason codegen_wrapper_hash codegen_raw_hash)
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
			-DRECORD_ONLY=OFF
			-DRECORDED_DIFFERENCE_REASON=${codegen_difference_reason}
			-DEXPECTED_WRAPPER_PROFILE_SHA256=${codegen_wrapper_hash}
			-DEXPECTED_RAW_PROFILE_SHA256=${codegen_raw_hash}
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
		COMMAND ${CMAKE_COMMAND} -DRECORD_FILE=${record_file} -DEXPECTED_POLICY_MODE=ENFORCE -DEXPECTED_CONFIGURATION=$<CONFIG> -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ValidateCodegenRecords.cmake)
	set_tests_properties(PartialRegisterArithmeticCodegen.${target_suffix} PROPERTIES LABELS "PARTIAL_REGISTER;CODEGEN;${isa_profile}" RUN_SERIAL TRUE)
	simdlib_register_development_test(PartialRegisterArithmeticCodegen.${target_suffix} OPTIMIZED_CODEGEN)
endfunction()

# @brief Adds paired general PartialRegister operation-family fixtures.
# @param register_width Native register width selected for the fixture.
# @param isa_profile ISA profile used to compile both fixture sides.
function(simdlib_add_partial_general_codegen_gate register_width isa_profile)
	simdlib_partial_retained_profiles(general ${register_width} ${isa_profile}
		general_difference_reason general_wrapper_hash general_raw_hash)
	if(isa_profile STREQUAL "SSE42")
		set(target_suffix "${register_width}Sse42")
		set(artifact_profile sse42)
	else()
		set(target_suffix "${register_width}Avx2")
		set(artifact_profile avx2)
	endif()
	set(wrapper_target PartialRegisterGeneralWrapper${target_suffix})
	set(raw_target PartialRegisterGeneralRaw${target_suffix})
	add_library(${wrapper_target} OBJECT tests/codegen/PartialRegisterGeneralCodegen.cpp)
	add_library(${raw_target} OBJECT tests/codegen/PartialRegisterGeneralCodegenRaw.cpp)
	foreach(target IN ITEMS ${wrapper_target} ${raw_target})
		simdlib_register_development_target(${target} OPTIMIZED_CODEGEN)
		target_link_libraries(${target} PRIVATE SimdLib::Register)
		target_compile_definitions(${target} PRIVATE SIMDLIB_PARTIAL_GENERAL_CODEGEN_WIDTH=${register_width})
		simdlib_enable_development_warnings(${target})
		if(isa_profile STREQUAL "SSE42")
			simdlib_enable_register_sse42(${target})
		else()
			simdlib_enable_register_avx2(${target})
		endif()
	endforeach()
	set(artifact_directory "${CMAKE_CURRENT_BINARY_DIR}/partial-register-codegen/${artifact_profile}/${register_width}/general")
	set(record_file "${artifact_directory}/comparison.record.json")
	add_custom_command(OUTPUT "${record_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}"
		COMMAND ${CMAKE_COMMAND} -DWRAPPER_OBJECT=$<TARGET_OBJECTS:${wrapper_target}> -DRAW_OBJECT=$<TARGET_OBJECTS:${raw_target}> -DOBJDUMP=${CMAKE_OBJDUMP} -DARTIFACT_DIRECTORY=${artifact_directory} -DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID} -DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION} -DCOMPILER_PATH=${CMAKE_CXX_COMPILER} -DSYSTEM_NAME=${CMAKE_SYSTEM_NAME} -DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR} -DCONFIGURATION=$<CONFIG> -DREGISTER_WIDTH=${register_width} -DISA_PROFILE=${isa_profile} -DVECTORCALL_ENABLED=0 -DSTACK_PROTECTOR_MODE=default -DRECORD_ONLY=OFF -DRECORDED_DIFFERENCE_REASON=${general_difference_reason} -DEXPECTED_WRAPPER_PROFILE_SHA256=${general_wrapper_hash} -DEXPECTED_RAW_PROFILE_SHA256=${general_raw_hash} -DCODEGEN_PROFILE=partial-register-general "-DSYMBOL_PATTERN=simdlib_partial_general_codegen_" -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS ${wrapper_target} ${raw_target} $<TARGET_OBJECTS:${wrapper_target}> $<TARGET_OBJECTS:${raw_target}> cmake/CompareRegisterCodegen.cmake VERBATIM)
	add_custom_target(PartialRegisterGeneral${target_suffix} DEPENDS "${record_file}")
	add_dependencies(PartialRegisterGeneral${target_suffix} ${wrapper_target} ${raw_target})
	simdlib_register_development_target(PartialRegisterGeneral${target_suffix} OPTIMIZED_CODEGEN)
	add_test(NAME PartialRegisterGeneral.${target_suffix} COMMAND ${CMAKE_COMMAND} -DRECORD_FILE=${record_file} -DEXPECTED_POLICY_MODE=ENFORCE -DEXPECTED_CONFIGURATION=$<CONFIG> -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ValidateCodegenRecords.cmake)
	set_tests_properties(PartialRegisterGeneral.${target_suffix} PROPERTIES LABELS "PARTIAL_REGISTER;CODEGEN;${isa_profile}" RUN_SERIAL TRUE)
	simdlib_register_development_test(PartialRegisterGeneral.${target_suffix} OPTIMIZED_CODEGEN)
endfunction()

# @brief Adds paired non-inlined PartialRegister/native ABI mirrors.
# @param register_width Native register width selected for the fixture.
# @param isa_profile ISA profile used to compile both fixture sides.
function(simdlib_add_partial_abi_codegen_gate register_width isa_profile)
	simdlib_partial_retained_profiles(abi ${register_width} ${isa_profile}
		abi_difference_reason abi_wrapper_hash abi_raw_hash)
	if(isa_profile STREQUAL "SSE42")
		set(target_suffix "${register_width}Sse42")
		set(artifact_profile sse42)
	else()
		set(target_suffix "${register_width}Avx2")
		set(artifact_profile avx2)
	endif()
	set(wrapper_target PartialRegisterAbiWrapper${target_suffix})
	set(raw_target PartialRegisterAbiRaw${target_suffix})
	add_library(${wrapper_target} OBJECT tests/codegen/PartialRegisterAbi.cpp)
	add_library(${raw_target} OBJECT tests/codegen/PartialRegisterAbiRaw.cpp)
	foreach(target IN ITEMS ${wrapper_target} ${raw_target})
		simdlib_register_development_target(${target} OPTIMIZED_CODEGEN)
		target_link_libraries(${target} PRIVATE SimdLib::Register)
		target_compile_definitions(${target} PRIVATE SIMDLIB_PARTIAL_ABI_WIDTH=${register_width})
		simdlib_enable_development_warnings(${target})
		if(isa_profile STREQUAL "SSE42")
			simdlib_enable_register_sse42(${target})
		else()
			simdlib_enable_register_avx2(${target})
		endif()
	endforeach()
	set(artifact_directory "${CMAKE_CURRENT_BINARY_DIR}/partial-register-codegen/${artifact_profile}/${register_width}/abi")
	set(record_file "${artifact_directory}/comparison.record.json")
	add_custom_command(OUTPUT "${record_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}"
		COMMAND ${CMAKE_COMMAND} -DWRAPPER_OBJECT=$<TARGET_OBJECTS:${wrapper_target}> -DRAW_OBJECT=$<TARGET_OBJECTS:${raw_target}> -DOBJDUMP=${CMAKE_OBJDUMP} -DARTIFACT_DIRECTORY=${artifact_directory} -DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID} -DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION} -DCOMPILER_PATH=${CMAKE_CXX_COMPILER} -DSYSTEM_NAME=${CMAKE_SYSTEM_NAME} -DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR} -DCONFIGURATION=$<CONFIG> -DREGISTER_WIDTH=${register_width} -DISA_PROFILE=${isa_profile} -DVECTORCALL_ENABLED=1 -DSTACK_PROTECTOR_MODE=default -DRECORD_ONLY=OFF -DRECORDED_DIFFERENCE_REASON=${abi_difference_reason} -DEXPECTED_WRAPPER_PROFILE_SHA256=${abi_wrapper_hash} -DEXPECTED_RAW_PROFILE_SHA256=${abi_raw_hash} -DCODEGEN_PROFILE=partial-register-abi "-DSYMBOL_PATTERN=simdlib_partial_abi_" -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		DEPENDS ${wrapper_target} ${raw_target} $<TARGET_OBJECTS:${wrapper_target}> $<TARGET_OBJECTS:${raw_target}> cmake/CompareRegisterCodegen.cmake VERBATIM)
	add_custom_target(PartialRegisterAbi${target_suffix} DEPENDS "${record_file}")
	add_dependencies(PartialRegisterAbi${target_suffix} ${wrapper_target} ${raw_target})
	simdlib_register_development_target(PartialRegisterAbi${target_suffix} OPTIMIZED_CODEGEN)
	add_test(NAME PartialRegisterAbi.${target_suffix} COMMAND ${CMAKE_COMMAND} -DRECORD_FILE=${record_file} -DEXPECTED_POLICY_MODE=ENFORCE -DEXPECTED_CONFIGURATION=$<CONFIG> -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ValidateCodegenRecords.cmake)
	set_tests_properties(PartialRegisterAbi.${target_suffix} PROPERTIES LABELS "PARTIAL_REGISTER;ABI;CODEGEN;${isa_profile}" RUN_SERIAL TRUE)
	simdlib_register_development_test(PartialRegisterAbi.${target_suffix} OPTIMIZED_CODEGEN)
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
	simdlib_add_partial_general_codegen_gate(128 SSE42)
	simdlib_add_partial_general_codegen_gate(256 AVX2)
	simdlib_add_partial_abi_codegen_gate(128 SSE42)
	simdlib_add_partial_abi_codegen_gate(256 AVX2)
	simdlib_add_api_partial_transfer_codegen_gate(128 SSE42)
	simdlib_add_api_partial_transfer_codegen_gate(256 AVX2)
	add_custom_target(PartialRegisterMaskCodegen DEPENDS PartialRegisterMaskCodegen128Sse42 PartialRegisterMaskCodegen256Avx2)
	simdlib_register_development_target(PartialRegisterMaskCodegen OPTIMIZED_CODEGEN)
	add_dependencies(RegisterCodegen PartialRegisterMaskCodegen)
	add_custom_target(PartialRegisterArithmeticCodegen DEPENDS PartialRegisterArithmeticCodegen128Sse42 PartialRegisterArithmeticCodegen256Avx2)
	simdlib_register_development_target(PartialRegisterArithmeticCodegen OPTIMIZED_CODEGEN)
	add_dependencies(RegisterCodegen PartialRegisterArithmeticCodegen)
	add_custom_target(PartialRegisterGeneral DEPENDS PartialRegisterGeneral128Sse42 PartialRegisterGeneral256Avx2)
	simdlib_register_development_target(PartialRegisterGeneral OPTIMIZED_CODEGEN)
	add_dependencies(RegisterCodegen PartialRegisterGeneral)
	add_custom_target(PartialRegisterAbi DEPENDS PartialRegisterAbi128Sse42 PartialRegisterAbi256Avx2)
	simdlib_register_development_target(PartialRegisterAbi OPTIMIZED_CODEGEN)
	add_dependencies(RegisterCodegen PartialRegisterAbi)
	add_custom_target(ApiPartialTransferCodegen DEPENDS ApiPartialTransferCodegen128Sse42 ApiPartialTransferCodegen256Avx2)
	simdlib_register_development_target(ApiPartialTransferCodegen OPTIMIZED_CODEGEN)
	add_dependencies(RegisterCodegen ApiPartialTransferCodegen)
endif()

endblock()
