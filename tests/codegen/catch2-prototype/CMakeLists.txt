cmake_minimum_required(VERSION 3.31)
project(SimdLibCatch2CodegenPrototype LANGUAGES CXX)
include(CTest)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Catch2 3 CONFIG QUIET)
if(NOT TARGET Catch2::Catch2WithMain)
    set(SIMDLIB_CATCH2_SOURCE "" CACHE PATH "Existing pinned Catch2 source for this isolated prototype")
    if(NOT EXISTS "${SIMDLIB_CATCH2_SOURCE}/CMakeLists.txt")
        message(FATAL_ERROR "Install Catch2 3 or supply SIMDLIB_CATCH2_SOURCE; no implicit download")
    endif()
    add_subdirectory("${SIMDLIB_CATCH2_SOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/catch2" EXCLUDE_FROM_ALL)
    list(APPEND CMAKE_MODULE_PATH "${SIMDLIB_CATCH2_SOURCE}/extras")
endif()
get_filename_component(SIMDLIB_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_subdirectory("${SIMDLIB_SOURCE_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/library" EXCLUDE_FROM_ALL)
include("${SIMDLIB_SOURCE_ROOT}/cmake/development/CodegenTests.cmake")
simdlib_select_codegen_tools()
set(symbol simdlib_contract_transfer_load_operate_store)
set(supplement no-call-stack)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(supplement msvc-cookie)
endif()
if(MSVC)
    set(options /O1 /Ob2 /Gy /arch:AVX2 /GS)
else()
    set(options -O1 -ffunction-sections -mavx2 -mno-fma -fstack-protector-strong)
endif()
file(GLOB_RECURSE inputs CONFIGURE_DEPENDS
    "${SIMDLIB_SOURCE_ROOT}/include/*" "${SIMDLIB_SOURCE_ROOT}/cmake/codegen/*"
    "${SIMDLIB_SOURCE_ROOT}/tests/codegen/contracts/*"
    "${SIMDLIB_SOURCE_ROOT}/tests/codegen/register/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/*")
simdlib_add_codegen_fixture(TARGET Catch2CodegenObject
    SOURCE "${SIMDLIB_SOURCE_ROOT}/tests/codegen/register/transfer.cpp"
    CONFIGURATION "${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}-avx2-provisional"
    GEOMETRY f32-w128-nall OPTIONS ${options}
    DEFINITIONS SIMDLIB_CONTRACT_WIDTH=128 SIMDLIB_HAS_FMA=0 NDEBUG INPUTS ${inputs})
get_target_property(fixture_root Catch2CodegenObject SIMDLIB_CODEGEN_ROOT)
configure_file(Configuration.h.in "${CMAKE_CURRENT_BINARY_DIR}/Configuration.h.in" @ONLY)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Configuration.h"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/Configuration.h.in")
add_executable(Catch2CodegenPrototype Process.cpp Codegen.tests.cpp)
target_compile_features(Catch2CodegenPrototype PRIVATE cxx_std_20)
set_property(TARGET Catch2CodegenPrototype PROPERTY CXX_SCAN_FOR_MODULES OFF)
target_include_directories(Catch2CodegenPrototype PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(Catch2CodegenPrototype PRIVATE Catch2::Catch2WithMain)
add_dependencies(Catch2CodegenPrototype Catch2CodegenObjectReceipt)
include(Catch)
catch_discover_tests(Catch2CodegenPrototype TEST_PREFIX "Catch2Codegen."
    PROPERTIES LABELS "CODEGEN_PROTOTYPE")
