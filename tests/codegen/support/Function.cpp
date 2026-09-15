#include "CodegenFixture.h"
#include "Inspection.h"
#include "FileCheck.h"
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <regex>
#include <sstream>

namespace Codegen
{
#pragma region Rule assertions
Function::Function(std::shared_ptr<Inspection> data, std::shared_ptr<Coverage> coverage)
    : data_(std::move(data)), coverage_(std::move(coverage)) {}

void Function::apply(const std::string& rule, bool supplemental, bool constants) const
{
    const auto kind = constants ? "constant" : supplemental ? "supplement" : "primary";
    coverage_->observed.insert(std::string(data_->symbol.name) + "|" + kind + "|" + rule);
    const auto reg = Configuration::width == 128 ? "xmm" : "ymm";
    std::string target;
    for (const auto& symbol : Configuration::symbols)
        if (std::string_view(symbol.name) == "simdlib_contract_abi_binary_callee") target = symbol.emitted;
    const auto& object = Configuration::objects.at(data_->symbol.object);
    std::ifstream provenance(std::filesystem::u8path(object.provenance));
    const std::string build_details{std::istreambuf_iterator<char>(provenance), {}};
    INFO("Function: " << data_->symbol.emitted << "\n" << kind << ": " << rule
        << "\nConfiguration: " << Configuration::configuration << "\nCompiler version: " << Configuration::version
        << "\nObject: " << object.path << "\nBuild provenance: " << object.provenance
        << "\nArtifacts: " << utf8(data_->directory) << "\n" << build_details);
    // Every rule gets a new FileCheck process, independent captures, and the same input.
    // Exceptions become nonfatal assertions so later supplemental facts still execute.
    try
    {
        const auto result = file_check(Configuration::filecheck,
            std::filesystem::u8path(Configuration::root) / "tests/codegen" / rule,
            constants ? data_->constants : data_->input, {
            "-DREG=" + std::string(reg), "-DMEM=" + std::string(reg) + "word",
            "-DMASK=" + std::string(Configuration::width == 128 ? "0x8" : "0xe0"),
            "-DTARGET=" + std::string(target)});
        INFO(result.output);
        CHECK(result.exit_code == 0);
    }
    catch (const std::exception& error) { FAIL_CHECK(error.what()); }
}

void Function::check(const std::string& rule) const { apply(rule, false, false); }
void Function::check_for(Scenario compiler, const std::string& rule) const
{
    if (selected(compiler)) apply(rule, true, false);
}
void Function::constants_for(Scenario compiler, const std::string& rule) const
{
    if (selected(compiler)) apply(rule, true, true);
}

void Function::allow(const std::string& mnemonics) const
{
    coverage_->observed.insert(std::string(data_->symbol.name) + "|allow|" + mnemonics);
    try
    {
        for (const auto& line : forbidden_mnemonics(data_->body, mnemonics))
            FAIL_CHECK("Forbidden instruction family in " << data_->symbol.emitted << ": " << line);
    }
    catch (const std::exception& error) { FAIL_CHECK("Invalid mnemonic rule: " << error.what()); }
}

void Function::allow_for(Scenario compiler, const std::string& mnemonics) const
{
    if (selected(compiler)) allow(mnemonics);
}
#pragma endregion
}
