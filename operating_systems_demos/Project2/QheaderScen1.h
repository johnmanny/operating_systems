/*
    Author: John Nemeth (revisions) - original by UO dept staff
    Sources: Class material
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

#define MAXTOPICS 1
#define MAXENTRIES 100
#define QUACKSIZE 140
#define STR2(x)
#define STR(X) STR2(x)

struct topicentry{
    int entryNum;
    struct timeval timestamp;
    int pubID;
    char message[QUACKSIZE];
};

struct queue{
    pthread_mutex_t topicLock;
    struct topicentry circBuffer[MAXENTRIES];
    int in;
    int out;
    int topicCounter;
    int allRead;
};

struct pub_process{
    pid_t pid;
    pthread_t pubTID;
    int pub_to_quacker[2];
    int quacker_to_pub[2];
    int connected;
    int terminated;
    int pubid;
    int topicList[MAXTOPICS];
};

struct sub_process{
    pid_t pid;
    pthread_t subTID;
    int sub_to_quacker[2];
    int quacker_to_sub[2];
    int connected;
    int terminated;
    int subid;
    int topicList[MAXTOPICS];
};

// function declarations
int topicServer(struct pub_process pubs[], int TOTAL_PUBS, struct sub_process subs[], int TOTAL_SUBS);
void print_pubs(struct pub_process pubs[], int num_pubs);
void print_subs(struct sub_process subs[], int num_subs);
