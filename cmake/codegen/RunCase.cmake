cmake_minimum_required(VERSION 3.31)
include("${CHECK_MANIFEST}")
include("${CASE_MANIFEST}")
file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY}")
file(REMOVE "${OUTPUT_DIRECTORY}/passed.txt")
include("${CMAKE_CURRENT_LIST_DIR}/InputReceipt.cmake")
simdlib_verify_codegen_build("${INPUT_MANIFEST}" "${OBJECT_FILE}" "${BUILD_RECEIPT}")
file(READ "${CONFIGURATION_FILE}" configuration)
file(WRITE "${OUTPUT_DIRECTORY}/result.txt"
    "case=${CASE_ID}\ncheck=${CHECK_ID}\nsymbols=${SYMBOLS}\nrules=${RULES}\n${configuration}\nstatus=started\n")
string(TIMESTAMP started "%s" UTC)
foreach(symbol IN LISTS SYMBOLS)
    # ABI groups may choose a different rule for each required emitted member.
    set(selected_rules ${RULES})
    if(DEFINED RULES_${symbol})
        list(APPEND selected_rules ${RULES_${symbol}})
    endif()
    set(selected_allow "${ALLOW_MNEMONICS}")
    if(DEFINED ALLOW_MNEMONICS_${symbol})
        set(selected_allow "${ALLOW_MNEMONICS_${symbol}}")
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}"
        "-DBODY_FILE=${ARTIFACT_DIRECTORY}/extracted/${symbol}/body.txt"
        "-DFILECHECK=${FILECHECK}" "-DRULES=${selected_rules}"
        "-DFILECHECK_ARGS=${FILECHECK_ARGS}" "-DALLOW_MNEMONICS=${selected_allow}"
        "-DOUTPUT_DIRECTORY=${OUTPUT_DIRECTORY}/${symbol}"
        -P "${CMAKE_CURRENT_LIST_DIR}/CheckInstructions.cmake"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    file(APPEND "${OUTPUT_DIRECTORY}/result.txt" "function=${symbol}\n${output}${error}\n")
    if(NOT result EQUAL 0)
        file(APPEND "${OUTPUT_DIRECTORY}/result.txt" "status=failed\n")
        message(FATAL_ERROR "${CASE_ID} ${CHECK_ID} ${symbol}: ${configuration}\n${error}\nArtifacts: ${OUTPUT_DIRECTORY}")
    endif()
    if(CONSTANT_RULES)
        execute_process(COMMAND "${CMAKE_COMMAND}"
            "-DBODY_FILE=${ARTIFACT_DIRECTORY}/extracted/${symbol}/constants.txt"
            "-DINPUT_KIND=CONSTANTS" "-DFILECHECK=${FILECHECK}"
            "-DRULES=${CONSTANT_RULES}" "-DFILECHECK_ARGS=${FILECHECK_ARGS}"
            "-DOUTPUT_DIRECTORY=${OUTPUT_DIRECTORY}/${symbol}/constants"
            -P "${CMAKE_CURRENT_LIST_DIR}/CheckInstructions.cmake"
            RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
        file(APPEND "${OUTPUT_DIRECTORY}/result.txt" "constants=${symbol}\n${output}${error}\n")
        if(NOT result EQUAL 0)
            file(APPEND "${OUTPUT_DIRECTORY}/result.txt" "status=failed\n")
            message(FATAL_ERROR "${CASE_ID} ${CHECK_ID} constant expectations failed: ${error}")
        endif()
    endif()
endforeach()
string(TIMESTAMP finished "%s" UTC)
math(EXPR elapsed "${finished} - ${started}")
file(APPEND "${OUTPUT_DIRECTORY}/result.txt" "status=passed\nelapsed_seconds=${elapsed}\n")
file(WRITE "${OUTPUT_DIRECTORY}/passed.txt" "${CASE_ID} ${CHECK_ID}\n")
