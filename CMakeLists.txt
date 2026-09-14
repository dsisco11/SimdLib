cmake_minimum_required(VERSION 3.31)

project(SimdLib VERSION 0.2.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(SimdLib INTERFACE)
add_library(SimdLib::SimdLib ALIAS SimdLib)
target_compile_features(SimdLib INTERFACE cxx_std_20)
target_include_directories(SimdLib INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
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

set(SIMDLIB_MSVC_STYLE_DRIVER ${MSVC})
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang"
        AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
    set(SIMDLIB_MSVC_STYLE_DRIVER OFF)
endif()

set(SIMDLIB_REGISTER_COMPILER_SUPPORTED OFF)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"
        AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.44)
    set(SIMDLIB_REGISTER_COMPILER_SUPPORTED ON)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
        AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 20)
    set(SIMDLIB_REGISTER_COMPILER_SUPPORTED ON)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14
        AND CMAKE_SYSTEM_NAME STREQUAL "Linux"
        AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SIMDLIB_REGISTER_COMPILER_SUPPORTED ON)
endif()
set_property(TARGET SimdLibRegister PROPERTY
    SIMDLIB_REGISTER_COMPILER_SUPPORTED ${SIMDLIB_REGISTER_COMPILER_SUPPORTED})
set_property(TARGET SimdLibRegister PROPERTY EXPORT_NAME Register)

install(TARGETS SimdLib SimdLibRegister EXPORT SimdLibTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT SimdLibTargets
	FILE SimdLibTargets.cmake
	NAMESPACE SimdLib::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SimdLib)

configure_package_config_file(
	cmake/SimdLibConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/SimdLibConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SimdLib)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/SimdLibConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion)
install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/SimdLibConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/SimdLibConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SimdLib)

if(PROJECT_IS_TOP_LEVEL)
    include(cmake/development/Development.cmake)
endif()
