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
