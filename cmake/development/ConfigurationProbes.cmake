include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "ConfigurationProbes.cmake is available only to top-level SimdLib builds")
endif()
if(NOT TARGET SimdLib OR NOT TARGET SimdLibRegister)
    message(FATAL_ERROR "ConfigurationProbes.cmake requires the production SimdLib targets")
endif()

block(SCOPE_FOR VARIABLES)

if(SIMDLIB_BUILD_CONFIGURATION_PROBES)
    foreach(config_probe IN ITEMS
        ConfigDefaultProbe
        ConfigOverrideVectorcallProbe
        ConfigOverrideForceInlineProbe
        ConfigOverrideFlattenProbe
        ConfigOverridePreconditionProbe
        ConfigDisabledInstructionsProbe
        ConfigDisabledPublicHeadersProbe
        ConfigClangUnsupportedTargetProbe
        ConfigVendorAttributeProbe
        ConstexprProbe)
        add_library(${config_probe} OBJECT tests/config/${config_probe}.cpp)
        target_link_libraries(${config_probe} PRIVATE SimdLib::SimdLib)
        simdlib_enable_development_warnings(${config_probe})
    endforeach()
endif()

# @brief Adds a compile-only language-availability probe with an exact standard mode.
# @param target Target name used in compiler diagnostics.
# @param source Translation unit containing the availability assertions.
# @param standard C++ standard level requested for the probe.
# @param dependency Public SimdLib target whose usage requirements are under test.
function(simdlib_add_language_probe target source standard dependency)
	add_library(${target} OBJECT ${source})
	target_link_libraries(${target} PRIVATE ${dependency})
	set_target_properties(${target} PROPERTIES
		CXX_STANDARD ${standard}
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF)
	simdlib_enable_development_warnings(${target})
endfunction()

# @brief Verifies that one intentionally invalid translation unit fails with the focused diagnostic.
# @param probe_name Stable name used for the try-compile directory and log.
# @param source Translation unit that must fail to compile.
# @param standard Exact C++ standard level used for the negative probe.
# @param expected_diagnostic Stable diagnostic token required in compiler output.
function(simdlib_expect_language_probe_failure probe_name source standard expected_diagnostic)
	try_compile(probe_compiled
		SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${source}
		NO_CACHE
		CXX_STANDARD ${standard}
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
		CMAKE_FLAGS
			-DINCLUDE_DIRECTORIES=${CMAKE_CURRENT_SOURCE_DIR}/include
		OUTPUT_VARIABLE probe_output)
	file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${probe_name}.log" "${probe_output}")
	if(probe_compiled)
		message(FATAL_ERROR "${probe_name} unexpectedly compiled successfully")
	endif()
	if(NOT probe_output MATCHES "${expected_diagnostic}")
		message(FATAL_ERROR
			"${probe_name} did not emit ${expected_diagnostic}; see ${CMAKE_CURRENT_BINARY_DIR}/${probe_name}.log")
	endif()
endfunction()

