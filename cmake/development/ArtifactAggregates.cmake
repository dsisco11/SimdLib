include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "ArtifactAggregates.cmake is available only to top-level SimdLib builds")
endif()

block(SCOPE_FOR VARIABLES)

# @brief Collects every build-system target declared by project-owned directories.
# @param directory Configured directory whose targets and children are inspected.
# @param output_variable Variable that receives the recursively collected targets.
function(simdlib_collect_project_targets directory output_variable)
    get_property(directory_targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    set(collected_targets ${directory_targets})

    get_property(child_directories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(child_directory IN LISTS child_directories)
        get_property(child_source_directory DIRECTORY "${child_directory}" PROPERTY SOURCE_DIR)
        cmake_path(IS_PREFIX CMAKE_SOURCE_DIR "${child_source_directory}"
            NORMALIZE child_is_project_owned)
        cmake_path(RELATIVE_PATH child_source_directory
            BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE child_source_relative)
        if(child_source_relative MATCHES
                "^(out|_deps|\\.git)(/|$)|^build($|[-_/])")
            set(child_is_project_owned FALSE)
        endif()
        if(child_is_project_owned)
            simdlib_collect_project_targets("${child_directory}" child_targets)
            list(APPEND collected_targets ${child_targets})
        endif()
    endforeach()

    set(${output_variable} ${collected_targets} PARENT_SCOPE)
endfunction()

# @brief Adds a globally unique aggregate for one validation category.
# @param aggregate Target name used by build profiles.
# @param category Sole target category owned by the aggregate.
function(simdlib_add_category_aggregate aggregate category)
    add_custom_target(${aggregate})
    set(category_targets ${simdlib_targets_${category}})
    if(category_targets)
        add_dependencies(${aggregate} ${category_targets})
    endif()
    set_property(TARGET ${aggregate} PROPERTY
        SIMDLIB_AGGREGATE_CATEGORY ${category})
endfunction()

set(simdlib_category_aggregate_COMPILER_CONTRACT
    SimdLibCompilerContractArtifacts)
set(simdlib_category_aggregate_CONSTEXPR_CONTRACT
    SimdLibConstexprContractArtifacts)
set(simdlib_category_aggregate_RUNTIME_VALIDATION
    SimdLibRuntimeValidationArtifacts)
set(simdlib_category_aggregate_CHECKS_VALIDATION
    SimdLibChecksValidationArtifacts)
set(simdlib_category_aggregate_SMOKE_VALIDATION
    SimdLibSmokeValidationArtifacts)
set(simdlib_category_aggregate_OPTIMIZED_CODEGEN
    SimdLibOptimizedCodegenArtifacts)
set(simdlib_category_aggregate_DEBUG_DIAGNOSTIC
    SimdLibDebugDiagnosticArtifacts)
set(simdlib_category_aggregate_COVERAGE_SUPPORT
    SimdLibCoverageSupportArtifacts)
set(simdlib_category_aggregate_BENCHMARK
    BenchmarkArtifacts)

set(simdlib_profile_allowed_CUSTOM ${SIMDLIB_VALIDATION_CATEGORIES})
set(simdlib_profile_selected_CUSTOM
    COMPILER_CONTRACT CONSTEXPR_CONTRACT
    RUNTIME_VALIDATION CHECKS_VALIDATION SMOKE_VALIDATION
    OPTIMIZED_CODEGEN DEBUG_DIAGNOSTIC)
set(simdlib_profile_allowed_RELEASE
    COMPILER_CONTRACT CONSTEXPR_CONTRACT
    RUNTIME_VALIDATION CHECKS_VALIDATION SMOKE_VALIDATION
    OPTIMIZED_CODEGEN BENCHMARK)
set(simdlib_profile_selected_RELEASE
    COMPILER_CONTRACT CONSTEXPR_CONTRACT
    RUNTIME_VALIDATION CHECKS_VALIDATION SMOKE_VALIDATION
    OPTIMIZED_CODEGEN)
set(simdlib_profile_allowed_DEBUG
    RUNTIME_VALIDATION CHECKS_VALIDATION)
set(simdlib_profile_selected_DEBUG ${simdlib_profile_allowed_DEBUG})
set(simdlib_profile_allowed_SANITIZER
    RUNTIME_VALIDATION CHECKS_VALIDATION)
set(simdlib_profile_selected_SANITIZER ${simdlib_profile_allowed_SANITIZER})
set(simdlib_profile_allowed_COVERAGE
    RUNTIME_VALIDATION CHECKS_VALIDATION COVERAGE_SUPPORT)
set(simdlib_profile_selected_COVERAGE
    RUNTIME_VALIDATION CHECKS_VALIDATION)
set(simdlib_profile_allowed_CODEGEN_DIAGNOSTIC
    DEBUG_DIAGNOSTIC)
set(simdlib_profile_selected_CODEGEN_DIAGNOSTIC
    ${simdlib_profile_allowed_CODEGEN_DIAGNOSTIC})
set(simdlib_profile_allowed_COMPILER_CONTRACTS
    COMPILER_CONTRACT)
set(simdlib_profile_selected_COMPILER_CONTRACTS
    ${simdlib_profile_allowed_COMPILER_CONTRACTS})

set(simdlib_allowed_categories
    ${simdlib_profile_allowed_${SIMDLIB_VALIDATION_PROFILE}})
set(simdlib_selected_categories
    ${simdlib_profile_selected_${SIMDLIB_VALIDATION_PROFILE}})
if(NOT simdlib_allowed_categories)
    message(FATAL_ERROR
        "No artifact ownership contract exists for profile "
        "${SIMDLIB_VALIDATION_PROFILE}")
endif()

if(SIMDLIB_BUILD_REGISTER_CODEGEN_GATES AND
        SIMDLIB_REGISTER_CODEGEN_MODE STREQUAL "OFF")
    message(FATAL_ERROR
        "Register generated-code targets require ENFORCE or RECORD policy")
elseif(NOT SIMDLIB_BUILD_REGISTER_CODEGEN_GATES AND
        NOT SIMDLIB_REGISTER_CODEGEN_MODE STREQUAL "OFF")
    message(FATAL_ERROR
        "Register generated-code policy must be OFF when its targets are disabled")
endif()

if(SIMDLIB_VALIDATION_PROFILE STREQUAL "RELEASE")
    foreach(simdlib_release_contract_option IN ITEMS
        SIMDLIB_BUILD_CONFIGURATION_PROBES
        SIMDLIB_BUILD_CONSTEXPR_PROBES
        SIMDLIB_BUILD_HEADER_PROBES
        SIMDLIB_BUILD_METHOD_FLAGS_CODEGEN_GATES)
        if(NOT ${simdlib_release_contract_option})
            message(FATAL_ERROR
                "Release validation requires ${simdlib_release_contract_option}=ON")
        endif()
    endforeach()
    if(NOT SIMDLIB_DEFAULT_CHECKS_PROBE STREQUAL "RELEASE")
        message(FATAL_ERROR
            "Release validation requires SIMDLIB_DEFAULT_CHECKS_PROBE=RELEASE")
    endif()
    if(SIMDLIB_REGISTER_COMPILER_SUPPORTED)
        if(NOT SIMDLIB_BUILD_REGISTER_CODEGEN_GATES OR
                NOT SIMDLIB_REGISTER_CODEGEN_MODE STREQUAL "ENFORCE")
            message(FATAL_ERROR
                "Register-capable Release validation requires enforced "
                "Register generated-code gates")
        endif()
    elseif(SIMDLIB_BUILD_REGISTER_CODEGEN_GATES OR
            NOT SIMDLIB_REGISTER_CODEGEN_MODE STREQUAL "OFF")
        message(FATAL_ERROR
            "Core-only Release validation cannot enable Register codegen")
    endif()
elseif(SIMDLIB_VALIDATION_PROFILE STREQUAL "COMPILER_CONTRACTS")
    if(NOT SIMDLIB_BUILD_CONFIGURATION_PROBES OR
            NOT SIMDLIB_BUILD_HEADER_PROBES)
        message(FATAL_ERROR
            "Compiler-contract validation requires configuration and header probes")
    endif()
    if(NOT SIMDLIB_DEFAULT_CHECKS_PROBE STREQUAL "RELEASE")
        message(FATAL_ERROR
            "Compiler-contract validation requires the Release default-checks probe")
    endif()
    if(SIMDLIB_BUILD_METHOD_FLAGS_CODEGEN_GATES)
        message(FATAL_ERROR
            "Compiler-contract validation excludes method-flags generated-code gates")
    endif()
elseif(SIMDLIB_VALIDATION_PROFILE MATCHES
        "^(DEBUG|SANITIZER|COVERAGE|CODEGEN_DIAGNOSTIC)$")
    foreach(simdlib_forbidden_contract_option IN ITEMS
        SIMDLIB_BUILD_CONFIGURATION_PROBES
        SIMDLIB_BUILD_CONSTEXPR_PROBES
        SIMDLIB_BUILD_HEADER_PROBES
        SIMDLIB_BUILD_METHOD_FLAGS_CODEGEN_GATES)
        if(${simdlib_forbidden_contract_option})
            message(FATAL_ERROR
                "Validation profile ${SIMDLIB_VALIDATION_PROFILE} excludes "
                "${simdlib_forbidden_contract_option}")
        endif()
    endforeach()
    foreach(simdlib_forbidden_public_surface_option IN ITEMS
        SIMDLIB_BUILD_EXAMPLES
        SIMDLIB_BUILD_SMOKE_TESTS)
        if(${simdlib_forbidden_public_surface_option})
            message(FATAL_ERROR
                "Validation profile ${SIMDLIB_VALIDATION_PROFILE} excludes "
                "${simdlib_forbidden_public_surface_option}")
        endif()
    endforeach()
    if(SIMDLIB_VALIDATION_PROFILE MATCHES "^(DEBUG|SANITIZER)$" AND
            NOT SIMDLIB_DEFAULT_CHECKS_PROBE STREQUAL "DEBUG")
        message(FATAL_ERROR
            "Validation profile ${SIMDLIB_VALIDATION_PROFILE} requires the "
            "checks-enabled Debug state probe")
    endif()
    if(SIMDLIB_VALIDATION_PROFILE STREQUAL "CODEGEN_DIAGNOSTIC")
        if(NOT SIMDLIB_BUILD_REGISTER_CODEGEN_GATES OR
                NOT SIMDLIB_REGISTER_CODEGEN_MODE STREQUAL "RECORD")
            message(FATAL_ERROR
                "Diagnostic codegen requires record-only Register generated-code targets")
        endif()
    elseif(SIMDLIB_BUILD_REGISTER_CODEGEN_GATES OR
            NOT SIMDLIB_REGISTER_CODEGEN_MODE STREQUAL "OFF")
        message(FATAL_ERROR
            "Validation profile ${SIMDLIB_VALIDATION_PROFILE} excludes "
            "Register generated-code targets and policy")
    endif()
endif()

simdlib_collect_project_targets("${CMAKE_CURRENT_SOURCE_DIR}"
    simdlib_development_targets)
list(REMOVE_DUPLICATES simdlib_development_targets)
list(FILTER simdlib_development_targets EXCLUDE
    REGEX "^(Continuous|Experimental|Nightly)")
list(SORT simdlib_development_targets)

set(simdlib_owned_targets "")
foreach(simdlib_development_target IN LISTS simdlib_development_targets)
    get_target_property(simdlib_development_target_type
        ${simdlib_development_target} TYPE)
    if(simdlib_development_target_type STREQUAL "INTERFACE_LIBRARY")
        continue()
    endif()

    get_target_property(simdlib_target_category
        ${simdlib_development_target} SIMDLIB_VALIDATION_CATEGORY)
    if(NOT simdlib_target_category)
        message(FATAL_ERROR
            "Development target ${simdlib_development_target} has no "
            "validation-category owner")
    endif()
    if(NOT simdlib_target_category IN_LIST simdlib_allowed_categories)
        message(FATAL_ERROR
            "Development target ${simdlib_development_target} belongs to "
            "${simdlib_target_category}, which is excluded by validation "
            "profile ${SIMDLIB_VALIDATION_PROFILE}")
    endif()

    list(APPEND simdlib_targets_${simdlib_target_category}
        ${simdlib_development_target})
    list(APPEND simdlib_owned_targets ${simdlib_development_target})
endforeach()

foreach(simdlib_category IN LISTS SIMDLIB_VALIDATION_CATEGORIES)
    list(SORT simdlib_targets_${simdlib_category})
    simdlib_add_category_aggregate(
        ${simdlib_category_aggregate_${simdlib_category}}
        ${simdlib_category})
    get_target_property(simdlib_aggregate_dependencies
        ${simdlib_category_aggregate_${simdlib_category}}
        MANUALLY_ADDED_DEPENDENCIES)
    if(NOT simdlib_aggregate_dependencies)
        set(simdlib_aggregate_dependencies "")
    endif()
    list(SORT simdlib_aggregate_dependencies)
    if(NOT "${simdlib_aggregate_dependencies}" STREQUAL
            "${simdlib_targets_${simdlib_category}}")
        message(FATAL_ERROR
            "Aggregate ${simdlib_category_aggregate_${simdlib_category}} "
            "does not exactly own category ${simdlib_category}")
    endif()
endforeach()

add_custom_target(SimdLibSanitizerValidationArtifacts)
add_dependencies(SimdLibSanitizerValidationArtifacts
    SimdLibRuntimeValidationArtifacts
    SimdLibChecksValidationArtifacts)

add_custom_target(SimdLibCoverageValidationArtifacts)
add_dependencies(SimdLibCoverageValidationArtifacts
    SimdLibRuntimeValidationArtifacts
    SimdLibChecksValidationArtifacts)

add_custom_target(ExhaustiveArtifacts)
if(SIMDLIB_VALIDATION_PROFILE STREQUAL "SANITIZER")
    add_dependencies(ExhaustiveArtifacts
        SimdLibSanitizerValidationArtifacts)
elseif(SIMDLIB_VALIDATION_PROFILE STREQUAL "COVERAGE")
    add_dependencies(ExhaustiveArtifacts
        SimdLibCoverageValidationArtifacts)
else()
    foreach(simdlib_selected_category IN LISTS simdlib_selected_categories)
        add_dependencies(ExhaustiveArtifacts
            ${simdlib_category_aggregate_${simdlib_selected_category}})
    endforeach()
endif()

if(SIMDLIB_VALIDATE_EXHAUSTIVE_TARGETS)
    set(simdlib_required_nonempty_categories ${simdlib_selected_categories})
    if(SIMDLIB_VALIDATION_PROFILE STREQUAL "RELEASE")
        list(APPEND simdlib_required_nonempty_categories BENCHMARK)
    endif()
    foreach(simdlib_required_category IN LISTS simdlib_required_nonempty_categories)
        if(NOT simdlib_targets_${simdlib_required_category})
            message(FATAL_ERROR
                "Validation profile ${SIMDLIB_VALIDATION_PROFILE} requires "
                "at least one ${simdlib_required_category} target")
        endif()
    endforeach()
endif()

set(simdlib_ownership_rows
    "target\tcategory\towning_aggregate\tselected")
set(simdlib_profile_targets "")
foreach(simdlib_owned_target IN LISTS simdlib_owned_targets)
    get_target_property(simdlib_target_category
        ${simdlib_owned_target} SIMDLIB_VALIDATION_CATEGORY)
    if(simdlib_target_category IN_LIST simdlib_selected_categories)
        set(simdlib_target_selected YES)
        list(APPEND simdlib_profile_targets ${simdlib_owned_target})
    else()
        set(simdlib_target_selected NO)
    endif()
    list(APPEND simdlib_ownership_rows
        "${simdlib_owned_target}\t${simdlib_target_category}\t${simdlib_category_aggregate_${simdlib_target_category}}\t${simdlib_target_selected}")
endforeach()
list(SORT simdlib_profile_targets)

set(simdlib_aggregate_targets
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
list(SORT simdlib_aggregate_targets)

set(simdlib_inventory_targets
    ${simdlib_development_targets} ${simdlib_aggregate_targets})
list(REMOVE_DUPLICATES simdlib_inventory_targets)
list(SORT simdlib_inventory_targets)
string(REPLACE ";" "\n" simdlib_development_target_inventory
    "${simdlib_inventory_targets}")
file(WRITE "${CMAKE_BINARY_DIR}/development-targets.txt"
    "${simdlib_development_target_inventory}\n")
string(REPLACE ";" "\n" simdlib_profile_target_inventory
    "${simdlib_profile_targets}")
file(WRITE "${CMAKE_BINARY_DIR}/development-profile-targets.txt"
    "${simdlib_profile_target_inventory}\n")
string(REPLACE ";" "\n" simdlib_ownership_inventory
    "${simdlib_ownership_rows}")
file(WRITE "${CMAKE_BINARY_DIR}/development-target-ownership.tsv"
    "${simdlib_ownership_inventory}\n")

set(simdlib_compiler_contract_rows
    "target\tcompile_definitions\tcompile_options\tlink_options\tcxx_standard")
set(simdlib_compiler_contract_source_rows "target\tsource")
foreach(simdlib_compiler_contract_target IN LISTS simdlib_targets_COMPILER_CONTRACT)
    set(simdlib_contract_property_row "${simdlib_compiler_contract_target}")
    foreach(simdlib_contract_property IN ITEMS
        COMPILE_DEFINITIONS COMPILE_OPTIONS LINK_OPTIONS CXX_STANDARD)
        get_target_property(simdlib_contract_property_value
            ${simdlib_compiler_contract_target} ${simdlib_contract_property})
        if(NOT simdlib_contract_property_value)
            set(simdlib_contract_property_value "")
        endif()
        string(REPLACE ";" "," simdlib_contract_property_value
            "${simdlib_contract_property_value}")
        string(REPLACE "\t" " " simdlib_contract_property_value
            "${simdlib_contract_property_value}")
        string(APPEND simdlib_contract_property_row
            "\t${simdlib_contract_property_value}")
    endforeach()
    list(APPEND simdlib_compiler_contract_rows
        "${simdlib_contract_property_row}")

    get_target_property(simdlib_contract_sources
        ${simdlib_compiler_contract_target} SOURCES)
    get_target_property(simdlib_contract_source_directory
        ${simdlib_compiler_contract_target} SOURCE_DIR)
    if(simdlib_contract_sources)
        foreach(simdlib_contract_source IN LISTS simdlib_contract_sources)
            if(simdlib_contract_source MATCHES "^\\$<")
                message(FATAL_ERROR
                    "Compiler-contract target ${simdlib_compiler_contract_target} "
                    "uses a generated source expression")
            endif()
            cmake_path(ABSOLUTE_PATH simdlib_contract_source
                BASE_DIRECTORY "${simdlib_contract_source_directory}"
                NORMALIZE OUTPUT_VARIABLE simdlib_contract_source_absolute)
            list(APPEND simdlib_compiler_contract_source_rows
                "${simdlib_compiler_contract_target}\t${simdlib_contract_source_absolute}")
        endforeach()
    endif()
endforeach()
string(REPLACE ";" "\n" simdlib_compiler_contract_inventory
    "${simdlib_compiler_contract_rows}")
file(WRITE "${CMAKE_BINARY_DIR}/compiler-contract-properties.tsv"
    "${simdlib_compiler_contract_inventory}\n")
string(REPLACE ";" "\n" simdlib_compiler_contract_source_inventory
    "${simdlib_compiler_contract_source_rows}")
file(WRITE "${CMAKE_BINARY_DIR}/compiler-contract-sources.tsv"
    "${simdlib_compiler_contract_source_inventory}\n")

set(simdlib_checks_contract_rows "target\tcompile_definitions\tsources")
foreach(simdlib_checks_contract_target IN LISTS simdlib_targets_CHECKS_VALIDATION)
    get_target_property(simdlib_checks_contract_definitions
        ${simdlib_checks_contract_target} COMPILE_DEFINITIONS)
    if(NOT simdlib_checks_contract_definitions)
        set(simdlib_checks_contract_definitions "")
    endif()
    string(REPLACE ";" "," simdlib_checks_contract_definitions
        "${simdlib_checks_contract_definitions}")

    get_target_property(simdlib_checks_contract_sources
        ${simdlib_checks_contract_target} SOURCES)
    get_target_property(simdlib_checks_contract_source_directory
        ${simdlib_checks_contract_target} SOURCE_DIR)
    set(simdlib_checks_contract_absolute_sources "")
    foreach(simdlib_checks_contract_source IN LISTS simdlib_checks_contract_sources)
        cmake_path(ABSOLUTE_PATH simdlib_checks_contract_source
            BASE_DIRECTORY "${simdlib_checks_contract_source_directory}"
            NORMALIZE OUTPUT_VARIABLE simdlib_checks_contract_source_absolute)
        list(APPEND simdlib_checks_contract_absolute_sources
            "${simdlib_checks_contract_source_absolute}")
    endforeach()
    string(REPLACE ";" "," simdlib_checks_contract_absolute_sources
        "${simdlib_checks_contract_absolute_sources}")
    list(APPEND simdlib_checks_contract_rows
        "${simdlib_checks_contract_target}\t${simdlib_checks_contract_definitions}\t${simdlib_checks_contract_absolute_sources}")
endforeach()
string(REPLACE ";" "\n" simdlib_checks_contract_inventory
    "${simdlib_checks_contract_rows}")
file(WRITE "${CMAKE_BINARY_DIR}/checks-contract-properties.tsv"
    "${simdlib_checks_contract_inventory}\n")

set(simdlib_aggregate_rows "")
foreach(simdlib_category IN LISTS SIMDLIB_VALIDATION_CATEGORIES)
    list(APPEND simdlib_aggregate_rows
        "${simdlib_category_aggregate_${simdlib_category}}\t${simdlib_category}")
endforeach()
list(APPEND simdlib_aggregate_rows
    "ExhaustiveArtifacts\tPROFILE:${SIMDLIB_VALIDATION_PROFILE}"
    "SimdLibSanitizerValidationArtifacts\tPROFILE:SANITIZER"
    "SimdLibCoverageValidationArtifacts\tPROFILE:COVERAGE")
list(SORT simdlib_aggregate_rows)
string(REPLACE ";" "\n" simdlib_aggregate_inventory
    "${simdlib_aggregate_rows}")
file(WRITE "${CMAKE_BINARY_DIR}/development-aggregates.tsv"
    "aggregate\tcategory\n${simdlib_aggregate_inventory}\n")

set(simdlib_membership_rows "")
foreach(simdlib_category IN LISTS SIMDLIB_VALIDATION_CATEGORIES)
    foreach(simdlib_category_target IN LISTS simdlib_targets_${simdlib_category})
        list(APPEND simdlib_membership_rows
            "${simdlib_category_aggregate_${simdlib_category}}\t${simdlib_category_target}")
    endforeach()
endforeach()
foreach(simdlib_profile_aggregate IN ITEMS
    SimdLibSanitizerValidationArtifacts
    SimdLibCoverageValidationArtifacts
    ExhaustiveArtifacts)
    get_target_property(simdlib_profile_dependencies
        ${simdlib_profile_aggregate} MANUALLY_ADDED_DEPENDENCIES)
    if(simdlib_profile_dependencies)
        foreach(simdlib_profile_dependency IN LISTS simdlib_profile_dependencies)
            list(APPEND simdlib_membership_rows
                "${simdlib_profile_aggregate}\t${simdlib_profile_dependency}")
        endforeach()
    endif()
endforeach()
list(SORT simdlib_membership_rows)
string(REPLACE ";" "\n" simdlib_membership_inventory
    "${simdlib_membership_rows}")
file(WRITE "${CMAKE_BINARY_DIR}/development-aggregate-membership.tsv"
    "aggregate\tdependency\n${simdlib_membership_inventory}\n")

set(simdlib_external_consumer_targets CoreConsumerSmoke)
if(SIMDLIB_REGISTER_COMPILER_SUPPORTED)
    list(APPEND simdlib_external_consumer_targets RegisterConsumerSmoke)
endif()
string(REPLACE ";" "\n" simdlib_external_consumer_inventory
    "${simdlib_external_consumer_targets}")
file(WRITE "${CMAKE_BINARY_DIR}/external-consumer-targets.txt"
    "${simdlib_external_consumer_inventory}\n")

if(BUILD_TESTING)
    add_test(NAME ArtifactAggregates.ProfileMembership
        COMMAND ${CMAKE_COMMAND}
            "-DOWNERSHIP_FILE=${CMAKE_BINARY_DIR}/development-target-ownership.tsv"
            "-DAGGREGATE_FILE=${CMAKE_BINARY_DIR}/development-aggregates.tsv"
            "-DMEMBERSHIP_FILE=${CMAKE_BINARY_DIR}/development-aggregate-membership.tsv"
            "-DPROFILE=${SIMDLIB_VALIDATION_PROFILE}"
            "-DSELECTED_CATEGORIES=${simdlib_selected_categories}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyArtifactAggregateInventory.cmake)
    set_tests_properties(ArtifactAggregates.ProfileMembership PROPERTIES
        LABELS "CONFIGURATION;ARTIFACT_OWNERSHIP")
    simdlib_register_development_test(ArtifactAggregates.ProfileMembership PROFILE_AUDIT)

    add_test(NAME ArtifactAggregates.PublicConsumption
        COMMAND ${CMAKE_COMMAND}
            "-DOWNERSHIP_FILE=${CMAKE_BINARY_DIR}/development-target-ownership.tsv"
            "-DCONSUMER_TARGET_FILE=${CMAKE_BINARY_DIR}/external-consumer-targets.txt"
            "-DPROFILE=${SIMDLIB_VALIDATION_PROFILE}"
            "-DREGISTER_SUPPORTED=${SIMDLIB_REGISTER_COMPILER_SUPPORTED}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyPublicConsumptionProfile.cmake)
    set_tests_properties(ArtifactAggregates.PublicConsumption PROPERTIES
        LABELS "CONFIGURATION;ARTIFACT_OWNERSHIP;PUBLIC_CONSUMPTION")
    simdlib_register_development_test(ArtifactAggregates.PublicConsumption PROFILE_AUDIT)

    if(simdlib_targets_COMPILER_CONTRACT)
        add_test(NAME ArtifactAggregates.CompilerContractIndependence
            COMMAND ${CMAKE_COMMAND}
                "-DPROPERTY_FILE=${CMAKE_BINARY_DIR}/compiler-contract-properties.tsv"
                "-DSOURCE_FILE=${CMAKE_BINARY_DIR}/compiler-contract-sources.tsv"
                "-DDEFAULT_CHECKS_PROBE=${SIMDLIB_DEFAULT_CHECKS_PROBE}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyCompilerContractIndependence.cmake)
        set_tests_properties(
            ArtifactAggregates.CompilerContractIndependence PROPERTIES
            LABELS "CONFIGURATION;ARTIFACT_OWNERSHIP;COMPILER_CONTRACT")
        simdlib_register_development_test(ArtifactAggregates.CompilerContractIndependence PROFILE_AUDIT)
    endif()

    if(simdlib_targets_CHECKS_VALIDATION)
        add_test(NAME ArtifactAggregates.ChecksConfiguration
            COMMAND ${CMAKE_COMMAND}
                "-DPROPERTY_FILE=${CMAKE_BINARY_DIR}/checks-contract-properties.tsv"
                "-DDEFAULT_CHECKS_PROBE=${SIMDLIB_DEFAULT_CHECKS_PROBE}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyChecksConfiguration.cmake)
        set_tests_properties(ArtifactAggregates.ChecksConfiguration PROPERTIES
            LABELS "CONFIGURATION;ARTIFACT_OWNERSHIP;CHECKS")
        simdlib_register_development_test(ArtifactAggregates.ChecksConfiguration PROFILE_AUDIT)
    endif()

    foreach(simdlib_failure_case IN ITEMS UNOWNED MULTIPLE EXCLUDED)
        add_test(NAME ArtifactAggregates.Reject${simdlib_failure_case}
            COMMAND ${CMAKE_COMMAND}
                "-DCASE=${simdlib_failure_case}"
                "-DSOURCE_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}"
                "-DBINARY_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/artifact-aggregate-negative/${simdlib_failure_case}"
                "-DGENERATOR=${CMAKE_GENERATOR}"
                "-DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyArtifactAggregateFailure.cmake)
        set_tests_properties(
            ArtifactAggregates.Reject${simdlib_failure_case} PROPERTIES
            LABELS "CONFIGURATION;ARTIFACT_OWNERSHIP")
        simdlib_register_development_test(ArtifactAggregates.Reject${simdlib_failure_case} PROFILE_AUDIT)
    endforeach()

	if(SIMDLIB_VALIDATION_PROFILE MATCHES
			"^(DEBUG|SANITIZER|COVERAGE|CODEGEN_DIAGNOSTIC)$")
		set(simdlib_codegen_isolation_mode OFF)
		if(SIMDLIB_VALIDATION_PROFILE STREQUAL "CODEGEN_DIAGNOSTIC")
			set(simdlib_codegen_isolation_mode RECORD)
		endif()
		add_test(NAME ArtifactAggregates.CodegenIsolation
			COMMAND ${CMAKE_COMMAND}
				"-DBINARY_DIRECTORY=${CMAKE_BINARY_DIR}"
				"-DOWNERSHIP_FILE=${CMAKE_BINARY_DIR}/development-target-ownership.tsv"
				"-DPROFILE=${SIMDLIB_VALIDATION_PROFILE}"
				"-DCODEGEN_MODE=${simdlib_codegen_isolation_mode}"
				-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyCodegenProfileIsolation.cmake)
		set_tests_properties(ArtifactAggregates.CodegenIsolation PROPERTIES
			LABELS "CONFIGURATION;ARTIFACT_OWNERSHIP;CODEGEN_ISOLATION")
		simdlib_register_development_test(ArtifactAggregates.CodegenIsolation PROFILE_AUDIT)
	endif()

	if(SIMDLIB_VALIDATION_PROFILE MATCHES "^(RELEASE|CODEGEN_DIAGNOSTIC)$")
		add_test(NAME CodegenPolicy.RejectRecordAsEnforced
			COMMAND ${CMAKE_COMMAND}
				"-DSOURCE_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}"
				"-DBINARY_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/codegen-policy-separation"
				-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyCodegenPolicySeparation.cmake)
		set_tests_properties(CodegenPolicy.RejectRecordAsEnforced PROPERTIES
			LABELS "CONFIGURATION;CODEGEN;CODEGEN_POLICY")
		simdlib_register_development_test(CodegenPolicy.RejectRecordAsEnforced PROFILE_AUDIT)
	endif()
endif()

endblock()
