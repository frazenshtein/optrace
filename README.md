# optrace
optrace records output files written by each process and accumulates total written data size.

## Usage example
`-> cat sample.py`
```python
import os

print("dumping data...")
with open("out.txt", "w") as afile:
        afile.write("world\nhello")
print("sorting data...")
os.system("cat out.txt | sort > out.sort.txt")
os.abort()
```
`-> optrace -hf python sample.py >log`
```
Output tracer summary report (limit: 24)
   380KiB /tmp/core (pid:13830|37900288)
      32b /tmp/log (pid:13830|37900288)
      12b /tmp/out.sort.txt (pid:13833|37900016)
      11b /tmp/out.txt (pid:13830|37900288)
Proc legend:
  13833|37900016 (ppid:13831) sort
  13830|37900288 (ppid:0) python sample.py
Total output: 380KiB
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
