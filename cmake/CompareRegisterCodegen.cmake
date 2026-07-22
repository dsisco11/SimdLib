cmake_minimum_required(VERSION 4.4)

foreach(required_variable IN ITEMS
	WRAPPER_OBJECT RAW_OBJECT OBJDUMP ARTIFACT_DIRECTORY COMPILER_ID
	COMPILER_VERSION COMPILER_PATH SYSTEM_NAME SYSTEM_PROCESSOR CONFIGURATION REGISTER_WIDTH
	VECTORCALL_ENABLED STACK_PROTECTOR_MODE)
	if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
		message(FATAL_ERROR "CompareRegisterCodegen requires ${required_variable}")
	endif()
endforeach()
if(NOT DEFINED SYMBOL_PATTERN OR "${SYMBOL_PATTERN}" STREQUAL "")
	set(SYMBOL_PATTERN "simdlib_codegen_")
endif()

# @brief Disassembles one generated-code fixture object.
# @param object_file Compiled object containing the fixture functions.
# @param output_variable Variable that receives the disassembly.
function(simdlib_disassemble object_file output_variable)
	execute_process(
		COMMAND "${OBJDUMP}" -d "${object_file}"
		RESULT_VARIABLE disassembly_result
		OUTPUT_VARIABLE disassembly
		ERROR_VARIABLE disassembly_error)
	if(NOT disassembly_result EQUAL 0)
		message(FATAL_ERROR "Unable to disassemble ${object_file}: ${disassembly_error}")
	endif()
	set(${output_variable} "${disassembly}" PARENT_SCOPE)
endfunction()

# @brief Removes object identity, instruction addresses, and encoded bytes while retaining instructions.
# @param input_text Raw object disassembly.
# @param output_variable Variable that receives normalized disassembly.
function(simdlib_normalize_disassembly input_text output_variable)
	set(normalized "${input_text}")
	string(REPLACE "\r\n" "\n" normalized "${normalized}")
	string(REPLACE "\n" ";" disassembly_lines "${normalized}")
	set(fixture_only "")
	set(in_fixture OFF)
	foreach(disassembly_line IN LISTS disassembly_lines)
		if(disassembly_line MATCHES "<[^>]*${SYMBOL_PATTERN}[^>]*>:")
			set(in_fixture ON)
			string(APPEND fixture_only "<symbol>:\n")
		elseif(disassembly_line MATCHES "^[ \t]*[0-9A-Fa-f]+[ \t]+<[^>]+>:")
			set(in_fixture OFF)
		elseif(in_fixture AND NOT disassembly_line MATCHES "^Disassembly of section")
			string(APPEND fixture_only "${disassembly_line}\n")
			if(disassembly_line MATCHES "[ \t]ret[qwl]?([ \t]|$)")
				set(in_fixture OFF)
			endif()
		endif()
	endforeach()
	set(normalized "${fixture_only}")
	string(REGEX REPLACE "[^\n]*file format[^\n]*\n" "" normalized "${normalized}")
	string(REGEX REPLACE "(^|\n)[ \t]*[0-9A-Fa-f]+[ \t]+<" "\\1<" normalized "${normalized}")
	string(REGEX REPLACE "(^|\n)[ \t]*[0-9A-Fa-f]+:[ \t]+([0-9A-Fa-f][0-9A-Fa-f][ \t]+)+" "\\1" normalized "${normalized}")
	string(REGEX REPLACE "<[^>]+>" "<symbol>" normalized "${normalized}")
	string(REGEX REPLACE "[0-9A-Fa-f]+[ \t]+<symbol>" "<target>" normalized "${normalized}")
	string(REGEX REPLACE "[ \t]+\n" "\n" normalized "${normalized}")
	string(REGEX REPLACE "\n+" "\n" normalized "${normalized}")
	string(STRIP "${normalized}" normalized)
	set(${output_variable} "${normalized}" PARENT_SCOPE)
endfunction()

# @brief Removes allocator-selected vector-register identities while retaining all operations and memory operands.
# @param input_text Normalized fixture disassembly.
# @param output_variable Variable that receives the allocation-independent instruction profile.
function(simdlib_profile_disassembly input_text output_variable)
	set(profile "${input_text}")
	string(REGEX REPLACE "%[xyz]mm[0-9]+" "%vreg" profile "${profile}")
	set(${output_variable} "${profile}" PARENT_SCOPE)
endfunction()

simdlib_disassemble("${WRAPPER_OBJECT}" wrapper_disassembly)
simdlib_disassemble("${RAW_OBJECT}" raw_disassembly)
simdlib_normalize_disassembly("${wrapper_disassembly}" wrapper_normalized)
simdlib_normalize_disassembly("${raw_disassembly}" raw_normalized)
simdlib_profile_disassembly("${wrapper_normalized}" wrapper_profile)
simdlib_profile_disassembly("${raw_normalized}" raw_profile)

file(WRITE "${ARTIFACT_DIRECTORY}/wrapper.disassembly.txt" "${wrapper_disassembly}")
file(WRITE "${ARTIFACT_DIRECTORY}/raw.disassembly.txt" "${raw_disassembly}")
file(WRITE "${ARTIFACT_DIRECTORY}/wrapper.normalized.txt" "${wrapper_normalized}\n")
file(WRITE "${ARTIFACT_DIRECTORY}/raw.normalized.txt" "${raw_normalized}\n")
file(WRITE "${ARTIFACT_DIRECTORY}/wrapper.profile.txt" "${wrapper_profile}\n")
file(WRITE "${ARTIFACT_DIRECTORY}/raw.profile.txt" "${raw_profile}\n")
file(WRITE "${ARTIFACT_DIRECTORY}/provenance.txt"
	"compiler_id=${COMPILER_ID}\n"
	"compiler_version=${COMPILER_VERSION}\n"
	"compiler_path=${COMPILER_PATH}\n"
	"system_name=${SYSTEM_NAME}\n"
	"system_processor=${SYSTEM_PROCESSOR}\n"
	"configuration=${CONFIGURATION}\n"
	"register_width=${REGISTER_WIDTH}\n"
	"vectorcall_enabled=${VECTORCALL_ENABLED}\n"
	"stack_protector_mode=${STACK_PROTECTOR_MODE}\n"
	"wrapper_object=${WRAPPER_OBJECT}\n"
	"raw_object=${RAW_OBJECT}\n")

if(NOT wrapper_profile STREQUAL raw_profile)
	message(FATAL_ERROR
		"Register wrapper generated code differs from the raw fixture; inspect ${ARTIFACT_DIRECTORY}")
endif()
