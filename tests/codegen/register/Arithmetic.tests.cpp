#include "CodegenFixture.h"
#include <catch2/catch_test_macros.hpp>
using namespace Codegen;

#pragma region Complete register contracts
TEST_CASE_METHOD(CodegenFixture, "operation.add_f32", "[codegen][register]")
{
    const auto function = inspect("simdlib_contract_operation_add_f32");
    function.check("contracts/no-call-stack.check");
    function.check("contracts/return.check");
    function.check("contracts/register-only.check");
    function.check("register/add.check");
    function.allow("vaddps|ret|nop");
    function.check_for(Compiler::Clang, "register/no-extra-move.check");
}

TEST_CASE_METHOD(CodegenFixture, "operation.zero_f32", "[codegen][register]")
{
    const auto function = inspect("simdlib_contract_operation_zero_f32");
    function.check("contracts/no-call-stack.check");
    function.check("contracts/return.check");
    function.check("contracts/register-only.check");
    function.check("register/zero.check");
    function.allow("vxorps|vpxor|xorps|pxor|ret|nop");
}

TEST_CASE_METHOD(CodegenFixture, "expression.native", "[codegen][register]")
{
    const auto function = inspect("simdlib_contract_expression_native");
    function.check("contracts/no-call-stack.check");
    function.check("contracts/return.check");
    function.check("contracts/register-only.check");
    function.allow("ret|nop");
}
#pragma endregion
