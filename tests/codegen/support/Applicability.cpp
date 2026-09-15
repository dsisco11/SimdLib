#include "Applicability.h"
#include "Configuration.h"
#include <array>
#include <charconv>
#include <stdexcept>

namespace Codegen
{
/** @brief Parses bounded numeric release components without lexical version ordering. */
static std::array<unsigned, 4> release(std::string_view text)
{
    std::array<unsigned, 4> result{};
    for (auto& component : result)
    {
        const auto end = text.find('.');
        const auto field = text.substr(0, end);
        const auto parsed = std::from_chars(field.data(), field.data() + field.size(), component);
        if (parsed.ec != std::errc{} || parsed.ptr != field.data() + field.size())
            throw std::runtime_error("Invalid compiler version selector");
        if (end == std::string_view::npos) return result;
        text.remove_prefix(end + 1);
    }
    throw std::runtime_error("Too many compiler version components");
}

bool matches(Scenario scenario, std::string_view driver, std::string_view version, std::string_view configuration)
{
    if (scenario.family != "msvc" && scenario.family != "gcc" && scenario.family != "clang")
        throw std::runtime_error("Unknown compiler family selector");
    const auto family = driver == "clang-cl" ? "clang" : driver;
    return scenario.family == family && (scenario.driver.empty() || scenario.driver == driver) &&
        (scenario.minimum_version.empty() || release(version) >= release(scenario.minimum_version)) &&
        (scenario.before_version.empty() || release(version) < release(scenario.before_version)) &&
        (scenario.configuration.empty() || configuration == scenario.configuration);
}

bool selected(Scenario scenario)
{
    return matches(scenario, Configuration::driver, Configuration::version, Configuration::configuration);
}
}
