include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/../codegen/ToolIdentity.cmake")

# @brief Selects development tools only from explicit cache paths or an explicit LLVM directory.
# All three tools must identify the same supported LLVM release. No PATH fallback,
# global install, runtime target linkage, or compiler selection is performed.
function(simdlib_select_codegen_tools)
    set(SIMDLIB_CODEGEN_LLVM_ROOT "$ENV{SIMDLIB_CODEGEN_LLVM_ROOT}" CACHE PATH
        "Explicit LLVM development-tool prefix containing bin/FileCheck, llvm-objdump and llvm-readobj")
    set(provenance "")
    foreach(pair IN ITEMS "FILECHECK|FileCheck" "LLVM_OBJDUMP|llvm-objdump" "LLVM_READOBJ|llvm-readobj")
        string(REPLACE "|" ";" pair "${pair}")
        list(GET pair 0 key)
        list(GET pair 1 executable)
        if(NOT SIMDLIB_${key} AND SIMDLIB_CODEGEN_LLVM_ROOT)
            find_program(SIMDLIB_${key} NAMES "${executable}"
                PATHS "${SIMDLIB_CODEGEN_LLVM_ROOT}/bin" NO_DEFAULT_PATH)
        endif()
        set(SIMDLIB_${key} "${SIMDLIB_${key}}" CACHE FILEPATH "Explicit ${executable} executable")
        simdlib_codegen_tool_identity("${SIMDLIB_${key}}" "${executable}" tool)
        if(DEFINED selected_version AND NOT selected_version STREQUAL tool_VERSION)
            message(FATAL_ERROR "FileCheck, llvm-objdump and llvm-readobj must use the same LLVM release")
        endif()
        set(selected_version "${tool_VERSION}")
        string(APPEND provenance "${executable}=${tool_PATH}\n${executable}_version=${tool_VERSION}\n${executable}_sha256=${tool_SHA256}\n")
        set(SIMDLIB_${key} "${SIMDLIB_${key}}" PARENT_SCOPE)
    endforeach()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/codegen-tools.txt" "${provenance}")
endfunction()
