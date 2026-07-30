cmake_minimum_required(VERSION 4.4)

foreach(required_variable IN ITEMS
    SOURCE_DIRECTORY SOURCE_DIGEST SOURCE_REVISION RESULT_FILE
    METHOD_FLAGS_LEGACY_COUNT METHOD_FLAGS_LEGACY_SHA256
    METHOD_FLAGS_REGISTER_ONLY_COUNT METHOD_FLAGS_REGISTER_ONLY_SHA256)
    if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} is required")
    endif()
endforeach()

include("${SOURCE_DIRECTORY}/cmake/AuditPublicHeaderAssertions.cmake")

file(GLOB_RECURSE public_consumer_sources
    "${SOURCE_DIRECTORY}/examples/*.cpp"
    "${SOURCE_DIRECTORY}/tests/consumer/*.cpp"
    "${SOURCE_DIRECTORY}/tests/format_odr/*.cpp"
    "${SOURCE_DIRECTORY}/tests/headers/*.cpp"
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

get_filename_component(result_directory "${RESULT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${result_directory}")
file(WRITE "${RESULT_FILE}"
    "{\n"
    "  \"schema\": \"simdlib.repository-audit.v1\",\n"
    "  \"status\": \"complete\",\n"
    "  \"sourceDigest\": \"${SOURCE_DIGEST}\",\n"
    "  \"sourceRevision\": \"${SOURCE_REVISION}\",\n"
    "  \"publicHeaderStaticAssertions\": ${assertion_count},\n"
    "  \"staticAssertionAllowlistEntries\": ${allowlist_count},\n"
    "  \"publicConsumerSources\": ${public_consumer_source_count},\n"
    "  \"legacyMethodFlagDeclarations\": ${METHOD_FLAGS_LEGACY_COUNT},\n"
    "  \"legacyMethodFlagInventorySha256\": \"${METHOD_FLAGS_LEGACY_SHA256}\",\n"
    "  \"registerOnlyDeclarations\": ${METHOD_FLAGS_REGISTER_ONLY_COUNT},\n"
    "  \"registerOnlyInventorySha256\": \"${METHOD_FLAGS_REGISTER_ONLY_SHA256}\"\n"
    "}\n")

message(STATUS
    "Repository audit recorded ${assertion_count} public-header assertions, "
    "${public_consumer_source_count} public consumer sources, "
    "${METHOD_FLAGS_LEGACY_COUNT} legacy method-flag declarations, and "
    "${METHOD_FLAGS_REGISTER_ONLY_COUNT} RegisterOnly declarations")
