include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "RuntimeTests.cmake is available only to top-level SimdLib builds")
endif()
if(NOT TARGET SimdLib OR NOT TARGET SimdLibRegister)
    message(FATAL_ERROR "RuntimeTests.cmake requires the production SimdLib targets")
endif()

block(SCOPE_FOR VARIABLES)

if(SIMDLIB_BUILD_RUNTIME_TESTS)
    include(Catch)
    # Build receipts inventory CTest immediately after compilation, so keeping
    # discovery at build time avoids hidden discovery work during receipt reuse.
    set(CMAKE_CATCH_DISCOVER_TESTS_DISCOVERY_MODE POST_BUILD)

    # @brief Applies labels after Catch2 has populated its deferred discovery list.
    # @param test_list_variable Name of the Catch2-generated test-list variable.
    # @param labels Semicolon-separated labels applied to every discovered test.
    # @param owner Validation category that owns every discovered test.
    function(simdlib_label_discovered_tests test_list_variable labels owner)
        set(label_file "${CMAKE_CURRENT_BINARY_DIR}/${test_list_variable}-labels.cmake")
        file(WRITE "${label_file}"
            "foreach(discovered_test IN LISTS ${test_list_variable})\n"
            "    set_tests_properties(\"\${discovered_test}\" PROPERTIES LABELS \"${labels};SIMDLIB_OWNER_${owner}\")\n"
            "endforeach()\n")
        set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${label_file}")
    endfunction()

    # @brief Adds and discovers one Catch2 executable with stable labels.
    # @param target Development executable target name.
    # @param source Translation unit that owns the Catch2 cases.
    # @param test_prefix Prefix applied to every discovered CTest identity.
    # @param labels Semicolon-separated labels applied to every discovered case.
    # @param category Optional validation category; defaults to RUNTIME_VALIDATION.
    function(simdlib_add_catch_test target source test_prefix labels)
        set(validation_category RUNTIME_VALIDATION)
        if(ARGC GREATER 4)
            set(validation_category ${ARGV4})
        endif()
        add_executable(${target} ${source})
        simdlib_register_development_target(${target} ${validation_category})
        target_link_libraries(${target} PRIVATE SimdLib::SimdLib Catch2::Catch2WithMain)
        simdlib_enable_development_warnings(${target})
        simdlib_set_coverage_profile_prefix(${target} "${test_prefix}")
        set(test_list_variable "${target}_DISCOVERED_TESTS")
        catch_discover_tests(${target}
            TEST_PREFIX "${test_prefix}."
            TEST_LIST ${test_list_variable})
        simdlib_label_discovered_tests(${test_list_variable} "${labels}"
            ${validation_category})
    endfunction()

	if(SIMDLIB_REGISTER_COMPILER_SUPPORTED)
		simdlib_add_catch_test(RegisterAvx2Tests tests/Register.tests.cpp
			Register.AVX2 "REGISTER;AVX2")
		target_sources(RegisterAvx2Tests PRIVATE
			tests/RegisterBasicOperations.tests.cpp
			tests/RegisterSpecializedOperations.tests.cpp
			tests/RegisterRearrangementConversion.tests.cpp
			tests/LogicalShuffleRegister.tests.cpp
			tests/RegisterOperationMatrix.tests.cpp)
		target_link_libraries(RegisterAvx2Tests PRIVATE SimdLib::Register)
		target_compile_definitions(RegisterAvx2Tests PRIVATE
			SIMDLIB_REGISTER_TEST_ENABLE_256=1)
		simdlib_enable_register_avx2(RegisterAvx2Tests)

		simdlib_add_catch_test(RegisterSse42Tests tests/Register.tests.cpp
			Register.SSE42 "REGISTER;SSE42")
		target_sources(RegisterSse42Tests PRIVATE
			tests/RegisterBasicOperations.tests.cpp
			tests/RegisterSpecializedOperations.tests.cpp
			tests/RegisterRearrangementConversion.tests.cpp
			tests/LogicalShuffleRegister.tests.cpp
			tests/RegisterOperationMatrix.tests.cpp)
		target_link_libraries(RegisterSse42Tests PRIVATE SimdLib::Register)
		target_compile_definitions(RegisterSse42Tests PRIVATE
			SIMDLIB_REGISTER_TEST_ENABLE_256=0)
		simdlib_enable_register_sse42(RegisterSse42Tests)

		add_executable(RegisterPreconditionTests tests/RegisterPreconditionFailure.tests.cpp)
		simdlib_register_development_target(RegisterPreconditionTests
			CHECKS_VALIDATION)
		target_link_libraries(RegisterPreconditionTests PRIVATE SimdLib::Register Catch2::Catch2WithMain)
		simdlib_enable_development_warnings(RegisterPreconditionTests)
		target_compile_definitions(RegisterPreconditionTests PRIVATE SIMDLIB_ENABLE_CHECKS=1)
		simdlib_set_coverage_profile_prefix(RegisterPreconditionTests
			"Register.AVX2Preconditions")
		simdlib_enable_register_sse42(RegisterPreconditionTests)
		catch_discover_tests(RegisterPreconditionTests
			TEST_PREFIX "Register.AVX2Preconditions."
			TEST_LIST RegisterPreconditionTests_DISCOVERED_TESTS
			PROPERTIES
				PASS_REGULAR_EXPRESSION "SIMDLIB_REGISTER_PRECONDITION_FAILURE_EXPECTED_61B4C2"
				TIMEOUT 10)
		simdlib_label_discovered_tests(RegisterPreconditionTests_DISCOVERED_TESTS
			"REGISTER;PARTIAL_REGISTER;PRECONDITIONS;AVX2" CHECKS_VALIDATION)

		simdlib_add_catch_test(PartialRegisterAvx2Tests tests/PartialRegisterObjectModel.tests.cpp
			PartialRegister.AVX2 "PARTIAL_REGISTER;AVX2")
		target_sources(PartialRegisterAvx2Tests PRIVATE
			tests/PartialRegisterConstructionTransfer.tests.cpp)
		target_link_libraries(PartialRegisterAvx2Tests PRIVATE SimdLib::Register)
		target_compile_definitions(PartialRegisterAvx2Tests PRIVATE
			SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256=1)
		if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			target_compile_options(PartialRegisterAvx2Tests PRIVATE /bigobj)
		endif()
		simdlib_enable_register_avx2(PartialRegisterAvx2Tests)

		simdlib_add_catch_test(PartialRegisterSse42Tests tests/PartialRegisterObjectModel.tests.cpp
			PartialRegister.SSE42 "PARTIAL_REGISTER;SSE42")
		target_sources(PartialRegisterSse42Tests PRIVATE
			tests/PartialRegisterConstructionTransfer.tests.cpp)
		target_link_libraries(PartialRegisterSse42Tests PRIVATE SimdLib::Register)
		target_compile_definitions(PartialRegisterSse42Tests PRIVATE
			SIMDLIB_PARTIAL_REGISTER_TEST_ENABLE_256=0)
		if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			target_compile_options(PartialRegisterSse42Tests PRIVATE /bigobj)
		endif()
		simdlib_enable_register_sse42(PartialRegisterSse42Tests)
	endif()

    simdlib_add_catch_test(BmiPortableTests tests/Bmi.tests.cpp
        BmiPortable "BMI;PORTABLE")
    target_compile_definitions(BmiPortableTests PRIVATE
        SIMDLIB_HAS_BMI1=0 SIMDLIB_HAS_BMI2=0
        SIMDLIB_BMI_EXPECT_BMI1=0 SIMDLIB_BMI_EXPECT_BMI2=0)
	if(NOT SIMDLIB_MSVC_STYLE_DRIVER)
        target_compile_options(BmiPortableTests PRIVATE -mno-bmi -mno-bmi2)
    endif()

    simdlib_add_catch_test(FormatTests tests/Format.tests.cpp
        Format "FORMAT;SSE42")
	if(SIMDLIB_MSVC_STYLE_DRIVER)
		target_compile_definitions(FormatTests PRIVATE
			SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			target_compile_options(FormatTests PRIVATE /arch:AVX2)
		endif()
    else()
        target_compile_options(FormatTests PRIVATE -msse4.2)
    endif()

	if(SIMDLIB_BUILD_SMOKE_TESTS)
		add_executable(FormatOdr
			tests/format_odr/main.cpp
			tests/format_odr/second_translation_unit.cpp)
		simdlib_register_development_target(FormatOdr SMOKE_VALIDATION)
		target_link_libraries(FormatOdr PRIVATE SimdLib::SimdLib)
		simdlib_enable_development_warnings(FormatOdr)
		add_test(NAME FormatOdr COMMAND FormatOdr)
		set_tests_properties(FormatOdr PROPERTIES LABELS "FORMAT;ODR")
	simdlib_register_development_test(FormatOdr SMOKE_VALIDATION)
		simdlib_set_coverage_profile_prefix(FormatOdr "FormatOdr")
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_definitions(FormatOdr PRIVATE
				SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
			if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
				target_compile_options(FormatOdr PRIVATE /arch:AVX2)
			endif()
		else()
			target_compile_options(FormatOdr PRIVATE -msse4.2)
		endif()
	endif()

    if(SIMDLIB_BUILD_API_SSE42_TESTS)
		simdlib_add_catch_test(ImplHalfTransfer128Tests tests/ImplementationHalfTransfer.tests.cpp
			Implementation.HalfTransfer128 "IMPLEMENTATION;PARTIAL_TRANSFER;SSE42")
		target_compile_definitions(ImplHalfTransfer128Tests PRIVATE
			SIMDLIB_IMPLEMENTATION_HALF_TRANSFER_TEST_WIDTH=128)
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_definitions(ImplHalfTransfer128Tests PRIVATE
				SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
			if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
				target_compile_options(ImplHalfTransfer128Tests PRIVATE /arch:AVX2)
			endif()
		else()
			target_compile_options(ImplHalfTransfer128Tests PRIVATE -msse4.2)
		endif()

        simdlib_add_catch_test(LogicalShuffleImpl128Tests tests/LogicalShuffleImpl128.tests.cpp
            LogicalShuffle.Impl128 "LOGICAL_SHUFFLE;SSE42")
        if(SIMDLIB_MSVC_STYLE_DRIVER)
            target_compile_definitions(LogicalShuffleImpl128Tests PRIVATE
                SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
            if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                target_compile_options(LogicalShuffleImpl128Tests PRIVATE /arch:AVX2)
            endif()
        else()
            target_compile_options(LogicalShuffleImpl128Tests PRIVATE -msse4.2)
        endif()

        simdlib_add_catch_test(ApiSse42Tests tests/Api128.tests.cpp
            Api.SSE42 "SSE42")
		target_sources(ApiSse42Tests PRIVATE
			tests/ApiPartialTransfer.tests.cpp
			tests/LogicalShuffleApi.tests.cpp
			tests/ImmediateControlSlowPaths.tests.cpp
			tests/CompleteRegisterShift.tests.cpp)
		target_compile_definitions(ApiSse42Tests PRIVATE
			SIMDLIB_API_PARTIAL_TRANSFER_TEST_WIDTH=128
			SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH=128
			SIMDLIB_IMMEDIATE_CONTROL_TEST_WIDTH=128
			SIMDLIB_COMPLETE_SHIFT_TEST_WIDTH=128)
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_definitions(ApiSse42Tests PRIVATE
				SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
			if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
				target_compile_options(ApiSse42Tests PRIVATE /arch:AVX2)
			endif()
        else()
            target_compile_options(ApiSse42Tests PRIVATE -msse4.2)
        endif()

        simdlib_add_catch_test(UInt128OptimizedTests tests/UInt128.tests.cpp
            UInt128Optimized "UINT128;OPTIMIZED;SSE42")
        simdlib_add_catch_test(UInt128PortableTests tests/UInt128.tests.cpp
            UInt128Portable "UINT128;PORTABLE;SSE42")
        simdlib_add_catch_test(UInt128ScalarTests tests/UInt128.tests.cpp
            UInt128Scalar "UINT128;PORTABLE;SCALAR")
        foreach(uint128_target IN ITEMS
            UInt128OptimizedTests UInt128PortableTests UInt128ScalarTests)
            target_compile_definitions(${uint128_target} PRIVATE
                SIMDLIB_TEST_CONSTEXPR_ASSERTIONS=$<BOOL:${SIMDLIB_BUILD_CONSTEXPR_PROBES}>)
        endforeach()
        target_compile_definitions(UInt128PortableTests PRIVATE
            SIMDLIB_USE_COMPILER_CARRY_INTRINSICS=0 SIMDLIB_EXPECT_CARRY_PATH=0)
		target_compile_definitions(UInt128ScalarTests PRIVATE SIMDLIB_EXPECT_CARRY_PATH=0)
		if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			target_compile_definitions(UInt128OptimizedTests PRIVATE SIMDLIB_EXPECT_CARRY_PATH=1)
		elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
			target_compile_definitions(UInt128OptimizedTests PRIVATE SIMDLIB_EXPECT_CARRY_PATH=2)
		endif()
		target_compile_definitions(UInt128ScalarTests PRIVATE
			SIMDLIB_USE_COMPILER_CARRY_INTRINSICS=0
			SIMDLIB_HAS_SSE=0 SIMDLIB_HAS_SSE2=0 SIMDLIB_HAS_SSE3=0 SIMDLIB_HAS_SSSE3=0
			SIMDLIB_HAS_SSE41=0 SIMDLIB_HAS_SSE42=0 SIMDLIB_HAS_AVX=0 SIMDLIB_HAS_AVX2=0
			SIMDLIB_HAS_FMA=0 SIMDLIB_HAS_BMI1=0 SIMDLIB_HAS_BMI2=0)
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_definitions(UInt128OptimizedTests PRIVATE
				SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
			target_compile_definitions(UInt128PortableTests PRIVATE
				SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
			if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
				target_compile_options(UInt128OptimizedTests PRIVATE /arch:AVX2)
				target_compile_options(UInt128PortableTests PRIVATE /arch:AVX2)
			endif()
        else()
            target_compile_options(UInt128OptimizedTests PRIVATE -msse4.2)
            target_compile_options(UInt128PortableTests PRIVATE -msse4.2)
        endif()
        add_test(NAME UInt128ResultSetEquivalence
            COMMAND ${CMAKE_COMMAND}
                -DPORTABLE_EXECUTABLE=$<TARGET_FILE:UInt128PortableTests>
                -DOPTIMIZED_EXECUTABLE=$<TARGET_FILE:UInt128OptimizedTests>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareUInt128ResultSets.cmake)
        set_tests_properties(UInt128ResultSetEquivalence PROPERTIES LABELS "UINT128;EQUIVALENCE;SSE42")
        simdlib_register_development_test(UInt128ResultSetEquivalence RUNTIME_VALIDATION)

		add_test(NAME UInt128ScalarResultSetEquivalence
			COMMAND ${CMAKE_COMMAND}
				-DPORTABLE_EXECUTABLE=$<TARGET_FILE:UInt128ScalarTests>
				-DOPTIMIZED_EXECUTABLE=$<TARGET_FILE:UInt128OptimizedTests>
				-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareUInt128ResultSets.cmake)
		set_tests_properties(UInt128ScalarResultSetEquivalence PROPERTIES LABELS "UINT128;EQUIVALENCE;SCALAR")
		simdlib_register_development_test(UInt128ScalarResultSetEquivalence RUNTIME_VALIDATION)
    endif()

    if(SIMDLIB_BUILD_API_AVX2_TESTS)
		simdlib_add_catch_test(ImplHalfTransfer256Tests tests/ImplementationHalfTransfer.tests.cpp
			Implementation.HalfTransfer256 "IMPLEMENTATION;PARTIAL_TRANSFER;AVX2")
		target_compile_definitions(ImplHalfTransfer256Tests PRIVATE
			SIMDLIB_IMPLEMENTATION_HALF_TRANSFER_TEST_WIDTH=256)
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_options(ImplHalfTransfer256Tests PRIVATE /arch:AVX2)
		else()
			target_compile_options(ImplHalfTransfer256Tests PRIVATE -mavx2)
		endif()

        simdlib_add_catch_test(LogicalShuffleImpl256Tests tests/LogicalShuffleImpl256.tests.cpp
            LogicalShuffle.Impl256 "LOGICAL_SHUFFLE;AVX2")
        if(SIMDLIB_MSVC_STYLE_DRIVER)
            target_compile_options(LogicalShuffleImpl256Tests PRIVATE /arch:AVX2)
        else()
            target_compile_options(LogicalShuffleImpl256Tests PRIVATE -mavx2)
        endif()

        simdlib_add_catch_test(ApiAvx2Tests tests/Api256.tests.cpp
            Api.AVX2 "AVX2")
		target_sources(ApiAvx2Tests PRIVATE
			tests/ApiPartialTransfer.tests.cpp
			tests/LogicalShuffleApi.tests.cpp
			tests/ImmediateControlSlowPaths.tests.cpp
			tests/CompleteRegisterShift.tests.cpp)
		target_compile_definitions(ApiAvx2Tests PRIVATE
			SIMDLIB_API_PARTIAL_TRANSFER_TEST_WIDTH=256
			SIMDLIB_LOGICAL_SHUFFLE_TEST_WIDTH=256
			SIMDLIB_IMMEDIATE_CONTROL_TEST_WIDTH=256
			SIMDLIB_COMPLETE_SHIFT_TEST_WIDTH=256)
		if(SIMDLIB_MSVC_STYLE_DRIVER)
            target_compile_options(ApiAvx2Tests PRIVATE /arch:AVX2)
        else()
            target_compile_options(ApiAvx2Tests PRIVATE -mavx2)
        endif()
    endif()

    if(SIMDLIB_BUILD_FMA_TESTS)
        simdlib_add_catch_test(FmaEnabledTests tests/SimdFma.tests.cpp
            FMA.Enabled "FMA;ENABLED")
        simdlib_add_catch_test(FmaDisabledTests tests/SimdFma.tests.cpp
            FMA.Disabled "FMA;DISABLED")
		target_compile_definitions(FmaEnabledTests PRIVATE SIMDLIB_HAS_FMA=1 SIMDLIB_EXPECT_FMA=1)
        target_compile_definitions(FmaDisabledTests PRIVATE SIMDLIB_HAS_FMA=0 SIMDLIB_EXPECT_FMA=0)
		if(SIMDLIB_MSVC_STYLE_DRIVER)
            target_compile_options(FmaEnabledTests PRIVATE /arch:AVX2)
            target_compile_options(FmaDisabledTests PRIVATE /arch:AVX2)
        else()
            target_compile_options(FmaEnabledTests PRIVATE -mavx2 -mfma)
            target_compile_options(FmaDisabledTests PRIVATE -mavx2 -mno-fma)
        endif()
    endif()

    if(SIMDLIB_BUILD_BMI_TESTS)
        # @brief Adds one runtime test executable for a BMI feature combination.
        # @param profile_name Stable suffix identifying the enabled BMI features.
        # @param bmi1 Whether BMI1 is enabled for this profile.
        # @param bmi2 Whether BMI2 is enabled for this profile.
        function(simdlib_add_bmi_profile profile_name bmi1 bmi2)
            set(target Bmi${profile_name}Tests)
            set(test_name Bmi.Bmi${profile_name})
            simdlib_add_catch_test(${target} tests/Bmi.tests.cpp ${test_name}
                "BMI;${profile_name};OPTIONAL")
            target_compile_definitions(${target} PRIVATE
                SIMDLIB_HAS_BMI1=${bmi1} SIMDLIB_HAS_BMI2=${bmi2}
                SIMDLIB_BMI_EXPECT_BMI1=${bmi1} SIMDLIB_BMI_EXPECT_BMI2=${bmi2})
			if(SIMDLIB_MSVC_STYLE_DRIVER)
                target_compile_options(${target} PRIVATE /arch:AVX2)
            else()
                target_compile_options(${target} PRIVATE -mno-bmi -mno-bmi2)
                if(bmi1)
                    target_compile_options(${target} PRIVATE -mbmi)
                endif()
                if(bmi2)
                    target_compile_options(${target} PRIVATE -mbmi2)
                endif()
            endif()
            set(equivalence_name Bmi.Bmi${profile_name}.Equivalence)
            add_test(NAME ${equivalence_name}
                COMMAND ${CMAKE_COMMAND}
                    -DPORTABLE_EXECUTABLE=$<TARGET_FILE:BmiPortableTests>
                    -DENABLED_EXECUTABLE=$<TARGET_FILE:${target}>
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareBmiResultSets.cmake)
            set_tests_properties(${equivalence_name} PROPERTIES LABELS "BMI;EQUIVALENCE;${profile_name};OPTIONAL")
            simdlib_register_development_test(${equivalence_name} RUNTIME_VALIDATION)
        endfunction()

        simdlib_add_bmi_profile(1 1 0)
        simdlib_add_bmi_profile(2 0 1)
        simdlib_add_bmi_profile(1Bmi2 1 1)
    endif()

    if(SIMDLIB_BUILD_VECTOR_ALGORITHM_TESTS)
        add_executable(VectorAlgorithmsTests
            tests/SimdVector.tests.cpp
            tests/SimdAlgo.tests.cpp
            tests/PreconditionBoundary.tests.cpp
            tests/SimdResample.tests.cpp)
        simdlib_register_development_target(VectorAlgorithmsTests
            RUNTIME_VALIDATION)
        target_link_libraries(VectorAlgorithmsTests PRIVATE SimdLib::SimdLib Catch2::Catch2WithMain)
        simdlib_enable_development_warnings(VectorAlgorithmsTests)
        simdlib_set_coverage_profile_prefix(VectorAlgorithmsTests
            "VectorAlgorithms")
        catch_discover_tests(VectorAlgorithmsTests
            TEST_PREFIX "VectorAlgorithms."
            TEST_LIST VectorAlgorithmsTests_DISCOVERED_TESTS)
        simdlib_label_discovered_tests(VectorAlgorithmsTests_DISCOVERED_TESTS
            "VECTOR_ALGORITHMS;AVX2" RUNTIME_VALIDATION)
		if(SIMDLIB_MSVC_STYLE_DRIVER)
            target_compile_options(VectorAlgorithmsTests PRIVATE /arch:AVX2)
        else()
            target_compile_options(VectorAlgorithmsTests PRIVATE -mavx2 -mfma)
        endif()

		simdlib_add_catch_test(VectorChecksTests tests/SimdVectorChecks.tests.cpp
			VectorChecks "VECTOR_ALGORITHMS;AVX2;CHECKS" CHECKS_VALIDATION)
		target_compile_definitions(VectorChecksTests PRIVATE SIMDLIB_ENABLE_CHECKS=1)
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_options(VectorChecksTests PRIVATE /arch:AVX2)
		else()
			target_compile_options(VectorChecksTests PRIVATE -mavx2 -mfma)
		endif()

		add_executable(PreconditionTests tests/PreconditionFailure.tests.cpp)
		simdlib_register_development_target(PreconditionTests CHECKS_VALIDATION)
		target_link_libraries(PreconditionTests PRIVATE SimdLib::SimdLib Catch2::Catch2WithMain)
		simdlib_enable_development_warnings(PreconditionTests)
		simdlib_set_coverage_profile_prefix(PreconditionTests
			"Preconditions")
		target_compile_definitions(PreconditionTests PRIVATE SIMDLIB_ENABLE_CHECKS=1)
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_options(PreconditionTests PRIVATE /arch:AVX2)
		else()
			target_compile_options(PreconditionTests PRIVATE -mavx2 -mfma)
		endif()
		catch_discover_tests(PreconditionTests
			TEST_PREFIX "Preconditions."
			TEST_LIST PreconditionTests_DISCOVERED_TESTS
			PROPERTIES
				PASS_REGULAR_EXPRESSION "SIMDLIB_PRECONDITION_FAILURE_EXPECTED_18A7E3"
				TIMEOUT 10)
		simdlib_label_discovered_tests(PreconditionTests_DISCOVERED_TESTS
			"PRECONDITIONS;CHECKS;AVX2" CHECKS_VALIDATION)

        add_executable(ResampleScalarTests tests/SimdResample.tests.cpp)
        simdlib_register_development_target(ResampleScalarTests
            RUNTIME_VALIDATION)
        target_link_libraries(ResampleScalarTests PRIVATE SimdLib::SimdLib Catch2::Catch2WithMain)
        simdlib_enable_development_warnings(ResampleScalarTests)
        simdlib_set_coverage_profile_prefix(ResampleScalarTests
            "ResampleScalar")
        target_compile_definitions(ResampleScalarTests PRIVATE
            SIMDLIB_HAS_SSE3=0 SIMDLIB_HAS_SSSE3=0 SIMDLIB_HAS_SSE41=0 SIMDLIB_HAS_SSE42=0
            SIMDLIB_HAS_AVX=0 SIMDLIB_HAS_AVX2=0 SIMDLIB_HAS_FMA=0)
        catch_discover_tests(ResampleScalarTests
            TEST_PREFIX "ResampleScalar."
            TEST_LIST ResampleScalarTests_DISCOVERED_TESTS)
        simdlib_label_discovered_tests(ResampleScalarTests_DISCOVERED_TESTS
            "VECTOR_ALGORITHMS;SCALAR" RUNTIME_VALIDATION)
    endif()
endif()

endblock()
