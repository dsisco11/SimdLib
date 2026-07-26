cmake_minimum_required(VERSION 4.4)

foreach(required_variable IN ITEMS TEST_DIRECTORY CMAKE_CTEST_COMMAND AUDIT_FILE REGISTER_REQUIRED)
	if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
		message(FATAL_ERROR "VerifyRuntimeTestInventory requires ${required_variable}")
	endif()
endforeach()
if(NOT REGISTER_REQUIRED MATCHES "^(ON|OFF)$")
	message(FATAL_ERROR "REGISTER_REQUIRED must be ON or OFF")
endif()

cmake_path(GET AUDIT_FILE PARENT_PATH audit_directory)
file(MAKE_DIRECTORY "${audit_directory}")
string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef audit_temporary_suffix)
set(audit_temporary_file "${AUDIT_FILE}.${audit_temporary_suffix}.tmp")
file(WRITE "${audit_temporary_file}"
	"schema=simdlib.runtime-test-inventory-audit.v1\n"
	"test_directory=${TEST_DIRECTORY}\n"
	"register_required=${REGISTER_REQUIRED}\n")

# @brief Requires one CTest selection to contain at least one registered test.
# @param selection Stable audit name written to the evidence record.
# @param remaining_arguments CTest selection arguments such as --label-regex or --tests-regex.
function(simdlib_require_test_selection selection)
	set(ctest_arguments --test-dir "${TEST_DIRECTORY}" -N)
	if(DEFINED CONFIGURATION AND NOT "${CONFIGURATION}" STREQUAL "")
		list(APPEND ctest_arguments -C "${CONFIGURATION}")
	endif()
	list(APPEND ctest_arguments ${ARGN})
	execute_process(
		COMMAND "${CMAKE_CTEST_COMMAND}" ${ctest_arguments}
		RESULT_VARIABLE ctest_result
		OUTPUT_VARIABLE ctest_output
		ERROR_VARIABLE ctest_error)
	if(NOT ctest_result EQUAL 0)
		message(FATAL_ERROR
			"Unable to enumerate mandatory test selection ${selection}: ${ctest_error}")
	endif()

	string(REPLACE "\r\n" "\n" ctest_output "${ctest_output}")
	string(REGEX MATCH "Total Tests: ([0-9]+)" total_match "${ctest_output}")
	if(NOT total_match OR CMAKE_MATCH_1 LESS 1)
		message(FATAL_ERROR
			"Mandatory runtime-test selection ${selection} is absent from ${TEST_DIRECTORY}")
	endif()
	file(APPEND "${audit_temporary_file}" "${selection}=${CMAKE_MATCH_1}\n")
endfunction()

simdlib_require_test_selection(total)
foreach(required_label IN ITEMS AVX2 FMA BMI SCALAR)
	simdlib_require_test_selection(
		"label.${required_label}" --label-regex "^${required_label}$")
endforeach()

set(required_test_families
	"Api.SSE42|^Api\\.SSE42\\."
	"Api.AVX2|^Api\\.AVX2\\."
	"FMA.Enabled|^FMA\\.Enabled\\."
	"FMA.Disabled|^FMA\\.Disabled\\."
	"BmiPortable|^BmiPortable\\."
	"UInt128Scalar|^UInt128Scalar\\.")
file(STRINGS "${TEST_DIRECTORY}/CMakeCache.txt" bmi_test_setting
	REGEX "^SIMDLIB_BUILD_BMI_TESTS:BOOL=ON$")
if(bmi_test_setting)
	list(APPEND required_test_families
		"Bmi.Bmi1|^Bmi\\.Bmi1\\."
		"Bmi.Bmi2|^Bmi\\.Bmi2\\."
		"Bmi.Bmi1Bmi2|^Bmi\\.Bmi1Bmi2\\.")
endif()
if(REGISTER_REQUIRED)
	list(APPEND required_test_families
		"Register.SSE42|^Register\\.SSE42\\."
		"Register.AVX2|^Register\\.AVX2\\.")
endif()
foreach(required_test_family IN LISTS required_test_families)
	string(REPLACE "|" ";" family_fields "${required_test_family}")
	list(GET family_fields 0 family_name)
	list(GET family_fields 1 family_regex)
	simdlib_require_test_selection(
		"family.${family_name}" --tests-regex "${family_regex}")
endforeach()

file(RENAME "${audit_temporary_file}" "${AUDIT_FILE}")
