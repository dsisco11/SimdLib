#include "NativeProcess.h"
#include <array>
#include <stdexcept>
#include <thread>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace Codegen
{
#pragma region Platform resource ownership
#ifdef _WIN32
/** @brief Closes an owned Windows kernel handle. */
struct Handle
{
    HANDLE value = nullptr;
    /** @brief Releases the handle on every exit path. */
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};

/** @brief Converts explicit UTF-8 tool paths and arguments to the Windows API encoding. */
static std::wstring wide(const std::string& text)
{
    if (text.empty()) return {};
    const auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
        static_cast<int>(text.size()), nullptr, 0);
    if (!size) throw std::runtime_error("Invalid UTF-8 process argument");
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
        static_cast<int>(text.size()), result.data(), size);
    return result;
}

/** @brief Applies CRT quoting, including embedded quotes and trailing backslashes. */
static std::wstring quote(const std::string& argument)
{
    std::wstring result = L"\"";
    std::size_t slashes = 0;
    for (const auto c : wide(argument))
    {
        if (c == L'\\') { ++slashes; continue; }
        result.append(c == L'"' ? 2 * slashes + 1 : slashes, L'\\');
        result += c;
        slashes = 0;
    }
    result.append(2 * slashes, L'\\');
    return result + L'"';
}
#else
/** @brief Owns a pipe descriptor. */
struct Descriptor
{
    int value;
    /** @brief Closes the descriptor on exit. */
    ~Descriptor() { if (value >= 0) close(value); }
};
/** @brief Reaps and terminates an unfinished child process group on failure. */
struct Child
{
    pid_t pid;
    bool reaped = false;
    /** @brief Prevents orphaned descendants and zombies after errors/timeouts. */
    ~Child()
    {
        kill(-pid, SIGKILL);
        if (!reaped) { int status; while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {} }
    }
};
#endif
#pragma endregion

#pragma region Bounded process execution
ProcessResult run_process(const std::vector<std::string>& arguments, std::chrono::milliseconds timeout)
{
    if (arguments.empty() || arguments.front().empty()) throw std::runtime_error("Missing process executable");
    ProcessResult result{-1, {}};
    std::array<char, 8192> buffer{};
    const auto deadline = std::chrono::steady_clock::now() + timeout;
#ifdef _WIN32
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    Handle reader, writer, job, process, thread, input;
    if (!CreatePipe(&reader.value, &writer.value, &security, 0) ||
        !SetHandleInformation(reader.value, HANDLE_FLAG_INHERIT, 0))
        throw std::runtime_error("Cannot create process output pipe");
    input.value = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        &security, OPEN_EXISTING, 0, nullptr);
    if (input.value == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot open process input");
    job.value = CreateJobObjectW(nullptr, nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!job.value || !SetInformationJobObject(job.value, JobObjectExtendedLimitInformation,
        &limits, sizeof(limits))) throw std::runtime_error("Cannot create bounded process job");
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input.value;
    startup.hStdOutput = startup.hStdError = writer.value;
    PROCESS_INFORMATION information{};
    std::wstring command;
    for (const auto& argument : arguments) command += quote(argument) + L' ';
    if (!CreateProcessW(wide(arguments.front()).c_str(), command.data(), nullptr, nullptr,
        TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, nullptr, &startup, &information))
        throw std::runtime_error("Cannot launch " + arguments.front() + ": Windows error " + std::to_string(GetLastError()));
    process.value = information.hProcess;
    thread.value = information.hThread;
    if (!AssignProcessToJobObject(job.value, process.value))
    {
        TerminateProcess(process.value, 1);
        throw std::runtime_error("Cannot assign process job");
    }
    if (ResumeThread(thread.value) == static_cast<DWORD>(-1)) throw std::runtime_error("Cannot resume process");
    CloseHandle(writer.value);
    writer.value = nullptr;
    bool exited = false;
    // Poll the pipe and child together: inherited pipes cannot defeat the deadline.
    for (;;)
    {
        DWORD available = 0, count = 0;
        if (!PeekNamedPipe(reader.value, nullptr, 0, nullptr, &available, nullptr))
        {
            if (GetLastError() != ERROR_BROKEN_PIPE) throw std::runtime_error("Cannot read process pipe");
            available = 0;
        }
        if (available)
        {
            if (!ReadFile(reader.value, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr))
                throw std::runtime_error("Process output read failed");
            result.output.append(buffer.data(), count);
        }
        const auto waited = WaitForSingleObject(process.value, 0);
        if (waited == WAIT_FAILED) throw std::runtime_error("Process wait failed");
        exited = waited == WAIT_OBJECT_0;
        if (exited && !available) break;
        if (std::chrono::steady_clock::now() >= deadline)
            throw std::runtime_error("Process timeout: " + arguments.front() + "\n" + result.output);
        if (!available) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    DWORD code = 0;
    if (!GetExitCodeProcess(process.value, &code)) throw std::runtime_error("Cannot read process exit status");
    result.exit_code = static_cast<int>(code);
#else
    int pipes[2];
    if (pipe(pipes) != 0) throw std::runtime_error("Cannot create process pipe");
    Descriptor reader{pipes[0]}, writer{pipes[1]};
    std::vector<char*> argv;
    for (const auto& argument : arguments) argv.push_back(const_cast<char*>(argument.c_str()));
    argv.push_back(nullptr);
    const auto pid = fork();
    if (pid < 0) throw std::runtime_error("Cannot fork process");
    if (pid == 0)
    {
        setpgid(0, 0);
        close(reader.value);
        if (dup2(writer.value, STDOUT_FILENO) < 0 || dup2(writer.value, STDERR_FILENO) < 0) _exit(126);
        close(writer.value);
        execv(argv.front(), argv.data());
        const char message[] = "Cannot exec selected tool\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        _exit(127);
    }
    Child child{pid};
    setpgid(pid, pid);
    close(writer.value);
    writer.value = -1;
    if (fcntl(reader.value, F_SETFL, O_NONBLOCK) < 0) throw std::runtime_error("Cannot configure process pipe");
    int status = 0;
    for (;;)
    {
        const auto count = read(reader.value, buffer.data(), buffer.size());
        if (count > 0) result.output.append(buffer.data(), static_cast<std::size_t>(count));
        else if (count < 0 && errno != EAGAIN && errno != EINTR) throw std::runtime_error("Process output read failed");
        if (!child.reaped)
        {
            const auto waited = waitpid(pid, &status, WNOHANG);
            if (waited < 0 && errno != EINTR) throw std::runtime_error("Process wait failed");
            child.reaped = waited == pid;
        }
        if (child.reaped && count == 0) break;
        if (std::chrono::steady_clock::now() >= deadline)
            throw std::runtime_error("Process timeout: " + arguments.front() + "\n" + result.output);
        if (count <= 0) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
#endif
    return result;
}
#pragma endregion
}
