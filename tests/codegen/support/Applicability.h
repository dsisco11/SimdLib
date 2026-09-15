#pragma once
#include <string_view>

namespace Codegen
{
/** @brief Explicit supplemental selection, with optional driver/version/configuration bounds. */
struct Scenario
{
    std::string_view family;
    std::string_view driver{};
    std::string_view minimum_version{};
    std::string_view before_version{};
    std::string_view configuration{};
};
namespace Compiler
{
inline constexpr Scenario MSVC{"msvc"};
inline constexpr Scenario GCC{"gcc"};
inline constexpr Scenario Clang{"clang"};
}
/** @brief Matches all explicitly declared dimensions, treating versions numerically. */
bool matches(Scenario scenario, std::string_view driver, std::string_view version, std::string_view configuration);
/** @brief Applies a named scenario to the runner's fixed build configuration. */
bool selected(Scenario scenario);
}
