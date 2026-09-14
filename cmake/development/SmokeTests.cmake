include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "SmokeTests.cmake is available only to top-level SimdLib builds")
endif()
if(NOT TARGET SimdLib OR NOT TARGET SimdLibRegister)
    message(FATAL_ERROR "SmokeTests.cmake requires the production SimdLib targets")
endif()

block(SCOPE_FOR VARIABLES)

if(SIMDLIB_BUILD_SMOKE_TESTS)
    add_executable(HeaderOnlySmoke
        tests/smoke/main.cpp
        tests/smoke/second_translation_unit.cpp)
    simdlib_register_development_target(HeaderOnlySmoke SMOKE_VALIDATION)
    target_link_libraries(HeaderOnlySmoke PRIVATE SimdLib::SimdLib)
    simdlib_enable_development_warnings(HeaderOnlySmoke)
    add_test(NAME HeaderOnlySmoke COMMAND HeaderOnlySmoke)
    simdlib_register_development_test(HeaderOnlySmoke SMOKE_VALIDATION)
    simdlib_set_coverage_profile_prefix(HeaderOnlySmoke
        "HeaderOnlySmoke")

	if(SIMDLIB_REGISTER_COMPILER_SUPPORTED)
		add_executable(RegisterOdr
			tests/register_odr/main.cpp
			tests/register_odr/second_translation_unit.cpp)
		simdlib_register_development_target(RegisterOdr SMOKE_VALIDATION)
		target_link_libraries(RegisterOdr PRIVATE SimdLib::Register)
		simdlib_enable_development_warnings(RegisterOdr)
		simdlib_enable_register_sse42(RegisterOdr)
		add_test(NAME RegisterOdr COMMAND RegisterOdr)
		set_tests_properties(RegisterOdr PROPERTIES LABELS "REGISTER;ODR;SSE42")
		simdlib_register_development_test(RegisterOdr SMOKE_VALIDATION)
		simdlib_set_coverage_profile_prefix(RegisterOdr "RegisterOdr")

		# @brief Adds one PartialRegister multi-translation-unit test for an ISA profile.
		# @param target Development executable target name.
		# @param bits Physical PartialRegister width used by the test.
		# @param isa_profile Test label identifying the selected ISA profile.
		function(simdlib_add_partial_register_odr_test target bits isa_profile)
			add_executable(${target}
				tests/partial_register_odr/main.cpp
				tests/partial_register_odr/second_translation_unit.cpp
				tests/partial_register_odr/fixture.h)
			simdlib_register_development_target(${target} SMOKE_VALIDATION)
			target_link_libraries(${target} PRIVATE SimdLib::Register)
			target_compile_definitions(${target} PRIVATE
				SIMDLIB_PARTIAL_REGISTER_ODR_BITS=${bits})
			simdlib_enable_development_warnings(${target})
			if(bits EQUAL 256)
				simdlib_enable_register_avx2(${target})
			else()
				simdlib_enable_register_sse42(${target})
			endif()
			add_test(NAME ${target} COMMAND ${target})
			set_tests_properties(${target} PROPERTIES
				LABELS "PARTIAL_REGISTER;ODR;${isa_profile}")
			simdlib_register_development_test(${target} SMOKE_VALIDATION)
			simdlib_set_coverage_profile_prefix(${target} "${target}")
		endfunction()

		simdlib_add_partial_register_odr_test(PartialRegisterOdrSse42 128 SSE42)
		simdlib_add_partial_register_odr_test(PartialRegisterOdrAvx2 256 AVX2)

		add_custom_target(InstalledPackagePartialRegisterConsumerArtifacts
			COMMAND ${CMAKE_COMMAND}
				-DSIMDLIB_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}
				-DSIMDLIB_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
				-DSIMDLIB_CONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/installed_consumer
				-DSIMDLIB_CONSUMER_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/installed-partial-register-consumer/$<CONFIG>
				-DSIMDLIB_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/installed-partial-register-package/$<CONFIG>
				-DSIMDLIB_CMAKE_COMMAND=${CMAKE_COMMAND}
				-DSIMDLIB_CTEST_COMMAND=${CMAKE_CTEST_COMMAND}
				-DSIMDLIB_GENERATOR=${CMAKE_GENERATOR}
				-DSIMDLIB_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}
				-DSIMDLIB_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}
				-DSIMDLIB_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
				-DSIMDLIB_CXX_COMPILER=${CMAKE_CXX_COMPILER}
				"-DSIMDLIB_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
				"-DSIMDLIB_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}"
				-DSIMDLIB_CXX_SCAN_FOR_MODULES=${CMAKE_CXX_SCAN_FOR_MODULES}
				-DSIMDLIB_CONFIG=$<CONFIG>
				-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunInstalledPartialRegisterConsumer.cmake
			VERBATIM)
		simdlib_register_development_target(InstalledPackagePartialRegisterConsumerArtifacts SMOKE_VALIDATION)
		add_test(NAME InstalledPackagePartialRegisterConsumer
			COMMAND ${CMAKE_CTEST_COMMAND}
				--test-dir ${CMAKE_CURRENT_BINARY_DIR}/installed-partial-register-consumer/$<CONFIG>
				-C $<CONFIG> --output-on-failure)
		set_tests_properties(InstalledPackagePartialRegisterConsumer PROPERTIES
			LABELS "PARTIAL_REGISTER;CONSUMER;INSTALL;SSE42;AVX2"
			RUN_SERIAL TRUE)
		simdlib_register_development_test(InstalledPackagePartialRegisterConsumer SMOKE_VALIDATION)
	endif()
endif()

endblock()
