# Register deliberate rule mutations as separate CTest results and output owners.
foreach(case IN ITEMS valid after-return extra-before extra-after width immediate
        operand target-valid target-wrong cookie-valid cookie-target cookie-extra
        cookie-cannot-override return-only empty allowlist add-valid add-same-input
        transfer-valid transfer-extra-before-add transfer-extra-after-add
        blend-valid blend-wrong-mask zero-valid zero-wrong-third
        abi-valid abi-target-suffix abi-target-prefix abi-extra-frame-before abi-extra-frame-after
        constant-valid constant-wrong-suffix)
    add_test(NAME Instructions.rules.${case} COMMAND "${CMAKE_COMMAND}"
        "-DCASE=${case}" "-DFILECHECK=${SIMDLIB_FILECHECK}"
        "-DRUNNER=$<TARGET_FILE:CodegenHarness>"
        "-DOUTPUT_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/rules/${case}"
        -P "${CMAKE_CURRENT_LIST_DIR}/RuleHarness.cmake")
    set_tests_properties(Instructions.rules.${case} PROPERTIES
        LABELS "CODEGEN_HARNESS;SIMDLIB_OWNER_OPTIMIZED_CODEGEN" TIMEOUT 90)
endforeach()
