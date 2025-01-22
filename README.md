# optrace
optrace records output files written by each process and accumulates total written data size.

## Usage example
`-> cat sample.py`
```python
import os

print("dumping data...")
with open("out.txt", "w") as afile:
        afile.write("world\nhello")
print("sorting data...", flush=True)
os.system("cat out.txt | sort > out.sort.txt")
os.abort()
```

```
-> optrace -h python sample.py >log

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

```
-> optrace -i '*out*' -I 2 -r0 python sample.py >log

Traceback (most recent call last):
  File "/tmp/sample.py", line 4, in <module>
    with open("out.txt", "w") as afile:
  File "/usr/lib/python3.10/codecs.py", line 186, in __init__
    def __init__(self, errors='strict'):
KeyboardInterrupt
```


## Help
```
Usage: optrace [-fJhaCDS] [-o FILE] [-c VAL]
               [-r VAL] [-j VAL] [-s SIG] PROG [ARGS]

Output format:
  -c|--cmdline-size VAL    maximum string size for cmd lines
                           (negative for unlimited, 0 to disable, default:120)
  -r|--report-size VAL     number of files to print in report
                           (negative for unlimited, 0 to disable, default:-1)
  -o|--output FILE         send report to FILE instead of stderr
  -a|--append              don't overwrite output FILE
  -h|--human-readable      print sizes in human readable format
  -i|--interruption-target VAL
                           send an interrupt signal to a process when it attempts to open a target file (fnmatch) in write mode
  -I|--interruption-sig VAL
                           signal that will be sent when the interrupt target is found (default: SIGABRT)

Behavior:
  -D|--no-coredumps        don't take into account core dump files
  -e|--empty-files         trace empty files
  -s|--forward-sig SIG     append signum to the list of forwarding signals to the PROG
                           (default: [SIGINT])
  -S|--forward-all-signals append signum to the list of forwarding signals to the PROG

Tracing:
  -w|--wait-daemons        wait for daemon processes when following forks
  -F|--no-follow-forks     don't follow forks
  -J|--no-jail-forks       don't kill all created processes, when optrace exits
  -C|--no-seccomp          don't use seccomp anyway
```

## Building
```
make -j
```
