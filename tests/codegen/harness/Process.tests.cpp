#include "HarnessConfiguration.h"
#include "CodegenFixture.h"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
using namespace Codegen;

TEST_CASE("Process preserves Unicode paths and quoted arguments", "[harness][process]")
{
    const auto directory = std::filesystem::u8path(Harness::output_root) /
        std::filesystem::path(u8"spaces and \u03bb");
    std::filesystem::create_directories(directory);
    const auto executable = directory / std::filesystem::u8path(Harness::process_probe).filename();
    std::filesystem::copy_file(std::filesystem::u8path(Harness::process_probe), executable,
        std::filesystem::copy_options::overwrite_existing);
    const std::vector<std::string> arguments{"", "two words", "a\"b", "trailing\\", "\\\"", "line\nbreak", "\xce\xbb"};
    std::vector<std::string> command{utf8(executable)};
    command.insert(command.end(), arguments.begin(), arguments.end());
    const auto result = run_process(command);
    INFO(result.output);
    REQUIRE(result.exit_code == 0);
    for (const auto& argument : arguments)
        CHECK(result.output.find(std::to_string(argument.size()) + ':' + argument + '\n') != std::string::npos);
    CHECK(result.output.find("stderr captured") != std::string::npos);
}

TEST_CASE("Process drains output and propagates failure", "[harness][process]")
{
    const auto flood = run_process({Harness::process_probe, "flood"});
    REQUIRE(flood.exit_code == 0);
    CHECK(flood.output.size() >= 1024 * 1024);
    CHECK(flood.output.find("stderr captured") != std::string::npos);
    CHECK(run_process({Harness::process_probe, "fail"}).exit_code == 7);
    bool failed = false;
    try { failed = run_process({std::string(Harness::process_probe) + ".missing"}).exit_code != 0; }
    catch (const std::exception&) { failed = true; }
    CHECK(failed);
}

TEST_CASE("Process timeout is bounded and retains diagnostics", "[harness][process]")
{
    const auto start = std::chrono::steady_clock::now();
    try
    {
        run_process({Harness::process_probe, "sleep"}, std::chrono::seconds(2));
        FAIL("Expected process timeout");
    }
    catch (const std::runtime_error& error)
    {
        const std::string message = error.what();
        CHECK(message.find("Process timeout") != std::string::npos);
        CHECK(message.find("timeout probe ready") != std::string::npos);
    }
    CHECK(std::chrono::steady_clock::now() - start < std::chrono::seconds(10));
}
