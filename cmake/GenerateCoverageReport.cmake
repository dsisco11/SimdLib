cmake_minimum_required(VERSION 3.31)

foreach(required_variable IN ITEMS
    BINARY_DIRECTORY
    SOURCE_DIRECTORY
    COVERAGE_MANIFEST
    LLVM_PROFDATA
    LLVM_COV
    LLVM_READOBJ)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} is required")
    endif()
endforeach()

# @brief Reverses the byte order of a hexadecimal GUID component.
# @param input_hex Even-length hexadecimal text.
# @param output_variable Caller-scope variable that receives the reversed text.
function(simdlib_reverse_hex_bytes input_hex output_variable)
    string(LENGTH "${input_hex}" input_length)
    math(EXPR byte_position "${input_length} - 2")
    set(reversed_hex "")
    while(byte_position GREATER_EQUAL 0)
        string(SUBSTRING "${input_hex}" ${byte_position} 2 byte_value)
        string(APPEND reversed_hex "${byte_value}")
        math(EXPR byte_position "${byte_position} - 2")
    endwhile()
    string(TOLOWER "${reversed_hex}" reversed_hex)
    set(${output_variable} "${reversed_hex}" PARENT_SCOPE)
endfunction()

if(NOT EXISTS "${COVERAGE_MANIFEST}")
    message(FATAL_ERROR "Coverage target manifest does not exist: ${COVERAGE_MANIFEST}")
endif()
file(STRINGS "${COVERAGE_MANIFEST}" coverage_manifest)
list(FILTER coverage_manifest EXCLUDE REGEX "^$")
if(NOT coverage_manifest)
    message(FATAL_ERROR "Coverage target manifest is empty: ${COVERAGE_MANIFEST}")
endif()

set(target_keys "")
set(seen_target_names "")
foreach(manifest_entry IN LISTS coverage_manifest)
    if(NOT manifest_entry MATCHES "^([^|]+)[|]([^|]+)[|](.+)$")
        message(FATAL_ERROR "Invalid coverage manifest entry: ${manifest_entry}")
    endif()
    set(target_name "${CMAKE_MATCH_1}")
    set(target_object "${CMAKE_MATCH_2}")
    set(target_prefix "${CMAKE_MATCH_3}")
    if(NOT EXISTS "${target_object}")
        message(FATAL_ERROR
            "Coverage object for ${target_name} does not exist: ${target_object}")
    endif()
    if(target_name IN_LIST seen_target_names)
        message(FATAL_ERROR
            "Coverage manifest contains duplicate target ${target_name}")
    endif()
    list(APPEND seen_target_names "${target_name}")
    string(SHA256 target_key "${target_name}")
    list(APPEND target_keys "${target_key}")
    set(target_name_${target_key} "${target_name}")
    set(target_object_${target_key} "${target_object}")
    set(target_prefix_${target_key} "${target_prefix}")
    set(target_profiles_${target_key} "")

    execute_process(
        COMMAND "${LLVM_READOBJ}" --coff-debug-directory --notes "${target_object}"
        RESULT_VARIABLE read_object_result
        OUTPUT_VARIABLE read_object_output
        ERROR_VARIABLE read_object_error)
    if(NOT read_object_result EQUAL 0)
        message(FATAL_ERROR
            "Cannot read coverage object identity for ${target_name}: ${read_object_error}")
    endif()
    if(read_object_output MATCHES
        "PDBGUID: [{]([0-9A-Fa-f]+)-([0-9A-Fa-f]+)-([0-9A-Fa-f]+)-([0-9A-Fa-f]+)-([0-9A-Fa-f]+)[}]")
        simdlib_reverse_hex_bytes("${CMAKE_MATCH_1}" guid_part_1)
        simdlib_reverse_hex_bytes("${CMAKE_MATCH_2}" guid_part_2)
        simdlib_reverse_hex_bytes("${CMAKE_MATCH_3}" guid_part_3)
        string(TOLOWER "${CMAKE_MATCH_4}${CMAKE_MATCH_5}" guid_tail)
        set(target_binary_id_${target_key}
            "${guid_part_1}${guid_part_2}${guid_part_3}${guid_tail}")
    elseif(read_object_output MATCHES "Build ID: ([0-9A-Fa-f]+)")
        string(TOLOWER "${CMAKE_MATCH_1}" target_binary_id_${target_key})
    else()
        message(FATAL_ERROR
            "Coverage object has no supported PDB or ELF build identity: ${target_object}")
    endif()
