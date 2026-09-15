#include "HarnessOptions.h"
#include "Configuration.h"
#include "CodegenFixture.h"
#include "FileCheck.h"
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <regex>
#include <sstream>

TEST_CASE("Synthetic rule input", "[.rule]")
{
    std::ifstream stream(std::filesystem::u8path(Harness::body), std::ios::binary);
    REQUIRE(stream.good());
    const std::string body{std::istreambuf_iterator<char>(stream), {}};
    INFO("Empty or invalid instruction input");
    REQUIRE(std::regex_search(body, std::regex(Harness::constants ?
        R"(0x[0-9A-Fa-f]+\s+[0-9A-Fa-f]+)" : R"([0-9A-Fa-f]+:\s+[0-9A-Fa-f]{2}\s+)")));
    const auto input = std::filesystem::u8path(Harness::body + ".input");
    {
        std::ofstream output;
        output.exceptions(std::ios::badbit | std::ios::failbit);
        output.open(input, std::ios::binary);
        output << "CODEGEN-BEGIN\n" << body << "\nCODEGEN-END\n";
    }
    if (!Harness::allow.empty())
        for (const auto& line : Codegen::forbidden_mnemonics(body, Harness::allow))
            FAIL_CHECK("Forbidden instruction family: " << line);
    std::istringstream rules(Harness::rules);
    std::string rule;
    while (std::getline(rules, rule, ';'))
    {
        const auto result = Codegen::file_check(Codegen::Configuration::filecheck,
            std::filesystem::u8path(rule), input,
            {"-DREG=xmm", "-DMEM=xmmword", "-DMASK=0x8", "-DTARGET=simdlib_contract_abi_binary_callee"});
        INFO("Instruction expectations failed: " << rule << "\n" << result.output);
        CHECK(result.exit_code == 0);
    }
}
