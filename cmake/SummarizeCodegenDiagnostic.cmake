cmake_minimum_required(VERSION 4.4)

foreach(required_variable IN ITEMS
	RECORD_INDEX OUTPUT_FILE COMPILE_COMMANDS SOURCE_REVISION SOURCE_DIGEST
	FINGERPRINT COMPILER_ID PRESET CONFIGURATION SANITIZER
	COMPILATION_SECONDS COMPARISON_SECONDS)
	if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
		message(FATAL_ERROR
			"SummarizeCodegenDiagnostic requires ${required_variable}")
	endif()
endforeach()
if(NOT EXISTS "${RECORD_INDEX}")
	message(FATAL_ERROR "Codegen record index is missing: ${RECORD_INDEX}")
endif()
if(NOT EXISTS "${COMPILE_COMMANDS}")
	message(FATAL_ERROR "Compiler-flag inventory is missing: ${COMPILE_COMMANDS}")
endif()

# @brief Escapes a string for inclusion as a JSON string value.
# @param input_text Unescaped text.
# @param output_variable Variable that receives escaped text.
function(simdlib_escape_json input_text output_variable)
	set(escaped "${input_text}")
	string(REPLACE "\\" "\\\\" escaped "${escaped}")
	string(REPLACE "\"" "\\\"" escaped "${escaped}")
	string(REPLACE "\r" "\\r" escaped "${escaped}")
	string(REPLACE "\n" "\\n" escaped "${escaped}")
	string(REPLACE "\t" "\\t" escaped "${escaped}")
	set(${output_variable} "${escaped}" PARENT_SCOPE)
endfunction()

file(STRINGS "${RECORD_INDEX}" record_files)
set(record_count 0)
set(record_total_seconds 0)
set(slowest_record_seconds -1)
set(slowest_record "")
set(slowest_record_profile "")
set(stack_protector_modes "")
set(disassembly_tool_keys "")
set(disassembly_tool_entries "")
foreach(record_file IN LISTS record_files)
	if(record_file STREQUAL "")
		continue()
	endif()
	file(READ "${record_file}" record_json)
	string(JSON policy_mode GET "${record_json}" policy mode)
	if(NOT policy_mode STREQUAL "RECORD")
		message(FATAL_ERROR
			"Diagnostic record does not use RECORD policy: ${record_file}")
	endif()
	string(JSON record_configuration GET "${record_json}" configuration)
	if(NOT record_configuration STREQUAL CONFIGURATION)
		message(FATAL_ERROR
			"Diagnostic record configuration is ${record_configuration}, expected "
			"${CONFIGURATION}: ${record_file}")
	endif()
	string(JSON record_seconds GET "${record_json}" timing total_seconds)
	string(JSON stack_protector_mode GET
		"${record_json}" stack_protector_mode)
	list(APPEND stack_protector_modes "${stack_protector_mode}")
	string(JSON disassembly_tool_path GET "${record_json}" tool path)
	string(JSON disassembly_tool_version GET "${record_json}" tool version)
	string(JSON disassembly_tool_hash GET "${record_json}" tool sha256)
	set(disassembly_tool_key
		"${disassembly_tool_path}|${disassembly_tool_version}|${disassembly_tool_hash}")
	if(NOT disassembly_tool_key IN_LIST disassembly_tool_keys)
		list(APPEND disassembly_tool_keys "${disassembly_tool_key}")
		simdlib_escape_json("${disassembly_tool_path}"
			disassembly_tool_path_json)
		simdlib_escape_json("${disassembly_tool_version}"
			disassembly_tool_version_json)
		list(APPEND disassembly_tool_entries
			"{\"path\": \"${disassembly_tool_path_json}\", \"version\": \"${disassembly_tool_version_json}\", \"sha256\": \"${disassembly_tool_hash}\"}")
	endif()
	string(JSON record_profile ERROR_VARIABLE record_profile_error
		GET "${record_json}" policy codegen_profile)
	if(record_profile_error)
		set(record_profile "default-abi")
	endif()
	math(EXPR record_count "${record_count} + 1")
	math(EXPR record_total_seconds "${record_total_seconds} + ${record_seconds}")
	if(record_seconds GREATER slowest_record_seconds)
		set(slowest_record_seconds ${record_seconds})
		set(slowest_record "${record_file}")
		set(slowest_record_profile "${record_profile}")
	endif()
endforeach()
if(record_count EQUAL 0)
	message(FATAL_ERROR "Diagnostic record index contains no records")
endif()

list(REMOVE_DUPLICATES stack_protector_modes)
list(SORT stack_protector_modes)
set(stack_protector_modes_json "")
foreach(stack_protector_mode IN LISTS stack_protector_modes)
	simdlib_escape_json("${stack_protector_mode}" stack_protector_mode_json)
	if(NOT stack_protector_modes_json STREQUAL "")
		string(APPEND stack_protector_modes_json ", ")
	endif()
	string(APPEND stack_protector_modes_json
		"\"${stack_protector_mode_json}\"")
