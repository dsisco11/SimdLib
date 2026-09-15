#include "CodegenFixture.h"
#include "Configuration.h"
#include <catch2/catch_test_macros.hpp>
using namespace Codegen;

TEST_CASE_METHOD(CodegenFixture, "abi.binary", "[codegen][register][abi]")
{
    const auto caller = inspect("simdlib_contract_abi_binary_caller");
    caller.check("contracts/function.check");
    caller.check("register/abi-caller.check");
    caller.check_for(Compiler::GCC, Configuration::width == 128 ?
        "register/abi-call.check" : "register/abi-call256.check");
    caller.check_for(Compiler::MSVC, "contracts/no-call-stack.check");
    caller.check_for(Compiler::MSVC, "contracts/register-only.check");
    caller.check_for(Compiler::MSVC, "register/abi-tail.check");
    caller.allow_for(Compiler::MSVC, "jmp|nop");
    caller.check_for(Compiler::Clang, "contracts/no-call-stack.check");
    caller.check_for(Compiler::Clang, "contracts/register-only.check");
    caller.check_for(Compiler::Clang, "register/abi-tail.check");
    caller.allow_for(Compiler::Clang, "jmp|nop");

    const auto callee = inspect("simdlib_contract_abi_binary_callee");
    callee.check("contracts/function.check");
    callee.check("register/add.check");
    callee.check("contracts/return.check");
    callee.check("contracts/no-call-stack.check");
    callee.check("contracts/register-only.check");
}
