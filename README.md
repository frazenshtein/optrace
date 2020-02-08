# optrace
Record output files written by each process

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

== Usage example
```
-> cat sample.py
import os

with open("out.txt", "w") as afile:
    afile.write("world\nhello")
os.system("cat out.txt | sort > out.sort.txt")
os.abort()

-> optrace -hf python sample.py
Output tracer summary report (limit: 24)
   3.2MiB /tmp/core (pid:19743|94441847461440)
    12.0b /tmp/out.sort.txt (pid:19746|94441847463072)
    11.0b /tmp/out.txt (pid:19743|94441847461440)
Proc legend:
  19746|94441847463072 (ppid:19744) sort
  19743|94441847461440 (ppid:0) python sample.py
Total output: 3.2MiB
```

== Building
```
make -j
```