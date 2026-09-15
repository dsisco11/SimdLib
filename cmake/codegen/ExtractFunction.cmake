cmake_minimum_required(VERSION 3.31)

# Inputs: OBJECT_FILE, EXPECTED_SYMBOL, OUTPUT_DIRECTORY, LLVM_OBJDUMP,
# LLVM_READOBJ. Optional CONSTANT_SYMBOLS names exact data symbols to expose.
# Outputs: full.disassembly.txt, metadata.txt, body.txt, constants.txt,
# extraction.txt, and stderr sidecars. Each invocation owns OUTPUT_DIRECTORY.
foreach(required IN ITEMS OBJECT_FILE EXPECTED_SYMBOL OUTPUT_DIRECTORY LLVM_OBJDUMP LLVM_READOBJ)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "ExtractFunction requires ${required}")
    endif()
endforeach()
if(NOT EXPECTED_SYMBOL MATCHES "^[A-Za-z_?$@][A-Za-z0-9_?$@.]*$")
    message(FATAL_ERROR "Unsupported exact symbol spelling: ${EXPECTED_SYMBOL}")
endif()
if(NOT EXISTS "${OBJECT_FILE}" OR IS_DIRECTORY "${OBJECT_FILE}")
    message(FATAL_ERROR "Missing object: ${OBJECT_FILE}")
endif()
file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY}")
# Invalidate success outputs before running tools, so a failed re-extraction
# cannot leave a previous successful body or receipt available to a checker.
file(REMOVE "${OUTPUT_DIRECTORY}/body.txt" "${OUTPUT_DIRECTORY}/constants.txt"
    "${OUTPUT_DIRECTORY}/extraction.txt")
include("${CMAKE_CURRENT_LIST_DIR}/ToolIdentity.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/ObjectMetadata.cmake")

file(SHA256 "${OBJECT_FILE}" initial_object_hash)
simdlib_codegen_tool_identity("${LLVM_OBJDUMP}" llvm-objdump disassembler)
simdlib_codegen_tool_identity("${LLVM_READOBJ}" llvm-readobj reader)
if(NOT disassembler_VERSION STREQUAL reader_VERSION)
    message(FATAL_ERROR "Object reader and disassembler versions must match")
endif()
execute_process(COMMAND "${LLVM_READOBJ}" --file-headers --sections --symbols
    --relocations "${OBJECT_FILE}"
    RESULT_VARIABLE result OUTPUT_VARIABLE metadata
    ERROR_FILE "${OUTPUT_DIRECTORY}/metadata.stderr.txt")
file(WRITE "${OUTPUT_DIRECTORY}/metadata.txt" "${metadata}")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Object metadata failed; see ${OUTPUT_DIRECTORY}/metadata.stderr.txt")
endif()
string(REPLACE "\r\n" "\n" metadata "${metadata}")
simdlib_codegen_function_metadata("${metadata}" "${EXPECTED_SYMBOL}" function)
execute_process(COMMAND "${LLVM_OBJDUMP}" --disassemble --reloc --x86-asm-syntax=intel "${OBJECT_FILE}"
    RESULT_VARIABLE result OUTPUT_VARIABLE disassembly
    ERROR_FILE "${OUTPUT_DIRECTORY}/disassembly.stderr.txt")
file(WRITE "${OUTPUT_DIRECTORY}/full.disassembly.txt" "${disassembly}")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Disassembly failed; see ${OUTPUT_DIRECTORY}/disassembly.stderr.txt")
endif()
string(REPLACE "\r\n" "\n" disassembly "${disassembly}")
string(TOLOWER "${function_FORMAT}" expected_format)
string(REGEX MATCHALL "file format [^\n]+" disassembly_formats "${disassembly}")
list(LENGTH disassembly_formats format_count)
if(NOT format_count EQUAL 1 OR NOT disassembly_formats STREQUAL "file format ${expected_format}")
    message(FATAL_ERROR "Disassembly format does not match object metadata")
endif()
# LLVM owns instruction decoding and range selection. Preserve its output intact.
execute_process(COMMAND "${LLVM_OBJDUMP}" ${function_SELECTION}
    --reloc --x86-asm-syntax=intel "${OBJECT_FILE}"
    RESULT_VARIABLE result OUTPUT_VARIABLE body ERROR_VARIABLE error)
