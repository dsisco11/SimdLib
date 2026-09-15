cmake_minimum_required(VERSION 3.31)
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}/composition"
    -B "${OUTPUT_DIRECTORY}" -G Ninja "-DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}"
    "-DCASE=${CASE}" "-DPROBE_OBJECT=${OBJECT_FILE}"
    "-DSIMDLIB_FILECHECK=${FILECHECK}" "-DSIMDLIB_LLVM_OBJDUMP=${LLVM_OBJDUMP}"
    "-DSIMDLIB_LLVM_READOBJ=${LLVM_READOBJ}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Composition configure failed: ${output}${error}")
endif()
# Selecting only qualification must automatically pull both independent checks
# and their shared extraction prerequisite into this CTest invocation.
execute_process(COMMAND "${CTEST}" --test-dir "${OUTPUT_DIRECTORY}" -j 4
    -R "[.]qualified$" --output-on-failure --output-junit "${OUTPUT_DIRECTORY}/results.xml"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
file(WRITE "${OUTPUT_DIRECTORY}/harness.txt" "exit=${result}\n${output}${error}")
set(case_root "${OUTPUT_DIRECTORY}/artifacts/composition")
if(CASE STREQUAL "valid")
    if(NOT result EQUAL 0 OR NOT EXISTS "${case_root}/primary/passed.txt" OR
            NOT EXISTS "${case_root}/supplemental.fact/passed.txt")
        message(FATAL_ERROR "Complete additive qualification failed: ${output}${error}")
    endif()
elseif(CASE MATCHES "^failed-")
    if(result EQUAL 0 OR NOT output MATCHES "Instruction expectations failed")
        message(FATAL_ERROR "A failed check was hidden: ${output}${error}")
    endif()
    if(CASE STREQUAL "failed-primary")
        set(passing supplemental.fact)
        set(failing primary)
    else()
        set(passing primary)
        set(failing supplemental.fact)
    endif()
    if(NOT EXISTS "${case_root}/${passing}/passed.txt" OR EXISTS "${case_root}/${failing}/passed.txt")
        message(FATAL_ERROR "Independent primary/supplemental results not preserved")
    endif()
elseif(result EQUAL 0 OR NOT output MATCHES "Stale codegen|Missing codegen")
    message(FATAL_ERROR "Missing/stale input did not fail specifically: ${output}${error}")
endif()
