# Pub/Subscriber Model Demonstration
## Description:
This project implements a publisher/subscriber data communication model. 
- Publishers connect to a server, notify of their interested topics and 
publish messages to those topics.
- Subcribers connect to a server, notify of the topics they would like
to subscribe to, and through various scenarios (wait until no new messages
for x minutes and other scenarios) disconnect from the server.
All publisher and subscriber communication to the server is achieved through
linux pipelines. In addition, their activities are fully simulated through
 relevant publisher.c/subscriber.c implementation files. Total synchronization is 
used to many entities can communicate with the server at the same time. 
This project is meant to demonstrate 4 main topics within conventional operating
systems: interprocess communication, threading, synchronization, and file I/O.
- Features multi-threading
- Most complete files are: QheaderScen5.h, QserverScen5.c, QtopicsScen5.c, pubScen5.c, subScen5.c
which can be run with 
```
make QScen5
make runScen5
```
## Important Notes:
- Developed on and for a linux environment (linux system calls used)
- This is the final project for an operating systems class at UO.
- I worked on this project starting from part 1 to part 6 before I 
implemented the test scenarios as described by project specs.
So, while the scenarios are meant to be based on only part 4 of the 
project, I asked if it was appropriate to develop the scenarios for 
further parts and I was told yes. There is full communication between 
the various pub/sub.c files using pipelines beginning from part 
1 of the project. I performed what error and memory 
leak checks I could before I moved onto the next part of the Project.
## Launching Test Scenarios:
Max Topic Queue and entry number values are listed in the header files
for each associated scenario. Files used for each make target is listed
in the makefile, but I will write the dependencies here. Output using
the runScenX target outputs to respective scenx.out files, as will be
echoed when a run target is made. It is also in those runScenx targets
where the number of publishers and subscribers, for each scenario, are
are passed as arguments to the server.c files.

output from development machine is contained in the directory and is
named by its respective scenario (scen1.out, scen2.out, etc)
in the shell,

for Scenario 1 - 
files: QheaderScen1.h, QserverScen1.c, QtopicsScen1.c, pubScen1.c, subScen1.c
```
make QScen1
make runScen1
```

for Scenario 2 - 
files: QheaderScen2.h, QserverScen2.c, QtopicsScen2.c, pubScen1.c, subScen1.c
```
make QScen2
make runScen2
```

for Scenario 3 - 
files: QheaderScen3.h, QserverScen2.c, QtopicsScen2.c, pubScen3.c, subScen3.c
```
make QScen3
make runScen2
```

for Scenario 4 - 
files: QheaderScen4.h, QserverScen4.c, QtopicsScen4.c, pubScen4.c, subScen4.c
```
make QScen4
make runScen4
```

for Scenario 5 - 
files: QheaderScen5.h, QserverScen5.c, QtopicsScen5.c, pubScen5.c, subScen5.c
```
make QScen5
make runScen5
```

## Additional Notes:
- Scenario specific code can be found in the pub/sub (subs read x entries per x
seconds, pubs send x msgs, sleep, etc) and in the topics.c files (subs 
terminate after x time). The number of pubs/subs is found in the makefile 
targets for each scenario. Max topic/queue is found in the header files
as #define'd values. Some Scen4 and Scen5 requirements are passed as
additional arguments in server.c files to their pub/subs(along with the pipes)
and Clean-up requirements are found in the del and dequeue functions in topics.c
files.

- earlier parts of the project required the threads all connect before 
they proceed to other operations and all threads are required to stall
termination until it's established that all threads are ready for
termination. I used this as a safety check when terminating the deletor 
thread (as well as the time check) - to make sure a pub wouldn't
outlast the deletor.

- Random number generation is used in the non-scenario files for 1. the 
number of topics to publish/subscribe to 2. length and content generation
for entries. (pub and sub files) Also, some random number generation is used 
in the pub and sub files of the scenarios, where appropriate.

- I consider the most 'completed' part of the assignment to be the files
in Scenario 5. I ended up back-propogating a lot of fixes in those scenarios
to files completed earlier.

## Sources:
All files are original unless stated otherwise in file headers or comments (and there are a few,
though not so much relevant to the main activities of the project)
