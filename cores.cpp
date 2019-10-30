#include "cores.h"
#include "utils.h"

#include <sstream>
#include <string>
#include <vector>

#include <dirent.h>
#include <fnmatch.h>

namespace NOPTrace {

    struct TCorePattern {
        std::string Path;
        std::string Mask;
    };

    std::string RecoverCoreDumpFile(pid_t pid, const std::string& comm, const std::string& cwd, int termSig) noexcept {
        std::string corePattern = ReadFileSafe("/proc/sys/kernel/core_pattern");

        TCorePattern defaultPattern{"", "*"};
        if (corePattern.find('/', 0) == 0) {
            defaultPattern.Path = GetDirName(corePattern);
        } else {
            defaultPattern.Path = cwd;
        }
        defaultPattern.Path += '/';

        if (corePattern.empty() || corePattern.find("|", 0) == 0) {
            std::string coreUsesPid = ReadFileSafe("/proc/sys/kernel/core_uses_pid");

            if (coreUsesPid.empty() || coreUsesPid == "0") {
                defaultPattern.Mask = "core";
            } else {
                defaultPattern.Mask = "core.%p";
            }
        } else {
            defaultPattern.Mask = GetBaseName(corePattern);
        }

        // Some widely distributed core dump dirs and masks
        std::vector<TCorePattern> patterns = {
            defaultPattern,
            {"/coredumps/", "%e.%p.%s"},
            {"/coredumps/", "*%p*"},
            {"/cores/", "*%p*"},
            {"/var/lib/systemd/coredump", "*%p*"},
        };

        auto ResolveMask = [&](const std::string& mask) -> std::string {
            std::stringstream res;
            for (auto it = mask.begin(); it != mask.end(); it++) {
                if (*it == '%') {
                    switch (*++it) {
                    case 0:
                        break;
                    case '%':
                        res << *it;
                        break;
                    case 'p':
                        res << pid;
                        break;
                    case 'e':
                        res << comm;
                        break;
                    case 's':
                        res << termSig;
                        break;
                    default:
                        res << "*";
                    }
                } else {
                    res << *it;
                }
            }
            return res.str();
        };

        DIR *dir;
        struct dirent *direntry;

        for (auto& pattern : patterns) {
            std::string mask = ResolveMask(pattern.Mask);

            dir = opendir(pattern.Path.c_str());
            if (!dir) {
                continue;
            } else {
                while ((direntry = readdir(dir)) != nullptr) {
                    if (direntry->d_type != DT_REG) {
                        continue;
                    }
                    if (fnmatch(mask.c_str(), direntry->d_name, FNM_NOESCAPE) == 0) {
                        std::stringstream res;
                        res << pattern.Path << direntry->d_name;
                        return res.str();
                    }
                }
                closedir(dir);
            }
        }
        return "";
    }
}
