cmake_minimum_required(VERSION 3.31)
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}/registration"
    -B "${OUTPUT_DIRECTORY}" -G Ninja "-DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}" "-DCASE=${CASE}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY}")
file(WRITE "${OUTPUT_DIRECTORY}/harness.txt" "exit=${result}\n${output}${error}")
if(CASE STREQUAL "valid")
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Valid registration rejected: ${output}${error}")
    endif()
elseif(result EQUAL 0 OR NOT error MATCHES "Codegen coverage mismatch")
    message(FATAL_ERROR "Missing registration/applicability did not fail coverage: ${output}${error}")
endif()
