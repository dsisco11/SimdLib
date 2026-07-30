cmake_minimum_required(VERSION 4.4)

foreach(required_variable IN ITEMS
    MATRIX_FILE CELL_ID BUILD_DIRECTORY CMAKE_CTEST_COMMAND RESULT_FILE)
    if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR
            "AuditValidationInventory requires ${required_variable}")
    endif()
endforeach()

# @brief Reads one JSON array into a CMake list.
# @param output_variable Variable that receives the array values.
# @param json_document JSON document.
# @param path Remaining arguments that identify the array.
function(simdlib_read_json_array output_variable json_document)
    string(JSON item_count LENGTH "${json_document}" ${ARGN})
    set(items "")
    if(item_count GREATER 0)
        math(EXPR last_index "${item_count} - 1")
        foreach(item_index RANGE 0 ${last_index})
            string(JSON item GET "${json_document}" ${ARGN} ${item_index})
            list(APPEND items "${item}")
        endforeach()
    endif()
    set(${output_variable} "${items}" PARENT_SCOPE)
endfunction()

file(READ "${MATRIX_FILE}" matrix_json)
string(JSON matrix_schema GET "${matrix_json}" schema)
if(NOT matrix_schema STREQUAL "simdlib.validation-matrix.v1")
    message(FATAL_ERROR "Validation matrix has an unsupported schema")
endif()
string(JSON cell_type ERROR_VARIABLE cell_error
    TYPE "${matrix_json}" cells "${CELL_ID}")
if(cell_error OR NOT cell_type STREQUAL "OBJECT")
    message(FATAL_ERROR "Validation matrix does not define cell ${CELL_ID}")
endif()
string(JSON profile GET "${matrix_json}" cells "${CELL_ID}" profile)
simdlib_read_json_array(allowed_target_categories "${matrix_json}"
    profiles "${profile}" allowedTargetCategories)
simdlib_read_json_array(selected_target_categories "${matrix_json}"
    profiles "${profile}" selectedTargetCategories)
simdlib_read_json_array(allowed_test_owners "${matrix_json}"
    profiles "${profile}" allowedTestOwners)

set(target_ownership_file
    "${BUILD_DIRECTORY}/development-target-ownership.tsv")
if(NOT EXISTS "${target_ownership_file}")
    message(FATAL_ERROR
        "Generated target ownership inventory is missing: "
        "${target_ownership_file}")
endif()
file(STRINGS "${target_ownership_file}" target_rows)
list(POP_FRONT target_rows target_header)
if(NOT target_header STREQUAL
        "target\tcategory\towning_aggregate\tselected")
    message(FATAL_ERROR
        "Generated target ownership inventory has an invalid header")
endif()

set(target_names "")
set(selected_target_count 0)
foreach(target_row IN LISTS target_rows)
    if(NOT target_row MATCHES "^([^\t]+)\t([^\t]+)\t([^\t]+)\t(YES|NO)$")
        message(FATAL_ERROR
            "Unowned or malformed target inventory row: ${target_row}")
    endif()
    set(target_name "${CMAKE_MATCH_1}")
    set(target_category "${CMAKE_MATCH_2}")
    set(target_selected "${CMAKE_MATCH_4}")
    if(target_name IN_LIST target_names)
        message(FATAL_ERROR
            "Duplicate target ownership entry: ${target_name}")
    endif()
    list(APPEND target_names "${target_name}")
    if(NOT target_category IN_LIST allowed_target_categories)
        message(FATAL_ERROR
            "Target ${target_name} has unexpected profile membership "
            "${target_category} in ${profile}")
    endif()
    if(target_category IN_LIST selected_target_categories)
        if(NOT target_selected STREQUAL "YES")
            message(FATAL_ERROR
                "Target ${target_name} is omitted from its owning profile")
        endif()
        math(EXPR selected_target_count "${selected_target_count} + 1")
    elseif(NOT target_selected STREQUAL "NO")
        message(FATAL_ERROR
            "Target ${target_name} is selected from non-default category "
            "${target_category}")
    endif()
