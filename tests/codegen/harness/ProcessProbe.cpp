#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif

/** @brief Emits controlled arguments, output volume, exit status, or a timeout. */
static int probe(const std::vector<std::string>& arguments)
{
    if (arguments.size() > 1 && arguments[1] == "sleep")
    {
        std::cout << "timeout probe ready\n" << std::flush;
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
    if (arguments.size() > 1 && arguments[1] == "flood")
        std::cout << std::string(1024 * 1024, 'x');
    else
        for (std::size_t i = 1; i < arguments.size(); ++i)
            std::cout << arguments[i].size() << ':' << arguments[i] << '\n';
    std::cerr << "stderr captured\n";
    return arguments.size() > 1 && arguments[1] == "fail" ? 7 : 0;
}

#ifdef _WIN32
/** @brief Preserves Unicode arguments before emitting their UTF-8 representation. */
int wmain(int argc, wchar_t** argv)
{
    // Keep byte-level argument evidence independent of CRT newline translation.
    _setmode(_fileno(stdout), _O_BINARY);
    _setmode(_fileno(stderr), _O_BINARY);
    std::vector<std::string> arguments;
    for (int i = 0; i < argc; ++i)
    {
        const auto size = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
        std::string text(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, text.data(), size, nullptr, nullptr);
        text.pop_back();
        arguments.push_back(text);
    }
    return probe(arguments);
}
#else
/** @brief Passes the platform's UTF-8 argv to the shared probe behavior. */
int main(int argc, char** argv) { return probe({argv, argv + argc}); }
#endif
