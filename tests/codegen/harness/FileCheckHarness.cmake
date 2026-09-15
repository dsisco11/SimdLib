cmake_minimum_required(VERSION 3.31)

# Establish the provisioned binary's basic positive/negative invocation contract.
file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY}")
file(WRITE "${OUTPUT_DIRECTORY}/check.txt" "CHECK: extraction-tool-smoke\n")
file(WRITE "${OUTPUT_DIRECTORY}/positive.txt" "extraction-tool-smoke\n")
file(WRITE "${OUTPUT_DIRECTORY}/negative.txt" "deliberate-mismatch\n")
foreach(case IN ITEMS positive negative)
    execute_process(COMMAND "${FILECHECK}" "${OUTPUT_DIRECTORY}/check.txt"
        "--input-file=${OUTPUT_DIRECTORY}/${case}.txt"
        RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
    file(WRITE "${OUTPUT_DIRECTORY}/${case}.result.txt" "exit=${result}\n${stdout}${stderr}")
    if((case STREQUAL "positive" AND NOT result EQUAL 0) OR
            (case STREQUAL "negative" AND NOT result EQUAL 1))
        message(FATAL_ERROR "FileCheck ${case} smoke check failed: ${result}: ${stdout}${stderr}")
    endif()
endforeach()
