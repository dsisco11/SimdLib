cmake_minimum_required(VERSION 3.31)
include("${CASE_MANIFEST}")
# Invalidate every old result before checking freshness or extracting. All check
# invocations depend on this CTest fixture and subsequently only read its body.
foreach(result IN LISTS RESULT_FILES)
    file(REMOVE "${result}")
endforeach()
include("${CMAKE_CURRENT_LIST_DIR}/InputReceipt.cmake")
simdlib_verify_codegen_build("${INPUT_MANIFEST}" "${OBJECT_FILE}" "${BUILD_RECEIPT}")
foreach(symbol IN LISTS SYMBOLS)
    execute_process(COMMAND "${CMAKE_COMMAND}"
        "-DOBJECT_FILE=${OBJECT_FILE}" "-DEXPECTED_SYMBOL=${symbol}"
        "-DOUTPUT_DIRECTORY=${ARTIFACT_DIRECTORY}/extracted/${symbol}"
        "-DLLVM_OBJDUMP=${LLVM_OBJDUMP}" "-DLLVM_READOBJ=${LLVM_READOBJ}"
        "-DCONSTANT_SYMBOLS=${CONSTANT_SYMBOLS}"
        -P "${CMAKE_CURRENT_LIST_DIR}/ExtractFunction.cmake"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${CASE_ID}/${symbol}: ${output}${error}")
    endif()
endforeach()
