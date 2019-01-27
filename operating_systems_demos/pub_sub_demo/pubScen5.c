/*
    Author: John Nemeth 
    Sources: class material, given lab examples, online
    Description: pub file for scenario 5
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <stdint.h>

#include "QheaderScen5.h"

int sendMessage(char * msg, int readEnd, int writeEnd) {

    char buff[1024];

    while (1) {
        write(writeEnd, msg, strlen(msg) + 1);
        read(readEnd, buff, 1024);

        fprintf(stdout, "\n---[PUB %d] RECIEVED '%s' FROM SERVER\n", getpid(), buff);
        if (strcmp(buff, "reject") == 0) {
            continue;
        }
        else if (strcmp(buff, "accept") == 0) {
            break;
        }
        else if (strcmp(buff, "retry") == 0) {
            usleep(100000);	// sleep for .1 secs - to reduce output spam
            continue;
        }
        else if (strcmp(buff, "successful") == 0) {
            break;
        }
        else if (strcmp(buff, "terminate") == 0) {
            return 0;	// sends false when given 'signal' to terminate
        }
        else {
            fprintf(stdout, "PUB %d MESSAGE HAS NO DEFINED RESPONSE\n", getpid());
            break;
        }
    }
    return 1;
}

int main(int argc, char * argv[]){

    if(argc != 4) {
        fprintf(stderr, "PUBLISHER error, bad args");
        exit(0);
    }

    int readEnd = atoi(argv[1]);
    int writeEnd = atoi(argv[2]);
    int topic = atoi(argv[3]);


    fprintf(stdout, "\n---[PUB PID %d] using topic %d\n", (int)getpid(), topic);
    fprintf(stderr, "\n---[PUB PID %d] using topic %d\n", (int)getpid(), topic);

    // generate connect message
    char msgString[1024];
    char msg[QUACKSIZE + 1];
    char toUse[] = { "abcdefghijklmnopqrstuvwxyz " };	// allowed chars for message
    int i, j, v;

    sprintf(msgString, "pub %d connect", (int)getpid());
    sendMessage(msgString, readEnd, writeEnd);   
    sendMessage("end", readEnd, writeEnd);   

    srand((uintptr_t) &writeEnd);

    // repeat 20 times (where each iteration is a second)
    for (i = 0; i < 20; i++) {
        // post 20 entries per second
        for (j = 0; j < 20; j++) {
            // choose random-length & random-char msg up to QUACKSIZE size
            for (v = 0; v < QUACKSIZE; v++) {
                msg[v] = toUse[rand() %28];	// want to include null ptr for variable size msgs
            }
            msg[QUACKSIZE] = '\0';	// just in case they're all not null ptr

            // only posts to one topic that was passed as argument in argv[3]
            sprintf(msgString, "topic %d %s", topic, msg);
            sendMessage(msgString, readEnd, writeEnd);
            sendMessage("end", readEnd, writeEnd);
        }
        sleep(1);		// sleep 1 second
    }
    
    sendMessage("terminate", readEnd, writeEnd);

    return 0;
}
