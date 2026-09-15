#include "Configuration.h"
#include "Process.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace CodegenPrototype
{
namespace fs = std::filesystem;

#pragma region Shared inspection lifetime
/** @brief Owns one extraction per test process, shared across Catch2 sections.
 * Separate CTest processes receive distinct directories without global lock files.
 */
struct Inspection
{
    fs::path directory;
    fs::path input;
    std::string body;
    std::size_t processes = 0;

    /** @brief Verifies the built object and retains LLVM's complete selected body. */
    Inspection()
    {
        auto nonce = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        fs::create_directories(Configuration::artifacts);
        // Reserve the directory atomically, including parallel CTest processes.
        do { directory = fs::path(Configuration::artifacts) / std::to_string(nonce++); }
        while (!fs::create_directory(directory));
        input = directory / "input.txt";
        const auto inspected = run_process({Configuration::cmake,
            "-DSCRIPTS=" + std::string(Configuration::scripts),
            "-DINPUT_MANIFEST=" + std::string(Configuration::input_manifest),
            "-DBUILD_RECEIPT=" + std::string(Configuration::receipt),
            "-DOBJECT_FILE=" + std::string(Configuration::object),
            "-DEXPECTED_SYMBOL=" + std::string(Configuration::symbol),
            "-DOUTPUT_DIRECTORY=" + directory.string(),
            "-DLLVM_OBJDUMP=" + std::string(Configuration::objdump),
            "-DLLVM_READOBJ=" + std::string(Configuration::readobj),
            "-P", Configuration::inspect});
        ++processes;
        if (inspected.exit_code != 0) throw std::runtime_error(inspected.output);
        std::ifstream stream(directory / "body.txt", std::ios::binary);
        body.assign(std::istreambuf_iterator<char>(stream), {});
        if (body.empty()) throw std::runtime_error("Empty extracted body");
        std::ofstream(input, std::ios::binary) << "CODEGEN-BEGIN\n" << body << "\nCODEGEN-END\n";
    }
};

/** @brief Fixture delegates inspection to a process-local shared value; Catch2
 * may reconstruct this fixture for each SECTION without rerunning LLVM.
 */
class InstructionFixture
{
public:
    /** @brief Returns the single inspection shared by checks within this process. */
    static Inspection& inspection()
    {
        static Inspection value;
        return value;
    }

    /** @brief Applies a rule and returns diagnostics directly to Catch2 in memory. */
    static ProcessResult check(const fs::path& rule, const fs::path& input)
    {
        ++inspection().processes;
        return run_process({Configuration::filecheck, rule.string(),
            "--input-file=" + input.string(), "--dump-input=fail", "-DREG=xmm", "-DMEM=xmmword"});
    }

    /** @brief Runs every primary rule, retaining every failure independently. */
    static std::vector<ProcessResult> primary(const fs::path& input)
    {
        std::vector<ProcessResult> results;
        for (const auto* rule : {"contracts/return.check", "register/transfer.check"})
        {
            auto path = fs::path(Configuration::rules) / rule;
            if (std::getenv("SIMDLIB_PROTOTYPE_FAIL_PRIMARY"))
            {
                path = inspection().directory / "injected-primary.check";
                std::ofstream(path) << "CHECK: deliberately_missing_primary_fact\n";
            }
            results.push_back(check(path, input));
        }
        return results;
    }

    /** @brief Executes the mandatory compiler-specific fact on the same input. */
    static ProcessResult supplement(const fs::path& input)
    {
        auto rule = fs::path(Configuration::rules) / "contracts" /
            (std::string(Configuration::supplement) + ".check");
        if (std::getenv("SIMDLIB_PROTOTYPE_FAIL_SUPPLEMENT"))
        {
            rule = inspection().directory / "injected-supplement.check";
            std::ofstream(rule) << "CHECK: deliberately_missing_supplement_fact\n";
        }
        return check(rule, input);
    }
};
#pragma endregion

#pragma region Production contract and deliberate failure evidence
TEST_CASE_METHOD(InstructionFixture, "production transfer primary and supplement", "[codegen][prototype]")
{
    const auto& data = inspection();
    INFO("Function: " << Configuration::symbol << "\nObject: " << Configuration::object
        << "\nArtifacts: " << data.directory.string());
    // CHECK continues after failure, so every applicable fact still reports.
    for (const auto& result : primary(data.input))
    {
        INFO("Primary FileCheck output:\n" << result.output);
        CHECK(result.exit_code == 0);
    }
    const auto additional = supplement(data.input);
    INFO("Supplement " << Configuration::supplement << ":\n" << additional.output);
    CHECK(additional.exit_code == 0);
    SUCCEED("Runner launched " << data.processes << " child processes; LLVM descendants are separate");
}

TEST_CASE_METHOD(InstructionFixture, "failed primary cannot be hidden", "[codegen][prototype][negative]")
{
    auto& data = inspection();
    const auto wrong = data.directory / "wrong-primary.check";
    std::ofstream(wrong) << "CHECK: deliberate_missing_instruction\n";
    const auto required = check(wrong, data.input);
    const auto additional = supplement(data.input);
    INFO(required.output);
    CHECK(required.exit_code != 0);
    CHECK(required.output.find("expected string not found") != std::string::npos);
    CHECK(additional.exit_code == 0);
    CHECK_FALSE((required.exit_code == 0 && additional.exit_code == 0));
}

TEST_CASE_METHOD(InstructionFixture, "failed supplement cannot be hidden", "[codegen][prototype][negative]")
{
    auto& data = inspection();
    const auto common = primary(data.input);
    REQUIRE(std::all_of(common.begin(), common.end(), [](const auto& r) { return r.exit_code == 0; }));
    const auto wrong = data.directory / "wrong-supplement.check";
    std::ofstream(wrong) << "CHECK: deliberate_missing_supplement\n";
    const auto additional = check(wrong, data.input);
    INFO(additional.output);
    CHECK(additional.exit_code != 0);
    CHECK(additional.output.find("expected string not found") != std::string::npos);
}

TEST_CASE_METHOD(InstructionFixture, "primary rejects an extra operation after return", "[codegen][prototype][negative]")
{
    auto& data = inspection();
    const auto mutated = data.directory / "extra-operation.txt";
    std::ofstream(mutated) << "CODEGEN-BEGIN\n" << data.body
        << "\n ff: c5 f8 58 c0  vaddps xmm0, xmm0, xmm0\nCODEGEN-END\n";
    const auto result = check(fs::path(Configuration::rules) / "register/transfer.check", mutated);
    INFO(result.output);
    CHECK(result.exit_code != 0);
    CHECK(result.output.find("excluded string found") != std::string::npos);
}
#pragma endregion
}
