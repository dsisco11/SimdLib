cmake_minimum_required(VERSION 4.4)

foreach(required_variable IN ITEMS
	BINARY_DIRECTORY OWNERSHIP_FILE PROFILE CODEGEN_MODE)
	if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
		message(FATAL_ERROR
			"VerifyCodegenProfileIsolation requires ${required_variable}")
	endif()
endforeach()
if(NOT CODEGEN_MODE MATCHES "^(OFF|RECORD)$")
	message(FATAL_ERROR "Unsupported isolation mode: ${CODEGEN_MODE}")
endif()
if(NOT EXISTS "${OWNERSHIP_FILE}")
	message(FATAL_ERROR "Ownership inventory is missing: ${OWNERSHIP_FILE}")
endif()

file(STRINGS "${OWNERSHIP_FILE}" ownership_rows)
list(POP_FRONT ownership_rows ownership_header)
if(NOT ownership_header STREQUAL
		"target\tcategory\towning_aggregate\tselected")
	message(FATAL_ERROR "Ownership inventory has an invalid header")
endif()

set(codegen_target_count 0)
foreach(ownership_row IN LISTS ownership_rows)
	if(NOT ownership_row MATCHES
			"^([^\t]+)\t([^\t]+)\t([^\t]+)\t(YES|NO)$")
		message(FATAL_ERROR "Malformed ownership row: ${ownership_row}")
	endif()
	set(target "${CMAKE_MATCH_1}")
	set(category "${CMAKE_MATCH_2}")
	if(category MATCHES "^(OPTIMIZED_CODEGEN|DEBUG_DIAGNOSTIC)$")
		math(EXPR codegen_target_count "${codegen_target_count} + 1")
		if(CODEGEN_MODE STREQUAL "OFF")
			message(FATAL_ERROR
				"Profile ${PROFILE} unexpectedly configures codegen target ${target}")
		endif()
	elseif(CODEGEN_MODE STREQUAL "RECORD")
		message(FATAL_ERROR
			"Diagnostic profile ${PROFILE} configures unrelated target ${target} "
			"from ${category}")
	endif()
endforeach()

file(GLOB_RECURSE generated_codegen_files LIST_DIRECTORIES FALSE
	"${BINARY_DIRECTORY}/register-codegen/*"
	"${BINARY_DIRECTORY}/method-flags-codegen/*")
if(CODEGEN_MODE STREQUAL "OFF" AND generated_codegen_files)
	list(GET generated_codegen_files 0 unexpected_codegen_file)
	message(FATAL_ERROR
		"Profile ${PROFILE} produced a generated-code artifact: "
		"${unexpected_codegen_file}")
endif()
if(CODEGEN_MODE STREQUAL "RECORD")
	if(codegen_target_count EQUAL 0)
		message(FATAL_ERROR
			"Diagnostic profile ${PROFILE} configures no codegen targets")
	endif()
	set(record_files ${generated_codegen_files})
	list(FILTER record_files INCLUDE REGEX "\\.record\\.json$")
	if(NOT record_files)
		message(FATAL_ERROR
			"Diagnostic profile ${PROFILE} produced no codegen records")
	endif()
endif()

message(STATUS
	"Validated codegen isolation for profile ${PROFILE} in mode ${CODEGEN_MODE}")
