#pragma once

#include "types.h"

#include <queue>
#include <vector>

namespace NOPTrace {
    struct TFileSizeGreater {
        bool operator()(TOutputFilePtr f1, TOutputFilePtr f2) const {
            return f1->Size > f2->Size;
        }
    };

    class TFileStorage {
    public:
        TFileStorage(long capacity, bool storeEmptyFiles)
            : Capacity(capacity)
            , OutputSize(0)
            , StoreEmptyFiles(storeEmptyFiles)
            , Queue(TFileSizeGreater())
        {
        }

        void AddFileEntry(TOutputFilePtr file) noexcept;
        std::vector<TOutputFilePtr> GetLargestFiles() const noexcept;

        size_t GetOutputSize() const noexcept {
            return OutputSize;
        }

    private:
        long Capacity;
        size_t OutputSize;
        bool StoreEmptyFiles;

        using TMaxOutFilesQueue = std::priority_queue<TOutputFilePtr, std::vector<TOutputFilePtr>, TFileSizeGreater>;
        TMaxOutFilesQueue Queue;
    };
}
