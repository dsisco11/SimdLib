#pragma once
#include "Configuration.h"
#include <filesystem>
#include <string>
#include <memory>

namespace Codegen
{
/** @brief Complete selected LLVM text and its isolated, retained diagnostic directory. */
struct Inspection
{
    Configuration::Symbol symbol;
    std::array<std::string, 4> object_strings;
    Configuration::Object object;
    std::string receipt_snapshot;
    std::filesystem::path directory;
    std::filesystem::path input;
    std::filesystem::path constants;
    std::string body;
    /** @brief Verifies freshness and extracts the exact build-declared function. */
    Inspection(Configuration::Symbol identity, Configuration::Object source);
    /** @brief Rejects changed sources, tools, objects or configuration before reuse. */
    void verify() const;
};
/** @brief Reuses an exact object/symbol/configuration inspection only after revalidation. */
std::shared_ptr<Inspection> inspect_function(Configuration::Symbol symbol, Configuration::Object object);
}
