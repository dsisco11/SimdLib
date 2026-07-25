include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "SourceAudits.cmake is available only to top-level SimdLib builds")
endif()
if(NOT TARGET SimdLib OR NOT TARGET SimdLibRegister)
    message(FATAL_ERROR "SourceAudits.cmake requires the production SimdLib targets")
endif()

block(SCOPE_FOR VARIABLES)

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


if(SIMDLIB_BUILD_CONFIGURATION_PROBES)
	add_custom_target(PublicHeaderAssertionAudit
		COMMAND ${CMAKE_COMMAND}
			-DSOURCE_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AuditPublicHeaderAssertions.cmake
		COMMENT "Auditing production-header static assertions"
		VERBATIM)
	add_test(NAME PublicHeaderStaticAssertAudit
		COMMAND ${CMAKE_COMMAND}
			-DSOURCE_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AuditPublicHeaderAssertions.cmake)
	set_tests_properties(PublicHeaderStaticAssertAudit PROPERTIES LABELS "CONSTEXPR;AUDIT")
endif()

endblock()
