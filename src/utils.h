#pragma once

#include <string>

namespace NOPTrace {
    bool IsFile(int fd) noexcept;
    int MyHighestFd() noexcept;
    bool KernelVerGreaterOrEqual(const char* ver) noexcept;
    std::string ReadFileSafe(const std::string& filename, long limit=-1) noexcept;
    std::string GetDirName(const std::string& filename) noexcept;
    std::string GetBaseName(const std::string& filename) noexcept;
    std::string GetCwd() noexcept;
    size_t GetFileLength(const std::string& filename) noexcept;
    std::string ReadLink(const std::string& filename) noexcept;
    std::string GetCommandLine(pid_t pid, long limit=-1) noexcept;
    std::string HumanReadableSize(size_t bytes) noexcept;
    void StripString(std::string &str) noexcept;
}
