include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "HeaderProbes.cmake is available only to top-level SimdLib builds")
endif()
if(NOT TARGET SimdLib OR NOT TARGET SimdLibRegister)
    message(FATAL_ERROR "HeaderProbes.cmake requires the production SimdLib targets")
endif()

block(SCOPE_FOR VARIABLES)

if(SIMDLIB_BUILD_HEADER_PROBES)
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