endforeach()

set(seen_binary_ids "")
foreach(target_key IN LISTS target_keys)
    set(target_binary_id "${target_binary_id_${target_key}}")
    if(target_binary_id IN_LIST seen_binary_ids)
        message(FATAL_ERROR
            "Coverage manifest maps more than one executable to binary identity ${target_binary_id}")
    endif()
    list(APPEND seen_binary_ids "${target_binary_id}")
endforeach()

file(GLOB_RECURSE coverage_profiles LIST_DIRECTORIES FALSE
    "${BINARY_DIRECTORY}/*.profraw"
    "${BINARY_DIRECTORY}/*.profdata")
list(FILTER coverage_profiles EXCLUDE REGEX "[/\\\\]coverage-work[/\\\\]")
list(FILTER coverage_profiles EXCLUDE REGEX "[/\\\\]coverage[.]profdata$")
if(NOT coverage_profiles)
    message(FATAL_ERROR
        "No LLVM profiles were found. Run CTest with LLVM coverage enabled first.")
endif()
list(SORT coverage_profiles)

set(assigned_profile_count 0)
set(excluded_profile_count 0)
foreach(coverage_profile IN LISTS coverage_profiles)
    get_filename_component(profile_name "${coverage_profile}" NAME)
    if(profile_name MATCHES "^probe-[0-9]+[.]profraw$")
        math(EXPR excluded_profile_count "${excluded_profile_count} + 1")
        continue()
    endif()

    execute_process(
        COMMAND "${LLVM_PROFDATA}" show --binary-ids "${coverage_profile}"
        RESULT_VARIABLE profile_show_result
        OUTPUT_VARIABLE profile_show_output
        ERROR_VARIABLE profile_show_error)
    if(NOT profile_show_result EQUAL 0)
        message(FATAL_ERROR
            "Cannot read profile identity for ${coverage_profile}: ${profile_show_error}")
    endif()
    string(REPLACE "\r\n" "\n" profile_show_output "${profile_show_output}")
    string(REPLACE "\n" ";" profile_show_lines "${profile_show_output}")
    set(profile_binary_ids "")
    foreach(profile_show_line IN LISTS profile_show_lines)
        string(STRIP "${profile_show_line}" profile_show_line)
        if(profile_show_line MATCHES "^[0-9A-Fa-f]+$")
            string(LENGTH "${profile_show_line}" profile_id_length)
            if(profile_id_length GREATER_EQUAL 32)
                string(TOLOWER "${profile_show_line}" profile_binary_id)
                list(APPEND profile_binary_ids "${profile_binary_id}")
            endif()
        endif()
    endforeach()
    list(REMOVE_DUPLICATES profile_binary_ids)
    list(LENGTH profile_binary_ids profile_binary_id_count)
    if(profile_binary_id_count GREATER 1)
        math(EXPR excluded_profile_count "${excluded_profile_count} + 1")
        continue()
    elseif(profile_binary_id_count EQUAL 0)
        message(FATAL_ERROR
            "Profile has no embedded executable identity: ${coverage_profile}")
    endif()
    list(GET profile_binary_ids 0 profile_binary_id)

    set(matching_target_keys "")
    foreach(target_key IN LISTS target_keys)
        if(profile_binary_id STREQUAL target_binary_id_${target_key})
            list(APPEND matching_target_keys "${target_key}")
        endif()
    endforeach()
    list(LENGTH matching_target_keys matching_target_count)
    if(matching_target_count EQUAL 0)
        message(FATAL_ERROR
            "Profile binary identity has no executable provenance mapping: ${coverage_profile} (${profile_binary_id})")
    elseif(matching_target_count GREATER 1)
        message(FATAL_ERROR
            "Profile binary identity matches more than one executable: ${coverage_profile}")
    endif()
    list(GET matching_target_keys 0 target_key)
    string(FIND "${profile_name}" "${target_prefix_${target_key}}" prefix_position)
    if(NOT profile_name MATCHES "^ctest-[0-9]+-" AND NOT prefix_position EQUAL 0)
        message(FATAL_ERROR
            "Profile filename prefix disagrees with its embedded executable identity: ${coverage_profile}")
    endif()
    list(APPEND target_profiles_${target_key} "${coverage_profile}")
    math(EXPR assigned_profile_count "${assigned_profile_count} + 1")
