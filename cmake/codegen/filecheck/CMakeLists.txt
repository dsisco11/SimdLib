cmake_minimum_required(VERSION 3.31)
project(SimdLibFileCheckTool LANGUAGES CXX)

# Build the upstream command against the upstream binary SDK, avoiding LLVM's
# source-tree configure step and its mandatory Python discovery.
if(NOT MSVC OR NOT EXISTS "${LLVM_SDK}/lib/LLVMFileCheck.lib" OR
        NOT EXISTS "${FILECHECK_SOURCE}")
    message(FATAL_ERROR "FileCheck provisioning requires the pinned Windows SDK and source")
endif()
add_executable(FileCheck "${FILECHECK_SOURCE}")
target_compile_features(FileCheck PRIVATE cxx_std_17)
target_include_directories(FileCheck PRIVATE "${LLVM_SDK}/include")
target_compile_definitions(FileCheck PRIVATE NDEBUG _CRT_SECURE_NO_WARNINGS)
target_compile_options(FileCheck PRIVATE /O2 /EHsc)
set_property(TARGET FileCheck PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)
target_link_libraries(FileCheck PRIVATE
    "${LLVM_SDK}/lib/LLVMFileCheck.lib"
    "${LLVM_SDK}/lib/LLVMSupport.lib"
    "${LLVM_SDK}/lib/LLVMDemangle.lib"
    psapi shell32 ole32 uuid advapi32 ws2_32 ntdll delayimp)
install(TARGETS FileCheck RUNTIME DESTINATION bin)
