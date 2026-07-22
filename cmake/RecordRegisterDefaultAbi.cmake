cmake_minimum_required(VERSION 4.4)

foreach(required_variable IN ITEMS
	WRAPPER_OBJECT RAW_OBJECT OBJDUMP ARTIFACT_DIRECTORY COMPILER_ID
	COMPILER_VERSION COMPILER_PATH SYSTEM_NAME SYSTEM_PROCESSOR CONFIGURATION REGISTER_WIDTH
	VECTORCALL_ENABLED STACK_PROTECTOR_MODE)
	if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
		message(FATAL_ERROR "RecordRegisterDefaultAbi requires ${required_variable}")
	endif()
endforeach()

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
	"calling_convention=platform-default\n"
	"vectorcall_enabled=${VECTORCALL_ENABLED}\n"
	"stack_protector_mode=${STACK_PROTECTOR_MODE}\n"
	"wrapper_object=${WRAPPER_OBJECT}\n"
	"raw_object=${RAW_OBJECT}\n")
