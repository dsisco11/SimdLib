#include "CodegenFixture.h"
#include <catch2/catch_test_macros.hpp>
using namespace Codegen;

TEST_CASE_METHOD(CodegenFixture, "transfer.load_operate_store", "[codegen][register][transfer]")
{
    const auto function = inspect("simdlib_contract_transfer_load_operate_store");
    function.check("contracts/return.check");
    function.check("register/transfer.check");
    function.check_for(Compiler::MSVC, "contracts/msvc-cookie.check");
    function.check_for(Compiler::GCC, "contracts/no-call-stack.check");
    function.check_for(Compiler::Clang, "contracts/no-call-stack.check");
}
