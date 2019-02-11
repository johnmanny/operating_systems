# Pub/Subscriber Model Demonstration
## Description:
This project implements a publisher/subscriber data communication pattern. 
Publishers connect to a server, notify of their interested topics and 
publish messages to those topics.
Subcribers connect to a server, notify of the topics they would like
to subscribe to, and through various scenarios (wait until no new messages
for x minutes and other scenarios) disconnect from the server.
All publisher and subscriber communication to the server is achieved through
linux pipelines. In addition, their activities are fully simulated through
 relevant publisher.c/subscriber.c implementation files. Total synchronization is 
used to many entities can communicate with the server at the same time. 
This project is meant to demonstrate 4 main topics within conventional operating
systems: interprocess communication, threading, synchronization, and file I/O.

## Important Notes:
- Features multi-threading
- Developed on and for a linux environment (linux system calls used)
- More detailed project instructions can be found in project_details.pdf
- The earlier_parts directory has earlier portions of the project but the
files in this directory represent the final result. 

## Use
This should be run in a linux environment. The following make commands
work in the terminal:

```
make 				# compile the code
make runQ			# run the system (outputs most info to scen5.out file)
make clean			# clean up directory
```

## System operation description
The operation of this system can be described by this scenario: 

5 publishers, 1 subscriber, 5 topics, queue length of 100

Each publisher posts to one topic. Each topic has one publisher.
Publishers post 20 entries each second for 20 seconds.

Subscriber sleeps for 15 seconds.
Then subscriber reads 30 topics each second from topics in round-robin order.
Subscriber terminates when there is nothing new to read for 5 seconds.

Clean-up removes entries in round-robin order, selecting entries that are older than 4 seconds.
Clean-up terminates when there is nothing to terminate for more than 10 seconds.

## Additional Notes:
- Scenario specific code can be found in the pub/sub (subs read x entries per x
seconds, pubs send x msgs, sleep, etc) and in the topics.c files (subs 
terminate after x time). The number of pubs/subs is found in the makefile 
targets for each scenario. Max topic/queue is found in the header files
as #define'd values. Some requirements are passed as
additional arguments in server.c files to their pub/subs(along with the pipes)
and Clean-up requirements are found in the del and dequeue functions in topics.c
files.

- Earlier parts of the project required the threads all connect before 
they proceed to other operations and all threads are required to stall
termination until it's established that all threads are ready for
termination. I used this as a safety check when terminating the deletor 
thread (as well as the time check) - to make sure a pub wouldn't
outlast the deletor.

- This directory provides the final implementation and documentation effort for this
project. Files in the earlier_parts directory were work-in-progress and do not contain
accurate source information, documentation material, or bug-fixed code. However, by using
the makefile targets, it is possible to run different scenarios and earlier parts of the 
project to gauge how it progressed. 

## Sources:
The vast majority of this code is original. However, some struct templates and other
minor code was based on academic material cited both in the project description and 
in comments within the source code. Also, details regarding provided files
listed in project_details.pdf are not accurate. No examples or code was used or 
provided in the construction of the publisher and subscriber source files nor for any
code that used linux pipes for inter-process communication. 
