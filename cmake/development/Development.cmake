include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "Development.cmake is available only to top-level SimdLib builds")
endif()
if(NOT TARGET SimdLib OR NOT TARGET SimdLibRegister)
    message(FATAL_ERROR "Development.cmake requires the production SimdLib targets")
endif()

include(CTest)

block(SCOPE_FOR VARIABLES)

set(simdlib_development_modules
    Options
    TargetConfiguration
    ArtifactOwnership
    Dependencies
    ConfigurationProbes
	ConfigurationStateProbes
	MethodFlagsCodegen
    ConstexprProbes
    HeaderProbes
    RegisterCodegen
    SmokeTests
    RuntimeTests
    Examples
    Benchmarks
    Coverage
    ArtifactAggregates)
foreach(simdlib_development_module IN LISTS simdlib_development_modules)
    set(simdlib_development_module_path
        "${CMAKE_CURRENT_LIST_DIR}/${simdlib_development_module}.cmake")
    if(NOT EXISTS "${simdlib_development_module_path}")
        message(FATAL_ERROR
            "Development coordinator cannot locate ${simdlib_development_module_path}")
    endif()
    include("${simdlib_development_module_path}")
endforeach()

include("${CMAKE_CURRENT_LIST_FILE}")

endblock()
