# optrace
Record output files written by each process

## Usage example
`-> cat sample.py`
```python
import os

with open("out.txt", "w") as afile:
    afile.write("world\nhello")
os.system("cat out.txt | sort > out.sort.txt")
os.abort()
```
`-> optrace -hf python sample.py`
```
Output tracer summary report (limit: 24)
   3.2MiB /tmp/core (pid:22000|94249052452416)
      12b /tmp/out.sort.txt (pid:22003|94249052451712)
      11b /tmp/out.txt (pid:22000|94249052452416)
Proc legend:
  22003|94249052451712 (ppid:22001) sort
  22000|94249052452416 (ppid:0) python sample.py
Total output: 3.2MiB
```
## Help
```
Usage: optrace [-fJhaCDS] [-o FILE] [-c VAL]
               [-r VAL] [-j VAL] [-s SIG] PROG [ARGS]

Output format:
  -c|--cmdline-size VAL    maximum string size for cmd lines
                           (negative for unlimited, 0 to disable, default:100)
  -r|--report-size VAL     number of files to print in report
                           (negative for unlimited, 0 to disable, default:24)
  -o|--output FILE         send report to FILE instead of stderr
  -a|--append              don't overwrite output FILE
  -h|--human-readable      print sizes in human readable format

Behavior:
  -D|--no-coredumps        don't take into account core dump files
  -s|--forward-sig SIG     append signum to the list of forwarding signals to the PROG
                           (default: [SIGINT])
  -S|--forward-all-signals append signum to the list of forwarding signals to the PROG

Tracing:
  -j|--threads VAL         use VAL threads to trace
                           (default: 2)
  -f|--follow-forks        follow forks
  -J|--no-jail-forks       don't kill all created processes, when optrace exits
  -C|--no-seccomp          don't use seccomp anyway
```

## Building
```
make -j
```