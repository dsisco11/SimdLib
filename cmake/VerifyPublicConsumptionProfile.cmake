cmake_minimum_required(VERSION 3.31)

foreach(required_variable IN ITEMS
    OWNERSHIP_FILE CONSUMER_TARGET_FILE PROFILE REGISTER_SUPPORTED)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "Missing required variable ${required_variable}")
    endif()
endforeach()

foreach(required_file IN ITEMS OWNERSHIP_FILE CONSUMER_TARGET_FILE)
    if(NOT EXISTS "${${required_file}}")
        message(FATAL_ERROR
            "Public-consumption inventory does not exist: ${${required_file}}")
    endif()
endforeach()

set(expected_smoke_targets
    ApiExamples
    FormatOdr
    HeaderOnlySmoke)
set(expected_consumer_targets CoreConsumerSmoke)
if(REGISTER_SUPPORTED)
    list(APPEND expected_smoke_targets
		InstalledPackagePartialRegisterConsumerArtifacts
		PartialRegisterOdrAvx2
		PartialRegisterOdrSse42
        RegisterExamples
        RegisterOdr)
    list(APPEND expected_consumer_targets RegisterConsumerSmoke)
endif()
list(SORT expected_smoke_targets)
list(SORT expected_consumer_targets)

file(STRINGS "${OWNERSHIP_FILE}" ownership_rows)
list(POP_FRONT ownership_rows ownership_header)
if(NOT ownership_header STREQUAL
        "target\tcategory\towning_aggregate\tselected")
    message(FATAL_ERROR "Ownership inventory has an invalid header")
endif()

set(actual_smoke_targets "")
set(selected_smoke_targets "")
foreach(ownership_row IN LISTS ownership_rows)
    if(NOT ownership_row MATCHES
            "^([^\t]+)\t([^\t]+)\t([^\t]+)\t(YES|NO)$")
        message(FATAL_ERROR "Malformed ownership row: ${ownership_row}")
    endif()
    if(CMAKE_MATCH_2 STREQUAL "SMOKE_VALIDATION")
        list(APPEND actual_smoke_targets "${CMAKE_MATCH_1}")
        if(CMAKE_MATCH_4 STREQUAL "YES")
            list(APPEND selected_smoke_targets "${CMAKE_MATCH_1}")
        endif()
    endif()
endforeach()
list(SORT actual_smoke_targets)
list(SORT selected_smoke_targets)

if(PROFILE STREQUAL "RELEASE")
    if(NOT actual_smoke_targets STREQUAL expected_smoke_targets)
        message(FATAL_ERROR
            "Release public-surface targets differ from their compiler contract: "
            "expected '${expected_smoke_targets}', received '${actual_smoke_targets}'")
    endif()
    if(NOT selected_smoke_targets STREQUAL expected_smoke_targets)
        message(FATAL_ERROR
            "Release did not select every public-surface target: "
            "${selected_smoke_targets}")
    endif()
elseif(NOT PROFILE STREQUAL "CUSTOM" AND (actual_smoke_targets OR selected_smoke_targets))
    message(FATAL_ERROR
        "Profile ${PROFILE} configured Release-owned public-surface targets: "
        "${actual_smoke_targets}")
endif()

file(STRINGS "${CONSUMER_TARGET_FILE}" actual_consumer_targets)
list(SORT actual_consumer_targets)
if(NOT actual_consumer_targets STREQUAL expected_consumer_targets)
    message(FATAL_ERROR
        "External-consumer capability inventory differs from compiler support: "
        "expected '${expected_consumer_targets}', received "
        "'${actual_consumer_targets}'")
endif()

message(STATUS
    "Validated public-consumption ownership for profile ${PROFILE}")
