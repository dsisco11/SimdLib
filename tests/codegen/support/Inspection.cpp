#include "Inspection.h"
#include "NativeProcess.h"
#include "Paths.h"
#include <chrono>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <map>

namespace Codegen
{
#pragma region Inspection input and artifact ownership
/** @brief Supplies the same object and input identity for initial and cached inspection. */
static std::vector<std::string> command(const Configuration::Object& object)
{
    return {Configuration::cmake,
        "-DSCRIPTS=" + std::string(Configuration::root) + "/cmake/codegen",
        "-DOBJECT_FILE=" + std::string(object.path),
        "-DINPUT_MANIFEST=" + std::string(object.manifest),
        "-DBUILD_RECEIPT=" + std::string(object.receipt)};
}

/** @brief Reads the exact build snapshot to distinguish a fresh rebuild from cached output. */
static std::string snapshot(const char* receipt)
{
    std::ifstream stream(std::filesystem::u8path(receipt), std::ios::binary);
    if (!stream) throw std::runtime_error("Missing codegen build receipt");
    return {std::istreambuf_iterator<char>(stream), {}};
}

void Inspection::verify() const
{
    auto arguments = command(object);
    arguments.insert(arguments.end(), {"-DVERIFY_ONLY=ON", "-P",
        std::string(Configuration::root) + "/tests/codegen/support/Inspect.cmake"});
    const auto result = run_process(arguments);
    if (result.exit_code) throw std::runtime_error(result.output);
    if (snapshot(object.receipt) != receipt_snapshot)
        throw std::runtime_error("Cached inspection snapshot changed; restart the test after rebuilding");
}

Inspection::Inspection(Configuration::Symbol identity, Configuration::Object source) : symbol(identity),
    object_strings{source.path, source.manifest, source.receipt, source.provenance},
    object{object_strings[0].c_str(), object_strings[1].c_str(), object_strings[2].c_str(), object_strings[3].c_str()}
{
    receipt_snapshot = snapshot(object.receipt);
    // Atomically reserve a process-owned directory; old runs remain diagnostic artifacts.
    auto nonce = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::u8path(Configuration::artifacts);
    std::filesystem::create_directories(root);
    do { directory = root / std::to_string(nonce++); } while (!std::filesystem::create_directory(directory));
    auto arguments = command(object);
    arguments.insert(arguments.end(), {
        "-DEXPECTED_SYMBOL=" + std::string(symbol.emitted),
        "-DCONSTANT_SYMBOLS=" + std::string(symbol.constants),
        "-DOUTPUT_DIRECTORY=" + utf8(directory),
        "-DLLVM_OBJDUMP=" + std::string(Configuration::objdump),
        "-DLLVM_READOBJ=" + std::string(Configuration::readobj), "-P",
        std::string(Configuration::root) + "/tests/codegen/support/Inspect.cmake"});
    const auto result = run_process(arguments);
    if (result.exit_code) throw std::runtime_error(result.output + "\nArtifacts: " + utf8(directory));
    if (snapshot(object.receipt) != receipt_snapshot)
        throw std::runtime_error("Build snapshot changed during LLVM inspection");
    input = directory / "input.txt";
    constants = directory / "constant-input.txt";
    // Sentinels bound whole-body negatives without modifying the extracted LLVM text.
    for (const auto& [source, target] : {std::pair{"body.txt", input}, {"constants.txt", constants}})
    {
        std::ifstream stream(directory / source, std::ios::binary);
        if (!stream) throw std::runtime_error("Missing extracted input");
        std::string text{std::istreambuf_iterator<char>(stream), {}};
        if (std::string(source) == "body.txt")
        {
            if (text.empty()) throw std::runtime_error("Empty extracted function");
            body = text;
        }
        else if (!*symbol.constants) continue;
        std::ofstream output;
        output.exceptions(std::ios::failbit | std::ios::badbit);
        output.open(target, std::ios::binary);
        output << "CODEGEN-BEGIN\n" << text << "\nCODEGEN-END\n";
    }
}
std::shared_ptr<Inspection> inspect_function(Configuration::Symbol symbol, Configuration::Object object)
{
    // Include metadata inputs, constants and configuration, not just a basename.
    const auto key = std::string(object.path) + "\n" + object.manifest + "\n" + object.receipt + "\n" +
        symbol.emitted + "\n" + symbol.constants + "\n" + Configuration::configuration;
    static std::map<std::string, std::shared_ptr<Inspection>> cache;
    auto& data = cache[key];
    if (data) data->verify();
    else data = std::make_shared<Inspection>(symbol, object);
    return data;
}
#pragma endregion
}
