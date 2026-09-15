include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/CodegenTools.cmake")
set(SIMDLIB_CODEGEN_SCRIPTS "${CMAKE_CURRENT_LIST_DIR}/../codegen")

# region Fixture compilation

# @brief Creates one normally dependency-tracked object and a build-input receipt.
# Geometry/configuration are explicit metadata; options and definitions determine
# compilation. Related primary/supplemental cases share this target.
function(simdlib_add_codegen_fixture)
    cmake_parse_arguments(P "" "TARGET;SOURCE;CONFIGURATION;GEOMETRY"
        "OPTIONS;DEFINITIONS;INPUTS" ${ARGN})
    if(NOT CMAKE_GENERATOR STREQUAL "Ninja" OR CMAKE_CONFIGURATION_TYPES)
        message(FATAL_ERROR "Codegen pilot requires single-configuration Ninja")
    endif()
    foreach(key IN ITEMS TARGET SOURCE CONFIGURATION GEOMETRY)
        if(NOT P_${key})
            message(FATAL_ERROR "Codegen fixture requires ${key}")
        endif()
    endforeach()
    add_library(${P_TARGET} OBJECT "${P_SOURCE}")
    set_property(TARGET ${P_TARGET} PROPERTY CXX_SCAN_FOR_MODULES OFF)
    target_link_libraries(${P_TARGET} PRIVATE SimdLib::Register)
    target_compile_options(${P_TARGET} PRIVATE ${P_OPTIONS})
    target_compile_definitions(${P_TARGET} PRIVATE ${P_DEFINITIONS})
    set(root "${CMAKE_CURRENT_BINARY_DIR}/codegen-contracts/${P_TARGET}")
    set(config "${root}/configuration.txt")
    string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type)
    file(READ "${CMAKE_CURRENT_BINARY_DIR}/codegen-tools.txt" tool_provenance)
    file(GENERATE OUTPUT "${config}" CONTENT
        "compiler=${CMAKE_CXX_COMPILER}\ncompiler_id=${CMAKE_CXX_COMPILER_ID}\ncompiler_version=${CMAKE_CXX_COMPILER_VERSION}\nfrontend=${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}\ntarget=${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}\ngenerator=${CMAKE_GENERATOR}\nconfiguration=${P_CONFIGURATION}\ngeometry=${P_GEOMETRY}\nglobal_flags=${CMAKE_CXX_FLAGS}\nbuild_type=${CMAKE_BUILD_TYPE}\nbuild_type_flags=${CMAKE_CXX_FLAGS_${build_type}}\noptions=$<TARGET_PROPERTY:${P_TARGET},COMPILE_OPTIONS>\ndefinitions=$<TARGET_PROPERTY:${P_TARGET},COMPILE_DEFINITIONS>\nstandard=$<TARGET_PROPERTY:${P_TARGET},CXX_STANDARD>\ncompile_commands=${CMAKE_BINARY_DIR}/compile_commands.json\n${tool_provenance}" CONDITION "$<COMPILE_LANGUAGE:CXX>" TARGET ${P_TARGET})
    # Bind source/header/configuration and selected executables at build time.
    # The ordinary compiler dependency graph owns recompilation; this conservative
    # input set also rejects test-only reuse after changes to checking policy.
    set(manifest "${root}/inputs.cmake")
    set(receipt "${root}/build.sha256")
    set(inputs "${P_SOURCE};${P_INPUTS};${config};${CMAKE_BINARY_DIR}/CMakeCache.txt;${CMAKE_BINARY_DIR}/build.ninja;${CMAKE_BINARY_DIR}/compile_commands.json;${CMAKE_CXX_COMPILER};${SIMDLIB_FILECHECK};${SIMDLIB_LLVM_OBJDUMP};${SIMDLIB_LLVM_READOBJ};${CMAKE_CURRENT_BINARY_DIR}/codegen-tools.txt")
    file(GENERATE OUTPUT "${manifest}" CONTENT "set(INPUTS [==[${inputs}]==])\n")
    add_custom_command(OUTPUT "${receipt}"
        COMMAND "${CMAKE_COMMAND}" "-DINPUT_MANIFEST=${manifest}"
            "-DOBJECT_FILE=$<TARGET_OBJECTS:${P_TARGET}>" "-DBUILD_RECEIPT=${receipt}"
            -P "${SIMDLIB_CODEGEN_SCRIPTS}/RecordBuild.cmake"
        DEPENDS ${P_TARGET} $<TARGET_OBJECTS:${P_TARGET}> ${P_SOURCE} ${P_INPUTS}
            "${config}" "${manifest}" "${CMAKE_BINARY_DIR}/CMakeCache.txt"
            "${CMAKE_BINARY_DIR}/build.ninja" "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${SIMDLIB_FILECHECK}" "${SIMDLIB_LLVM_OBJDUMP}" "${SIMDLIB_LLVM_READOBJ}"
        VERBATIM)
    add_custom_target(${P_TARGET}Receipt DEPENDS "${receipt}")
    foreach(pair IN ITEMS "ROOT|${root}" "CONFIGURATION|${P_CONFIGURATION}"
            "GEOMETRY|${P_GEOMETRY}" "INPUT_MANIFEST|${manifest}"
            "BUILD_RECEIPT|${receipt}" "CONFIGURATION_FILE|${config}")
        string(REPLACE "|" ";" pair "${pair}")
        list(GET pair 0 key)
        list(GET pair 1 value)
        set_property(TARGET ${P_TARGET} PROPERTY SIMDLIB_CODEGEN_${key} "${value}")
    endforeach()
    if(COMMAND simdlib_register_development_target)
        simdlib_register_development_target(${P_TARGET} OPTIMIZED_CODEGEN)
        simdlib_register_development_target(${P_TARGET}Receipt OPTIMIZED_CODEGEN)
    endif()
