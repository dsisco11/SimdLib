include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "PartialRegisterCodegen.cmake is available only to top-level SimdLib builds")
endif()

block(SCOPE_FOR VARIABLES)

set(simdlib_partial_codegen_profiles_file
	"${CMAKE_CURRENT_LIST_DIR}/PartialRegisterCodegenProfiles.json")
if(NOT EXISTS "${simdlib_partial_codegen_profiles_file}")
	message(FATAL_ERROR
		"PartialRegister retained code-generation profiles are missing: "
		"${simdlib_partial_codegen_profiles_file}")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
	"${simdlib_partial_codegen_profiles_file}")
file(READ "${simdlib_partial_codegen_profiles_file}"
	simdlib_partial_codegen_profiles_json)

# @brief Reads one required property from a retained code-generation profile.
# @param output_variable Variable receiving the decoded property value.
# @param entry_index Zero-based profile entry index.
# @param member_name Required JSON member name.
# @param expected_type Required JSON value type.
function(simdlib_partial_codegen_profile_property output_variable entry_index member_name expected_type)
	string(JSON actual_type ERROR_VARIABLE property_error TYPE
		"${simdlib_partial_codegen_profiles_json}"
		entries ${entry_index} "${member_name}")
	if(property_error)
		message(FATAL_ERROR
			"PartialRegister code-generation profile ${entry_index} does not define "
			"${member_name}: ${property_error}")
	endif()
	if(NOT actual_type STREQUAL expected_type)
		message(FATAL_ERROR
			"PartialRegister code-generation profile ${entry_index} member "
			"${member_name} must be ${expected_type}, not ${actual_type}")
	endif()
	string(JSON property_value GET "${simdlib_partial_codegen_profiles_json}"
		entries ${entry_index} "${member_name}")
	set(${output_variable} "${property_value}" PARENT_SCOPE)
endfunction()

string(JSON simdlib_partial_codegen_root_member_count
	ERROR_VARIABLE simdlib_partial_codegen_root_error LENGTH
	"${simdlib_partial_codegen_profiles_json}")
if(simdlib_partial_codegen_root_error OR
		NOT simdlib_partial_codegen_root_member_count EQUAL 2)
	message(FATAL_ERROR
		"PartialRegister code-generation profile root must contain exactly "
		"schemaVersion and entries: ${simdlib_partial_codegen_root_error}")
endif()
string(JSON simdlib_partial_codegen_schema_type
	ERROR_VARIABLE simdlib_partial_codegen_schema_error TYPE
	"${simdlib_partial_codegen_profiles_json}" schemaVersion)
string(JSON simdlib_partial_codegen_entries_type
	ERROR_VARIABLE simdlib_partial_codegen_entries_type_error TYPE
	"${simdlib_partial_codegen_profiles_json}" entries)
if(simdlib_partial_codegen_schema_error OR
		NOT simdlib_partial_codegen_schema_type STREQUAL "NUMBER" OR
		simdlib_partial_codegen_entries_type_error OR
		NOT simdlib_partial_codegen_entries_type STREQUAL "ARRAY")
	message(FATAL_ERROR
		"PartialRegister code-generation profile root requires numeric schemaVersion "
		"and array entries")
endif()
string(JSON simdlib_partial_codegen_schema_version GET
	"${simdlib_partial_codegen_profiles_json}" schemaVersion)
if(NOT simdlib_partial_codegen_schema_version EQUAL 1)
	message(FATAL_ERROR
		"PartialRegister code-generation profile schemaVersion must be 1")
endif()
string(JSON simdlib_partial_codegen_profile_count
	ERROR_VARIABLE simdlib_partial_codegen_entries_error LENGTH
	"${simdlib_partial_codegen_profiles_json}" entries)
if(simdlib_partial_codegen_entries_error OR
		simdlib_partial_codegen_profile_count EQUAL 0)
	message(FATAL_ERROR
		"PartialRegister code-generation profiles must define a non-empty entries array: "
		"${simdlib_partial_codegen_entries_error}")
endif()
math(EXPR simdlib_partial_codegen_profile_last
	"${simdlib_partial_codegen_profile_count} - 1")
