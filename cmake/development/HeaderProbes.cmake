include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "HeaderProbes.cmake is available only to top-level SimdLib builds")
endif()
if(NOT TARGET SimdLib OR NOT TARGET SimdLibRegister)
    message(FATAL_ERROR "HeaderProbes.cmake requires the production SimdLib targets")
endif()

block(SCOPE_FOR VARIABLES)

if(SIMDLIB_BUILD_HEADER_PROBES)
    set(simdlib_installed_header_root
        "${CMAKE_CURRENT_BINARY_DIR}/installed-header-probe/include")
    file(GLOB_RECURSE simdlib_installed_headers
        CONFIGURE_DEPENDS
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/SimdLib/*.h")
    foreach(simdlib_installed_header IN LISTS simdlib_installed_headers)
        get_filename_component(simdlib_installed_header_directory
            "${simdlib_installed_header}" DIRECTORY)
        file(MAKE_DIRECTORY
            "${simdlib_installed_header_root}/${simdlib_installed_header_directory}")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/include/${simdlib_installed_header}"
            "${simdlib_installed_header_root}/${simdlib_installed_header}"
            COPYONLY)
    endforeach()

    # @brief Configures a compiler-contract target against only the copied public headers.
    # @param target Existing target that consumes the isolated header image.
    # @param standard Exact C++ language standard required by the target.
    function(simdlib_configure_installed_header_probe target standard)
        target_include_directories(${target} PRIVATE
            "${simdlib_installed_header_root}")
        set_target_properties(${target} PROPERTIES
            CXX_STANDARD ${standard}
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF)
        simdlib_register_development_target(${target} COMPILER_CONTRACT)
        simdlib_enable_development_warnings(${target})
    endfunction()

    add_library(InstalledConfigHeaderProbe OBJECT
        tests/headers/InstalledConfigHeaderProbe.cpp)
    simdlib_configure_installed_header_probe(InstalledConfigHeaderProbe 20)

    add_library(InstalledUmbrellaHeaderProbe OBJECT
        tests/headers/InstalledUmbrellaHeaderProbe.cpp)
    simdlib_configure_installed_header_probe(InstalledUmbrellaHeaderProbe 20)

    add_library(InstalledDisabledHeaderProbe OBJECT
        tests/headers/InstalledDisabledHeaderProbe.cpp)
    simdlib_configure_installed_header_probe(InstalledDisabledHeaderProbe 20)
    target_compile_definitions(InstalledDisabledHeaderProbe PRIVATE
        SIMDLIB_HAS_SSE=0 SIMDLIB_HAS_SSE2=0 SIMDLIB_HAS_SSE3=0
        SIMDLIB_HAS_SSSE3=0 SIMDLIB_HAS_SSE41=0 SIMDLIB_HAS_SSE42=0
        SIMDLIB_HAS_AVX=0 SIMDLIB_HAS_AVX2=0 SIMDLIB_HAS_FMA=0
        SIMDLIB_HAS_BMI1=0 SIMDLIB_HAS_BMI2=0)

    add_executable(InstalledHeaderOdrProbe
        tests/headers/InstalledHeaderOdrDefinition.cpp
        tests/headers/InstalledHeaderOdrConsumer.cpp
        tests/headers/InstalledHeaderOdrFixture.h)
    simdlib_configure_installed_header_probe(InstalledHeaderOdrProbe 20)
    add_test(NAME InstalledHeaderOdr COMMAND InstalledHeaderOdrProbe)
    set_tests_properties(InstalledHeaderOdr PROPERTIES
        LABELS "HEADERS;METHOD_FLAGS;ODR")
    simdlib_register_development_test(InstalledHeaderOdr COMPILER_CONTRACT)
    foreach(header_probe IN ITEMS
        Config
        TemplateTools
        IApi
        IImpl
        IRegister
        IRegisterMask
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
        add_library(Header${header_probe}Probe OBJECT tests/headers/${header_probe}HeaderProbe.cpp)
        simdlib_register_development_target(Header${header_probe}Probe
            COMPILER_CONTRACT)
        target_link_libraries(Header${header_probe}Probe PRIVATE SimdLib::SimdLib)
        simdlib_enable_development_warnings(Header${header_probe}Probe)
    endforeach()

	if(SIMDLIB_REGISTER_COMPILER_SUPPORTED)
		add_library(InstalledRegisterHeaderProbe OBJECT
			tests/headers/InstalledRegisterHeaderProbe.cpp)
        simdlib_configure_installed_header_probe(
            InstalledRegisterHeaderProbe 23)
        target_compile_definitions(InstalledRegisterHeaderProbe PRIVATE
            SIMDLIB_REQUIRE_REGISTER_INTERFACE=1)
		simdlib_enable_register_sse42(InstalledRegisterHeaderProbe)

		add_library(InstalledPartialRegisterHeaderProbe OBJECT
			tests/headers/InstalledPartialRegisterHeaderProbe.cpp)
		simdlib_configure_installed_header_probe(
			InstalledPartialRegisterHeaderProbe 23)
		target_compile_definitions(InstalledPartialRegisterHeaderProbe PRIVATE
			SIMDLIB_REQUIRE_REGISTER_INTERFACE=1)
		simdlib_enable_register_sse42(InstalledPartialRegisterHeaderProbe)

		add_library(HeaderAliasesProbe OBJECT
			tests/headers/AliasesHeaderProbe.cpp)
		simdlib_register_development_target(HeaderAliasesProbe COMPILER_CONTRACT)
		target_link_libraries(HeaderAliasesProbe PRIVATE SimdLib::Register)
		simdlib_enable_development_warnings(HeaderAliasesProbe)
		simdlib_enable_register_avx2(HeaderAliasesProbe)

		add_library(HeaderRegisterProbe OBJECT
			tests/headers/RegisterHeaderProbe.cpp)
		simdlib_register_development_target(HeaderRegisterProbe COMPILER_CONTRACT)
		target_link_libraries(HeaderRegisterProbe PRIVATE SimdLib::Register)
		simdlib_enable_development_warnings(HeaderRegisterProbe)

		add_library(HeaderPartialRegisterFwdProbe OBJECT
			tests/headers/PartialRegisterFwdHeaderProbe.cpp)
		simdlib_register_development_target(HeaderPartialRegisterFwdProbe COMPILER_CONTRACT)
		target_link_libraries(HeaderPartialRegisterFwdProbe PRIVATE SimdLib::Register)
		simdlib_enable_development_warnings(HeaderPartialRegisterFwdProbe)
		simdlib_enable_register_sse42(HeaderPartialRegisterFwdProbe)

		add_library(HeaderPartialRegisterProbe OBJECT
			tests/headers/PartialRegisterHeaderProbe.cpp)
		simdlib_register_development_target(HeaderPartialRegisterProbe COMPILER_CONTRACT)
		target_link_libraries(HeaderPartialRegisterProbe PRIVATE SimdLib::Register)
		simdlib_enable_development_warnings(HeaderPartialRegisterProbe)
		simdlib_enable_register_sse42(HeaderPartialRegisterProbe)

		add_library(HeaderRegisterMaskProbe OBJECT
			tests/headers/RegisterMaskHeaderProbe.cpp)
		simdlib_register_development_target(HeaderRegisterMaskProbe COMPILER_CONTRACT)
		target_link_libraries(HeaderRegisterMaskProbe PRIVATE SimdLib::Register)
		simdlib_enable_development_warnings(HeaderRegisterMaskProbe)

		add_library(HeaderSimdLibRegisterProbe OBJECT
			tests/headers/SimdLibRegisterHeaderProbe.cpp)
		simdlib_register_development_target(HeaderSimdLibRegisterProbe
			COMPILER_CONTRACT)
		target_link_libraries(HeaderSimdLibRegisterProbe PRIVATE SimdLib::Register)
		simdlib_enable_development_warnings(HeaderSimdLibRegisterProbe)
		simdlib_enable_register_sse42(HeaderSimdLibRegisterProbe)
	endif()
endif()

endblock()
