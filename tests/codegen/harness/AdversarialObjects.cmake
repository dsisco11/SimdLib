# Explicit harness assemblers create object-format edge cases that C++ optimizers cannot guarantee.
set(SIMDLIB_HARNESS_CLANG "${SIMDLIB_HARNESS_CLANG}" CACHE FILEPATH "Explicit Clang integrated assembler")
set(SIMDLIB_HARNESS_OBJCOPY "${SIMDLIB_HARNESS_OBJCOPY}" CACHE FILEPATH "Explicit LLVM objcopy for duplicate-symbol fixtures")
foreach(tool IN ITEMS SIMDLIB_HARNESS_CLANG SIMDLIB_HARNESS_OBJCOPY)
    if(NOT IS_ABSOLUTE "${${tool}}" OR NOT EXISTS "${${tool}}")
        message(FATAL_ERROR "Select an existing absolute ${tool} for assembled regression fixtures")
    endif()
endforeach()
set(adversarial_directory "${CMAKE_CURRENT_BINARY_DIR}/adversarial")
file(MAKE_DIRECTORY "${adversarial_directory}")
foreach(format IN ITEMS Elf Coff)
    if(format STREQUAL "Elf")
        set(triple x86_64-unknown-linux-gnu)
    else()
        set(triple x86_64-pc-windows-msvc)
    endif()
    set(object "${adversarial_directory}/${format}.o")
    add_custom_command(OUTPUT "${object}"
        COMMAND "${SIMDLIB_HARNESS_CLANG}" "--target=${triple}" -c
            "${CMAKE_CURRENT_LIST_DIR}/Adversarial${format}.s" -o "${object}"
        DEPENDS "${CMAKE_CURRENT_LIST_DIR}/Adversarial${format}.s" VERBATIM)
    list(APPEND adversarial_objects "${object}")
endforeach()
set(duplicate_object "${adversarial_directory}/Duplicate.o")
add_custom_command(OUTPUT "${duplicate_object}"
    COMMAND "${SIMDLIB_HARNESS_OBJCOPY}" --redefine-sym duplicate_two=duplicate
        "${adversarial_directory}/Elf.o" "${duplicate_object}"
    DEPENDS "${adversarial_directory}/Elf.o" VERBATIM)
add_custom_target(AdversarialObjects ALL DEPENDS ${adversarial_objects} "${duplicate_object}")

# @brief Registers one real-object selection or rejection regression.
# @param case Stable test suffix.
# @param format Object fixture basename.
# @param symbol Exact requested function.
# @param constant Optional exact constant symbol.
# @param diagnostic Optional required rejection message.
function(simdlib_adversarial_case case format symbol constant diagnostic)
    set(arguments "-DOBJECT_FILE=${adversarial_directory}/${format}.o"
        "-DEXPECTED_SYMBOL=${symbol}"
        "-DOUTPUT_DIRECTORY=${adversarial_directory}/results/${case}"
        "-DLLVM_OBJDUMP=${SIMDLIB_LLVM_OBJDUMP}"
        "-DLLVM_READOBJ=${SIMDLIB_LLVM_READOBJ}")
    if(NOT constant STREQUAL "")
        list(APPEND arguments "-DCONSTANT_SYMBOL=${constant}")
    endif()
    if(NOT diagnostic STREQUAL "")
        list(APPEND arguments "-DEXPECTED_DIAGNOSTIC=${diagnostic}")
    endif()
    add_test(NAME Extraction.adversarial.${case} COMMAND "${CMAKE_COMMAND}" ${arguments}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/AdversarialHarness.cmake")
endfunction()
simdlib_adversarial_case(elf-interior Elf adversarial_primary "" "")
simdlib_adversarial_case(coff-interior Coff adversarial_primary "" "")
simdlib_adversarial_case(elf-padding Elf adversarial_padding "" "")
simdlib_adversarial_case(elf-constant Elf adversarial_primary constant_one "")
simdlib_adversarial_case(coff-constant-one Coff adversarial_primary constant_one "")
simdlib_adversarial_case(coff-constant-two Coff adversarial_primary constant_two "")
simdlib_adversarial_case(missing-symbol Elf missing "" "Expected one exact symbol 'missing', found 0")
simdlib_adversarial_case(duplicate-symbol Duplicate duplicate "" "Expected one exact symbol 'duplicate', found 2")
simdlib_adversarial_case(empty-function Elf adversarial_empty "" "Empty function metadata")
simdlib_adversarial_case(missing-constant Coff adversarial_primary missing "Missing or ambiguous defined constant")

# Malformed inputs are real files; LLVM owns rejecting their binary encoding.
file(WRITE "${adversarial_directory}/Empty.o" "")
file(WRITE "${adversarial_directory}/Malformed.o" "not an object\n")
set(unsupported_object "${adversarial_directory}/Unsupported.o")
add_custom_command(OUTPUT "${unsupported_object}"
    COMMAND "${SIMDLIB_HARNESS_CLANG}" --target=i686-unknown-linux-gnu -c
        "${CMAKE_CURRENT_LIST_DIR}/AdversarialElf.s" -o "${unsupported_object}"
    DEPENDS "${CMAKE_CURRENT_LIST_DIR}/AdversarialElf.s" VERBATIM)
set(duplicate_constant "${adversarial_directory}/DuplicateConstant.o")
add_custom_command(OUTPUT "${duplicate_constant}"
    COMMAND "${SIMDLIB_HARNESS_OBJCOPY}" --redefine-sym constant_two=constant_one
        "${adversarial_directory}/Coff.o" "${duplicate_constant}"
    DEPENDS "${adversarial_directory}/Coff.o" VERBATIM)
add_custom_target(InvalidObjects ALL DEPENDS "${unsupported_object}" "${duplicate_constant}")
simdlib_adversarial_case(missing-object Absent adversarial_primary "" "Missing object")
simdlib_adversarial_case(empty-object Empty adversarial_primary "" "Object metadata failed")
simdlib_adversarial_case(malformed-object Malformed adversarial_primary "" "Object metadata failed")
simdlib_adversarial_case(unsupported-format Unsupported adversarial_primary "" "Unsupported object format")
simdlib_adversarial_case(coff-shared-section Coff adversarial_shared "" "exclusive /Gy function section")
simdlib_adversarial_case(duplicate-constant DuplicateConstant adversarial_primary constant_one "Missing or ambiguous defined constant")

set(invalid_constant "${adversarial_directory}/InvalidConstant.o")
add_custom_command(OUTPUT "${invalid_constant}"
    COMMAND "${SIMDLIB_HARNESS_OBJCOPY}" --add-symbol out_of_range=.rodata:0xffff
        "${adversarial_directory}/Elf.o" "${invalid_constant}"
    DEPENDS "${adversarial_directory}/Elf.o" VERBATIM)
add_custom_target(InvalidConstantObject ALL DEPENDS "${invalid_constant}")
simdlib_adversarial_case(out-of-range-constant InvalidConstant adversarial_primary out_of_range "Constant lies outside its defining section")

simdlib_adversarial_case(incomplete-instruction Elf adversarial_decode "" "LLVM did not produce valid selected disassembly")