set(simdlib_partial_codegen_wildcard_keys "")
set(simdlib_partial_codegen_exact_base_keys "")
set(simdlib_partial_codegen_exact_versions "")
foreach(entry_index RANGE ${simdlib_partial_codegen_profile_last})
	string(JSON entry_member_count LENGTH
		"${simdlib_partial_codegen_profiles_json}" entries ${entry_index})
	if(NOT entry_member_count EQUAL 8)
		message(FATAL_ERROR
			"PartialRegister code-generation profile ${entry_index} must contain "
			"exactly the eight schema members")
	endif()
	simdlib_partial_codegen_profile_property(entry_compiler ${entry_index} compiler STRING)
	simdlib_partial_codegen_profile_property(entry_profile ${entry_index} profile STRING)
	simdlib_partial_codegen_profile_property(entry_register_width ${entry_index} registerWidth NUMBER)
	simdlib_partial_codegen_profile_property(entry_isa_profile ${entry_index} isaProfile STRING)
	simdlib_partial_codegen_profile_property(entry_reason ${entry_index} reason STRING)
	simdlib_partial_codegen_profile_property(entry_wrapper_hash ${entry_index} wrapperSha256 STRING)
	simdlib_partial_codegen_profile_property(entry_raw_hash ${entry_index} rawSha256 STRING)
	if(NOT entry_compiler STREQUAL "clang-cl" AND
			NOT entry_compiler STREQUAL "msvc")
		message(FATAL_ERROR
			"PartialRegister code-generation profile ${entry_index} has unsupported "
			"compiler ${entry_compiler}")
	endif()
	if(NOT entry_profile MATCHES "^(predicate|value|arithmetic|general|abi)$")
		message(FATAL_ERROR
			"PartialRegister code-generation profile ${entry_index} has unsupported "
			"profile ${entry_profile}")
	endif()
	if(NOT entry_register_width EQUAL 128 AND
			NOT entry_register_width EQUAL 256)
		message(FATAL_ERROR
			"PartialRegister code-generation profile ${entry_index} has unsupported "
			"registerWidth ${entry_register_width}")
	endif()
	if(NOT entry_isa_profile STREQUAL "SSE42" AND
			NOT entry_isa_profile STREQUAL "AVX2")
		message(FATAL_ERROR
			"PartialRegister code-generation profile ${entry_index} has unsupported "
			"isaProfile ${entry_isa_profile}")
	endif()
	if(entry_reason STREQUAL "")
		message(FATAL_ERROR
			"PartialRegister code-generation profile ${entry_index} has an empty reason")
	endif()
	foreach(hash_name IN ITEMS wrapper raw)
		set(hash_value "${entry_${hash_name}_hash}")
		string(LENGTH "${hash_value}" hash_length)
		if(NOT hash_length EQUAL 64 OR NOT hash_value MATCHES "^[0-9a-f]+$")
			message(FATAL_ERROR
				"PartialRegister code-generation profile ${entry_index} has an invalid "
				"${hash_name} SHA-256 value")
		endif()
	endforeach()
	string(JSON entry_version_count ERROR_VARIABLE entry_versions_error LENGTH
		"${simdlib_partial_codegen_profiles_json}"
		entries ${entry_index} compilerVersions)
	if(entry_versions_error OR entry_version_count EQUAL 0)
		message(FATAL_ERROR
			"PartialRegister code-generation profile ${entry_index} must define "
			"at least one compiler version: ${entry_versions_error}")
	endif()
	math(EXPR entry_version_last "${entry_version_count} - 1")
	set(entry_base_key
		"${entry_compiler}|${entry_profile}|${entry_register_width}|${entry_isa_profile}")
	foreach(version_index RANGE ${entry_version_last})
		string(JSON entry_version_type TYPE
			"${simdlib_partial_codegen_profiles_json}"
			entries ${entry_index} compilerVersions ${version_index})
		string(JSON entry_version GET "${simdlib_partial_codegen_profiles_json}"
			entries ${entry_index} compilerVersions ${version_index})
		if(NOT entry_version_type STREQUAL "STRING" OR entry_version STREQUAL "")
			message(FATAL_ERROR
				"PartialRegister code-generation profile ${entry_index} has an invalid "
				"compiler version at index ${version_index}")
		endif()
		if(entry_version STREQUAL "*")
			list(FIND simdlib_partial_codegen_wildcard_keys
				"${entry_base_key}" duplicate_wildcard_index)
			list(FIND simdlib_partial_codegen_exact_base_keys
				"${entry_base_key}" wildcard_exact_overlap_index)
			if(NOT duplicate_wildcard_index EQUAL -1 OR
					NOT wildcard_exact_overlap_index EQUAL -1)
				message(FATAL_ERROR
					"PartialRegister code-generation profile ${entry_index} overlaps "
					"another retained profile for ${entry_base_key}")
			endif()
			list(APPEND simdlib_partial_codegen_wildcard_keys "${entry_base_key}")
		else()
			if(NOT entry_version MATCHES "^[0-9]+(\\.[0-9]+)*$")
				message(FATAL_ERROR
					"PartialRegister code-generation profile ${entry_index} has invalid "
					"compiler version ${entry_version}")
			endif()
			list(FIND simdlib_partial_codegen_wildcard_keys
				"${entry_base_key}" exact_wildcard_overlap_index)
			if(NOT exact_wildcard_overlap_index EQUAL -1)
				message(FATAL_ERROR
					"PartialRegister code-generation profile ${entry_index} overlaps "
					"a wildcard retained profile for ${entry_base_key}")
			endif()
			list(LENGTH simdlib_partial_codegen_exact_base_keys exact_key_count)
			if(exact_key_count GREATER 0)
				math(EXPR exact_key_last "${exact_key_count} - 1")
				foreach(exact_key_index RANGE ${exact_key_last})
					list(GET simdlib_partial_codegen_exact_base_keys
						${exact_key_index} existing_base_key)
					list(GET simdlib_partial_codegen_exact_versions
						${exact_key_index} existing_version)
					if("${existing_base_key}" STREQUAL "${entry_base_key}" AND
							"${entry_version}" VERSION_EQUAL "${existing_version}")
						message(FATAL_ERROR
							"PartialRegister code-generation profile ${entry_index} overlaps "
							"another retained profile for ${entry_base_key}, ${entry_version}")
					endif()
				endforeach()
			endif()
			list(APPEND simdlib_partial_codegen_exact_base_keys "${entry_base_key}")
			list(APPEND simdlib_partial_codegen_exact_versions "${entry_version}")
		endif()
	endforeach()
