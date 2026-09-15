#include "HarnessOptions.h"
#include "CodegenFixture.h"
#include "Configuration.h"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
using namespace Codegen;

/** @brief Runs isolated variants against the real transfer case's independent ledger. */
struct TransferFixture : CodegenFixture
{
    /** @brief Retains the actual production case's coverage requirements. */
    TransferFixture() : CodegenFixture("transfer.load_operate_store") {}
};

TEST_CASE_METHOD(TransferFixture, "Composite failure probe", "[.composition]")
{
    const auto function = inspect("simdlib_contract_transfer_load_operate_store");
    if (Harness::fault != "missing-primary") function.check("contracts/return.check");
    function.check(Harness::fault == "failed-primary" ? "harness/rules/impossible.check" : "register/transfer.check");
    if (Harness::fault == "invalid-allow") function.allow("(");
    std::cout << "PRIMARY_COMPLETED\n";
    if (Harness::fault != "missing-supplement")
    {
        const auto rule = Harness::fault == "failed-supplement" ? "harness/rules/impossible.check" :
            selected(Compiler::MSVC) ? "contracts/msvc-cookie.check" : "contracts/no-call-stack.check";
        const auto compiler = Harness::fault == "wrong-applicability" ?
            (selected(Compiler::MSVC) ? Compiler::GCC : Compiler::MSVC) :
            selected(Compiler::MSVC) ? Compiler::MSVC : selected(Compiler::GCC) ? Compiler::GCC : Compiler::Clang;
        function.check_for(compiler, rule);
    }
    std::cout << "SUPPLEMENT_COMPLETED\n";
}

TEST_CASE("Composite rules and required coverage reject independent failures", "[harness]")
{
    for (const auto* fault : {"valid", "failed-primary", "failed-supplement", "missing-primary",
            "missing-supplement", "wrong-applicability", "invalid-allow"})
    {
        const auto result = run_process({Harness::executable, "[.composition]", "--fault", fault});
        INFO(fault << "\n" << result.output);
        CHECK((result.exit_code == 0) == (std::string(fault) == "valid"));
        CHECK(result.output.find("PRIMARY_COMPLETED") != std::string::npos);
        CHECK(result.output.find("SUPPLEMENT_COMPLETED") != std::string::npos);
        if (std::string(fault).starts_with("failed"))
            CHECK(result.output.find("expected string not found") != std::string::npos);
        if (std::string(fault) == "invalid-allow")
            CHECK(result.output.find("Invalid mnemonic rule") != std::string::npos);
        if (std::string(fault).starts_with("missing") || std::string(fault) == "wrong-applicability")
            CHECK(result.output.find("Required rule coverage") != std::string::npos);
    }
}
