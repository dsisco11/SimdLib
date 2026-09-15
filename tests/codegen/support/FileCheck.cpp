#include "FileCheck.h"
#include "Paths.h"
#include <regex>
#include <sstream>

namespace Codegen
{
ProcessResult file_check(const std::string& executable, const std::filesystem::path& rule,
    const std::filesystem::path& input, const std::vector<std::string>& parameters)
{
    std::vector<std::string> arguments{executable,
        utf8(rule), "--input-file=" + utf8(input),
        "--dump-input=fail"};
    arguments.insert(arguments.end(), parameters.begin(), parameters.end());
    return run_process(arguments);
}

std::vector<std::string> forbidden_mnemonics(const std::string& body, const std::string& allowed)
{
    std::vector<std::string> rejected;
    const std::regex instruction(R"(^\s*[0-9A-Fa-f]+:\s+(?:[0-9A-Fa-f]{2}\s+)+([a-z][a-z0-9.]*)(?:\s|$))");
    const std::regex permitted("^(" + allowed + ")$");
    std::istringstream stream(body);
    std::string line;
    // This narrow lexical check supplements FileCheck; it does not decode bytes.
    while (std::getline(stream, line))
    {
        std::smatch match;
        if (std::regex_search(line, match, instruction) && !std::regex_match(match[1].str(), permitted))
            rejected.push_back(line);
    }
    return rejected;
}
}