if(SIMDLIB_BUILD_CONFIGURATION_PROBES)
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/include/SimdLib/Config.h
		${CMAKE_CURRENT_SOURCE_DIR}/include/SimdLib/Register.h
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterHeaderCxx20.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterRequirementCxx20.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterAvailabilityOverride.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterUnsupportedCompiler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterPartialLaneList.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterOversizedLaneList.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterDynamicTransfer.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterImplicitScalar.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterImplicitNative.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterNativeOrder.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterUninitialized.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterInvalidShuffleSelector.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterWrongShuffleSelectorCount.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterInvalidRearrangementImmediate.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterUnsupportedConversionTarget.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterUnavailableWidthChange.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterCompatibilityRearrangement.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterCollectionOperations.cpp)

	simdlib_add_language_probe(RegisterCxx20UmbrellaProbe
		tests/availability/RegisterCxx20UmbrellaProbe.cpp 20 SimdLib::SimdLib)

	if(SIMDLIB_REGISTER_COMPILER_SUPPORTED)
		simdlib_add_language_probe(RegisterEnabledProbe
			tests/availability/RegisterEnabledProbe.cpp 23 SimdLib::Register)

		foreach(register_width IN ITEMS 128 256)
			add_library(RegisterRepresentation${register_width} OBJECT
				tests/register/RegisterRepresentation.tests.cpp)
			add_library(RegisterConstexpr${register_width}Probe OBJECT
				tests/constexpr/RegisterConstexpr.tests.cpp)
			target_link_libraries(RegisterRepresentation${register_width} PRIVATE SimdLib::Register)
			target_link_libraries(RegisterConstexpr${register_width}Probe PRIVATE SimdLib::Register)
			target_compile_definitions(RegisterRepresentation${register_width} PRIVATE
				SIMDLIB_REGISTER_TEST_WIDTH=${register_width})
			target_compile_definitions(RegisterConstexpr${register_width}Probe PRIVATE
				SIMDLIB_REGISTER_TEST_WIDTH=${register_width})
			simdlib_enable_development_warnings(RegisterRepresentation${register_width})
			simdlib_enable_development_warnings(RegisterConstexpr${register_width}Probe)
			if(register_width EQUAL 128)
				simdlib_enable_register_sse42(RegisterRepresentation${register_width})
				simdlib_enable_register_sse42(RegisterConstexpr${register_width}Probe)
			else()
				simdlib_enable_register_avx2(RegisterRepresentation${register_width})
				simdlib_enable_register_avx2(RegisterConstexpr${register_width}Probe)
			endif()
		endforeach()

		simdlib_expect_language_probe_failure(RegisterPartialLaneListFailure
			tests/compile_fail/register/RegisterPartialLaneList.cpp 23
			SIMDLIB_REGISTER_REJECTS_PARTIAL_LANE_LIST)
		simdlib_expect_language_probe_failure(RegisterOversizedLaneListFailure
			tests/compile_fail/register/RegisterOversizedLaneList.cpp 23
			SIMDLIB_REGISTER_REJECTS_OVERSIZED_LANE_LIST)
		simdlib_expect_language_probe_failure(RegisterDynamicTransferFailure
			tests/compile_fail/register/RegisterDynamicTransfer.cpp 23
			SIMDLIB_REGISTER_REJECTS_DYNAMIC_TRANSFER)
		simdlib_expect_language_probe_failure(RegisterImplicitScalarFailure
			tests/compile_fail/register/RegisterImplicitScalar.cpp 23
			SIMDLIB_REGISTER_REJECTS_IMPLICIT_SCALAR)
		simdlib_expect_language_probe_failure(RegisterImplicitNativeFailure
			tests/compile_fail/register/RegisterImplicitNative.cpp 23
			SIMDLIB_REGISTER_REJECTS_IMPLICIT_NATIVE)
		simdlib_expect_language_probe_failure(RegisterNativeOrderFailure
			tests/compile_fail/register/RegisterNativeOrder.cpp 23
			SIMDLIB_REGISTER_REJECTS_NATIVE_ORDER_CONSTRUCTION)
		simdlib_expect_language_probe_failure(RegisterUninitializedFailure
			tests/compile_fail/register/RegisterUninitialized.cpp 23
			SIMDLIB_REGISTER_REJECTS_UNINITIALIZED_CONSTRUCTION)
		simdlib_expect_language_probe_failure(RegisterInvalidShuffleSelectorFailure
			tests/compile_fail/register/RegisterInvalidShuffleSelector.cpp 23
			SIMDLIB_REGISTER_REJECTS_INVALID_SHUFFLE_SELECTOR)
		simdlib_expect_language_probe_failure(RegisterWrongShuffleSelectorCountFailure
			tests/compile_fail/register/RegisterWrongShuffleSelectorCount.cpp 23
			SIMDLIB_REGISTER_REJECTS_WRONG_SHUFFLE_SELECTOR_COUNT)
		simdlib_expect_language_probe_failure(RegisterInvalidRearrangementImmediateFailure
			tests/compile_fail/register/RegisterInvalidRearrangementImmediate.cpp 23
			SIMDLIB_REGISTER_REJECTS_INVALID_REARRANGEMENT_IMMEDIATE)
		simdlib_expect_language_probe_failure(RegisterUnsupportedConversionTargetFailure
			tests/compile_fail/register/RegisterUnsupportedConversionTarget.cpp 23
			SIMDLIB_REGISTER_REJECTS_UNSUPPORTED_CONVERSION_TARGET)
		simdlib_expect_language_probe_failure(RegisterUnavailableWidthChangeFailure
			tests/compile_fail/register/RegisterUnavailableWidthChange.cpp 23
			SIMDLIB_REGISTER_REJECTS_UNAVAILABLE_WIDTH_CHANGE)
		simdlib_expect_language_probe_failure(RegisterCompatibilityRearrangementFailure
			tests/compile_fail/register/RegisterCompatibilityRearrangement.cpp 23
			SIMDLIB_REGISTER_REJECTS_COMPATIBILITY_REARRANGEMENT)
		simdlib_expect_language_probe_failure(RegisterCollectionOperationsFailure
			tests/compile_fail/register/RegisterCollectionOperations.cpp 23
			SIMDLIB_REGISTER_REJECTS_COLLECTION_OPERATIONS)
		if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			simdlib_add_language_probe(RegisterMsvcFallbackProbe
				tests/availability/RegisterMsvcFallbackProbe.cpp 23 SimdLib::Register)
		elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND SIMDLIB_MSVC_STYLE_DRIVER)
			simdlib_add_language_probe(RegisterClangClFallbackExclusionProbe
				tests/availability/RegisterClangClFallbackExclusionProbe.cpp 23 SimdLib::SimdLib)
		endif()
	endif()

	simdlib_expect_language_probe_failure(RegisterHeaderCxx20Failure
		tests/compile_fail/register/RegisterHeaderCxx20.cpp 20
		SIMDLIB_REGISTER_HEADER_REQUIRES_CXX23)
	simdlib_expect_language_probe_failure(RegisterRequirementCxx20Failure
		tests/compile_fail/register/RegisterRequirementCxx20.cpp 20
		SIMDLIB_REGISTER_INTERFACE_UNAVAILABLE)
	simdlib_expect_language_probe_failure(RegisterAvailabilityOverrideFailure
		tests/compile_fail/register/RegisterAvailabilityOverride.cpp 20
		SIMDLIB_REGISTER_INTERFACE_AVAILABILITY_IS_COMPUTED)
	if(NOT SIMDLIB_REGISTER_COMPILER_SUPPORTED)
		simdlib_expect_language_probe_failure(RegisterUnsupportedCompilerFailure
			tests/compile_fail/register/RegisterUnsupportedCompiler.cpp 23
			SIMDLIB_REGISTER_INTERFACE_UNAVAILABLE)
	endif()
endif()
add_library(AvailabilityDisabledProbe OBJECT tests/availability/ApiDisabledProbe.cpp)
target_link_libraries(AvailabilityDisabledProbe PRIVATE SimdLib::SimdLib)
simdlib_enable_development_warnings(AvailabilityDisabledProbe)

add_library(AvailabilityEnabledProbe OBJECT tests/availability/ApiEnabledProbe.cpp)
target_link_libraries(AvailabilityEnabledProbe PRIVATE SimdLib::SimdLib)
simdlib_enable_development_warnings(AvailabilityEnabledProbe)
if(SIMDLIB_MSVC_STYLE_DRIVER)
    target_compile_options(AvailabilityEnabledProbe PRIVATE /arch:AVX2)
else()
    target_compile_options(AvailabilityEnabledProbe PRIVATE -mavx2)
endif()

endblock()
