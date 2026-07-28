cmake_minimum_required(VERSION 4.4)

if(NOT DEFINED VERIFICATION_FILE OR "${VERIFICATION_FILE}" STREQUAL "")
	message(FATAL_ERROR "VerifyMethodFlagsCodegenRecords requires VERIFICATION_FILE")
endif()
if(NOT EXISTS "${VERIFICATION_FILE}")
	message(FATAL_ERROR "Method-flags generated-code verification is missing: ${VERIFICATION_FILE}")
endif()
include("${CMAKE_CURRENT_LIST_DIR}/ValidateCodegenRecords.cmake")

