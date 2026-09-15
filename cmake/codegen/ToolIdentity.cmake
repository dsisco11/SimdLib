include_guard(GLOBAL)

# @brief Validates an explicitly selected LLVM tool and records its executable identity.
# @param path Absolute executable path; PATH fallback is deliberately unavailable.
# @param name Expected LLVM executable basename.
# @param prefix Prefix receiving VERSION, SHA256 and PATH.
function(simdlib_codegen_tool_identity path name prefix)
    if(NOT IS_ABSOLUTE "${path}" OR NOT EXISTS "${path}" OR IS_DIRECTORY "${path}")
        message(FATAL_ERROR "Select an existing absolute ${name} executable: '${path}'")
    endif()
    get_filename_component(filename "${path}" NAME)
    if(NOT filename STREQUAL name AND NOT filename STREQUAL "${name}.exe")
        message(FATAL_ERROR "Expected ${name}, not ${filename}")
    endif()
    execute_process(COMMAND "${path}" --version RESULT_VARIABLE result
        OUTPUT_VARIABLE version_text ERROR_VARIABLE error)
    if(NOT result EQUAL 0 OR NOT version_text MATCHES "LLVM version ([0-9]+\\.[0-9]+\\.[0-9]+)")
        message(FATAL_ERROR "Cannot identify LLVM ${name}: ${version_text}${error}")
    endif()
    set(version "${CMAKE_MATCH_1}")
    if(NOT version MATCHES "^(18\\.1\\.8|20\\.1\\.8|22\\.1\\.(3|8))$")
        message(FATAL_ERROR "Unsupported ${name} version ${version}; qualify its output before expanding the tool matrix")
    endif()
    file(SHA256 "${path}" hash)
    set(${prefix}_PATH "${path}" PARENT_SCOPE)
    set(${prefix}_VERSION "${version}" PARENT_SCOPE)
    set(${prefix}_SHA256 "${hash}" PARENT_SCOPE)
endfunction()
