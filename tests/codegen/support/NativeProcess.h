#pragma once
#include <chrono>
#include <string>
#include <vector>

namespace Codegen
{
/** @brief Captured child status and combined standard output/error. */
struct ProcessResult
{
    int exit_code;
    std::string output;
};

/** @brief Executes UTF-8 arguments without a shell, terminating the process tree on timeout. */
ProcessResult run_process(const std::vector<std::string>& arguments,
    std::chrono::milliseconds timeout = std::chrono::seconds(30));
}
