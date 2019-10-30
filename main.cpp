#include "optrace.h"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include <getopt.h>
#include <signal.h>
#include <unistd.h>

NOPTrace::TOptions GetDefaults() {
    return {
        .Output="",
        .AppendOutput=false,
        .FollowForks=true,
        .JailForks=true,
        .HumanReadableSizes=false,
        .FilesInReport=24,
        .CommandLengthLimit=100,
        .UseSecComp=true,
        .SearchForCoreDumps=true,
        .ForwardingSignals={SIGINT},
    };
}

void printHelp() {
    auto defaultOpts = GetDefaults();

    std::cout << "Usage: optrace [-fJhaSC] [-o FILE] [-c VAL]\n"
              << "               [-r VAL] [-s SIG] PROG [ARGS]\n"
              << "\nOutput format:\n"
              << "  -c|--cmdline-size VAL maximum string size for cmd lines\n"
              << "                        (negative for unlimited, 0 to disable, default:" << defaultOpts.CommandLengthLimit  << ")\n"
              << "  -r|--report-size VAL  number of files to print in report\n"
              << "                        (negative for unlimited, 0 to disable, default:" << defaultOpts.FilesInReport  << ")\n"
              << "  -o|--output FILE      send report to FILE instead of stderr\n"
              << "  -a|--append           don't overwrite output FILE\n"
              << "  -h|--human-readable   print sizes in human readable format\n"
              << "\nBehavior:\n"
              << "  -C|--no-coredumps     don't take into account core dump files\n"
              << "  -s|--forward-sig SIG  append signum to the list of forwarding signals to the PROG\n"
              << "                        (default: [SIGINT])\n"
              << "\nTracing:\n"
              // TODO << "  -f|--follow-forks     follow forks\n"
              << "  -J|--no-jail-forks    don't kill all created processes, when optrace exits\n"
              << "  -S|--no-seccomp       don't use seccomp anyway\n";
}

int main(int argc, char* argv[]) {
    auto optraceOpts = GetDefaults();

    const char* const short_cli_options = "+fJho:ac:r:SCs:h";
    const struct option cli_options[] = {
        {"follow-forks",    no_argument,        0, 'f'},
        {"no-jail-forks",   no_argument,        0, 'J'},
        {"human-readable",  no_argument,        0, 'h'},
        {"output",          no_argument,        0, 'o'},
        {"append",          no_argument,        0, 'a'},
        {"cmdline-size",    required_argument,  0, 'c'},
        {"report-size",     required_argument,  0, 'r'},
        {"no-seccomp",      no_argument,        0, 'S'},
        {"no-coredumps",    no_argument,        0, 'C'},
        {"forward-sig",     required_argument,  0, 's'},
        {"help",            no_argument,        0, 0},
        {0, 0, 0, 0}
    };

    int signum;
    char c = 0;
    while (c != -1) {
        c = getopt_long(argc, argv, short_cli_options, cli_options, nullptr);
        switch(c) {
            case 0:
                printHelp();
                return 0;
            case 'f':
                optraceOpts.FollowForks = true;
                break;
            case 'J':
                optraceOpts.JailForks = false;
                break;
            case 'h':
                optraceOpts.HumanReadableSizes = true;
                break;
            case 'o':
                optraceOpts.Output = optarg;
                break;
            case 'a':
                optraceOpts.AppendOutput = true;
                break;
            case 'c':
                if (optarg[0] == '-') {
                    optraceOpts.CommandLengthLimit = -1;
                } else {
                    optraceOpts.CommandLengthLimit = atoi(optarg);
                }
                break;
            case 'r':
                if (optarg[0] == '-') {
                    optraceOpts.FilesInReport = -1;
                } else {
                    optraceOpts.FilesInReport = atoi(optarg);
                }
                break;
            case 'S':
                optraceOpts.UseSecComp = false;
                break;
            case 'C':
                optraceOpts.SearchForCoreDumps = false;
                break;
            case 's':
                signum = atoi(optarg);
                if (signum > 31) {
                    std::cerr << "Invalid signal number: " << signum << std::endl;
                }
                optraceOpts.ForwardingSignals.push_back(signum);
                break;
            // Unknown option/Missing argument (getopt machinery prints error message)
            case '?':
                return 1;
        }
    }

    if (optind == argc) {
        std::cerr << "optrace: must have PROG [ARGS]\n"
                  << "Try 'optrace --help' for more information." << std::endl;
        return 1;
    }

    // Check permissions for output file
    if (!optraceOpts.Output.empty()) {
        auto flags = std::ofstream::out;
        if (optraceOpts.AppendOutput) {
            flags |= std::ofstream::app;
        } else {
            flags |= std::ofstream::trunc;
        }

        std::ofstream ofstream(optraceOpts.Output, flags);
        if (ofstream.fail()) {
            std::cerr << "Failed to open " << optraceOpts.Output << ": " << strerror(errno) << std::endl;
            return 1;
        }
    }

    return NOPTrace::TraceProgram(argv + optind, optraceOpts);
}
