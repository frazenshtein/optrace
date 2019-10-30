#pragma once

#include <string>

namespace NOPTrace {
    std::string RecoverCoreDumpFile(pid_t pid, const std::string& comm, const std::string& cwd, int termSig) noexcept;
}
