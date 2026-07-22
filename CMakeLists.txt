cmake_minimum_required(VERSION 4.4)

project(SimdLib VERSION 0.2.0 LANGUAGES CXX)

option(SIMDLIB_BUILD_SMOKE_TESTS "Build header-only ODR smoke tests" ON)
option(SIMDLIB_BUILD_TESTS "Build SimdLib Catch2 tests" OFF)
option(SIMDLIB_BUILD_TESTS_128 "Build 128-bit SSE4.2 tests" ON)
option(SIMDLIB_BUILD_TESTS_256 "Build 256-bit AVX2 tests" ON)
option(SIMDLIB_BUILD_TESTS_FMA "Build FMA tests" ON)
option(SIMDLIB_BUILD_TESTS_OPTIONAL "Build optional BMI-family tests" OFF)
option(SIMDLIB_BUILD_VECTOR_ALGORITHM_TESTS "Build SimdVector, SimdAlgo, and resampling parity tests" ON)
option(SIMDLIB_BUILD_BENCHMARKS "Build SimdLib Catch2 benchmarks" OFF)
option(SIMDLIB_BUILD_EXAMPLES "Build the executable API example" OFF)
option(SIMDLIB_BUILD_CONFIGURATION_TESTS "Build compile-only configuration probes" ON)
option(SIMDLIB_BUILD_HEADER_TESTS "Build first-and-only public-header probes" ON)
option(SIMDLIB_FETCH_TEST_DEPENDENCIES "Fetch missing test-only dependencies" ON)
option(SIMDLIB_STRICT_WARNINGS "Treat warnings in SimdLib-owned targets as errors" OFF)
option(SIMDLIB_ENABLE_COVERAGE "Instrument SimdLib-owned targets for source coverage" OFF)
option(SIMDLIB_BUILD_REGISTER_CODEGEN "Build mandatory Register generated-code comparisons" OFF)

# CTest 4.4 uses this setting during its dashboard Test step to assign a
# collision-free LLVM_PROFILE_FILE to every discovered test invocation.
if(SIMDLIB_ENABLE_COVERAGE)
	set(CTEST_TEST_COVERAGE_TOOL "LLVM-COV")
endif()
include(CTest)

add_library(SimdLib INTERFACE)
add_library(SimdLib::SimdLib ALIAS SimdLib)

set(SIMDLIB_MSVC_STYLE_DRIVER ${MSVC})
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
	set(SIMDLIB_MSVC_STYLE_DRIVER OFF)
endif()

target_compile_features(SimdLib INTERFACE cxx_std_20)
target_include_directories(SimdLib INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_sources(SimdLib INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/visualizers/SimdLib.natvis>)

add_library(SimdLibRegister INTERFACE)
add_library(SimdLib::Register ALIAS SimdLibRegister)
target_link_libraries(SimdLibRegister INTERFACE SimdLib::SimdLib)
target_compile_features(SimdLibRegister INTERFACE cxx_std_23)
target_compile_definitions(SimdLibRegister INTERFACE
	SIMDLIB_REQUIRE_REGISTER_INTERFACE=1)
target_compile_options(SimdLibRegister INTERFACE
	$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/std:c++latest>)

set(SIMDLIB_REGISTER_COMPILER_SUPPORTED OFF)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.44)
	set(SIMDLIB_REGISTER_COMPILER_SUPPORTED ON)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 22)
	set(SIMDLIB_REGISTER_COMPILER_SUPPORTED ON)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14)
	set(SIMDLIB_REGISTER_COMPILER_SUPPORTED ON)
endif()
set_property(TARGET SimdLibRegister PROPERTY
	SIMDLIB_REGISTER_COMPILER_SUPPORTED ${SIMDLIB_REGISTER_COMPILER_SUPPORTED})

