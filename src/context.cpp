#include "context.h"

#include "cores.h"
#include "syscall.h"
#include "utils.h"

#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <sched.h>
#include <fcntl.h>

namespace NOPTrace {
    void TContext::RegisterTracee(pid_t pid) noexcept {
        assert(ProcMap.find(pid) == ProcMap.end());

        auto proc = NewProcState(pid, 0);
        ProcMap[pid] = proc;
        GroupLeaders.emplace(pid);

        FillFds(proc.get());
    }

    void TContext::RegisterExec(pid_t pid, pid_t /*execpid*/) noexcept {
        auto oldproc = GetProcState(pid);
        auto newproc = NewProcState(pid, oldproc->ProcInfo->Ppid);

        // Drop empty and file descriptors with O_CLOEXEC flag
        const int highestFd = GetHighestFd(oldproc->Fds, true);
        if (highestFd >= 0) {
            auto& newfds = newproc->Fds;
            auto& oldfds = oldproc->Fds;

            newfds.resize(highestFd + 1);
            for (int i = 0; i <= highestFd; i++) {
                if (oldfds[i] && !oldfds[i]->IsCloexecSet()) {
                    newfds[i] = std::make_shared<TFileState>(*oldfds[i]);
                }
            }
        }

        VanishProcess(pid);

        GroupLeaders.emplace(pid);
        ProcMap[pid] = newproc;
    }

    TProcStatePtr TContext::NewProcState(size_t pid, size_t ppid) const noexcept {
        std::string cmdline = GetCommandLine(pid, Options.CommandLengthLimit);
        std::string comm;
        if (Options.SearchForCoreDumps) {
            std::stringstream ss;
            ss << "/proc/" << pid << "/comm";
            comm = ReadFileSafe(ss.str());
        }

        return std::make_shared<TProcState>(pid, ppid, comm, cmdline);
    }

    int TContext::GetHighestFd(const std::vector<TFileStatePtr>& fds, bool cloexecFree) const noexcept {
        for (int fd = fds.size() - 1; fd >= 0; fd--) {
            if (!!fds[fd]) {
                if (cloexecFree) {
                    if (!fds[fd]->IsCloexecSet()) {
                        return fd;
                    }
                } else {
                    return fd;
                }
            }
        }
        return -1;
    }

    void TContext::RegisterProcess(pid_t parent, pid_t child) noexcept {
        assert(ProcMap.find(child) == ProcMap.end());
        auto pproc = GetProcState(parent);

        auto newproc = NewProcState(child, parent);

        const int highestFd = GetHighestFd(pproc->Fds, false);
        if (highestFd >= 0) {
            auto& newfds = newproc->Fds;
            auto& pprocfds = pproc->Fds;

            newfds.resize(highestFd + 1);
            for (int i = 0; i <= highestFd; i++) {
                if (pprocfds[i]) {
                    newfds[i] = std::make_shared<TFileState>(*pprocfds[i]);
                }
            }
        }

        GroupLeaders.emplace(child);
        ProcMap[child] = newproc;
    }

    void TContext::RegisterThread(pid_t pid, pid_t thread) noexcept {
        assert(ProcMap.find(pid) != ProcMap.end());
        ProcMap[thread] = ProcMap[pid];
    }

    void TContext::TearDownFd(TFileStatePtr& file, TProcInfoPtr pinfo) noexcept {
        assert(!!file);

        // Add an entry to the storage only when closing the last ref to the FileState
        if (file.use_count() == 1) {
            auto output = std::make_shared<TOutputFile>(file.get(), pinfo);
            FileStorage.AddFileEntry(output);
        }

        file = nullptr;
    }

    void TContext::VanishProcess(pid_t pid) noexcept {
        if (GroupLeaders.find(pid) != GroupLeaders.end()) {
            auto proc = GetProcState(pid);
            auto pinfo = proc->ProcInfo;

            for (auto& file : proc->Fds) {
                if (file) {
                    TearDownFd(file, pinfo);
                }
            }
            GroupLeaders.erase(pid);
        }

        assert(ProcMap.find(pid) != ProcMap.end());
        ProcMap.erase(pid);
    }

    void TContext::RegisterCoreDump(pid_t pid, int termSig) noexcept {
        auto pinfo = GetProcState(pid)->ProcInfo;

        if (Options.SearchForCoreDumps) {
            SearchAndRegisterCoreDumpFile(pinfo, termSig);
        }
    }

    void TContext::SearchAndRegisterCoreDumpFile(TProcInfoPtr pinfo, int termSig) noexcept {
        // TODO We need to use pinfo->Cwd and trace (f)chdir if SearchForCoreDumps is specified
        // check process creation and finish time with core m_time, store (path, m_time, size)
        // to avoid discovering same core more than once
        std::string filename = RecoverCoreDumpFile(pinfo->Pid, pinfo->CommandName, GetCwd(), termSig);
        if (!filename.empty()) {
            auto output = std::make_shared<TOutputFile>(filename, GetFileLength(filename), pinfo);
            FileStorage.AddFileEntry(output);
        }
    }

