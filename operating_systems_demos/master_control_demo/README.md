# Master Control Program
This project demonstrates the function of a master control program. 
Throughout different sections of the project, the MCP reads a list of 
programs with arguments to run from a file, start up and run the 
programs as processes, and schedule processes to run concurrently in 
various time-sliced ways. This is accomplished at different stages
throughout the project. Detailed information is provided in the 
project_details.pdf. 

## Features
Throughout the entire project, OS (linux) system calls are used for synchronization
and scheduling. This includes sigwait, alarm, kill, fork, and so on.
This means this project can only run on linux. 

## Use
This project contains 4, distinct, individually compiled and operating parts. P1 to P4.

Makefile targets are provided.
```
make P1
./P1 testprgms		// or ./P1 < testprgms

make P2
./P2 testprgms

// until P4
```

## Sources/Acknowledgements
p1fxns.c, p1fxns.h, cpubound.c, and iobound.c were not written by me and I take no credit for them. 
The University of Oregon's CIS department can take credit for them. Unless stated in comments in code
or in file headers, all other work is original - written by me.
