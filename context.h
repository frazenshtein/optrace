#pragma once

#include "optrace.h"
#include "ptrace.h"
#include "storage.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <sys/user.h>

namespace NOPTrace {
    class TContext {
    public:
        TContext(struct TOptions opts)
            : Options(opts)
            , FileStorage(opts.FilesInReport)
        {
        }

        void RegisterTracee(pid_t pid) noexcept;
        void RegisterThread(pid_t pid, pid_t thread) noexcept;
        void RegisterProcess(pid_t parent, pid_t child) noexcept;
        void RegisterExec(pid_t pid, pid_t execpid) noexcept;
        void RegisterCoreDump(pid_t pid, int termSig) noexcept;
        void VanishProcess(pid_t pid) noexcept;

        int SyscallEnter(pid_t pid, const user_regs_struct& registers) noexcept;
        int SyscallExit(pid_t pid, const user_regs_struct& registers) noexcept;

        int PostProcess(int rc) noexcept;

    private:
        void FillFds(TProcState* proc) noexcept;
        int GetHighestFd(const std::vector<TFileStatePtr>& fds, bool cloexecFree) const noexcept;
        void TearDownFd(TFileStatePtr& file, TProcInfoPtr pinfo) noexcept;

        TProcStatePtr NewProcState(size_t pid, size_t ppid) const noexcept;
        TProcState* GetProcState(pid_t pid) noexcept;
        void SearchAndRegisterCoreDumpFile(TProcInfoPtr pinfo, int termSig) noexcept;

        void OpOpenFile(pid_t pid, size_t fd, size_t flags) noexcept;
        void OpOpenWriteFile(pid_t pid, size_t fd, size_t flags) noexcept;
        bool OpDup(pid_t pid, size_t fd, size_t newfd) noexcept;
        bool OpDup2(pid_t pid, size_t oldfd, size_t newfd) noexcept;
        bool OpDup3(pid_t pid, size_t oldfd, size_t newfd, size_t flags) noexcept;
        void OpResize(pid_t pid, size_t fd, size_t size) noexcept;
        void OpSeek(pid_t pid, size_t fd, size_t pos) noexcept;
        void OpSetFlag(pid_t pid, size_t fd, size_t flags) noexcept;
        void OpWriteChangeOffset(pid_t pid, size_t fd, size_t offset) noexcept;
        void OpWriteNoOffsetChange(pid_t pid, size_t fd, size_t nbytes, size_t offset) noexcept;
        void OpClose(pid_t pid, size_t fd) noexcept;

        void PrintReport() const noexcept;

    private:
        const struct TOptions Options;

        std::unordered_set<pid_t> GroupLeaders;
        std::unordered_map<pid_t, TProcStatePtr> ProcMap;
        TFileStorage FileStorage;
    };
}