    void TContext::FillFds(TProcState* proc) noexcept {
        const int highestFd = MyHighestFd();
        if (highestFd < 0) {
            return;
        }

        auto& fds = proc->Fds;
        fds.resize(highestFd + 1);

        std::stringstream ss;
        ss << "/proc/" << proc->ProcInfo->Pid << "/fd/";
        std::string prefix(ss.str());
        int prefixSize = prefix.size();
        prefix.resize(prefix.size() + std::log10(highestFd) + 1);
        char* prefixEnd = const_cast<char*>(prefix.c_str()) + prefixSize;

        int flags = -1;
        for (int fd = highestFd; fd >= 0; fd--) {
            flags = fcntl(fd, F_GETFD);

            if (flags >= 0 && IsFile(fd)) {
                snprintf(prefixEnd, prefixSize, "%d", fd);
                fds[fd] = std::make_shared<TFileState>(ReadLink(prefix), flags);
            }
        }
        return;
    }

    int TContext::SyscallEnter(pid_t, const user_regs_struct&) noexcept {
        return 0;
    }

    TProcState* TContext::GetProcState(pid_t pid) noexcept {
        assert(ProcMap.find(pid) != ProcMap.end());
        return ProcMap[pid].get();
    }

    void TContext::OpWriteChangeOffset(pid_t pid, size_t fd, size_t offset) noexcept {
        auto& fds = GetProcState(pid)->Fds;

        if (fds.size() > fd && !!fds[fd]) {
            fds[fd]->Enroll(offset);
        }
    }

    void TContext::OpWriteNoOffsetChange(pid_t pid, size_t fd, size_t nbytes, size_t offset) noexcept {
        auto& fds = GetProcState(pid)->Fds;

        if ((fds.size() > fd) && !!fds[fd]) {
            fds[fd]->EnrollNoShift(nbytes, offset);
        }
    }

    void TContext::OpOpenWriteFile(pid_t pid, size_t fd, size_t flags) noexcept {
        auto& fds = GetProcState(pid)->Fds;

        if (fd >= fds.size()) {
            fds.resize(fd + 1);
        }

        std::stringstream ss;
        ss << "/proc/" << pid << "/fd/" << fd;

        std::string filename = ReadLink(ss.str());
        fds[fd] = std::make_shared<TFileState>(filename, flags);
    }

    void TContext::OpOpenFile(pid_t pid, size_t fd, size_t flags) noexcept {
        if ((flags & O_WRONLY) || (flags & O_RDWR)) {
            OpOpenWriteFile(pid, fd, flags);
        }
    }

    void TContext::OpClose(pid_t pid, size_t fd) noexcept {
        auto proc = GetProcState(pid);
        auto& fds = proc->Fds;

        if ((fd < fds.size()) && !!fds[fd]) {
            auto& file = fds[fd];

            if (file) {
                TearDownFd(file, proc->ProcInfo);
            }
        }
    }

    bool TContext::OpDup(pid_t pid, size_t fd, size_t newfd) noexcept {
        return OpDup2(pid, fd, newfd);
    }

    bool TContext::OpDup2(pid_t pid, size_t oldfd, size_t newfd) noexcept {
        auto& fds = GetProcState(pid)->Fds;

        if (oldfd == newfd) {
            return false;
        }
        // newfd is valid previously opened file descriptor, we need to close it
        if (newfd < fds.size() && !!fds[newfd]) {
            OpClose(pid, newfd);
        }

        // oldfd is not a file descriptor opened for writing
        if ((oldfd >= fds.size()) || !fds[oldfd]) {
            return false;
        }

        if (newfd >= fds.size()) {
            fds.resize(newfd + 1);
        }
        fds[newfd] = fds[oldfd];
        return true;
    }

    bool TContext::OpDup3(pid_t pid, size_t oldfd, size_t newfd, size_t flags) noexcept {
        if (!OpDup(pid, oldfd, newfd)) {
            return false;
        }

        auto proc = GetProcState(pid);
        proc->Fds[newfd]->SetCloexecFlag(flags & O_CLOEXEC);
        return true;
    }

    void TContext::OpResize(pid_t pid, size_t fd, size_t size) noexcept {
        auto& fds = GetProcState(pid)->Fds;

        if ((fds.size() > fd) && !!fds[fd]) {
            fds[fd]->EnrollNoShift(size, 0);
        }
    }

    void TContext::OpSeek(pid_t pid, size_t fd, size_t pos) noexcept {
        auto& fds = GetProcState(pid)->Fds;

        if ((fds.size() > fd) && !!fds[fd]) {
            fds[fd]->SetCurrPos(pos);
        }
    }

    void TContext::OpSetFlag(pid_t pid, size_t fd, size_t flags) noexcept {
        auto& fds = GetProcState(pid)->Fds;

        if ((fds.size() > fd) && !!fds[fd]) {
            fds[fd]->SetFlags(flags);
        }
    }