endforeach()
string(JOIN ", " disassembly_tools_json ${disassembly_tool_entries})

file(SHA256 "${RECORD_INDEX}" record_index_hash)
file(SHA256 "${COMPILE_COMMANDS}" compile_commands_hash)
math(EXPR invocation_total_seconds
	"${COMPILATION_SECONDS} + ${COMPARISON_SECONDS}")
set(measured_compilation_seconds ${COMPILATION_SECONDS})
set(measured_comparison_seconds ${COMPARISON_SECONDS})
if(EXISTS "${OUTPUT_FILE}")
	file(READ "${OUTPUT_FILE}" prior_provenance_json)
	string(JSON prior_compile_commands_hash ERROR_VARIABLE prior_compile_hash_error
		GET "${prior_provenance_json}" compiler_flags sha256)
	string(JSON prior_record_index_hash ERROR_VARIABLE prior_record_hash_error
		GET "${prior_provenance_json}" records sha256)
	if(NOT prior_compile_hash_error AND NOT prior_record_hash_error AND
			prior_compile_commands_hash STREQUAL compile_commands_hash AND
			prior_record_index_hash STREQUAL record_index_hash)
		string(JSON prior_compilation_seconds ERROR_VARIABLE prior_measured_error
			GET "${prior_provenance_json}" timing measured compilation_seconds)
		string(JSON prior_comparison_seconds ERROR_VARIABLE prior_comparison_error
			GET "${prior_provenance_json}" timing measured comparison_seconds)
		if(prior_measured_error OR prior_comparison_error)
			string(JSON prior_compilation_seconds ERROR_VARIABLE prior_legacy_error
				GET "${prior_provenance_json}" timing compilation_seconds)
			string(JSON prior_comparison_seconds ERROR_VARIABLE prior_legacy_comparison_error
				GET "${prior_provenance_json}" timing comparison_seconds)
			if(prior_legacy_error OR prior_legacy_comparison_error)
				set(prior_compilation_seconds 0)
				set(prior_comparison_seconds 0)
			endif()
		endif()
		if(prior_compilation_seconds GREATER measured_compilation_seconds)
			set(measured_compilation_seconds ${prior_compilation_seconds})
		endif()
		if(prior_comparison_seconds GREATER measured_comparison_seconds)
			set(measured_comparison_seconds ${prior_comparison_seconds})
		endif()
	endif()
endif()
math(EXPR measured_total_seconds
	"${measured_compilation_seconds} + ${measured_comparison_seconds}")
foreach(json_value IN ITEMS
	RECORD_INDEX COMPILE_COMMANDS SOURCE_REVISION SOURCE_DIGEST FINGERPRINT
	COMPILER_ID PRESET CONFIGURATION SANITIZER slowest_record slowest_record_profile)
	simdlib_escape_json("${${json_value}}" "${json_value}_json")
endforeach()
file(WRITE "${OUTPUT_FILE}"
	"{\n"
	"  \"schema\": \"simdlib.codegen-diagnostic-provenance.v1\",\n"
	"  \"operation\": \"record-codegen\",\n"
	"  \"status\": \"complete\",\n"
	"  \"source_revision\": \"${SOURCE_REVISION_json}\",\n"
	"  \"source_digest\": \"${SOURCE_DIGEST_json}\",\n"
	"  \"fingerprint\": \"${FINGERPRINT_json}\",\n"
	"  \"compiler_id\": \"${COMPILER_ID_json}\",\n"
	"  \"configuration\": {\"preset\": \"${PRESET_json}\", \"build_profile\": \"${CONFIGURATION_json}\", \"sanitizer\": \"${SANITIZER_json}\", \"codegen_mode\": \"RECORD\"},\n"
	"  \"compiler_flags\": {\"path\": \"${COMPILE_COMMANDS_json}\", \"sha256\": \"${compile_commands_hash}\"},\n"
	"  \"stack_protector_modes\": [${stack_protector_modes_json}],\n"
	"  \"disassembly_tools\": [${disassembly_tools_json}],\n"
	"  \"records\": {\"index\": \"${RECORD_INDEX_json}\", \"sha256\": \"${record_index_hash}\", \"count\": ${record_count}, \"reported_seconds\": ${record_total_seconds}},\n"
	"  \"slowest_record\": {\"path\": \"${slowest_record_json}\", \"profile\": \"${slowest_record_profile_json}\", \"seconds\": ${slowest_record_seconds}},\n"
	"  \"timing\": {\n"
	"    \"invocation\": {\"compilation_seconds\": ${COMPILATION_SECONDS}, \"comparison_seconds\": ${COMPARISON_SECONDS}, \"total_seconds\": ${invocation_total_seconds}},\n"
	"    \"measured\": {\"compilation_seconds\": ${measured_compilation_seconds}, \"comparison_seconds\": ${measured_comparison_seconds}, \"total_seconds\": ${measured_total_seconds}}\n"
	"  }\n"
	"}\n")
