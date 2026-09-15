cmake_minimum_required(VERSION 3.31)

# Exercise the public script boundary, then inspect facts the fixture makes
# observable. This is extraction evidence, not a production instruction policy.
execute_process(COMMAND "${CMAKE_COMMAND}"
    "-DOBJECT_FILE=${OBJECT_FILE}" "-DEXPECTED_SYMBOL=${EXPECTED_SYMBOL}"
    "-DOUTPUT_DIRECTORY=${OUTPUT_DIRECTORY}"
    "-DLLVM_OBJDUMP=${LLVM_OBJDUMP}" "-DLLVM_READOBJ=${LLVM_READOBJ}"
    -DCONSTANT_SYMBOLS=extraction_data
    -P "${CMAKE_CURRENT_LIST_DIR}/../../../cmake/codegen/ExtractFunction.cmake"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Real object extraction failed: ${stdout}${stderr}")
endif()
file(READ "${OUTPUT_DIRECTORY}/body.txt" body)
file(READ "${OUTPUT_DIRECTORY}/constants.txt" constants)
string(TOLOWER "${constants}" constants)
if(NOT constants MATCHES "name: extraction_data" OR NOT constants MATCHES "182d4454 fb210940")
    message(FATAL_ERROR "Named constant identity or double bytes were not preserved")
endif()
if(EXPECTED_SYMBOL STREQUAL "extraction_early")
    if(NOT body MATCHES "extraction_opaque" OR NOT body MATCHES "j[a-z]+" OR NOT body MATCHES "ret")
        message(FATAL_ERROR "Conditional control flow or opaque relocation target was lost")
    endif()
elseif(EXPECTED_SYMBOL STREQUAL "extraction_constant")
    if(NOT body MATCHES "extraction_data")
        message(FATAL_ERROR "Constant relocation target was lost")
    endif()
elseif(EXPECTED_SYMBOL STREQUAL "extraction_identity")
    if(NOT body MATCHES "ret")
        message(FATAL_ERROR "Valid return-only body was lost")
    endif()
elseif(EXPECTED_SYMBOL STREQUAL "extraction_vector128")
    if(NOT body MATCHES "xmm[0-9]+" OR NOT body MATCHES "addps")
        message(FATAL_ERROR "128-bit vector operation was lost")
    endif()
elseif(EXPECTED_SYMBOL STREQUAL "extraction_vector256")
    if(NOT body MATCHES "ymm[0-9]+" OR NOT body MATCHES "vaddps")
        message(FATAL_ERROR "256-bit vector operation was lost")
    endif()
endif()

