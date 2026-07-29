cmake_minimum_required(VERSION 4.4)

foreach(required_variable IN ITEMS
    OWNERSHIP_FILE AGGREGATE_FILE MEMBERSHIP_FILE PROFILE SELECTED_CATEGORIES)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "Missing required variable ${required_variable}")
    endif()
endforeach()

if(NOT EXISTS "${OWNERSHIP_FILE}")
    message(FATAL_ERROR "Ownership inventory does not exist: ${OWNERSHIP_FILE}")
endif()
if(NOT EXISTS "${AGGREGATE_FILE}")
    message(FATAL_ERROR "Aggregate inventory does not exist: ${AGGREGATE_FILE}")
endif()
if(NOT EXISTS "${MEMBERSHIP_FILE}")
    message(FATAL_ERROR "Aggregate membership does not exist: ${MEMBERSHIP_FILE}")
endif()

file(STRINGS "${OWNERSHIP_FILE}" ownership_rows)
list(POP_FRONT ownership_rows ownership_header)
if(NOT ownership_header STREQUAL
        "target\tcategory\towning_aggregate\tselected")
    message(FATAL_ERROR "Ownership inventory has an invalid header")
endif()

set(previous_target "")
set(seen_targets "")
foreach(ownership_row IN LISTS ownership_rows)
    if(NOT ownership_row MATCHES
            "^([^\t]+)\t([^\t]+)\t([^\t]+)\t(YES|NO)$")
        message(FATAL_ERROR "Malformed ownership row: ${ownership_row}")
    endif()
    set(target "${CMAKE_MATCH_1}")
    set(category "${CMAKE_MATCH_2}")
    set(aggregate "${CMAKE_MATCH_3}")
    set(selected "${CMAKE_MATCH_4}")

    if(target IN_LIST seen_targets)
        message(FATAL_ERROR "Target ${target} occurs more than once")
    endif()
    if(previous_target AND target STRLESS previous_target)
        message(FATAL_ERROR "Ownership rows are not sorted deterministically")
    endif()
    if(NOT aggregate MATCHES "^(SimdLib.+Artifacts|BenchmarkArtifacts)$")
        message(FATAL_ERROR "Target ${target} has invalid aggregate ${aggregate}")
    endif()
    if(category IN_LIST SELECTED_CATEGORIES)
        if(NOT selected STREQUAL "YES")
            message(FATAL_ERROR
                "Profile ${PROFILE} failed to select ${target} from ${category}")
        endif()
    elseif(NOT selected STREQUAL "NO")
        message(FATAL_ERROR
            "Profile ${PROFILE} selected forbidden target ${target} from ${category}")
    endif()
    if(category STREQUAL "BENCHMARK" AND selected STREQUAL "YES")
        message(FATAL_ERROR "Benchmarks entered the default validation aggregate")
    endif()

    list(APPEND seen_targets "${target}")
    set(previous_target "${target}")
endforeach()

file(STRINGS "${AGGREGATE_FILE}" aggregate_rows)
list(POP_FRONT aggregate_rows aggregate_header)
if(NOT aggregate_header STREQUAL "aggregate\tcategory")
    message(FATAL_ERROR "Aggregate inventory has an invalid header")
endif()
set(required_aggregates
    ExhaustiveArtifacts
    SimdLibCompilerContractArtifacts
    SimdLibConstexprContractArtifacts
    SimdLibRuntimeValidationArtifacts
    SimdLibChecksValidationArtifacts
    SimdLibSmokeValidationArtifacts
    SimdLibOptimizedCodegenArtifacts
    SimdLibDebugDiagnosticArtifacts
    SimdLibSanitizerValidationArtifacts
    SimdLibCoverageValidationArtifacts
    SimdLibCoverageSupportArtifacts
    BenchmarkArtifacts)
foreach(required_aggregate IN LISTS required_aggregates)
    set(aggregate_matches ${aggregate_rows})
    list(FILTER aggregate_matches INCLUDE REGEX "^${required_aggregate}\t")
    list(LENGTH aggregate_matches aggregate_match_count)
    if(NOT aggregate_match_count EQUAL 1)
        message(FATAL_ERROR
            "Aggregate inventory does not contain exactly one ${required_aggregate} row")
    endif()
endforeach()

file(STRINGS "${MEMBERSHIP_FILE}" membership_rows)
list(POP_FRONT membership_rows membership_header)
if(NOT membership_header STREQUAL "aggregate\tdependency")
    message(FATAL_ERROR "Aggregate membership has an invalid header")
endif()
foreach(ownership_row IN LISTS ownership_rows)
    if(NOT ownership_row MATCHES
            "^([^\t]+)\t([^\t]+)\t([^\t]+)\t(YES|NO)$")
        message(FATAL_ERROR "Malformed ownership row: ${ownership_row}")
    endif()
    set(expected_membership "${CMAKE_MATCH_3}\t${CMAKE_MATCH_1}")
    list(FIND membership_rows "${expected_membership}" membership_index)
    if(membership_index EQUAL -1)
        message(FATAL_ERROR
            "Owning aggregate membership is missing: ${expected_membership}")
    endif()
endforeach()

foreach(forbidden_membership IN ITEMS
    "ExhaustiveArtifacts\tBenchmarkArtifacts"
    "SimdLibSanitizerValidationArtifacts\tSimdLibCompilerContractArtifacts"
    "SimdLibSanitizerValidationArtifacts\tSimdLibConstexprContractArtifacts"
    "SimdLibSanitizerValidationArtifacts\tSimdLibOptimizedCodegenArtifacts"
    "SimdLibSanitizerValidationArtifacts\tSimdLibDebugDiagnosticArtifacts"
    "SimdLibCoverageValidationArtifacts\tSimdLibCompilerContractArtifacts"
    "SimdLibCoverageValidationArtifacts\tSimdLibConstexprContractArtifacts"
    "SimdLibCoverageValidationArtifacts\tSimdLibOptimizedCodegenArtifacts"
    "SimdLibCoverageValidationArtifacts\tSimdLibDebugDiagnosticArtifacts")
    if(forbidden_membership IN_LIST membership_rows)
        message(FATAL_ERROR
            "Forbidden aggregate membership exists: ${forbidden_membership}")
    endif()
endforeach()

message(STATUS
    "Validated scoped artifact ownership for profile ${PROFILE}: ${OWNERSHIP_FILE}")
