/*
    Author: John Nemeth
    Sources: class material, given lab examples
    Description: pub file that connects to server through pipes for scenario 4
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <stdint.h>

#include "QheaderScen4.h"

int sendMessage(char * msg, int readEnd, int writeEnd) {

    char buff[1024];

    while (1) {
        write(writeEnd, msg, strlen(msg) + 1);
        read(readEnd, buff, 1023);

        fprintf(stdout, "\n---[PUB %d] RECIEVED '%s' FROM SERVER\n", getpid(), buff);
        if (strcmp(buff, "reject") == 0) {
            continue;
        }
        else if (strcmp(buff, "accept") == 0) {
            break;
        }
        else if (strcmp(buff, "retry") == 0) {
            usleep(100000);		// to help with output overflow
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
    int topicsArray[2];	


    // define possible topics to post to per sub based on passed argument
    if (atoi(argv[3]) == 1) {
        topicsArray[0] = 0;
        topicsArray[1] = 1;
    }
    else if (atoi(argv[3]) == 2) {
        topicsArray[0] = 1;
        topicsArray[1] = 2;
    }
    else {
        topicsArray[0] = 2;
        topicsArray[1] = 0;
    }

    fprintf(stdout, "\n---[PUB PID %d] using topics %d and %d\n", (int)getpid(), topicsArray[0], topicsArray[1]);
    fprintf(stderr, "\n---[PUB PID %d] using topics %d and %d\n", (int)getpid(), topicsArray[0], topicsArray[1]);

    // generate connect message
    char msgString[1024];
    char msg[QUACKSIZE + 1];
    char toUse[] = { "abcdefghijklmnopqrstuvwxyz " };	// allowed chars for message
    int i, j, v;

    sprintf(msgString, "pub %d connect", (int)getpid());
    sendMessage(msgString, readEnd, writeEnd);   
    sendMessage("end", readEnd, writeEnd);   

    srand((uintptr_t) &writeEnd);


    // repeat 10 times
    for (i = 0; i < 10; i++) {
        // send 20 entries
        for (j = 0; j < 20; j++) {
            // choose random-length & random-char msg up to QUACKSIZE size
            for (v = 0; v < QUACKSIZE; v++) {
                msg[v] = toUse[rand() %28];	// want to include null ptr for variable size msgs
            }
            msg[QUACKSIZE] = '\0';	// just in case they're all not null ptr

            // randomly choose topic index for post
            sprintf(msgString, "topic %d %s", topicsArray[(rand() % 2)], msg);
            sendMessage(msgString, readEnd, writeEnd);
            sendMessage("end", readEnd, writeEnd);
        }
        sleep(1);		// sleep 1 second between topics
    }

    sendMessage("terminate", readEnd, writeEnd);

    return 0;
}