endforeach()

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
	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND SIMDLIB_MSVC_STYLE_DRIVER)
		set(compiler_key "clang-cl")
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		set(compiler_key "msvc")
	else()
		set(${reason_variable} "" PARENT_SCOPE)
		set(${wrapper_hash_variable} "" PARENT_SCOPE)
		set(${raw_hash_variable} "" PARENT_SCOPE)
		return()
	endif()
	set(match_count 0)
	foreach(entry_index RANGE ${simdlib_partial_codegen_profile_last})
		string(JSON entry_compiler GET "${simdlib_partial_codegen_profiles_json}"
			entries ${entry_index} compiler)
		string(JSON entry_profile GET "${simdlib_partial_codegen_profiles_json}"
			entries ${entry_index} profile)
		string(JSON entry_register_width GET "${simdlib_partial_codegen_profiles_json}"
			entries ${entry_index} registerWidth)
		string(JSON entry_isa_profile GET "${simdlib_partial_codegen_profiles_json}"
			entries ${entry_index} isaProfile)
		if(NOT entry_compiler STREQUAL "${compiler_key}" OR
				NOT entry_profile STREQUAL "${profile}" OR
				NOT entry_register_width EQUAL register_width OR
				NOT entry_isa_profile STREQUAL "${isa_profile}")
			continue()
		endif()
		string(JSON entry_version_count LENGTH
			"${simdlib_partial_codegen_profiles_json}"
			entries ${entry_index} compilerVersions)
		math(EXPR entry_version_last "${entry_version_count} - 1")
		set(version_matches FALSE)
		foreach(version_index RANGE ${entry_version_last})
			string(JSON entry_version GET "${simdlib_partial_codegen_profiles_json}"
				entries ${entry_index} compilerVersions ${version_index})
			if(entry_version STREQUAL "*" OR
					CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "${entry_version}")
				set(version_matches TRUE)
			endif()
		endforeach()
		if(NOT version_matches)
			continue()
		endif()
		math(EXPR match_count "${match_count} + 1")
		if(match_count GREATER 1)
			message(FATAL_ERROR
				"Multiple PartialRegister code-generation profiles match ${compiler_key} "
				"${CMAKE_CXX_COMPILER_VERSION}, ${profile}, ${register_width}, ${isa_profile}")
		endif()
		string(JSON reason GET "${simdlib_partial_codegen_profiles_json}"
			entries ${entry_index} reason)
		string(JSON wrapper_hash GET "${simdlib_partial_codegen_profiles_json}"
			entries ${entry_index} wrapperSha256)
		string(JSON raw_hash GET "${simdlib_partial_codegen_profiles_json}"
			entries ${entry_index} rawSha256)
	endforeach()
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
