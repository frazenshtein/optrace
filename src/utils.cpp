#include "utils.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include <fcntl.h>
#include <linux/limits.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>

namespace NOPTrace {
    int MyHighestFd() noexcept {
        struct rlimit rlim;
        assert(getrlimit(RLIMIT_NOFILE, &rlim) == 0);

        int fd = rlim.rlim_cur;
        while (fd >= 0) {
            if (fcntl(fd, F_GETFD) >= 0)
                break;
            fd--;
        }
        return fd;
    }

    bool KernelVerGreaterOrEqual(const char* ver) noexcept {
        struct utsname un;
        assert(uname(&un) == 0);
        return strverscmp(un.release, ver) >= 0;
    }

    bool IsFile(int fd) noexcept {
        static struct stat st;
        assert(fstat(fd, &st) >= 0);
        return S_ISREG(st.st_mode);
    }

    std::string ReadFileSafe(const std::string& filename, long limit) noexcept {
        std::ifstream file(filename);
        if (file.is_open()) {
            std::string res;
            if (limit < 0) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                res = buffer.str();
            } else if (limit > 0) {
                res.resize(limit);
                file.read(&res[0], limit);
            }
            StripString(res);
        }
        return "";
    }

    std::string GetDirName(const std::string& filename) noexcept {
        auto pos = filename.rfind("/");
        if (pos == std::string::npos) {
            return "/";
        }
        return std::string(filename.substr(0, pos));
    }

    std::string GetBaseName(const std::string& filename) noexcept {
        auto pos = filename.rfind("/");
        if (pos == std::string::npos) {
            return "";
        }
        return std::string(filename.substr(pos + 1));
    }

    std::string GetCwd() noexcept {
        char buff[PATH_MAX];
        if (!!getcwd(buff, sizeof(buff))) {
            return std::string(buff);
        } else {
            return "";
        }
    }

    size_t GetFileLength(const std::string& filename) noexcept {
        struct stat st;
        if (stat(filename.c_str(), &st) == 0) {
            return st.st_size;
        } else {
            return 0;
        }
    }

    std::string ReadLink(const std::string& filename) noexcept {
        char buff[PATH_MAX];
        ssize_t size = readlink(filename.c_str(), buff, sizeof(buff) - 1);
        if (size != -1) {
            buff[size] = '\0';
            return std::string(buff);
        } else {
            std::cerr << "Failed to readlink (" << filename.c_str() << "): " << strerror(errno) << std::endl;
            exit(2);
        }
    }

    void StripString(std::string &str) noexcept {
        static const char* ws = " \t\n\r";
        str.erase(str.find_last_not_of(ws) + 1);
        str.erase(0, str.find_first_not_of(ws));
    }

    std::string GetCommandLine(pid_t pid, long limit) noexcept {
        std::stringstream ss;
        ss << "/proc/" << pid << "/cmdline";
        std::string cmdline = ReadFileSafe(ss.str(), limit);
        // See `man 5 proc` format of the /proc/[pid]/cmdline for more info
        for (auto& s : cmdline) {
            if (s == '\0') {
                s = ' ';
            }
        }
        return cmdline;
    }

    std::string HumanReadableSize(size_t bytes) noexcept {
        if (bytes == 0) {
            return "0";
        }

        static const char* const suffixes[] = {
            "b",
            "KiB",
            "MiB",
            "GiB",
            "PiB"
        };
        static char buff[8];

        int log = std::log2(bytes);
        int k = log / 10;

        double val(bytes);
        for (int i = k; i > 0; i--) {
            val /= 1024;
        }

        if (val >= 100.0) {
            sprintf(&buff[0], "%d%s", int(val), suffixes[k]);
        } else {
            sprintf(&buff[0], "%3.1f%s", val, suffixes[k]);
        }
        return buff;
    }
}
