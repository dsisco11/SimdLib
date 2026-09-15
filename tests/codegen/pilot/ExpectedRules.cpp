#include "CodegenFixture.h"
#include "Configuration.h"
#include <stdexcept>

namespace Codegen
{
/** @brief Independently enumerates required pilot facts; never derives them from executed checks. */
std::multiset<std::string> expected_rules(const std::string& case_id)
{
    std::multiset<std::string> result;
    std::string symbol;
    // This ledger is deliberately separate from the authored test declarations.
    const auto rules = [&](const char* kind, std::initializer_list<const char*> paths)
    {
        for (const auto* path : paths) result.insert(symbol + "|" + kind + "|" + path);
    };
    const auto inspect = [&](const char* name)
    {
        symbol = name;
        result.insert(symbol + "|inspect");
    };
    const std::string driver = Configuration::driver;
    if (case_id == "operation.add_f32" || case_id == "operation.zero_f32" || case_id == "expression.native")
    {
        if (case_id == "operation.add_f32") inspect("simdlib_contract_operation_add_f32");
        else if (case_id == "operation.zero_f32") inspect("simdlib_contract_operation_zero_f32");
        else inspect("simdlib_contract_expression_native");
        rules("primary", {"contracts/no-call-stack.check", "contracts/return.check", "contracts/register-only.check"});
        if (case_id == "operation.add_f32")
        {
            rules("primary", {"register/add.check"});
            rules("allow", {"vaddps|ret|nop"});
            if (driver == "clang" || driver == "clang-cl") rules("supplement", {"register/no-extra-move.check"});
        }
        else if (case_id == "operation.zero_f32")
        {
            rules("primary", {"register/zero.check"});
            rules("allow", {"vxorps|vpxor|xorps|pxor|ret|nop"});
        }
        else rules("allow", {"ret|nop"});
    }
    else if (case_id == "transfer.load_operate_store" || case_id == "value.import")
    {
        const bool partial = case_id == "value.import";
        inspect(partial ? "simdlib_contract_value_import" : "simdlib_contract_transfer_load_operate_store");
        rules("primary", {"contracts/return.check", partial ? "partial-register/import.check" : "register/transfer.check"});
        rules("supplement", {driver == "msvc" ? "contracts/msvc-cookie.check" : "contracts/no-call-stack.check"});
        if (partial)
        {
            rules("supplement", {driver == "msvc" ? "partial-register/import-mask-msvc.check" :
                driver == "gcc" ? "partial-register/import-mask-gcc.check" : "partial-register/import-blend.check"});
            if (driver == "msvc" || driver == "gcc") rules("constant", {Configuration::width == 128 ?
                "partial-register/mask128.check" : "partial-register/mask256.check"});
        }
    }
    else if (case_id == "abi.binary")
    {
        inspect("simdlib_contract_abi_binary_caller");
        rules("primary", {"contracts/function.check", "register/abi-caller.check"});
        if (driver == "gcc") rules("supplement", {Configuration::width == 128 ?
            "register/abi-call.check" : "register/abi-call256.check"});
        else
        {
            rules("supplement", {"contracts/no-call-stack.check", "contracts/register-only.check", "register/abi-tail.check"});
            rules("allow", {"jmp|nop"});
        }
        inspect("simdlib_contract_abi_binary_callee");
        rules("primary", {"contracts/function.check", "register/add.check", "contracts/return.check",
            "contracts/no-call-stack.check", "contracts/register-only.check"});
    }
    else throw std::runtime_error("Missing independent expected case: " + case_id);
    return result;
}
}
