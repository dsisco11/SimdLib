if(NOT DEFINED PROBE_EXECUTABLE)
	message(FATAL_ERROR "PROBE_EXECUTABLE is required")
endif()
if(NOT DEFINED SCENARIO)
	message(FATAL_ERROR "SCENARIO is required")
endif()
if(NOT DEFINED EXPECTED_EXIT_CODE)
	message(FATAL_ERROR "EXPECTED_EXIT_CODE is required")
endif()

execute_process(
	COMMAND "${PROBE_EXECUTABLE}" "${SCENARIO}"
	RESULT_VARIABLE probe_result
	OUTPUT_VARIABLE probe_stdout
	ERROR_VARIABLE probe_stderr)

if(NOT "${probe_result}" STREQUAL "${EXPECTED_EXIT_CODE}")
	message(FATAL_ERROR
		"Precondition scenario '${SCENARIO}' returned '${probe_result}', expected '${EXPECTED_EXIT_CODE}'.\n"
		"stdout:\n${probe_stdout}\n"
		"stderr:\n${probe_stderr}")
endif()
