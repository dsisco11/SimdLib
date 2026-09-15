#pragma once
#include <string>
#include <vector>

namespace CodegenPrototype
{
/** @brief Exit status and combined diagnostics of one directly launched process. */
struct ProcessResult
{
    int exit_code;
    std::string output;
};

/** @brief Runs an absolute executable without a shell and captures stdout/stderr. */
ProcessResult run_process(const std::vector<std::string>& arguments);
}
