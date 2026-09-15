#pragma once
#include "NativeProcess.h"
#include "Applicability.h"
#include "Paths.h"
#include <filesystem>
#include <memory>
#include <set>
#include <string>

namespace Codegen
{
/** @brief Shared extraction retained for the lifetime of the test process. */
struct Inspection;
/** @brief Required rule identities collected independently from test declarations. */
struct Coverage
{
    std::multiset<std::string> observed;
};

/** @brief Assertions over one complete LLVM-selected function. */
class Function
{
    std::shared_ptr<Inspection> data_;
    std::shared_ptr<Coverage> coverage_;
    /** @brief Applies one independent rule and records its mandatory identity. */
    void apply(const std::string& rule, bool supplemental, bool constants) const;
public:
    /** @brief Binds immutable inspection to the current test's coverage collector. */
    Function(std::shared_ptr<Inspection> data, std::shared_ptr<Coverage> coverage);
    /** @brief Checks a shared primary rule without aborting subsequent assertions. */
    void check(const std::string& rule) const;
    /** @brief Checks an additional fact only for the explicitly named compiler family. */
    void check_for(Scenario compiler, const std::string& rule) const;
    /** @brief Checks referenced constant bytes for a named compiler family. */
    void constants_for(Scenario compiler, const std::string& rule) const;
    /** @brief Restricts mnemonics without decoding or rewriting LLVM instructions. */
    void allow(const std::string& mnemonics) const;
    /** @brief Adds a compiler-specific mnemonic restriction. */
    void allow_for(Scenario compiler, const std::string& mnemonics) const;
};

/** @brief Hides process/artifact bookkeeping behind readable instruction assertions. */
class CodegenFixture
{
    std::shared_ptr<Coverage> coverage_ = std::make_shared<Coverage>();
    std::string case_;
public:
    /** @brief Captures the active logical Catch2 case identity. */
    CodegenFixture();
    /** @brief Binds an explicit logical case for isolated harness failure probes. */
    explicit CodegenFixture(std::string case_id);
    /** @brief Rejects omitted or wrongly applicable rules against the independent ledger. */
    ~CodegenFixture() noexcept(false);
    /** @brief Resolves an exact build-declared symbol and verifies inspection freshness. */
    Function inspect(const std::string& symbol) const;
};

/** @brief Returns the independent required symbol/rule identities for one pilot case. */
std::multiset<std::string> expected_rules(const std::string& case_id);
}
