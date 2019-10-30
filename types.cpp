#include "types.h"
#include "utils.h"

#include <fcntl.h>

namespace NOPTrace {
    TFileState::TFileState(const std::string& filename, size_t flags)
        : MaxPos(0)
        , CurrPos(0)
        , Flags(flags)
        , Filename(filename)
    {
        InitSize = GetFileLength(filename);
        if (IsAppendSet()) {
            CurrPos = InitSize;
        } else {
            CurrPos = 0;
        }
    }

    TFileState::TFileState(const TFileState& s) {
        MaxPos = s.MaxPos;
        CurrPos = s.CurrPos;
        Flags = s.Flags;
        InitSize = GetFileLength(s.Filename);
        Filename = s.Filename;
    }

    bool TFileState::IsAppendSet() noexcept {
        return Flags & O_APPEND;
    }

    bool TFileState::IsCloexecSet() noexcept {
        return Flags & O_CLOEXEC;
    }

    void TFileState::SetFlags(size_t flags) noexcept {
        Flags = flags;
    }

    size_t TFileState::GetOutputSize() const noexcept {
        if (MaxPos > InitSize) {
            return MaxPos - InitSize;
        } else {
            return 0;
        }
    }

    void TFileState::SetCurrPos(size_t pos) noexcept {
        CurrPos = pos;
    }

    void TFileState::Enroll(size_t nbytes) noexcept {
        CurrPos += nbytes;
        if (CurrPos > MaxPos) {
            MaxPos = CurrPos;
        }
    }

    void TFileState::EnrollNoShift(size_t nbytes, size_t offset) noexcept {
        if (offset + nbytes > MaxPos) {
            MaxPos = offset + nbytes;
        }
    }

    void TFileState::SetCloexecFlag(bool on) noexcept {
        if (on) {
            Flags |= O_CLOEXEC;
        } else {
            Flags &= ~O_CLOEXEC;
        }
    }

    std::string TFileState::GetFilename() const noexcept {
        return Filename;
    }
}
