include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "TargetConfiguration.cmake is available only to top-level SimdLib builds")
endif()
if(NOT TARGET SimdLib OR NOT TARGET SimdLibRegister)
    message(FATAL_ERROR "TargetConfiguration.cmake requires the production SimdLib targets")
endif()

add_library(DevelopmentWarnings INTERFACE)
if(SIMDLIB_ENABLE_COVERAGE)
	if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR SIMDLIB_MSVC_STYLE_DRIVER)
		message(FATAL_ERROR "SIMDLIB_ENABLE_COVERAGE currently requires Clang with its GNU-like command-line driver")
	endif()
	target_compile_options(DevelopmentWarnings INTERFACE
		-fprofile-instr-generate -fcoverage-mapping)
	target_link_options(DevelopmentWarnings INTERFACE
		-fprofile-instr-generate)
endif()
if(SIMDLIB_STRICT_WARNINGS)
	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		if(SIMDLIB_MSVC_STYLE_DRIVER)
			target_compile_options(DevelopmentWarnings INTERFACE
				/W4 /WX /permissive-
				-Wno-unknown-attributes -Wno-ignored-attributes -Wno-c2y-extensions)
		else()
			target_compile_options(DevelopmentWarnings INTERFACE
				-Wall -Wextra -Wpedantic -Werror
				-Wno-unknown-attributes -Wno-ignored-attributes -Wno-c2y-extensions)
		endif()
	elseif(SIMDLIB_MSVC_STYLE_DRIVER)
		target_compile_options(DevelopmentWarnings INTERFACE /W4 /WX /permissive-)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(DevelopmentWarnings INTERFACE
            -Wall -Wextra -Wpedantic -Werror
            -Wno-attributes -Wno-ignored-attributes)
    endif()
endif()

# @brief Applies shared development warnings and coverage instrumentation.
# @param target Target that receives the development usage requirements.
function(simdlib_enable_development_warnings target)
    target_link_libraries(${target} PRIVATE DevelopmentWarnings)
    if(SIMDLIB_ENABLE_COVERAGE)
        get_target_property(target_type ${target} TYPE)
        if(target_type STREQUAL "EXECUTABLE")
            set_property(GLOBAL APPEND PROPERTY SIMDLIB_COVERAGE_TARGETS ${target})
        endif()
    endif()
endfunction()

# @brief Compiles one target for the supported 128-bit SSE4.2 Register profile.
# @param target Target that must not acquire AVX-family availability.
function(simdlib_enable_register_sse42 target)
	target_compile_definitions(${target} PRIVATE
		SIMDLIB_HAS_SSE=1 SIMDLIB_HAS_SSE2=1 SIMDLIB_HAS_SSE3=1
		SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1
		SIMDLIB_HAS_AVX=0 SIMDLIB_HAS_AVX2=0 SIMDLIB_HAS_FMA=0)
	if(SIMDLIB_MSVC_STYLE_DRIVER)
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			target_compile_options(${target} PRIVATE
				/clang:-msse4.2 /clang:-mno-avx /clang:-mno-avx2 /clang:-mno-fma)
		endif()
	else()
		target_compile_options(${target} PRIVATE
			-msse4.2 -mno-avx -mno-avx2 -mno-fma)
	endif()
endfunction()

# @brief Compiles one target for the supported 128-bit and 256-bit AVX2 Register profile.
# @param target Target that receives AVX2 code-generation options.
function(simdlib_enable_register_avx2 target)
	if(SIMDLIB_MSVC_STYLE_DRIVER)
		target_compile_options(${target} PRIVATE /arch:AVX2)
	else()
		target_compile_options(${target} PRIVATE -mavx2)
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
