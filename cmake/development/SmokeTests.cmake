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
	endif()
endif()

endblock()
