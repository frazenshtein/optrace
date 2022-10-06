#pragma once

#include <string>
#include <vector>

namespace NOPTrace {
    struct TOptions {
        std::string Output;
        bool AppendOutput;
        bool FollowForks;
        bool WaitDaemons;
        bool JailForks;
        bool HumanReadableSizes;
        int FilesInReport;
        int CommandLengthLimit;
        bool UseSecComp;
        bool SearchForCoreDumps;
        std::vector<int> ForwardingSignals;
        bool ForwardAllSignals;
        bool StoreEmptyFiles;
    };

    int TraceMe(const struct TOptions opts);
    int TraceProgram(char** argv, const struct TOptions opts);
}
