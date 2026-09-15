#include "HarnessConfiguration.h"
#include "CodegenFixture.h"
#include "Configuration.h"
#include <catch2/catch_test_macros.hpp>
using namespace Codegen;

TEST_CASE("Independent case inventory rejects missing and duplicated discovery", "[harness][coverage]")
{
    const auto script = std::string(Configuration::root) + "/tests/codegen/pilot/VerifyDiscovery.cmake";
    CHECK(run_process({Configuration::cmake, "-DRUNNER=" + std::string(Harness::production_runner), "-P", script}).exit_code == 0);
    for (const auto* cases : {"", "operation.add_f32", "operation.add_f32;operation.add_f32",
            "operation.add_f32;operation.zero_f32;expression.native;transfer.load_operate_store;value.import;abi.binary;unexpected"})
    {
        const auto result = run_process({Configuration::cmake, "-DOBSERVED_CASES=" + std::string(cases), "-P", script});
        INFO(result.output);
        CHECK(result.exit_code != 0);
        CHECK(result.output.find("Expected case coverage mismatch") != std::string::npos);
    }
}
