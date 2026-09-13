cmake_minimum_required(VERSION 3.31)

if(NOT DEFINED SOURCE_DIRECTORY OR "${SOURCE_DIRECTORY}" STREQUAL "")
    message(FATAL_ERROR "SOURCE_DIRECTORY is required")
endif()

file(GLOB_RECURSE public_consumer_sources
    "${SOURCE_DIRECTORY}/examples/*.cpp"
    "${SOURCE_DIRECTORY}/tests/consumer/*.cpp"
    "${SOURCE_DIRECTORY}/tests/format_odr/*.cpp"
    "${SOURCE_DIRECTORY}/tests/headers/*.cpp"
	"${SOURCE_DIRECTORY}/tests/installed_consumer/*.cpp"
	"${SOURCE_DIRECTORY}/tests/partial_register_odr/*.cpp"
    "${SOURCE_DIRECTORY}/tests/register_odr/*.cpp"
    "${SOURCE_DIRECTORY}/tests/smoke/*.cpp")
list(SORT public_consumer_sources)
foreach(consumer_source IN LISTS public_consumer_sources)
    file(READ "${consumer_source}" consumer_source_text)
    if(consumer_source_text MATCHES "SimdLib::Detail|<SimdLib/Detail/")
        message(FATAL_ERROR
            "Public consumer surface names implementation detail: ${consumer_source}")
    endif()
endforeach()
list(LENGTH public_consumer_sources public_consumer_source_count)
message(STATUS
    "Validated ${public_consumer_source_count} public consumer sources")
