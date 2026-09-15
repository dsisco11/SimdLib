#include "CodegenFixture.h"
#include "Inspection.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/interfaces/catch_interfaces_capture.hpp>
#include <exception>
#include <map>
#include <stdexcept>

namespace Codegen
{
#pragma region Applicability and fixture lifecycle
CodegenFixture::CodegenFixture() : case_(Catch::getResultCapture().getCurrentTestName()) {}
CodegenFixture::CodegenFixture(std::string case_id) : case_(std::move(case_id)) {}

CodegenFixture::~CodegenFixture() noexcept(false)
{
    // A fatal inspection exception already fails the test; avoid throwing during unwinding.
    if (std::uncaught_exceptions()) return;
    const auto expected = expected_rules(case_);
    INFO("Required rule coverage for " << case_ << " / " << Configuration::configuration);
    CHECK(coverage_->observed == expected);
}

Function CodegenFixture::inspect(const std::string& name) const
{
    // Each runner is bound to one configuration. Exact emitted identity and object
    // path make the cache key independent of how individual cases are scheduled.
    for (const auto& symbol : Configuration::symbols)
    {
        if (name != symbol.name) continue;
        const auto data = inspect_function(symbol, Configuration::objects.at(symbol.object));
        coverage_->observed.insert(name + "|inspect");
        return {data, coverage_};
    }
    throw std::runtime_error("Missing declared function: " + name);
}
#pragma endregion
}
