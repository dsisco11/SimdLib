#include "HarnessOptions.h"
#include <catch2/catch_session.hpp>
#include <catch2/internal/catch_clara.hpp>
#include <filesystem>
#include "Paths.h"
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

/** @brief Adds explicit synthetic-input options to the ordinary Catch2 runner. */
int main(int argc, char** argv)
{
#ifdef _WIN32
    std::wstring module(32768, L'\0');
    const auto count = GetModuleFileNameW(nullptr, module.data(), static_cast<DWORD>(module.size()));
    if (!count || count == module.size()) return 2;
    module.resize(count);
    Harness::executable = Codegen::utf8(std::filesystem::path(module));
#else
    Harness::executable = std::filesystem::absolute(argv[0]).string();
#endif
    Catch::Session session;
    using Catch::Clara::Opt;
    session.cli(session.cli() |
        Opt(Harness::body, "path")["--body"]("Synthetic LLVM body") |
        Opt(Harness::rules, "paths")["--rules"]("Independent rule paths") |
        Opt(Harness::allow, "mnemonics")["--allow"]("Optional mnemonic allowlist") |
        Opt(Harness::constants)["--constants"]("Input is an LLVM constant dump") |
        Opt(Harness::fault, "kind")["--fault"]("Deliberate isolated failure"));
    return session.run(argc, argv);
}
