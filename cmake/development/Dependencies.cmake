include_guard(GLOBAL)

if(NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "Dependencies.cmake is available only to top-level SimdLib builds")
endif()

block(SCOPE_FOR VARIABLES)

if(SIMDLIB_BUILD_RUNTIME_TESTS OR SIMDLIB_BUILD_BENCHMARKS)
    find_package(Catch2 3 CONFIG QUIET)
    if(NOT TARGET Catch2::Catch2WithMain AND SIMDLIB_FETCH_TEST_DEPENDENCIES)
        include(FetchContent)
        FetchContent_Declare(Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG 2b60af89e23d28eefc081bc930831ee9d45ea58b
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(Catch2)
    endif()
    if(NOT TARGET Catch2::Catch2WithMain)
        message(FATAL_ERROR
            "Catch2 3 is required; install it or enable SIMDLIB_FETCH_TEST_DEPENDENCIES")
    endif()
endif()

# Catch2 publishes Catch.cmake through CMAKE_MODULE_PATH for RuntimeTests.cmake.
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)

endblock()
