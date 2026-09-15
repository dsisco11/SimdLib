#pragma once
#include <filesystem>
#include <string>
namespace Codegen
{
/** @brief Encodes a filesystem path as UTF-8 at the native process boundary. */
inline std::string utf8(const std::filesystem::path& path)
{
    const auto value = path.u8string();
    return {reinterpret_cast<const char*>(value.data()), value.size()};
}
}