file(WRITE "${OUTPUT_DIRECTORY}/selection.stderr.txt" "${error}")
file(WRITE "${OUTPUT_DIRECTORY}/selection.txt" "${function_SELECTION}\n")
if(NOT result EQUAL 0 OR NOT error STREQUAL "" OR
        NOT body MATCHES "[0-9A-Fa-f]+ <[^>]+>:" OR
        NOT body MATCHES "[0-9A-Fa-f]+:[ \t]+[0-9A-Fa-f][0-9A-Fa-f][ \t]+" OR
        body MATCHES "<unknown>|[ \t]\\.byte[ \t]")
    message(FATAL_ERROR "LLVM did not produce valid selected disassembly: ${error}")
endif()

# Resolve data identity, then ask LLVM to dump the exact numeric section. Section
# names alone are insufficient because distinct COFF COMDATs can share a name.
set(constants "")
file(WRITE "${OUTPUT_DIRECTORY}/constants.diagnostic.txt" "")
foreach(constant IN LISTS CONSTANT_SYMBOLS)
    set(matches 0)
    foreach(block IN LISTS function_SYMBOLS)
        simdlib_codegen_symbol_identity("${block}" "${function_FORMAT}" name section)
        if(name STREQUAL constant)
            math(EXPR matches "${matches} + 1")
            set(constant_section "${section}")
            set(constant_identity "${block}")
        endif()
    endforeach()
    if(NOT matches EQUAL 1 OR constant_section LESS_EQUAL 0)
        message(FATAL_ERROR "Missing or ambiguous defined constant: ${constant}")
    endif()
    simdlib_codegen_metadata_field("${constant_identity}" Value constant_start)
    set(section_matches 0)
    foreach(block IN LISTS function_SECTIONS)
        if(function_FORMAT STREQUAL "COFF-x86-64")
            simdlib_codegen_metadata_field("${block}" Number index)
            set(size_field RawDataSize)
        else()
            simdlib_codegen_metadata_field("${block}" Index index)
            set(size_field Size)
        endif()
        if(index EQUAL constant_section)
            math(EXPR section_matches "${section_matches} + 1")
            simdlib_codegen_metadata_field("${block}" "${size_field}" constant_section_size)
        endif()
    endforeach()
    if(NOT section_matches EQUAL 1 OR constant_start GREATER_EQUAL constant_section_size)
        message(FATAL_ERROR "Constant lies outside its defining section: ${constant}")
    endif()
    if(function_FORMAT STREQUAL "elf64-x86-64")
        simdlib_codegen_metadata_field("${constant_identity}" Size constant_size)
        math(EXPR constant_end "${constant_start} + ${constant_size}")
        if(constant_end GREATER constant_section_size)
            message(FATAL_ERROR "Constant extends past its defining section: ${constant}")
        endif()
    endif()
    execute_process(COMMAND "${LLVM_READOBJ}" "--hex-dump=${constant_section}" "${OBJECT_FILE}"
        RESULT_VARIABLE result OUTPUT_VARIABLE dump ERROR_VARIABLE error)
    file(APPEND "${OUTPUT_DIRECTORY}/constants.diagnostic.txt"
        "symbol=${constant}\nsection=${constant_section}\nexit=${result}\n${dump}${error}\n")
    if(NOT result EQUAL 0 OR NOT error STREQUAL "" OR NOT dump MATCHES "0x[0-9A-Fa-f]+ [0-9A-Fa-f]+")
        message(FATAL_ERROR "LLVM did not produce constant bytes for ${constant}: ${error}")
    endif()
    string(APPEND constants "${constant_identity}${dump}\n")
endforeach()
file(SHA256 "${OBJECT_FILE}" object_hash)
if(NOT object_hash STREQUAL initial_object_hash)
    message(FATAL_ERROR "Object changed during extraction")
endif()
file(WRITE "${OUTPUT_DIRECTORY}/body.txt" "${body}")
file(WRITE "${OUTPUT_DIRECTORY}/constants.txt" "${constants}")
file(WRITE "${OUTPUT_DIRECTORY}/extraction.txt"
    "symbol=${EXPECTED_SYMBOL}\nformat=${function_FORMAT}\nsection=${function_SECTION}\n"
    "start=${function_START}\nsize=${function_SIZE}\nobject=${OBJECT_FILE}\nobject_sha256=${object_hash}\n"
    "objdump=${disassembler_PATH}\nobjdump_version=${disassembler_VERSION}\nobjdump_sha256=${disassembler_SHA256}\n"
    "readobj=${reader_PATH}\nreadobj_version=${reader_VERSION}\nreadobj_sha256=${reader_SHA256}\n")
