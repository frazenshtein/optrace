#pragma once

#include <memory>
#include <string>
#include <vector>

namespace NOPTrace {
    struct TProcInfo {
        TProcInfo(pid_t pid, pid_t ppid, std::string comm, std::string cmd)
            : Pid(pid)
            , Ppid(ppid)
            , CommandName(comm)
            , CommandLine(cmd)
        {
        }

        pid_t Pid;
        pid_t Ppid;
        std::string CommandName;
        std::string CommandLine;
    };

    using TProcInfoPtr = std::shared_ptr<const TProcInfo>;

    class TFileState {
    public:
        TFileState(const std::string& filename, size_t flags);
        TFileState(const TFileState& s);

        void Enroll(size_t nbytes) noexcept;
        void EnrollNoShift(size_t nbytes, size_t offset) noexcept;

        bool IsAppendSet() const noexcept;
        bool IsCloexecSet() const noexcept;

        void SetFlags(size_t flags) noexcept;
        void SetCloexecFlag(bool on) noexcept;
        void SetCurrPos(size_t pos) noexcept;

        size_t GetOutputSize() const noexcept;
        std::string GetFilename() const noexcept;

    private:
        size_t MaxPos;
        size_t CurrPos;
        size_t Flags;
        size_t InitSize;
        std::string Filename;
    };

    using TFileStatePtr = std::shared_ptr<TFileState>;

    struct TProcState {
        TProcState(pid_t pid, pid_t ppid, const std::string& comm, const std::string& cmd) {
            ProcInfo = std::make_shared<const TProcInfo>(pid, ppid, comm, cmd);
        }

        std::vector<TFileStatePtr> Fds;
        TProcInfoPtr ProcInfo;
    };

    using TProcStatePtr = std::shared_ptr<TProcState>;

    struct TOutputFile {
        TOutputFile(TFileState const* file, TProcInfoPtr pinfo)
            : Size(file->GetOutputSize())
            , Filename(file->GetFilename())
            , ProcInfo(pinfo)
        {
        }
        TOutputFile(const std::string& filename, size_t size, TProcInfoPtr pinfo)
            : Size(size)
            , Filename(filename)
            , ProcInfo(pinfo)
        {
        }

        size_t Size;
        std::string Filename;
        TProcInfoPtr ProcInfo;
    };

    using TOutputFilePtr = std::shared_ptr<const TOutputFile>;

}
