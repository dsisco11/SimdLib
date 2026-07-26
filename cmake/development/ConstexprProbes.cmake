include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "ConstexprProbes.cmake is available only to top-level SimdLib builds")
endif()
if(NOT TARGET SimdLib OR NOT TARGET SimdLibRegister)
    message(FATAL_ERROR "ConstexprProbes.cmake requires the production SimdLib targets")
endif()

block(SCOPE_FOR VARIABLES)

# @brief Adds a compile-only constexpr contract probe.
# @param target Target name used in compiler diagnostics.
# @param source Translation unit containing static assertions.
function(simdlib_add_constexpr_probe target source)
	add_library(${target} OBJECT ${source})
	target_link_libraries(${target} PRIVATE SimdLib::SimdLib)
	simdlib_enable_development_warnings(${target})
endfunction()

if(SIMDLIB_BUILD_CONSTEXPR_PROBES)
	set(simdlib_constexpr_targets "")

	# @brief Adds one BMI feature-macro compile profile.
	# @param profile_name Profile suffix used in the target name.
	# @param bmi1 Whether BMI1 declarations are enabled.
	# @param bmi2 Whether BMI2 declarations are enabled.
	function(simdlib_add_bmi_constexpr_profile profile_name bmi1 bmi2)
		set(target Bmi${profile_name}ConstexprProbe)
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
	simdlib_add_bmi_constexpr_profile(1 1 0)
	simdlib_add_bmi_constexpr_profile(2 0 1)
	simdlib_add_bmi_constexpr_profile(1Bmi2 1 1)

	foreach(uint128_profile IN ITEMS Optimized Portable Scalar)
		set(target UInt128${uint128_profile}ConstexprProbe)
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

	simdlib_add_constexpr_probe(ApiSse42ConstexprProbe tests/constexpr/Api128Constexpr.tests.cpp)
	list(APPEND simdlib_constexpr_targets ApiSse42ConstexprProbe)
	if(SIMDLIB_MSVC_STYLE_DRIVER)
		target_compile_definitions(ApiSse42ConstexprProbe PRIVATE
			SIMDLIB_HAS_SSE3=1 SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1)
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			target_compile_options(ApiSse42ConstexprProbe PRIVATE /arch:AVX2)
		endif()
	else()
		target_compile_options(ApiSse42ConstexprProbe PRIVATE -msse4.2)
	endif()

	simdlib_add_constexpr_probe(ApiAvx2ConstexprProbe tests/constexpr/Api256Constexpr.tests.cpp)
	list(APPEND simdlib_constexpr_targets ApiAvx2ConstexprProbe)
	if(SIMDLIB_MSVC_STYLE_DRIVER)
		target_compile_options(ApiAvx2ConstexprProbe PRIVATE /arch:AVX2)
	else()
		target_compile_options(ApiAvx2ConstexprProbe PRIVATE -mavx2)
	endif()

	simdlib_add_constexpr_probe(ApiDisabledConstexprProbe tests/constexpr/ApiDisabledConstexpr.tests.cpp)
	list(APPEND simdlib_constexpr_targets ApiDisabledConstexprProbe)

	set(constexpr_object_expressions "")
	foreach(constexpr_target IN LISTS simdlib_constexpr_targets)
		list(APPEND constexpr_object_expressions "$<TARGET_OBJECTS:${constexpr_target}>")
	endforeach()
	set(constexpr_record "${CMAKE_CURRENT_BINARY_DIR}/constexpr-probes/artifacts.record")
	add_custom_command(
		OUTPUT "${constexpr_record}"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/constexpr-probes"
		COMMAND ${CMAKE_COMMAND}
			-DMODE=RECORD
			-DRECORD_FILE=${constexpr_record}
			"-DARTIFACTS=$<JOIN:${constexpr_object_expressions},|>"
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RecordArtifactHashes.cmake
		DEPENDS ${constexpr_object_expressions} cmake/RecordArtifactHashes.cmake
		COMMENT "Recording constexpr probe artifacts"
		VERBATIM)
	add_custom_target(ConstexprProbes ALL DEPENDS "${constexpr_record}")
	add_dependencies(ConstexprProbes ${simdlib_constexpr_targets})
	if(TARGET PublicHeaderAssertionAudit)
		add_dependencies(ConstexprProbes PublicHeaderAssertionAudit)
	endif()
	add_test(NAME ConstexprProbes.Artifacts
		COMMAND ${CMAKE_COMMAND}
			-DMODE=VALIDATE
			-DRECORD_FILE=${constexpr_record}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RecordArtifactHashes.cmake)
	set_tests_properties(ConstexprProbes.Artifacts PROPERTIES
		LABELS "CONSTEXPR;COMPILE_ONLY" RUN_SERIAL TRUE)
endif()

endblock()
