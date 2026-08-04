cmake_minimum_required(VERSION 3.31)

string(TIMESTAMP codegen_start_epoch "%s" UTC)

foreach(required_variable IN ITEMS
	WRAPPER_OBJECT RAW_OBJECT OBJDUMP ARTIFACT_DIRECTORY COMPILER_ID
	COMPILER_VERSION COMPILER_PATH SYSTEM_NAME SYSTEM_PROCESSOR CONFIGURATION REGISTER_WIDTH
	ISA_PROFILE VECTORCALL_ENABLED STACK_PROTECTOR_MODE)
	if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
		message(FATAL_ERROR "RecordRegisterDefaultAbi requires ${required_variable}")
	endif()
endforeach()
if(NOT DEFINED RECORD_FILE OR "${RECORD_FILE}" STREQUAL "")
	set(RECORD_FILE "${ARTIFACT_DIRECTORY}/default-abi.record.json")
endif()
file(REMOVE "${RECORD_FILE}")
string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef record_temporary_suffix)
set(record_temporary_file "${RECORD_FILE}.${record_temporary_suffix}.tmp")

# @brief Disassembles one default-convention ABI fixture and writes the artifact.
# @param object_file Compiled fixture object.
# @param output_file Destination disassembly file.
function(simdlib_record_default_abi object_file output_file)
	execute_process(
		COMMAND "${OBJDUMP}" -d "${object_file}"
		RESULT_VARIABLE disassembly_result
		OUTPUT_VARIABLE disassembly
		ERROR_VARIABLE disassembly_error)
	if(NOT disassembly_result EQUAL 0)
		message(FATAL_ERROR "Unable to disassemble ${object_file}: ${disassembly_error}")
	endif()
	file(WRITE "${output_file}" "${disassembly}")
endfunction()

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

simdlib_record_default_abi("${WRAPPER_OBJECT}" "${ARTIFACT_DIRECTORY}/default-wrapper.disassembly.txt")
simdlib_record_default_abi("${RAW_OBJECT}" "${ARTIFACT_DIRECTORY}/default-raw.disassembly.txt")
file(WRITE "${ARTIFACT_DIRECTORY}/default-abi.provenance.txt"
	"compiler_id=${COMPILER_ID}\n"
	"compiler_version=${COMPILER_VERSION}\n"
	"compiler_path=${COMPILER_PATH}\n"
	"system_name=${SYSTEM_NAME}\n"
	"system_processor=${SYSTEM_PROCESSOR}\n"
	"configuration=${CONFIGURATION}\n"
	"register_width=${REGISTER_WIDTH}\n"
	"isa_profile=${ISA_PROFILE}\n"
	"calling_convention=platform-default\n"
	"vectorcall_enabled=${VECTORCALL_ENABLED}\n"
	"stack_protector_mode=${STACK_PROTECTOR_MODE}\n"
	"wrapper_object=${WRAPPER_OBJECT}\n"
	"raw_object=${RAW_OBJECT}\n")

file(SHA256 "${WRAPPER_OBJECT}" wrapper_hash)
file(SHA256 "${RAW_OBJECT}" raw_hash)
file(SHA256 "${OBJDUMP}" tool_hash)
execute_process(
	COMMAND "${OBJDUMP}" --version
	RESULT_VARIABLE tool_version_result
	OUTPUT_VARIABLE tool_version_output
	ERROR_VARIABLE tool_version_error)
if(NOT tool_version_result EQUAL 0)
	message(FATAL_ERROR "Unable to identify default-ABI recording tool: ${tool_version_error}")
endif()
string(REGEX REPLACE "\r?\n.*" "" tool_version "${tool_version_output}")
string(TIMESTAMP codegen_end_epoch "%s" UTC)
math(EXPR codegen_total_seconds
	"${codegen_end_epoch} - ${codegen_start_epoch}")
foreach(json_value IN ITEMS
	WRAPPER_OBJECT RAW_OBJECT OBJDUMP tool_version COMPILER_ID COMPILER_VERSION
	COMPILER_PATH SYSTEM_NAME SYSTEM_PROCESSOR CONFIGURATION ISA_PROFILE STACK_PROTECTOR_MODE)
	simdlib_escape_json("${${json_value}}" "${json_value}_json")
endforeach()
file(WRITE "${record_temporary_file}"
	"{\n"
	"  \"schema\": \"simdlib.codegen-record.v1\",\n"
	"  \"kind\": \"diagnostic\",\n"
	"  \"result\": \"recorded-diagnostic\",\n"
	"  \"accepted_exception\": \"none\",\n"
	"  \"inputs\": {\n"
	"    \"wrapper\": {\"path\": \"${WRAPPER_OBJECT_json}\", \"sha256\": \"${wrapper_hash}\"},\n"
	"    \"raw\": {\"path\": \"${RAW_OBJECT_json}\", \"sha256\": \"${raw_hash}\"}\n"
	"  },\n"
	"  \"tool\": {\"path\": \"${OBJDUMP_json}\", \"version\": \"${tool_version_json}\", \"sha256\": \"${tool_hash}\"},\n"
	"  \"policy\": {\"id\": \"register-default-abi-diagnostic-v1\", \"mode\": \"RECORD\", "
		"\"calling_convention\": \"platform-default\"},\n"
	"  \"compiler\": {\"id\": \"${COMPILER_ID_json}\", \"version\": \"${COMPILER_VERSION_json}\", "
		"\"path\": \"${COMPILER_PATH_json}\"},\n"
	"  \"platform\": {\"system\": \"${SYSTEM_NAME_json}\", \"processor\": \"${SYSTEM_PROCESSOR_json}\"},\n"
	"  \"configuration\": \"${CONFIGURATION_json}\",\n"
	"  \"timing\": {\"total_seconds\": ${codegen_total_seconds}},\n"
	"  \"register_width\": ${REGISTER_WIDTH},\n"
	"  \"isa_profile\": \"${ISA_PROFILE_json}\",\n"
	"  \"vectorcall_enabled\": ${VECTORCALL_ENABLED},\n"
	"  \"stack_protector_mode\": \"${STACK_PROTECTOR_MODE_json}\"\n"
	"}\n")
file(RENAME "${record_temporary_file}" "${RECORD_FILE}")
