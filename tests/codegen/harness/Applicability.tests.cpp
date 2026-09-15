#include "Applicability.h"
#include <catch2/catch_test_macros.hpp>
using namespace Codegen;

TEST_CASE("Supplemental selectors preserve driver version and configuration boundaries", "[harness][coverage]")
{
    const Scenario compatibility{.family="clang", .driver="clang-cl", .minimum_version="20.1.8",
        .before_version="23", .configuration="provisional"};
    CHECK(matches(compatibility, "clang-cl", "22.1.8", "provisional"));
    CHECK_FALSE(matches(compatibility, "clang", "22.1.8", "provisional"));
    CHECK_FALSE(matches(compatibility, "clang-cl", "20.1.7", "provisional"));
    CHECK_FALSE(matches(compatibility, "clang-cl", "23.0.0", "provisional"));
    CHECK_FALSE(matches(compatibility, "clang-cl", "22.1.8", "production"));
    CHECK(matches(Compiler::Clang, "clang", "22.1.3", "provisional"));
    CHECK(matches(Compiler::Clang, "clang-cl", "20.1.8", "provisional"));
    CHECK_FALSE(matches(Compiler::MSVC, "clang-cl", "22.1.8", "provisional"));
    CHECK_THROWS(matches(Scenario{.family="unknown"}, "gcc", "14.2", "provisional"));
}
