#include "HarnessConfiguration.h"
#include "CodegenFixture.h"
#include "Inspection.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <fstream>
using namespace Codegen;

TEST_CASE("Cached inspection rejects changed and missing disposable inputs", "[harness][freshness]")
{
    const auto root = std::filesystem::u8path(Harness::output_root) / "freshness";
    std::filesystem::create_directories(root);
    const auto path = utf8(root / "object.o");
    const auto manifest = utf8(root / "inputs.cmake");
    const auto receipt = utf8(root / "build.sha256");
    const auto input = utf8(root / "configuration.txt");
    // Only disposable inputs are modified; production objects remain read-only.
    std::filesystem::copy_file(std::filesystem::u8path(Configuration::objects[1].path),
        std::filesystem::u8path(path), std::filesystem::copy_options::overwrite_existing);
    std::ofstream(std::filesystem::u8path(input)) << "original configuration\n";
    std::ofstream(std::filesystem::u8path(manifest)) << "set(INPUTS [==[" << input << "]==])\n";
    const auto recorded = run_process({Configuration::cmake, "-DINPUT_MANIFEST=" + manifest,
        "-DOBJECT_FILE=" + path, "-DBUILD_RECEIPT=" + receipt, "-P",
        std::string(Configuration::root) + "/cmake/codegen/RecordBuild.cmake"});
    INFO(recorded.output);
    REQUIRE(recorded.exit_code == 0);
    const Configuration::Object object{path.c_str(), manifest.c_str(), receipt.c_str(), input.c_str()};
    const auto symbol = Configuration::symbols[3];
    const auto first = inspect_function(symbol, object);
    CHECK(inspect_function(symbol, object) == first);
    std::ofstream(std::filesystem::u8path(input)) << "changed configuration\n";
    CHECK_THROWS_WITH(inspect_function(symbol, object), Catch::Matchers::ContainsSubstring("Stale codegen"));
    // A valid new build receipt must not make the old cached disassembly current.
    const auto rerecorded = run_process({Configuration::cmake, "-DINPUT_MANIFEST=" + manifest,
        "-DOBJECT_FILE=" + path, "-DBUILD_RECEIPT=" + receipt, "-P",
        std::string(Configuration::root) + "/cmake/codegen/RecordBuild.cmake"});
    REQUIRE(rerecorded.exit_code == 0);
    CHECK_THROWS_WITH(inspect_function(symbol, object), Catch::Matchers::ContainsSubstring("Cached inspection snapshot changed"));
    std::ofstream(std::filesystem::u8path(input)) << "original configuration\n";
    REQUIRE(run_process({Configuration::cmake, "-DINPUT_MANIFEST=" + manifest,
        "-DOBJECT_FILE=" + path, "-DBUILD_RECEIPT=" + receipt, "-P",
        std::string(Configuration::root) + "/cmake/codegen/RecordBuild.cmake"}).exit_code == 0);
    CHECK(inspect_function(symbol, object) == first);
    std::ofstream(std::filesystem::u8path(path), std::ios::binary | std::ios::app) << 'x';
    CHECK_THROWS_WITH(inspect_function(symbol, object), Catch::Matchers::ContainsSubstring("Stale codegen"));
    REQUIRE(run_process({Configuration::cmake, "-DINPUT_MANIFEST=" + manifest,
        "-DOBJECT_FILE=" + path, "-DBUILD_RECEIPT=" + receipt, "-P",
        std::string(Configuration::root) + "/cmake/codegen/RecordBuild.cmake"}).exit_code == 0);
    CHECK_THROWS_WITH(inspect_function(symbol, object), Catch::Matchers::ContainsSubstring("Cached inspection snapshot changed"));
    std::filesystem::resize_file(std::filesystem::u8path(path), std::filesystem::file_size(std::filesystem::u8path(path)) - 1);
    REQUIRE(run_process({Configuration::cmake, "-DINPUT_MANIFEST=" + manifest,
        "-DOBJECT_FILE=" + path, "-DBUILD_RECEIPT=" + receipt, "-P",
        std::string(Configuration::root) + "/cmake/codegen/RecordBuild.cmake"}).exit_code == 0);
    std::filesystem::rename(std::filesystem::u8path(receipt), std::filesystem::u8path(receipt + ".saved"));
    CHECK_THROWS_WITH(inspect_function(symbol, object), Catch::Matchers::ContainsSubstring("Missing codegen build receipt"));
    std::filesystem::rename(std::filesystem::u8path(receipt + ".saved"), std::filesystem::u8path(receipt));
    std::filesystem::rename(std::filesystem::u8path(path), std::filesystem::u8path(path + ".saved"));
    CHECK_THROWS_WITH(inspect_function(symbol, object), Catch::Matchers::ContainsSubstring("Missing codegen build input"));
    std::filesystem::rename(std::filesystem::u8path(path + ".saved"), std::filesystem::u8path(path));
    CHECK(inspect_function(symbol, object) == first);
}
