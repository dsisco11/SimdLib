include_guard(GLOBAL)

# @brief Computes a source/configuration/tool/object receipt without instruction
# acceptance hashes. The declared input list is itself included by the caller.
function(simdlib_codegen_input_receipt inputs output)
    set(receipt "")
    foreach(path IN LISTS inputs)
        if(NOT EXISTS "${path}" OR IS_DIRECTORY "${path}")
            message(FATAL_ERROR "Missing codegen build input: ${path}; build fixtures first")
        endif()
        file(SHA256 "${path}" digest)
        string(APPEND receipt "${digest}  ${path}\n")
    endforeach()
    set(${output} "${receipt}" PARENT_SCOPE)
endfunction()

# @brief Rejects test-only reuse unless every bound input still matches the build.
function(simdlib_verify_codegen_build manifest object receipt_file)
    include("${manifest}")
    if(NOT EXISTS "${receipt_file}")
        message(FATAL_ERROR "Missing codegen build receipt: ${receipt_file}; build fixtures first")
    endif()
    simdlib_codegen_input_receipt("${INPUTS};${manifest};${object}" current)
    file(READ "${receipt_file}" built)
    if(NOT current STREQUAL built)
        message(FATAL_ERROR "Stale codegen object, tools or configuration; rebuild fixtures")
    endif()
endfunction()
