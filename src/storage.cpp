#include "storage.h"

namespace NOPTrace {
    void TFileStorage::AddFileEntry(TOutputFilePtr file) noexcept {
        size_t size = file->Size;

        if (!size || !Capacity) {
            return;
        }

        if ((Capacity < 0) || (Queue.size() < (size_t)Capacity)) {
            Queue.push(file);
        } else if (Queue.top()->Size < size) {
            Queue.pop();
            Queue.push(file);
        }

        OutputSize += size;
    }

    std::vector<TOutputFilePtr> TFileStorage::GetLargestFiles() const noexcept {
        TMaxOutFilesQueue temp = Queue;
        std::vector<TOutputFilePtr> res(Queue.size());

        while (!temp.empty()) {
            res[temp.size() - 1] = temp.top();
            temp.pop();
        }
        return res;
    }
}
