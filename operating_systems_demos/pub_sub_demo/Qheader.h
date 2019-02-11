/*
    Author: John Nemeth
	Description: Header file used for struct declarations and global preprocessors and variables
			used in each file
    Sources: academic materials, project documentation
*/
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define MAXTOPICS 5				// max total topics allowed for topics store
#define MAXENTRIES 100				// max entries allowed for each topic
#define QUACKSIZE 140				// max character size for each message 

// preprocessors used to implant str data into outputs
#define STR2(x)
#define STR(X) STR2(x)

// struct for a topicentry, used as object for 
struct topicentry{
    int entryNum;				// records where in topic queue entry is placed
    struct timeval timestamp;			// for hold old entries are. determines when to remove, etc
    int pubID;					// ID of process
    char message[QUACKSIZE];			// stored message
};

// queue struct used for each topic
struct queue{
    pthread_mutex_t topicLock;			// mutex lock to preseve syncrhonization when threads performing operations
    struct topicentry circBuffer[MAXENTRIES];	// a circular buffer of topic entries
    int in;					// position of where an insert goes in circular buffer
    int out;					// position of out entry in circular buffer (oldest entry)
    int topicCounter;				// keeps track of number of entries
    int allRead;				// flag for all entries being read
};

// struct used to track publishers - (heavy modifcations from original struct shown in academic materials)
struct pub_process{
    pid_t pid;					// stores process id
    pthread_t pubTID;				// stores thread id
    int pub_to_quacker[2];			// stores publisher to server pipe file descriptors
    int quacker_to_pub[2];			// stores server to publisher pipe file descriptors
    int connected;				// flag for publisher connected
    int terminated;				// flag for publisher terminated
    int pubid;					// secondary id value for publisher
    int topicList[MAXTOPICS];			// stores list of topics published to
};

// struct used to track subscribers - (heavy modifications from original struct shown in academic materials)
struct sub_process{
    pid_t pid;					// stores process ID of subscriber
    pthread_t subTID;				// stores threadID of proxy thread
    int sub_to_quacker[2];			// stores subscriber to server pipe file descriptors
    int quacker_to_sub[2];			// stores server to subscriber pipe file descriptors
    int connected;				// flag for subscriber successfully connected
    int terminated;				// flag for subscriber successfully terminated
    int subid;					// secondary id value for subscriber
    int topicList[MAXTOPICS];			// stores list of topics subscriber reads from
};

/* routine which runs as forked process started in Qserver.c
	pubs[]: array of connected publishers
	subs[]: array of connected subscribers
	TOTAL_PUBS: total publisher count - passed as argument in Qserver.c
	TOTAL_SUBS: total subscriber count - passed as argument in Qserver.c
*/
int topicServer(struct pub_process pubs[], int TOTAL_PUBS, struct sub_process subs[], int TOTAL_SUBS);
