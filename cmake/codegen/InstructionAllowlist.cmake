include_guard(GLOBAL)

# @brief Checks mnemonic membership on LLVM instruction lines, without decoding
# bytes, changing operands, selecting boundaries, or implementing match semantics.
function(simdlib_check_instruction_allowlist body allowed)
    string(REPLACE "\n" ";" lines "${body}")
    foreach(line IN LISTS lines)
        if(line MATCHES "^[ \t]*[0-9A-Fa-f]+:[ \t]+([0-9A-Fa-f][0-9A-Fa-f][ \t]+)+([a-z][a-z0-9.]*)($|[ \t])")
            set(mnemonic "${CMAKE_MATCH_2}")
            if(NOT mnemonic MATCHES "^(${allowed})$")
                message(FATAL_ERROR "Forbidden instruction family '${mnemonic}': ${line}")
            endif()
        endif()
    endforeach()
endfunction()
