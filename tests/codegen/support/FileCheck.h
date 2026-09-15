#pragma once
#include "NativeProcess.h"
#include <filesystem>

namespace Codegen
{
/** @brief Runs one independent FileCheck rule on a sentinel-bounded input. */
ProcessResult file_check(const std::string& executable, const std::filesystem::path& rule,
    const std::filesystem::path& input, const std::vector<std::string>& parameters);
/** @brief Returns forbidden LLVM mnemonic lines without changing the disassembly. */
std::vector<std::string> forbidden_mnemonics(const std::string& body, const std::string& allowed);
}
