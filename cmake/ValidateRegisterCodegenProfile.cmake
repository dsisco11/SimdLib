cmake_minimum_required(VERSION 4.4)

foreach(required_variable IN ITEMS
	ENFORCED_RECORD_INDEX DIAGNOSTIC_RECORD_INDEX CODEGEN_MODE CONFIGURATION)
	if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
		message(FATAL_ERROR
			"ValidateRegisterCodegenProfile requires ${required_variable}")
	endif()
endforeach()
if(NOT CODEGEN_MODE MATCHES "^(ENFORCE|RECORD)$")
	message(FATAL_ERROR "Unsupported Register codegen mode: ${CODEGEN_MODE}")
endif()

if(CODEGEN_MODE STREQUAL "ENFORCE")
	set(RECORD_INDEX "${ENFORCED_RECORD_INDEX}")
	set(EXPECTED_POLICY_MODE ENFORCE)
	set(EXPECTED_CONFIGURATION "${CONFIGURATION}")
	set(REQUIRE_RECORDS "${REQUIRE_ENFORCED_RECORDS}")
	include("${CMAKE_CURRENT_LIST_DIR}/ValidateCodegenRecords.cmake")
endif()

set(RECORD_INDEX "${DIAGNOSTIC_RECORD_INDEX}")
set(EXPECTED_POLICY_MODE RECORD)
set(EXPECTED_CONFIGURATION "${CONFIGURATION}")
set(REQUIRE_RECORDS ON)
include("${CMAKE_CURRENT_LIST_DIR}/ValidateCodegenRecords.cmake")
