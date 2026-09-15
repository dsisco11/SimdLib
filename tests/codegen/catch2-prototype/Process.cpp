#include "Process.h"
#include <array>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace CodegenPrototype
{
#pragma region Platform argument encoding
#ifdef _WIN32
/** @brief Quotes a single Windows CRT argument, preserving trailing backslashes. */
static std::string quote_argument(const std::string& value)
{
    std::string result = "\"";
    std::size_t slashes = 0;
    for (const char c : value)
    {
        if (c == '\\') { ++slashes; continue; }
        result.append(c == '"' ? 2 * slashes + 1 : slashes, '\\');
        result += c;
        slashes = 0;
    }
    result.append(2 * slashes, '\\');
    return result + '"';
}
#endif
#pragma endregion

#pragma region Process execution
ProcessResult run_process(const std::vector<std::string>& arguments)
{
    if (arguments.empty()) throw std::runtime_error("Missing process executable");
    ProcessResult result{-1, {}};
    std::array<char, 4096> buffer{};
#ifdef _WIN32
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE reader = nullptr, writer = nullptr;
    if (!CreatePipe(&reader, &writer, &security, 0)) throw std::runtime_error("CreatePipe failed");
    SetHandleInformation(reader, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = writer;
    startup.hStdError = writer;
    PROCESS_INFORMATION process{};
    std::string command;
    for (const auto& argument : arguments) command += quote_argument(argument) + ' ';
    const bool launched = CreateProcessA(arguments.front().c_str(), command.data(), nullptr,
        nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process) != FALSE;
    CloseHandle(writer);
    if (!launched)
    {
        CloseHandle(reader);
        throw std::runtime_error("Cannot launch " + arguments.front());
    }
    // Drain output before waiting so a full pipe cannot block the child.
    DWORD count = 0;
    while (ReadFile(reader, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr) && count)
        result.output.append(buffer.data(), count);
    CloseHandle(reader);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(process.hProcess, &exit_code);
    result.exit_code = static_cast<int>(exit_code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
#else
    int descriptors[2];
    if (pipe(descriptors) != 0) throw std::runtime_error("pipe failed");
    std::vector<char*> argv;
    for (const auto& argument : arguments) argv.push_back(const_cast<char*>(argument.c_str()));
    argv.push_back(nullptr);
    const auto child = fork();
    if (child == 0)
    {
        close(descriptors[0]);
        dup2(descriptors[1], STDOUT_FILENO);
        dup2(descriptors[1], STDERR_FILENO);
        close(descriptors[1]);
        execv(argv.front(), argv.data());
        _exit(127);
    }
    close(descriptors[1]);
    if (child < 0)
    {
        close(descriptors[0]);
        throw std::runtime_error("fork failed");
    }
    for (;;)
    {
        const auto count = read(descriptors[0], buffer.data(), buffer.size());
        if (count > 0) result.output.append(buffer.data(), static_cast<std::size_t>(count));
        else if (count < 0 && errno == EINTR) continue;
        else break;
    }
    close(descriptors[0]);
    int status = 0;
    while (waitpid(child, &status, 0) < 0)
        if (errno != EINTR) throw std::runtime_error("waitpid failed");
    result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
#endif
    return result;
}
#pragma endregion
}