    int TContext::SyscallExit(pid_t pid, const user_regs_struct& registers) noexcept {
        const unsigned long& syscall = registers.orig_rax;
        const unsigned long& retdata = registers.rax;

        switch (syscall) {
            case SYS_write:
            case SYS_writev:
                if ((ssize_t)retdata > 0) {
                    OpWriteChangeOffset(pid, registers.rdi, retdata);
                }
                break;
            case SYS_pwritev:
            case SYS_pwrite64:
            case SYS_pwritev2:
                if ((ssize_t)retdata > 0) {
                    OpWriteNoOffsetChange(pid, registers.rdi, retdata, registers.r10);
                }
                break;
            case SYS_creat:
                if ((int)retdata >= 0) {
                    OpOpenWriteFile(pid, retdata, registers.rsi);
                }
                break;
            case SYS_open:
                if ((int)retdata >= 0) {
                    OpOpenFile(pid, retdata, registers.rsi);
                }
                break;
            case SYS_openat:
                if ((int)retdata >= 0) {
                    OpOpenFile(pid, retdata, registers.rdx);
                }
                break;
            case SYS_close:
                OpClose(pid, registers.rdi);
                break;
            case SYS_fcntl:
                if ((int)retdata >= 0) {
                    switch (registers.rsi) {
                        case F_DUPFD:
                            OpDup(pid, registers.rdi, retdata);
                            break;
                        case F_DUPFD_CLOEXEC:
                            OpDup3(pid, registers.rdi, retdata, O_CLOEXEC);
                            break;
                        case F_SETFL:
                        case F_SETFD:
                            OpSetFlag(pid, registers.rdi, registers.rdx);
                            break;
                    }
                }
                break;
            case SYS_dup:
                if ((int)retdata >= 0) {
                    OpDup(pid, registers.rdi, retdata);
                }
                break;
            case SYS_dup2:
                if ((int)retdata >= 0) {
                    OpDup2(pid, registers.rdi, registers.rsi);
                }
                break;
            case SYS_dup3:
                if ((int)retdata >= 0) {
                    OpDup3(pid, registers.rdi, registers.rsi, registers.rdx);
                }
                break;
            case SYS_fallocate:
                if ((int)retdata >= 0) {
                    //                           v offset + len
                    OpResize(pid, registers.rdi, registers.rdx + registers.r10);
                }
                break;
            case SYS_ftruncate:
                if ((int)retdata >= 0) {
                    OpResize(pid, registers.rdi, registers.rsi);
                }
                break;
            case SYS_lseek:
                if ((int)retdata >= 0) {
                    OpSeek(pid, registers.rdi, retdata);
                }
                break;
        }
        return 0;
    }

    void TContext::PrintReport() const noexcept {
        std::streambuf* streamBufPtr(std::cerr.rdbuf());
        std::ofstream ofStream;

        if (!Options.Output.empty()) {
            auto flags = std::ofstream::out;
            if (Options.AppendOutput) {
                flags |= std::ofstream::app;
            } else {
                flags |= std::ofstream::trunc;
            }

            ofStream.open(Options.Output, flags);
            if (ofStream.fail()) {
                std::cerr << "Failed to open " << Options.Output << ": " << strerror(errno) << std::endl;
            } else {
                streamBufPtr = ofStream.rdbuf();
            }
        }

        std::ostream stream(streamBufPtr);

        stream << "Output tracer summary report";
        if (Options.FilesInReport > 0) {
            stream << " (limit: " << Options.FilesInReport << ")";
        }
        stream << std::endl;

        std::unordered_set<TProcInfo const*> procInfoSet;
        const auto files = FileStorage.GetLargestFiles();

        int padding = 9;
        if (!Options.HumanReadableSizes) {
            size_t max = 0;
            for (const auto& e : files) {
                max = std::max(max, e->Size);
            }
            padding = std::log10(max) + 3;
        }

        bool dumpProcLegend = Options.CommandLengthLimit != 0;

        for (const auto& entry : files) {
            if (Options.HumanReadableSizes) {
                stream << std::setw(padding) << HumanReadableSize(entry->Size);
            } else {
                stream << std::setw(padding) << entry->Size << "b";
            }
            stream << " " << entry->Filename;
            if (dumpProcLegend) {
                stream << " (pid:" << entry->ProcInfo->Pid << "|" << (unsigned long)entry->ProcInfo.get() << ")";
            }
            stream << std::endl;

            procInfoSet.emplace(entry->ProcInfo.get());
        }

        if (dumpProcLegend) {
            stream << "Proc legend:" << std::endl;
            for (auto& pinfo : procInfoSet) {
                stream << "  " << pinfo->Pid << "|" << (unsigned long)pinfo << " (ppid:" << pinfo->Ppid << ") " << pinfo->CommandLine << std::endl;
            }
        }

        const size_t outputSize = FileStorage.GetOutputSize();

        stream << "Total output: ";
        if (Options.HumanReadableSizes) {
            stream << HumanReadableSize(outputSize) << std::endl;
        } else {
            stream << outputSize << "b" << std::endl;
        }
    }

    int TContext::PostProcess(int rc) noexcept {
        while (ProcMap.size()) {
            VanishProcess(ProcMap.begin()->first);
        }

        if (Options.FilesInReport != 0) {
            PrintReport();
        }
        return rc;
    }
}
