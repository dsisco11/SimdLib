cmake_minimum_required(VERSION 3.31)

project(SimdLibInstalledConsumer LANGUAGES CXX)

find_package(SimdLib CONFIG REQUIRED)

if(NOT TARGET SimdLib::SimdLib OR NOT TARGET SimdLib::Register)
	message(FATAL_ERROR "The installed package must export both public SimdLib targets")
endif()

get_target_property(simdlib_register_features SimdLib::Register INTERFACE_COMPILE_FEATURES)
get_target_property(simdlib_register_definitions SimdLib::Register INTERFACE_COMPILE_DEFINITIONS)
get_target_property(simdlib_core_includes SimdLib::SimdLib INTERFACE_INCLUDE_DIRECTORIES)
if(NOT "cxx_std_23" IN_LIST simdlib_register_features)
	message(FATAL_ERROR "The installed Register target must retain its C++23 requirement")
endif()
if(NOT "SIMDLIB_REQUIRE_REGISTER_INTERFACE=1" IN_LIST simdlib_register_definitions)
	message(FATAL_ERROR "The installed Register target must retain its interface requirement signal")
endif()
foreach(simdlib_core_include IN LISTS simdlib_core_includes)
	cmake_path(IS_PREFIX SIMDLIB_FORBIDDEN_INCLUDE_DIR "${simdlib_core_include}" NORMALIZE simdlib_uses_source_tree)
	if(simdlib_uses_source_tree)
		message(FATAL_ERROR "Installed target exposes source-tree include path: ${simdlib_core_include}")
	endif()
endforeach()

enable_testing()

# @brief Adds one installed-package consumer for a PartialRegister ISA profile.
# @param target Downstream executable target name.
# @param bits Physical PartialRegister width exercised by the consumer.
function(simdlib_add_installed_partial_register_consumer target bits)
	add_executable(${target} main.cpp)
	target_link_libraries(${target} PRIVATE SimdLib::Register)
	target_compile_definitions(${target} PRIVATE
		SIMDLIB_INSTALLED_CONSUMER_BITS=${bits}
		SIMDLIB_HAS_SSE=1 SIMDLIB_HAS_SSE2=1 SIMDLIB_HAS_SSE3=1
		SIMDLIB_HAS_SSSE3=1 SIMDLIB_HAS_SSE41=1 SIMDLIB_HAS_SSE42=1
		SIMDLIB_HAS_FMA=0)
	if(bits EQUAL 256)
		target_compile_definitions(${target} PRIVATE SIMDLIB_HAS_AVX=1 SIMDLIB_HAS_AVX2=1)
		if(MSVC)
			target_compile_options(${target} PRIVATE /arch:AVX2)
		else()
			target_compile_options(${target} PRIVATE -mavx2)
		endif()
	else()
		target_compile_definitions(${target} PRIVATE SIMDLIB_HAS_AVX=0 SIMDLIB_HAS_AVX2=0)
		if(MSVC)
			if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
				target_compile_options(${target} PRIVATE /clang:-msse4.2 /clang:-mno-avx /clang:-mno-avx2)
			endif()
		else()
			target_compile_options(${target} PRIVATE -msse4.2 -mno-avx -mno-avx2)
		endif()
	endif()
	add_test(NAME ${target} COMMAND ${target})
endfunction()

simdlib_add_installed_partial_register_consumer(SimdLibInstalledConsumerSse42 128)
simdlib_add_installed_partial_register_consumer(SimdLibInstalledConsumerAvx2 256)