endforeach()

file(GLOB_RECURSE coverage_sources LIST_DIRECTORIES FALSE
    "${SOURCE_DIRECTORY}/include/SimdLib/*.h")
if(NOT coverage_sources)
    message(FATAL_ERROR "No SimdLib public headers were found for coverage")
endif()

set(coverage_work_directory "${BINARY_DIRECTORY}/coverage-work")
file(REMOVE_RECURSE "${coverage_work_directory}")
file(MAKE_DIRECTORY "${coverage_work_directory}")
set(object_trace_files "")
string(CONCAT coverage_provenance
    "schema\tsimdlib-coverage-provenance-v1\n"
    "merge_scope\tper-executable\n"
    "constexpr_evidence\texcluded\n"
    "target\tbinary_id\tprofile_count\tprofile_prefix\texecutable\n")
foreach(target_key IN LISTS target_keys)
    set(target_name "${target_name_${target_key}}")
    set(target_profiles "${target_profiles_${target_key}}")
    if(NOT target_profiles)
        message(FATAL_ERROR
            "Coverage object has no matching CTest profile after reset/run: ${target_name} (${target_object_${target_key}})")
    endif()

    set(profile_response_file
        "${coverage_work_directory}/${target_name}-profiles.rsp")
    file(WRITE "${profile_response_file}" "")
    foreach(target_profile IN LISTS target_profiles)
        file(APPEND "${profile_response_file}" "\"${target_profile}\"\n")
    endforeach()
    set(target_merged_profile
        "${coverage_work_directory}/${target_name}.profdata")
    execute_process(
        COMMAND "${LLVM_PROFDATA}" merge -sparse "@${profile_response_file}"
            -o "${target_merged_profile}"
        RESULT_VARIABLE merge_result
        ERROR_VARIABLE merge_error)
    if(NOT merge_result EQUAL 0)
        message(FATAL_ERROR
            "Failed to merge profiles assigned to ${target_name}: ${merge_error}")
    endif()

    set(target_trace_file
        "${coverage_work_directory}/${target_name}.info")
    execute_process(
        COMMAND "${LLVM_COV}" export
            "${target_object_${target_key}}"
            "-instr-profile=${target_merged_profile}"
            -format=lcov
            -sources ${coverage_sources}
        RESULT_VARIABLE export_result
        OUTPUT_FILE "${target_trace_file}"
        ERROR_VARIABLE export_error)
    string(STRIP "${export_error}" export_error)
    if(NOT export_result EQUAL 0 OR export_error)
        message(FATAL_ERROR
            "Profile/object validation failed for ${target_name}: ${export_error}")
    endif()
    file(READ "${target_trace_file}" target_trace)
    if(NOT target_trace MATCHES "(^|\n)SF:")
        message(FATAL_ERROR
            "Profile/object pair produced no usable source records for ${target_name}")
    endif()
    list(APPEND object_trace_files "${target_trace_file}")

    list(LENGTH target_profiles target_profile_count)
    string(APPEND coverage_provenance
        "${target_name}\t${target_binary_id_${target_key}}\t${target_profile_count}\t${target_prefix_${target_key}}\t${target_object_${target_key}}\n")
    message(STATUS
        "Mapped ${target_profile_count} profiles to ${target_name}")
endforeach()

set(TRACE_FILES "${object_trace_files}")
set(OUTPUT_FILE "${BINARY_DIRECTORY}/coverage.info")
include("${CMAKE_CURRENT_LIST_DIR}/MergeLcov.cmake")

set(coverage_provenance_file
    "${BINARY_DIRECTORY}/coverage-provenance.tsv")
set(coverage_provenance_temporary_file
    "${coverage_provenance_file}.tmp")
file(WRITE "${coverage_provenance_temporary_file}" "${coverage_provenance}")
file(RENAME "${coverage_provenance_temporary_file}"
    "${coverage_provenance_file}")

list(LENGTH target_keys target_count)
message(STATUS
    "Generated ${OUTPUT_FILE} and ${coverage_provenance_file} from ${assigned_profile_count} profiles mapped to ${target_count} executables; excluded ${excluded_profile_count} multi-executable/tool profiles")
