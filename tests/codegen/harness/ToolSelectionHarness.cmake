cmake_minimum_required(VERSION 3.31)

# Run the real discovery entry point in a child process, retaining its diagnostic.
file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY}")
file(WRITE "${OUTPUT_DIRECTORY}/select.cmake"
    "include(\"${CMAKE_CURRENT_LIST_DIR}/../../../cmake/codegen/ToolIdentity.cmake\")\n"
    "simdlib_codegen_tool_identity(\"${TOOL_PATH}\" FileCheck tool)\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -P "${OUTPUT_DIRECTORY}/select.cmake"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
file(WRITE "${OUTPUT_DIRECTORY}/result.txt" "exit=${result}\n${stdout}${stderr}")
if(result EQUAL 0 OR NOT "${stdout}${stderr}" MATCHES "${EXPECTED_DIAGNOSTIC}")
    message(FATAL_ERROR "Tool selection did not reject the invalid input: ${stdout}${stderr}")
endif()