endforeach()
list(LENGTH target_names target_count)

if(DEFINED TEST_JSON_FILE AND NOT "${TEST_JSON_FILE}" STREQUAL "")
    file(READ "${TEST_JSON_FILE}" ctest_json)
else()
    set(ctest_arguments
        --test-dir "${BUILD_DIRECTORY}" --show-only=json-v1)
    if(DEFINED CONFIGURATION AND NOT "${CONFIGURATION}" STREQUAL "")
        list(APPEND ctest_arguments -C "${CONFIGURATION}")
    endif()
    execute_process(
        COMMAND "${CMAKE_CTEST_COMMAND}" ${ctest_arguments}
        RESULT_VARIABLE ctest_result
        OUTPUT_VARIABLE ctest_json
        ERROR_VARIABLE ctest_error)
    if(NOT ctest_result EQUAL 0)
        message(FATAL_ERROR
            "Unable to enumerate CTest ownership in ${BUILD_DIRECTORY}: "
            "${ctest_error}")
    endif()
endif()

string(JSON ctest_schema_major GET "${ctest_json}" version major)
if(NOT ctest_schema_major EQUAL 1)
    message(FATAL_ERROR "CTest inventory has an unsupported JSON schema")
endif()
string(JSON test_count LENGTH "${ctest_json}" tests)
set(test_names "")
set(test_index 0)
while(test_index LESS test_count)
    string(JSON test_name GET "${ctest_json}" tests ${test_index} name)
    if(test_name IN_LIST test_names)
        message(FATAL_ERROR "Duplicate CTest identity: ${test_name}")
    endif()
    list(APPEND test_names "${test_name}")

    set(test_owner_labels "")
    string(JSON property_count LENGTH
        "${ctest_json}" tests ${test_index} properties)
    set(property_index 0)
    while(property_index LESS property_count)
        string(JSON property_name GET
            "${ctest_json}" tests ${test_index} properties
            ${property_index} name)
        if(property_name STREQUAL "LABELS")
            simdlib_read_json_array(test_labels "${ctest_json}"
                tests ${test_index} properties ${property_index} value)
            foreach(test_label IN LISTS test_labels)
                if(test_label MATCHES "^SIMDLIB_OWNER_(.+)$")
                    list(APPEND test_owner_labels "${CMAKE_MATCH_1}")
                endif()
            endforeach()
        endif()
        math(EXPR property_index "${property_index} + 1")
    endwhile()
    list(REMOVE_DUPLICATES test_owner_labels)
    list(LENGTH test_owner_labels test_owner_count)
    if(NOT test_owner_count EQUAL 1)
        message(FATAL_ERROR
            "CTest ${test_name} has ${test_owner_count} validation owners")
    endif()
    list(GET test_owner_labels 0 test_owner)
    if(NOT test_owner IN_LIST allowed_test_owners)
        message(FATAL_ERROR
            "CTest ${test_name} has unexpected profile membership "
            "${test_owner} in ${profile}")
    endif()
    math(EXPR test_index "${test_index} + 1")
endwhile()

get_filename_component(result_directory "${RESULT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${result_directory}")
file(WRITE "${RESULT_FILE}"
    "{\n"
    "  \"schema\": \"simdlib.validation-inventory-audit.v1\",\n"
    "  \"status\": \"complete\",\n"
    "  \"cell\": \"${CELL_ID}\",\n"
    "  \"profile\": \"${profile}\",\n"
    "  \"targets\": ${target_count},\n"
    "  \"selectedTargets\": ${selected_target_count},\n"
    "  \"tests\": ${test_count}\n"
    "}\n")

message(STATUS
    "Validation inventory audit passed for ${CELL_ID}: "
    "${target_count} owned targets, ${selected_target_count} selected targets, "
    "${test_count} owned tests")