endfunction()
# endregion

# region Case registration

# @brief Registers a primary with every mandatory ABI member; supplement specs
# are fact|driver-regex|configuration-regex|rule-definition. No primary selector exists.
function(simdlib_add_codegen_case)
    cmake_parse_arguments(P "" "ID;TARGET;PRIMARY;DRIVER" "SYMBOLS;SUPPLEMENTS;CONSTANT_SYMBOLS" ${ARGN})
    foreach(key IN ITEMS ID TARGET PRIMARY DRIVER SYMBOLS)
        if(NOT P_${key})
            message(FATAL_ERROR "Codegen case requires ${key}")
        endif()
    endforeach()
    if(NOT P_DRIVER MATCHES "^(msvc|clang-cl|clang|gcc)$" OR P_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unsupported driver or codegen case arguments")
    endif()
    set(unique_symbols ${P_SYMBOLS})
    list(REMOVE_DUPLICATES unique_symbols)
    if(NOT "${unique_symbols}" STREQUAL "${P_SYMBOLS}")
        message(FATAL_ERROR "Duplicate required emitted function in ${P_ID}")
    endif()
    foreach(key IN ITEMS ROOT CONFIGURATION GEOMETRY INPUT_MANIFEST BUILD_RECEIPT CONFIGURATION_FILE)
        get_target_property(${key} ${P_TARGET} SIMDLIB_CODEGEN_${key})
    endforeach()
    set(base "Codegen.${P_ID}.${GEOMETRY}.${CONFIGURATION}")
    set(directory "${ROOT}/${P_ID}")
    set(case_manifest "${directory}/case.cmake")
    set(selected "primary|${P_PRIMARY}")
    foreach(spec IN LISTS P_SUPPLEMENTS)
        string(REPLACE "|" ";" fields "${spec}")
        list(LENGTH fields length)
        if(NOT length EQUAL 4)
            message(FATAL_ERROR "Invalid supplemental declaration: ${spec}")
        endif()
        list(GET fields 0 fact)
        list(GET fields 1 drivers)
        list(GET fields 2 configurations)
        list(GET fields 3 definition)
        if(P_DRIVER MATCHES "^(${drivers})$" AND CONFIGURATION MATCHES "${configurations}")
            list(APPEND selected "supplemental.${fact}|${definition}")
        endif()
    endforeach()
    set(results "")
    foreach(check IN LISTS selected)
        string(REPLACE "|" ";" fields "${check}")
        list(GET fields 0 id)
        list(GET fields 1 definition)
        if(NOT EXISTS "${definition}")
            message(FATAL_ERROR "Missing case expectation definition: ${definition}")
        endif()
        set(output "${directory}/${id}")
        list(APPEND results "${output}/passed.txt")
        set(check_manifest "${directory}/${id}.cmake")
        file(GENERATE OUTPUT "${check_manifest}" CONTENT
            "include([==[${definition}]==])\nset(CASE_MANIFEST [==[${case_manifest}]==])\nset(CHECK_ID [==[${id}]==])\nset(OUTPUT_DIRECTORY [==[${output}]==])\n")
        add_test(NAME "${base}.${id}" COMMAND "${CMAKE_COMMAND}"
            "-DCHECK_MANIFEST=${check_manifest}" -P "${SIMDLIB_CODEGEN_SCRIPTS}/RunCase.cmake")
        set_tests_properties("${base}.${id}" PROPERTIES
            FIXTURES_REQUIRED "${base}.extracted" FIXTURES_SETUP "${base}.checked"
            LABELS "CODEGEN_CONTRACT;${CONFIGURATION};${id}")
        set_property(GLOBAL APPEND PROPERTY SIMDLIB_CODEGEN_ACTUAL "${base}.${id}")
        if(COMMAND simdlib_register_development_test)
            simdlib_register_development_test("${base}.${id}" OPTIMIZED_CODEGEN)
        endif()
    endforeach()
    file(GENERATE OUTPUT "${case_manifest}" CONTENT
        "set(CASE_ID [==[${P_ID}]==])\nset(SYMBOLS [==[${P_SYMBOLS}]==])\nset(CONSTANT_SYMBOLS [==[${P_CONSTANT_SYMBOLS}]==])\nset(OBJECT_FILE [==[$<TARGET_OBJECTS:${P_TARGET}>]==])\nset(INPUT_MANIFEST [==[${INPUT_MANIFEST}]==])\nset(BUILD_RECEIPT [==[${BUILD_RECEIPT}]==])\nset(CONFIGURATION_FILE [==[${CONFIGURATION_FILE}]==])\nset(ARTIFACT_DIRECTORY [==[${directory}]==])\nset(RESULT_FILES [==[${results}]==])\nset(LLVM_OBJDUMP [==[${SIMDLIB_LLVM_OBJDUMP}]==])\nset(LLVM_READOBJ [==[${SIMDLIB_LLVM_READOBJ}]==])\nset(FILECHECK [==[${SIMDLIB_FILECHECK}]==])\n")
    add_test(NAME "${base}.extract" COMMAND "${CMAKE_COMMAND}"
        "-DCASE_MANIFEST=${case_manifest}" -P "${SIMDLIB_CODEGEN_SCRIPTS}/PrepareCase.cmake")
    set_tests_properties("${base}.extract" PROPERTIES FIXTURES_SETUP "${base}.extracted"
        LABELS "CODEGEN_CONTRACT;${CONFIGURATION};extraction")
    add_test(NAME "${base}.qualified" COMMAND "${CMAKE_COMMAND}"
        "-DCASE_MANIFEST=${case_manifest}" -P "${SIMDLIB_CODEGEN_SCRIPTS}/VerifyCaseResults.cmake")
    set_tests_properties("${base}.qualified" PROPERTIES FIXTURES_REQUIRED "${base}.checked"
        LABELS "CODEGEN_CONTRACT;${CONFIGURATION};qualified")
    if(COMMAND simdlib_register_development_test)
        simdlib_register_development_test("${base}.extract" OPTIMIZED_CODEGEN)
        simdlib_register_development_test("${base}.qualified" OPTIMIZED_CODEGEN)
    endif()
endfunction()

# @brief Compares authored expected executions with actual CTest registrations.
# Call once after expanding the suite's independent coverage ledger; missing or
# incorrectly applicable supplements and missing primaries are configuration errors.
function(simdlib_finalize_codegen_cases)
    set(expected ${ARGN})
    set(unique_expected ${expected})
    list(REMOVE_DUPLICATES unique_expected)
    if(NOT expected OR NOT "${expected}" STREQUAL "${unique_expected}")
        message(FATAL_ERROR "Expected codegen coverage must be nonempty and unique")
    endif()
    get_property(actual GLOBAL PROPERTY SIMDLIB_CODEGEN_ACTUAL)
    list(SORT expected)
    list(SORT actual)
    if(NOT "${expected}" STREQUAL "${actual}")
        message(FATAL_ERROR "Codegen coverage mismatch\nExpected: ${expected}\nRegistered: ${actual}")
    endif()
    get_property(tests DIRECTORY PROPERTY TESTS)
    foreach(test IN LISTS expected)
        if(NOT test IN_LIST tests)
            message(FATAL_ERROR "Missing required CTest registration: ${test}")
        endif()
    endforeach()
    string(REPLACE ";" "\n" rows "${expected}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/codegen-expected.txt" "${rows}\n")
endfunction()
# endregion
