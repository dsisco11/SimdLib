include_guard(GLOBAL)
set(support "${SIMDLIB_SOURCE_ROOT}/tests/codegen/support")
set(harness "${SIMDLIB_SOURCE_ROOT}/tests/codegen/harness")
add_executable(CodegenProcessProbe "${harness}/ProcessProbe.cpp")
target_compile_features(CodegenProcessProbe PRIVATE cxx_std_20)
set_property(TARGET CodegenProcessProbe PROPERTY CXX_SCAN_FOR_MODULES OFF)
configure_file("${harness}/HarnessConfiguration.h.in" "${CMAKE_CURRENT_BINARY_DIR}/HarnessConfiguration.h.in" @ONLY)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/HarnessConfiguration.h"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/HarnessConfiguration.h.in")
add_executable(CodegenHarness "${harness}/Main.cpp" "${harness}/RuleInput.tests.cpp"
    "${harness}/Composition.tests.cpp" "${harness}/Process.tests.cpp" "${harness}/Freshness.tests.cpp"
    "${harness}/Discovery.tests.cpp" "${harness}/Applicability.tests.cpp"
    "${support}/Process.cpp" "${support}/FileCheck.cpp" "${support}/Inspection.cpp" "${support}/Applicability.cpp"
    "${support}/Function.cpp" "${support}/CodegenFixture.cpp"
    "${SIMDLIB_SOURCE_ROOT}/tests/codegen/pilot/ExpectedRules.cpp")
target_compile_features(CodegenHarness PRIVATE cxx_std_20)
if(MSVC)
    target_compile_options(CodegenHarness PRIVATE /utf-8)
    target_compile_options(CodegenProcessProbe PRIVATE /utf-8)
endif()
set_property(TARGET CodegenHarness PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_include_directories(CodegenHarness PRIVATE "${support}" "${harness}"
    "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/codegen-contracts/runner-128")
target_link_libraries(CodegenHarness PRIVATE Catch2::Catch2)
add_dependencies(CodegenHarness CodegenTests128 CodegenProcessProbe)
add_dependencies(CodegenPilot CodegenHarness)
include(Catch)
catch_discover_tests(CodegenHarness TEST_SPEC "[harness]" TEST_PREFIX "CodegenHarness."
    TEST_LIST CodegenHarness_DISCOVERED_TESTS DISCOVERY_MODE POST_BUILD)
simdlib_label_codegen_tests(CodegenHarness_DISCOVERED_TESTS CODEGEN_HARNESS)
if(COMMAND simdlib_register_development_target)
    simdlib_register_development_target(CodegenHarness OPTIMIZED_CODEGEN)
    simdlib_register_development_target(CodegenProcessProbe OPTIMIZED_CODEGEN)
endif()
include("${harness}/InstructionRules.cmake")
