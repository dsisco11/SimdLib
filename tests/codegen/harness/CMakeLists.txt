cmake_minimum_required(VERSION 3.31)
project(SimdLibExtractionHarness LANGUAGES CXX)
include(CTest)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
get_filename_component(SIMDLIB_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include("${SIMDLIB_SOURCE_ROOT}/cmake/development/CodegenTools.cmake")
simdlib_select_codegen_tools()
add_test(NAME Extraction.tools.FileCheck COMMAND "${CMAKE_COMMAND}"
    "-DFILECHECK=${SIMDLIB_FILECHECK}"
    "-DOUTPUT_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/tools/FileCheck"
    -P "${CMAKE_CURRENT_LIST_DIR}/FileCheckHarness.cmake")

add_executable(UnsupportedLlvmTool UnsupportedTool.cpp)
set_target_properties(UnsupportedLlvmTool PROPERTIES OUTPUT_NAME FileCheck
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/unsupported-tool")
add_test(NAME Extraction.tools.unsupported COMMAND "${CMAKE_COMMAND}"
    "-DTOOL_PATH=$<TARGET_FILE:UnsupportedLlvmTool>"
    "-DOUTPUT_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/tools/unsupported"
    "-DEXPECTED_DIAGNOSTIC=Unsupported FileCheck version 0.0.1"
    -P "${CMAKE_CURRENT_LIST_DIR}/ToolSelectionHarness.cmake")
add_test(NAME Extraction.tools.missing COMMAND "${CMAKE_COMMAND}"
    "-DTOOL_PATH=${CMAKE_CURRENT_BINARY_DIR}/absent/FileCheck${CMAKE_EXECUTABLE_SUFFIX}"
    "-DOUTPUT_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/tools/missing"
    "-DEXPECTED_DIAGNOSTIC=Select an existing absolute FileCheck"
    -P "${CMAKE_CURRENT_LIST_DIR}/ToolSelectionHarness.cmake")

include("${CMAKE_CURRENT_LIST_DIR}/AdversarialObjects.cmake")

add_library(ExtractionObjects OBJECT ObjectFixture.cpp)
target_compile_features(ExtractionObjects PRIVATE cxx_std_20)
if(MSVC)
    target_compile_options(ExtractionObjects PRIVATE /O1 /Gy /arch:AVX2)
else()
    target_compile_options(ExtractionObjects PRIVATE -O1 -ffunction-sections -mavx2)
endif()
foreach(symbol IN ITEMS extraction_straight extraction_early extraction_identity
        extraction_constant extraction_vector128 extraction_vector256)
    add_test(NAME Extraction.object.${symbol} COMMAND "${CMAKE_COMMAND}"
        "-DOBJECT_FILE=$<TARGET_OBJECTS:ExtractionObjects>"
        "-DEXPECTED_SYMBOL=${symbol}"
        "-DOUTPUT_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/objects/${symbol}"
        "-DLLVM_OBJDUMP=${SIMDLIB_LLVM_OBJDUMP}"
        "-DLLVM_READOBJ=${SIMDLIB_LLVM_READOBJ}"
        -P "${CMAKE_CURRENT_LIST_DIR}/RealObjectHarness.cmake")
endforeach()

