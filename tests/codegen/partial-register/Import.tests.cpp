#include "CodegenFixture.h"
#include "Configuration.h"
#include <catch2/catch_test_macros.hpp>
using namespace Codegen;

TEST_CASE_METHOD(CodegenFixture, "value.import", "[codegen][partial-register]")
{
    const auto function = inspect("simdlib_contract_value_import");
    function.check("contracts/return.check");
    function.check("partial-register/import.check");
    function.check_for(Compiler::MSVC, "contracts/msvc-cookie.check");
    function.check_for(Compiler::GCC, "contracts/no-call-stack.check");
    function.check_for(Compiler::Clang, "contracts/no-call-stack.check");
    function.check_for(Compiler::MSVC, "partial-register/import-mask-msvc.check");
    function.check_for(Compiler::GCC, "partial-register/import-mask-gcc.check");
    function.check_for(Compiler::Clang, "partial-register/import-blend.check");
    const auto mask = Configuration::width == 128 ?
        "partial-register/mask128.check" : "partial-register/mask256.check";
    function.constants_for(Compiler::MSVC, mask);
    function.constants_for(Compiler::GCC, mask);
}
