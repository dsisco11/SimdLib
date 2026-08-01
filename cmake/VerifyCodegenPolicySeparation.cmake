cmake_minimum_required(VERSION 3.31)

foreach(required_variable IN ITEMS SOURCE_DIRECTORY BINARY_DIRECTORY)
	if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
		message(FATAL_ERROR
			"VerifyCodegenPolicySeparation requires ${required_variable}")
	endif()
endforeach()

file(MAKE_DIRECTORY "${BINARY_DIRECTORY}")
set(wrapper_input "${BINARY_DIRECTORY}/wrapper.obj")
set(raw_input "${BINARY_DIRECTORY}/raw.obj")
set(tool_input "${BINARY_DIRECTORY}/objdump.exe")
set(record_file "${BINARY_DIRECTORY}/record.record.json")
set(record_index "${BINARY_DIRECTORY}/records.txt")
file(WRITE "${wrapper_input}" "wrapper\n")
file(WRITE "${raw_input}" "raw\n")
file(WRITE "${tool_input}" "tool\n")
file(SHA256 "${wrapper_input}" wrapper_hash)
file(SHA256 "${raw_input}" raw_hash)
file(SHA256 "${tool_input}" tool_hash)
foreach(path_variable IN ITEMS wrapper_input raw_input tool_input)
	file(TO_CMAKE_PATH "${${path_variable}}" ${path_variable}_json)
endforeach()
file(WRITE "${record_file}"
	"{\n"
	"  \"schema\": \"simdlib.codegen-record.v1\",\n"
	"  \"result\": \"recorded-diagnostic\",\n"
	"  \"policy\": {\"mode\": \"RECORD\"},\n"
	"  \"inputs\": {\n"
	"    \"wrapper\": {\"path\": \"${wrapper_input_json}\", \"sha256\": \"${wrapper_hash}\"},\n"
	"    \"raw\": {\"path\": \"${raw_input_json}\", \"sha256\": \"${raw_hash}\"}\n"
	"  },\n"
	"  \"tool\": {\"path\": \"${tool_input_json}\", \"sha256\": \"${tool_hash}\"}\n"
	"}\n")
file(WRITE "${record_index}" "${record_file}\n")

execute_process(
	COMMAND "${CMAKE_COMMAND}"
		"-DRECORD_INDEX=${record_index}"
		-DEXPECTED_POLICY_MODE=RECORD
		-DREQUIRE_RECORDS=ON
		-P "${SOURCE_DIRECTORY}/cmake/ValidateCodegenRecords.cmake"
	RESULT_VARIABLE record_result
	OUTPUT_VARIABLE record_output
	ERROR_VARIABLE record_error)
if(NOT record_result EQUAL 0)
	message(FATAL_ERROR
		"The record-only control validation failed unexpectedly:\n"
		"${record_output}${record_error}")
endif()

execute_process(
	COMMAND "${CMAKE_COMMAND}"
		"-DRECORD_INDEX=${record_index}"
		-DEXPECTED_POLICY_MODE=ENFORCE
		-DREQUIRE_RECORDS=ON
		-P "${SOURCE_DIRECTORY}/cmake/ValidateCodegenRecords.cmake"
	RESULT_VARIABLE enforce_result
	OUTPUT_VARIABLE enforce_output
	ERROR_VARIABLE enforce_error)
if(enforce_result EQUAL 0)
	message(FATAL_ERROR
		"A record-only result incorrectly satisfied ENFORCE validation")
endif()
if(NOT "${enforce_output}${enforce_error}" MATCHES
		"policy is RECORD, expected ENFORCE")
	message(FATAL_ERROR
		"ENFORCE validation failed for an unexpected reason:\n"
		"${enforce_output}${enforce_error}")
endif()

message(STATUS "Validated RECORD and ENFORCE policy separation")