# Consumer-facing examples and probes may use focused public headers, but must
# never depend on implementation-only Detail declarations or include paths.
file(GLOB_RECURSE SIMDLIB_PUBLIC_CONSUMER_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/tests/consumer/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/tests/headers/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke/*.cpp")
foreach(consumer_source IN LISTS SIMDLIB_PUBLIC_CONSUMER_SOURCES)
	file(READ "${consumer_source}" consumer_source_text)
	if(consumer_source_text MATCHES "SimdLib::Detail|<SimdLib/Detail/")
		message(FATAL_ERROR "Public consumer surface names implementation detail: ${consumer_source}")
	endif()
endforeach()

add_library(SimdLibDevelopmentWarnings INTERFACE)
if(SIMDLIB_ENABLE_COVERAGE)
	if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR SIMDLIB_MSVC_STYLE_DRIVER)
		message(FATAL_ERROR "SIMDLIB_ENABLE_COVERAGE currently requires Clang with its GNU-like command-line driver")
	endif()
	target_compile_options(SimdLibDevelopmentWarnings INTERFACE
		-fprofile-instr-generate -fcoverage-mapping)
	target_link_options(SimdLibDevelopmentWarnings INTERFACE
		-fprofile-instr-generate)
endif()
if(SIMDLIB_STRICT_WARNINGS)
	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_options(SimdLibDevelopmentWarnings INTERFACE
				/W4 /WX /permissive-
				-Wno-unknown-attributes -Wno-ignored-attributes -Wno-c2y-extensions)
		else()
			target_compile_options(SimdLibDevelopmentWarnings INTERFACE
				-Wall -Wextra -Wpedantic -Werror
				-Wno-unknown-attributes -Wno-ignored-attributes -Wno-c2y-extensions)
		endif()
	elseif(SIMDLIB_MSVC_STYLE_DRIVER)
		target_compile_options(SimdLibDevelopmentWarnings INTERFACE /W4 /WX /permissive-)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(SimdLibDevelopmentWarnings INTERFACE
            -Wall -Wextra -Wpedantic -Werror
            -Wno-attributes -Wno-ignored-attributes)
    endif()
endif()

function(simdlib_enable_development_warnings target)
    target_link_libraries(${target} PRIVATE SimdLibDevelopmentWarnings)
    if(SIMDLIB_ENABLE_COVERAGE)
        get_target_property(target_type ${target} TYPE)
        if(target_type STREQUAL "EXECUTABLE")
            set_property(GLOBAL APPEND PROPERTY SIMDLIB_COVERAGE_TARGETS ${target})
        endif()
    endif()
endfunction()

# @brief Associates an instrumented executable with the prefix CTest uses for
#        the profiles produced by that executable.
# @param target Instrumented executable target.
# @param profile_prefix Prefix shared by every CTest profile for the target.
function(simdlib_set_coverage_profile_prefix target profile_prefix)
    if(SIMDLIB_ENABLE_COVERAGE)
        set_property(TARGET ${target} PROPERTY
            SIMDLIB_COVERAGE_PROFILE_PREFIX "${profile_prefix}")
    endif()
endfunction()

if(SIMDLIB_BUILD_CONFIGURATION_TESTS)
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
        add_library(SimdLib${config_probe} OBJECT tests/config/${config_probe}.cpp)
        target_link_libraries(SimdLib${config_probe} PRIVATE SimdLib::SimdLib)
        simdlib_enable_development_warnings(SimdLib${config_probe})
    endforeach()
endif()

# @brief Adds a compile-only constexpr contract probe.
# @param target Target name used in compiler diagnostics.
# @param source Translation unit containing static assertions.
function(simdlib_add_constexpr_probe target source)
	add_library(${target} OBJECT ${source})
	target_link_libraries(${target} PRIVATE SimdLib::SimdLib)
	simdlib_enable_development_warnings(${target})
endfunction()

if(SIMDLIB_BUILD_CONFIGURATION_TESTS)
	set(simdlib_constexpr_targets "")

	# @brief Adds one BMI feature-macro compile profile.
	# @param profile_name Profile suffix used in the target name.
	# @param bmi1 Whether BMI1 declarations are enabled.
	# @param bmi2 Whether BMI2 declarations are enabled.
	function(simdlib_add_bmi_constexpr_profile profile_name bmi1 bmi2)
		set(target SimdLibConstexprBmi${profile_name})
		simdlib_add_constexpr_probe(${target} tests/constexpr/BmiConstexpr.tests.cpp)
		target_compile_definitions(${target} PRIVATE SIMDLIB_HAS_BMI1=${bmi1} SIMDLIB_HAS_BMI2=${bmi2})
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
		set(simdlib_constexpr_targets ${simdlib_constexpr_targets} ${target} PARENT_SCOPE)
	endfunction()

	simdlib_add_bmi_constexpr_profile(Portable 0 0)
	simdlib_add_bmi_constexpr_profile(Bmi1Only 1 0)
	simdlib_add_bmi_constexpr_profile(Bmi2Only 0 1)
	simdlib_add_bmi_constexpr_profile(Bmi1AndBmi2 1 1)

	foreach(uint128_profile IN ITEMS Optimized Portable Scalar)
		set(target SimdLibConstexprUInt128${uint128_profile})
		simdlib_add_constexpr_probe(${target} tests/constexpr/UInt128Constexpr.tests.cpp)
		list(APPEND simdlib_constexpr_targets ${target})
		if(uint128_profile STREQUAL "Portable" OR uint128_profile STREQUAL "Scalar")
			target_compile_definitions(${target} PRIVATE SIMDLIB_USE_COMPILER_CARRY_INTRINSICS=0)
		endif()
		if(uint128_profile STREQUAL "Scalar")
			target_compile_definitions(${target} PRIVATE
				SIMDLIB_HAS_SSE=0 SIMDLIB_HAS_SSE2=0 SIMDLIB_HAS_SSE3=0 SIMDLIB_HAS_SSSE3=0
				SIMDLIB_HAS_SSE41=0 SIMDLIB_HAS_SSE42=0 SIMDLIB_HAS_AVX=0 SIMDLIB_HAS_AVX2=0
				SIMDLIB_HAS_FMA=0 SIMDLIB_HAS_BMI1=0 SIMDLIB_HAS_BMI2=0)
		elseif(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_definitions(${target} PRIVATE
				SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
		else()
			target_compile_options(${target} PRIVATE -msse4.2)
		endif()
	endforeach()

	simdlib_add_constexpr_probe(SimdLibConstexprApi128 tests/constexpr/Api128Constexpr.tests.cpp)
	list(APPEND simdlib_constexpr_targets SimdLibConstexprApi128)
	if(SIMDLIB_MSVC_STYLE_DRIVER)
		target_compile_definitions(SimdLibConstexprApi128 PRIVATE
			SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			target_compile_options(SimdLibConstexprApi128 PRIVATE /arch:AVX2)
		endif()
	else()
		target_compile_options(SimdLibConstexprApi128 PRIVATE -msse4.2)
	endif()

	simdlib_add_constexpr_probe(SimdLibConstexprApi256 tests/constexpr/Api256Constexpr.tests.cpp)
	list(APPEND simdlib_constexpr_targets SimdLibConstexprApi256)
	if(SIMDLIB_MSVC_STYLE_DRIVER)
		target_compile_options(SimdLibConstexprApi256 PRIVATE /arch:AVX2)
	else()
		target_compile_options(SimdLibConstexprApi256 PRIVATE -mavx2)
	endif()

	simdlib_add_constexpr_probe(SimdLibConstexprApiDisabled tests/constexpr/ApiDisabledConstexpr.tests.cpp)
	list(APPEND simdlib_constexpr_targets SimdLibConstexprApiDisabled)

	add_custom_target(SimdLibPublicHeaderAssertionAudit
		COMMAND ${CMAKE_COMMAND}
			-DSOURCE_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AuditPublicHeaderAssertions.cmake
		COMMENT "Auditing production-header static assertions"
		VERBATIM)
	add_custom_target(SimdLibConstexprProbes ALL DEPENDS ${simdlib_constexpr_targets})
	add_dependencies(SimdLibConstexprProbes SimdLibPublicHeaderAssertionAudit)
	add_test(NAME SimdLib.PublicHeaderStaticAssertAudit
		COMMAND ${CMAKE_COMMAND}
			-DSOURCE_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AuditPublicHeaderAssertions.cmake)
	set_tests_properties(SimdLib.PublicHeaderStaticAssertAudit PROPERTIES LABELS "CONSTEXPR;AUDIT")
	add_test(NAME SimdLib.ConstexprProbes.Build
		COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config $<CONFIG> --target SimdLibConstexprProbes)
	set_tests_properties(SimdLib.ConstexprProbes.Build PROPERTIES LABELS "CONSTEXPR;COMPILE_ONLY" RUN_SERIAL TRUE)
endif()
if(SIMDLIB_BUILD_HEADER_TESTS)
    foreach(header_probe IN ITEMS
        Config
        TemplateTools
        Api
        SimdApi
        SimdVector
        SimdAlgo
        SimdResample
        Bmi
        UInt128
        Format
        SimdLib
        PublicSurface)
        add_library(SimdLibHeader${header_probe}Probe OBJECT tests/headers/${header_probe}HeaderProbe.cpp)
        target_link_libraries(SimdLibHeader${header_probe}Probe PRIVATE SimdLib::SimdLib)
        simdlib_enable_development_warnings(SimdLibHeader${header_probe}Probe)
    endforeach()

	if(SIMDLIB_REGISTER_COMPILER_SUPPORTED)
		add_library(SimdLibHeaderRegisterProbe OBJECT
			tests/headers/RegisterHeaderProbe.cpp)
		target_link_libraries(SimdLibHeaderRegisterProbe PRIVATE SimdLib::Register)
		simdlib_enable_development_warnings(SimdLibHeaderRegisterProbe)
	endif()
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

if(SIMDLIB_BUILD_CONFIGURATION_TESTS)
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
		${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail/register/RegisterUninitialized.cpp)

	simdlib_add_language_probe(SimdLibRegisterCxx20UmbrellaProbe
		tests/availability/RegisterCxx20UmbrellaProbe.cpp 20 SimdLib::SimdLib)

	if(SIMDLIB_REGISTER_COMPILER_SUPPORTED)
		simdlib_add_language_probe(SimdLibRegisterEnabledProbe
			tests/availability/RegisterEnabledProbe.cpp 23 SimdLib::Register)

		foreach(register_width IN ITEMS 128 256)
			add_library(SimdLibRegisterRepresentation${register_width} OBJECT
				tests/register/RegisterRepresentation.tests.cpp)
			add_library(SimdLibRegisterConstexpr${register_width} OBJECT
				tests/constexpr/RegisterConstexpr.tests.cpp)
			target_link_libraries(SimdLibRegisterRepresentation${register_width} PRIVATE SimdLib::Register)
			target_link_libraries(SimdLibRegisterConstexpr${register_width} PRIVATE SimdLib::Register)
			target_compile_definitions(SimdLibRegisterRepresentation${register_width} PRIVATE
				SIMDLIB_REGISTER_TEST_WIDTH=${register_width})
			target_compile_definitions(SimdLibRegisterConstexpr${register_width} PRIVATE
				SIMDLIB_REGISTER_TEST_WIDTH=${register_width})
			simdlib_enable_development_warnings(SimdLibRegisterRepresentation${register_width})
			simdlib_enable_development_warnings(SimdLibRegisterConstexpr${register_width})
			if(SIMDLIB_MSVC_STYLE_DRIVER)
				target_compile_options(SimdLibRegisterRepresentation${register_width} PRIVATE /arch:AVX2)
				target_compile_options(SimdLibRegisterConstexpr${register_width} PRIVATE /arch:AVX2)
			else()
				target_compile_options(SimdLibRegisterRepresentation${register_width} PRIVATE -mavx2)
				target_compile_options(SimdLibRegisterConstexpr${register_width} PRIVATE -mavx2)
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
		if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			simdlib_add_language_probe(SimdLibRegisterMsvcFallbackProbe
				tests/availability/RegisterMsvcFallbackProbe.cpp 23 SimdLib::Register)
		elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND SIMDLIB_MSVC_STYLE_DRIVER)
			simdlib_add_language_probe(SimdLibRegisterClangClFallbackExclusionProbe
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

# @brief Adds paired wrapper/raw object fixtures and a mandatory disassembly comparison.
# @param register_width Width of the compared native and wrapped register values.
function(simdlib_add_register_codegen_gate register_width)
	set(vectorcall_enabled 0)
	set(stack_protector_mode "compiler-default")
	if(WIN32 AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|amd64|x86_64|i[3-6]86)$" AND
		(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
		set(vectorcall_enabled 1)
	endif()
	if(NOT SIMDLIB_MSVC_STYLE_DRIVER AND
		(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
		set(stack_protector_mode "strong")
	endif()
	set(wrapper_target SimdLibRegisterCodegenWrapper${register_width})
	set(raw_target SimdLibRegisterCodegenRaw${register_width})
	set(default_wrapper_target SimdLibRegisterDefaultAbiWrapper${register_width})
	set(default_raw_target SimdLibRegisterDefaultAbiRaw${register_width})
	set(abi_wrapper_target SimdLibRegisterAbiWrapper${register_width})
	set(abi_raw_target SimdLibRegisterAbiRaw${register_width})
	add_library(${wrapper_target} OBJECT tests/codegen/RegisterCodegen.cpp)
	add_library(${raw_target} OBJECT tests/codegen/RegisterCodegenRaw.cpp)
	add_library(${default_wrapper_target} OBJECT tests/codegen/RegisterDefaultAbi.cpp)
	add_library(${default_raw_target} OBJECT tests/codegen/RegisterDefaultAbiRaw.cpp)
	add_library(${abi_wrapper_target} OBJECT tests/codegen/RegisterAbi.cpp)
	add_library(${abi_raw_target} OBJECT tests/codegen/RegisterAbiRaw.cpp)
	foreach(target IN ITEMS ${wrapper_target} ${raw_target} ${default_wrapper_target} ${default_raw_target}
		${abi_wrapper_target} ${abi_raw_target})
		target_link_libraries(${target} PRIVATE SimdLib::Register)
		target_compile_definitions(${target} PRIVATE SIMDLIB_REGISTER_TEST_WIDTH=${register_width})
		simdlib_enable_development_warnings(${target})
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_options(${target} PRIVATE /O2 /arch:AVX2)
		else()
			target_compile_options(${target} PRIVATE -O2 -mavx2 -fstack-protector-strong)
		endif()
	endforeach()

	set(artifact_directory "${CMAKE_CURRENT_BINARY_DIR}/register-codegen/${register_width}")
	set(stamp_file "${artifact_directory}/comparison.stamp")
	set(lane_stamp_file "${artifact_directory}/lane-comparison.stamp")
	set(default_abi_stamp_file "${artifact_directory}/default-abi.stamp")
	set(abi_stamp_file "${artifact_directory}/abi-comparison.stamp")
	add_custom_command(
		OUTPUT "${stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		COMMAND ${CMAKE_COMMAND} -E touch "${stamp_file}"
		DEPENDS ${wrapper_target} ${raw_target} cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit Register and raw generated code"
		VERBATIM)
	add_custom_command(
		OUTPUT "${lane_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/lanes"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/lanes
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DSYMBOL_PATTERN=simdlib_codegen_lane_
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		COMMAND ${CMAKE_COMMAND} -E touch "${lane_stamp_file}"
		DEPENDS
			$<TARGET_OBJECTS:${wrapper_target}>
			$<TARGET_OBJECTS:${raw_target}>
			cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit Register and raw constant-index lane extraction"
		VERBATIM)
	add_custom_command(
		OUTPUT "${abi_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}/abi"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${abi_wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${abi_raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}/abi
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-DSYMBOL_PATTERN=simdlib_abi_
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareRegisterCodegen.cmake
		COMMAND ${CMAKE_COMMAND} -E touch "${abi_stamp_file}"
		DEPENDS ${abi_wrapper_target} ${abi_raw_target} cmake/CompareRegisterCodegen.cmake
		COMMENT "Comparing ${register_width}-bit explicit-object and raw ABI mirrors"
		VERBATIM)
	add_custom_command(
		OUTPUT "${default_abi_stamp_file}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${artifact_directory}"
		COMMAND ${CMAKE_COMMAND}
			-DWRAPPER_OBJECT=$<TARGET_OBJECTS:${default_wrapper_target}>
			-DRAW_OBJECT=$<TARGET_OBJECTS:${default_raw_target}>
			-DOBJDUMP=${CMAKE_OBJDUMP}
			-DARTIFACT_DIRECTORY=${artifact_directory}
			-DCOMPILER_ID=${CMAKE_CXX_COMPILER_ID}
			-DCOMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
			-DCOMPILER_PATH=${CMAKE_CXX_COMPILER}
			-DSYSTEM_NAME=${CMAKE_SYSTEM_NAME}
			-DSYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}
			-DCONFIGURATION=$<CONFIG>
			-DREGISTER_WIDTH=${register_width}
			-DVECTORCALL_ENABLED=${vectorcall_enabled}
			-DSTACK_PROTECTOR_MODE=${stack_protector_mode}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RecordRegisterDefaultAbi.cmake
		COMMAND ${CMAKE_COMMAND} -E touch "${default_abi_stamp_file}"
		DEPENDS ${default_wrapper_target} ${default_raw_target} cmake/RecordRegisterDefaultAbi.cmake
		COMMENT "Recording ${register_width}-bit platform-default Register ABI"
		VERBATIM)
	set(codegen_gate_outputs
		"${lane_stamp_file}" "${abi_stamp_file}" "${default_abi_stamp_file}")
	if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		list(APPEND codegen_gate_outputs "${stamp_file}")
	endif()
	add_custom_target(SimdLibRegisterCodegen${register_width} ALL DEPENDS ${codegen_gate_outputs})
	add_test(NAME SimdLib.RegisterCodegen.${register_width}
		COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config $<CONFIG>
			--target SimdLibRegisterCodegen${register_width})
	set_tests_properties(SimdLib.RegisterCodegen.${register_width} PROPERTIES
		LABELS "REGISTER;CODEGEN;ABI" RUN_SERIAL TRUE)
endfunction()

if(SIMDLIB_BUILD_REGISTER_CODEGEN AND SIMDLIB_REGISTER_COMPILER_SUPPORTED)
	if(NOT CMAKE_OBJDUMP)
		find_program(CMAKE_OBJDUMP NAMES llvm-objdump llvm-objdump.exe)
	endif()
	if(NOT CMAKE_OBJDUMP)
		message(FATAL_ERROR "Register generated-code gates require an objdump-compatible disassembler")
	endif()
	simdlib_add_register_codegen_gate(128)
	simdlib_add_register_codegen_gate(256)
	add_custom_target(SimdLibRegisterCodegen DEPENDS
		SimdLibRegisterCodegen128 SimdLibRegisterCodegen256)
endif()

add_library(SimdLibAvailabilityDisabledProbe OBJECT tests/availability/ApiDisabledProbe.cpp)
target_link_libraries(SimdLibAvailabilityDisabledProbe PRIVATE SimdLib::SimdLib)
simdlib_enable_development_warnings(SimdLibAvailabilityDisabledProbe)

add_library(SimdLibAvailabilityEnabledProbe OBJECT tests/availability/ApiEnabledProbe.cpp)
target_link_libraries(SimdLibAvailabilityEnabledProbe PRIVATE SimdLib::SimdLib)
simdlib_enable_development_warnings(SimdLibAvailabilityEnabledProbe)
if(SIMDLIB_MSVC_STYLE_DRIVER)
    target_compile_options(SimdLibAvailabilityEnabledProbe PRIVATE /arch:AVX2)
else()
    target_compile_options(SimdLibAvailabilityEnabledProbe PRIVATE -mavx2)
endif()

if(SIMDLIB_BUILD_SMOKE_TESTS)
    add_executable(SimdLibHeaderOnlySmoke
        tests/smoke/main.cpp
        tests/smoke/second_translation_unit.cpp)
    target_link_libraries(SimdLibHeaderOnlySmoke PRIVATE SimdLib::SimdLib)
    simdlib_enable_development_warnings(SimdLibHeaderOnlySmoke)
    add_test(NAME SimdLib.HeaderOnlySmoke COMMAND SimdLibHeaderOnlySmoke)
    simdlib_set_coverage_profile_prefix(SimdLibHeaderOnlySmoke
        "SimdLib.HeaderOnlySmoke")
endif()

if(SIMDLIB_BUILD_TESTS)
    find_package(Catch2 3 CONFIG QUIET)
    if(NOT Catch2_FOUND AND SIMDLIB_FETCH_TEST_DEPENDENCIES)
        include(FetchContent)
        FetchContent_Declare(Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG 2b60af89e23d28eefc081bc930831ee9d45ea58b
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(Catch2)
    endif()
    if(NOT TARGET Catch2::Catch2WithMain)
        message(FATAL_ERROR "Catch2 3 is required; install it or enable SIMDLIB_FETCH_TEST_DEPENDENCIES")
    endif()
    include(Catch)

    # @brief Applies labels after Catch2 has populated its deferred discovery list.
    # @param test_list_variable Name of the Catch2-generated test-list variable.
    # @param labels Semicolon-separated labels applied to every discovered test.
    function(simdlib_label_discovered_tests test_list_variable labels)
        set(label_file "${CMAKE_CURRENT_BINARY_DIR}/${test_list_variable}-labels.cmake")
        file(WRITE "${label_file}"
            "foreach(discovered_test IN LISTS ${test_list_variable})\n"
            "    set_tests_properties(\"\${discovered_test}\" PROPERTIES LABELS \"${labels}\")\n"
            "endforeach()\n")
        set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${label_file}")
    endfunction()

    function(simdlib_add_catch_test target source test_prefix labels)
        add_executable(${target} ${source})
        target_link_libraries(${target} PRIVATE SimdLib::SimdLib Catch2::Catch2WithMain)
        simdlib_enable_development_warnings(${target})
        simdlib_set_coverage_profile_prefix(${target} "${test_prefix}")
        set(test_list_variable "${target}_DISCOVERED_TESTS")
        catch_discover_tests(${target}
            TEST_PREFIX "${test_prefix}."
            TEST_LIST ${test_list_variable})
        simdlib_label_discovered_tests(${test_list_variable} "${labels}")
    endfunction()

	if(SIMDLIB_REGISTER_COMPILER_SUPPORTED)
		simdlib_add_catch_test(SimdLibTestsRegister tests/Register.tests.cpp
			SimdLib.Tests.Register "REGISTER;AVX2")
		target_link_libraries(SimdLibTestsRegister PRIVATE SimdLib::Register)
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_options(SimdLibTestsRegister PRIVATE /arch:AVX2)
		else()
			target_compile_options(SimdLibTestsRegister PRIVATE -mavx2)
		endif()
	endif()

    simdlib_add_catch_test(SimdLibTestsBmiPortable tests/Bmi.tests.cpp
        SimdLib.Tests.BmiPortable "BMI;PORTABLE")
    target_compile_definitions(SimdLibTestsBmiPortable PRIVATE
        SIMDLIB_HAS_BMI1=0 SIMDLIB_HAS_BMI2=0
        SIMDLIB_BMI_EXPECT_BMI1=0 SIMDLIB_BMI_EXPECT_BMI2=0)
	if(NOT SIMDLIB_MSVC_STYLE_DRIVER)
        target_compile_options(SimdLibTestsBmiPortable PRIVATE -mno-bmi -mno-bmi2)
    endif()

    simdlib_add_catch_test(SimdLibTestsFormat tests/Format.tests.cpp
        SimdLib.Tests.Format "FORMAT;SSE42")
	add_executable(SimdLibFormatOdr
		tests/format_odr/main.cpp
		tests/format_odr/second_translation_unit.cpp)
	target_link_libraries(SimdLibFormatOdr PRIVATE SimdLib::SimdLib)
	simdlib_enable_development_warnings(SimdLibFormatOdr)
	add_test(NAME SimdLib.FormatOdr COMMAND SimdLibFormatOdr)
	set_tests_properties(SimdLib.FormatOdr PROPERTIES LABELS "FORMAT;ODR")
	simdlib_set_coverage_profile_prefix(SimdLibFormatOdr "SimdLib.FormatOdr")
	if(SIMDLIB_MSVC_STYLE_DRIVER)
		target_compile_definitions(SimdLibTestsFormat PRIVATE
			SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
		target_compile_definitions(SimdLibFormatOdr PRIVATE
			SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			target_compile_options(SimdLibTestsFormat PRIVATE /arch:AVX2)
			target_compile_options(SimdLibFormatOdr PRIVATE /arch:AVX2)
		endif()
    else()
        target_compile_options(SimdLibTestsFormat PRIVATE -msse4.2)
		target_compile_options(SimdLibFormatOdr PRIVATE -msse4.2)
    endif()

    if(SIMDLIB_BUILD_TESTS_128)
        simdlib_add_catch_test(SimdLibTests128 tests/Api128.tests.cpp
            SimdLib.Tests.SSE42 "SSE42")
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_definitions(SimdLibTests128 PRIVATE
				SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
			if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
				target_compile_options(SimdLibTests128 PRIVATE /arch:AVX2)
			endif()
        else()
            target_compile_options(SimdLibTests128 PRIVATE -msse4.2)
        endif()

        simdlib_add_catch_test(SimdLibTestsUInt128Optimized tests/UInt128.tests.cpp
            SimdLib.Tests.UInt128Optimized "UINT128;OPTIMIZED;SSE42")
        simdlib_add_catch_test(SimdLibTestsUInt128Portable tests/UInt128.tests.cpp
            SimdLib.Tests.UInt128Portable "UINT128;PORTABLE;SSE42")
        simdlib_add_catch_test(SimdLibTestsUInt128Scalar tests/UInt128.tests.cpp
            SimdLib.Tests.UInt128Scalar "UINT128;PORTABLE;SCALAR")
        target_compile_definitions(SimdLibTestsUInt128Portable PRIVATE
            SIMDLIB_USE_COMPILER_CARRY_INTRINSICS=0 SIMDLIB_EXPECT_CARRY_PATH=0)
		target_compile_definitions(SimdLibTestsUInt128Scalar PRIVATE SIMDLIB_EXPECT_CARRY_PATH=0)
		if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			target_compile_definitions(SimdLibTestsUInt128Optimized PRIVATE SIMDLIB_EXPECT_CARRY_PATH=1)
		elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
			target_compile_definitions(SimdLibTestsUInt128Optimized PRIVATE SIMDLIB_EXPECT_CARRY_PATH=2)
		endif()
		target_compile_definitions(SimdLibTestsUInt128Scalar PRIVATE
			SIMDLIB_USE_COMPILER_CARRY_INTRINSICS=0
			SIMDLIB_HAS_SSE=0 SIMDLIB_HAS_SSE2=0 SIMDLIB_HAS_SSE3=0 SIMDLIB_HAS_SSSE3=0
			SIMDLIB_HAS_SSE41=0 SIMDLIB_HAS_SSE42=0 SIMDLIB_HAS_AVX=0 SIMDLIB_HAS_AVX2=0
			SIMDLIB_HAS_FMA=0 SIMDLIB_HAS_BMI1=0 SIMDLIB_HAS_BMI2=0)
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_definitions(SimdLibTestsUInt128Optimized PRIVATE
				SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
			target_compile_definitions(SimdLibTestsUInt128Portable PRIVATE
				SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
			if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
				target_compile_options(SimdLibTestsUInt128Optimized PRIVATE /arch:AVX2)
				target_compile_options(SimdLibTestsUInt128Portable PRIVATE /arch:AVX2)
			endif()
        else()
            target_compile_options(SimdLibTestsUInt128Optimized PRIVATE -msse4.2)
            target_compile_options(SimdLibTestsUInt128Portable PRIVATE -msse4.2)
        endif()
        add_test(NAME SimdLib.Tests.UInt128ResultSetEquivalence
            COMMAND ${CMAKE_COMMAND}
                -DPORTABLE_EXECUTABLE=$<TARGET_FILE:SimdLibTestsUInt128Portable>
                -DOPTIMIZED_EXECUTABLE=$<TARGET_FILE:SimdLibTestsUInt128Optimized>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareUInt128ResultSets.cmake)
        set_tests_properties(SimdLib.Tests.UInt128ResultSetEquivalence PROPERTIES LABELS "UINT128;EQUIVALENCE;SSE42")

		add_test(NAME SimdLib.Tests.UInt128ScalarResultSetEquivalence
			COMMAND ${CMAKE_COMMAND}
				-DPORTABLE_EXECUTABLE=$<TARGET_FILE:SimdLibTestsUInt128Scalar>
				-DOPTIMIZED_EXECUTABLE=$<TARGET_FILE:SimdLibTestsUInt128Optimized>
				-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareUInt128ResultSets.cmake)
		set_tests_properties(SimdLib.Tests.UInt128ScalarResultSetEquivalence PROPERTIES LABELS "UINT128;EQUIVALENCE;SCALAR")
    endif()

    if(SIMDLIB_BUILD_TESTS_256)
        simdlib_add_catch_test(SimdLibTests256 tests/Api256.tests.cpp
            SimdLib.Tests.AVX2 "AVX2")
		if(SIMDLIB_MSVC_STYLE_DRIVER)
            target_compile_options(SimdLibTests256 PRIVATE /arch:AVX2)
        else()
            target_compile_options(SimdLibTests256 PRIVATE -mavx2)
        endif()
    endif()

    if(SIMDLIB_BUILD_TESTS_FMA)
        simdlib_add_catch_test(SimdLibTestsFmaEnabled tests/SimdFma.tests.cpp
            SimdLib.Tests.FMA.Enabled "FMA;ENABLED")
        simdlib_add_catch_test(SimdLibTestsFmaDisabled tests/SimdFma.tests.cpp
            SimdLib.Tests.FMA.Disabled "FMA;DISABLED")
		target_compile_definitions(SimdLibTestsFmaEnabled PRIVATE SIMDLIB_HAS_FMA=1 SIMDLIB_EXPECT_FMA=1)
        target_compile_definitions(SimdLibTestsFmaDisabled PRIVATE SIMDLIB_HAS_FMA=0 SIMDLIB_EXPECT_FMA=0)
		if(SIMDLIB_MSVC_STYLE_DRIVER)
            target_compile_options(SimdLibTestsFmaEnabled PRIVATE /arch:AVX2)
            target_compile_options(SimdLibTestsFmaDisabled PRIVATE /arch:AVX2)
        else()
            target_compile_options(SimdLibTestsFmaEnabled PRIVATE -mavx2 -mfma)
            target_compile_options(SimdLibTestsFmaDisabled PRIVATE -mavx2 -mno-fma)
        endif()
    endif()

    if(SIMDLIB_BUILD_TESTS_OPTIONAL)
        function(simdlib_add_bmi_profile profile_name bmi1 bmi2)
            set(target SimdLibTestsBmi${profile_name})
            set(test_name SimdLib.Tests.Bmi.${profile_name})
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
            set(equivalence_name SimdLib.Tests.Bmi.${profile_name}.Equivalence)
            add_test(NAME ${equivalence_name}
                COMMAND ${CMAKE_COMMAND}
                    -DPORTABLE_EXECUTABLE=$<TARGET_FILE:SimdLibTestsBmiPortable>
                    -DENABLED_EXECUTABLE=$<TARGET_FILE:${target}>
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompareBmiResultSets.cmake)
            set_tests_properties(${equivalence_name} PROPERTIES LABELS "BMI;EQUIVALENCE;${profile_name};OPTIONAL")
        endfunction()

        simdlib_add_bmi_profile(Bmi1Only 1 0)
        simdlib_add_bmi_profile(Bmi2Only 0 1)
        simdlib_add_bmi_profile(Bmi1AndBmi2 1 1)
    endif()

    if(SIMDLIB_BUILD_VECTOR_ALGORITHM_TESTS)
        add_executable(SimdLibTestsVectorAlgorithms
            tests/SimdVector.tests.cpp
            tests/SimdAlgo.tests.cpp
            tests/PreconditionBoundary.tests.cpp
            tests/SimdResample.tests.cpp)
        target_link_libraries(SimdLibTestsVectorAlgorithms PRIVATE SimdLib::SimdLib Catch2::Catch2WithMain)
        simdlib_enable_development_warnings(SimdLibTestsVectorAlgorithms)
        simdlib_set_coverage_profile_prefix(SimdLibTestsVectorAlgorithms
            "SimdLib.Tests.VectorAlgorithms")
        catch_discover_tests(SimdLibTestsVectorAlgorithms
            TEST_PREFIX "SimdLib.Tests.VectorAlgorithms."
            TEST_LIST SimdLibTestsVectorAlgorithms_DISCOVERED_TESTS)
        simdlib_label_discovered_tests(SimdLibTestsVectorAlgorithms_DISCOVERED_TESTS
            "VECTOR_ALGORITHMS;AVX2")
		if(SIMDLIB_MSVC_STYLE_DRIVER)
            target_compile_options(SimdLibTestsVectorAlgorithms PRIVATE /arch:AVX2)
        else()
            target_compile_options(SimdLibTestsVectorAlgorithms PRIVATE -mavx2 -mfma)
        endif()

		simdlib_add_catch_test(SimdLibTestsVectorChecks tests/SimdVectorChecks.tests.cpp
			SimdLib.Tests.VectorChecks "VECTOR_ALGORITHMS;AVX2;CHECKS")
		target_compile_definitions(SimdLibTestsVectorChecks PRIVATE SIMDLIB_ENABLE_CHECKS=1)
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_options(SimdLibTestsVectorChecks PRIVATE /arch:AVX2)
		else()
			target_compile_options(SimdLibTestsVectorChecks PRIVATE -mavx2 -mfma)
		endif()

		add_executable(SimdLibPreconditionTests tests/PreconditionFailure.tests.cpp)
		target_link_libraries(SimdLibPreconditionTests PRIVATE SimdLib::SimdLib Catch2::Catch2WithMain)
		simdlib_enable_development_warnings(SimdLibPreconditionTests)
		simdlib_set_coverage_profile_prefix(SimdLibPreconditionTests
			"SimdLib.Tests.Preconditions")
		target_compile_definitions(SimdLibPreconditionTests PRIVATE SIMDLIB_ENABLE_CHECKS=1)
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_options(SimdLibPreconditionTests PRIVATE /arch:AVX2)
		else()
			target_compile_options(SimdLibPreconditionTests PRIVATE -mavx2 -mfma)
		endif()
		catch_discover_tests(SimdLibPreconditionTests
			TEST_PREFIX "SimdLib.Tests.Preconditions."
			TEST_LIST SimdLibPreconditionTests_DISCOVERED_TESTS
			PROPERTIES
				PASS_REGULAR_EXPRESSION "SIMDLIB_PRECONDITION_FAILURE_EXPECTED_18A7E3"
				TIMEOUT 10)
		simdlib_label_discovered_tests(SimdLibPreconditionTests_DISCOVERED_TESTS
			"PRECONDITIONS;CHECKS;AVX2")

        add_executable(SimdLibTestsResampleScalar tests/SimdResample.tests.cpp)
        target_link_libraries(SimdLibTestsResampleScalar PRIVATE SimdLib::SimdLib Catch2::Catch2WithMain)
        simdlib_enable_development_warnings(SimdLibTestsResampleScalar)
        simdlib_set_coverage_profile_prefix(SimdLibTestsResampleScalar
            "SimdLib.Tests.ResampleScalar")
        target_compile_definitions(SimdLibTestsResampleScalar PRIVATE
            SIMDLIB_HAS_SSE3=0 SIMDLIB_HAS_SSSE3=0 SIMDLIB_HAS_SSE41=0 SIMDLIB_HAS_SSE42=0
            SIMDLIB_HAS_AVX=0 SIMDLIB_HAS_AVX2=0 SIMDLIB_HAS_FMA=0)
        catch_discover_tests(SimdLibTestsResampleScalar
            TEST_PREFIX "SimdLib.Tests.ResampleScalar."
            TEST_LIST SimdLibTestsResampleScalar_DISCOVERED_TESTS)
        simdlib_label_discovered_tests(SimdLibTestsResampleScalar_DISCOVERED_TESTS
            "VECTOR_ALGORITHMS;SCALAR")
    endif()
endif()

if(SIMDLIB_BUILD_BENCHMARKS)
    if(NOT TARGET Catch2::Catch2WithMain)
        find_package(Catch2 3 CONFIG QUIET)
    endif()
    if(NOT Catch2_FOUND AND NOT TARGET Catch2::Catch2WithMain AND SIMDLIB_FETCH_TEST_DEPENDENCIES)
        include(FetchContent)
        FetchContent_Declare(Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG 2b60af89e23d28eefc081bc930831ee9d45ea58b
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(Catch2)
    endif()
    if(NOT TARGET Catch2::Catch2WithMain)
        message(FATAL_ERROR "Catch2 3 is required; install it or enable SIMDLIB_FETCH_TEST_DEPENDENCIES")
    endif()
    add_executable(SimdLibBenchmarks benchmarks/SimdLib.benchmarks.cpp)
    target_link_libraries(SimdLibBenchmarks PRIVATE SimdLib::SimdLib Catch2::Catch2WithMain)
    simdlib_enable_development_warnings(SimdLibBenchmarks)
    target_compile_definitions(SimdLibBenchmarks PRIVATE SIMDLIB_HAS_BMI1=1 SIMDLIB_HAS_BMI2=1)
	if(SIMDLIB_MSVC_STYLE_DRIVER)
        target_compile_options(SimdLibBenchmarks PRIVATE /arch:AVX2)
    else()
        target_compile_options(SimdLibBenchmarks PRIVATE -mavx2 -mfma -mbmi -mbmi2)
    endif()
endif()

if(SIMDLIB_BUILD_EXAMPLES)
    add_executable(SimdLibApiExamples examples/ApiExamples.cpp)
    target_link_libraries(SimdLibApiExamples PRIVATE SimdLib::SimdLib)
    simdlib_enable_development_warnings(SimdLibApiExamples)
	if(SIMDLIB_MSVC_STYLE_DRIVER)
        target_compile_options(SimdLibApiExamples PRIVATE /arch:AVX2)
    else()
        target_compile_options(SimdLibApiExamples PRIVATE -mavx2 -mfma -mbmi -mbmi2)
    endif()
    add_test(NAME SimdLib.ApiExamples COMMAND SimdLibApiExamples)
    set_tests_properties(SimdLib.ApiExamples PROPERTIES LABELS "EXAMPLES;AVX2;FMA;BMI")
    simdlib_set_coverage_profile_prefix(SimdLibApiExamples "SimdLib.ApiExamples")
endif()

if(SIMDLIB_ENABLE_COVERAGE)
    get_filename_component(simdlib_compiler_directory "${CMAKE_CXX_COMPILER}" DIRECTORY)
    find_program(SIMDLIB_LLVM_PROFDATA
        NAMES llvm-profdata
        HINTS "${simdlib_compiler_directory}"
        REQUIRED)
    find_program(SIMDLIB_LLVM_COV
        NAMES llvm-cov
        HINTS "${simdlib_compiler_directory}"
        REQUIRED)
    find_program(SIMDLIB_LLVM_READOBJ
        NAMES llvm-readobj
        HINTS "${simdlib_compiler_directory}"
        REQUIRED)

    get_property(simdlib_coverage_targets GLOBAL PROPERTY SIMDLIB_COVERAGE_TARGETS)
    list(REMOVE_DUPLICATES simdlib_coverage_targets)
    if(NOT simdlib_coverage_targets)
        message(FATAL_ERROR "SIMDLIB_ENABLE_COVERAGE requires at least one executable target")
    endif()

    set(simdlib_coverage_manifest "")
    foreach(coverage_target IN LISTS simdlib_coverage_targets)
        get_target_property(coverage_profile_prefix ${coverage_target}
            SIMDLIB_COVERAGE_PROFILE_PREFIX)
        if(NOT coverage_profile_prefix)
            message(FATAL_ERROR
                "Coverage target ${coverage_target} has no CTest profile prefix")
        endif()
        string(APPEND simdlib_coverage_manifest
            "${coverage_target}|$<TARGET_FILE:${coverage_target}>|${coverage_profile_prefix}\n")
    endforeach()
    set(simdlib_coverage_manifest_file
        "${CMAKE_CURRENT_BINARY_DIR}/coverage-targets-$<CONFIG>.txt")
    file(GENERATE
        OUTPUT "${simdlib_coverage_manifest_file}"
        CONTENT "${simdlib_coverage_manifest}")

    add_custom_target(SimdLibCoverageReset
        COMMAND ${CMAKE_COMMAND}
            -DBINARY_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ResetCoverage.cmake
        COMMENT "Removing previous SimdLib coverage data"
        VERBATIM)

    add_custom_target(SimdLibCoverageReport
        COMMAND ${CMAKE_COMMAND}
            -DBINARY_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}
            -DSOURCE_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}
            -DCOVERAGE_MANIFEST=${simdlib_coverage_manifest_file}
            -DLLVM_PROFDATA=${SIMDLIB_LLVM_PROFDATA}
            -DLLVM_COV=${SIMDLIB_LLVM_COV}
            -DLLVM_READOBJ=${SIMDLIB_LLVM_READOBJ}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateCoverageReport.cmake
        DEPENDS ${simdlib_coverage_targets}
        COMMENT "Generating SimdLib LCOV coverage report"
        VERBATIM)
endif()
