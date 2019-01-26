/*
    Author: John Nemeth (revisions)
    Sources: class material, given lab examples
    Description:
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <stdint.h>

#include "QheaderScen1.h"

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
            usleep(100000);	// sleep for .1 seconds. keeps output from getting overwhelmed with 'retry' msgs
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

    if(argc != 3) {
        fprintf(stderr, "PUBLISHER error, bad args");
        exit(0);
    }

    int readEnd = atoi(argv[1]);
    int writeEnd = atoi(argv[2]);

    int pubID = (int)getpid();
    char msgString[1024];

    // generate connect message
    sprintf(msgString, "pub %d connect", pubID);
    sendMessage(msgString, readEnd, writeEnd);   
    sendMessage("end", readEnd, writeEnd);   

    srand((uintptr_t) &writeEnd);

    char msg[QUACKSIZE + 1];
    char toUse[] = { "abcdefghijklmnopqrstuvwxyz " };	// allowed chars for message

    // post 99 messages
    int i, j;
    for (i = 0; i < 99; i++) {

        // generate chars to put in message to send of size QUACKSIZE
        for (j = 0; j < QUACKSIZE; j++) {
            // want to include the null pointer to allow for string to terminate at any point
            msg[j] = toUse[rand() % 28];
        }
        msg[QUACKSIZE] = '\0';		// just in case they're all not the null ptr

        // chooses random topic to assign to
        sprintf(msgString, "topic %d %s", (rand() % MAXTOPICS), msg);
        sendMessage(msgString, readEnd, writeEnd);
        sendMessage("end", readEnd, writeEnd);
    }

    // sleep for 10 seconds
    sleep(10);

    // post 5 messages
    for (i = 0; i < 5; i++) {

        for (j = 0; j < QUACKSIZE; j++) {
            msg[j] = toUse[rand() % 28];
        }
        msg[QUACKSIZE] = '\0';		// just in case they're all not the null ptr

        sprintf(msgString, "topic %d %s", (rand() % MAXTOPICS), msg);
        sendMessage(msgString, readEnd, writeEnd);
        sendMessage("end", readEnd, writeEnd);
    }

    // post msg eveery second for 20 times
    for (i = 0; i < 20; i++) {

        for (j = 0; j < QUACKSIZE; j++) {
            msg[j] = toUse[rand() % 28];
        }
        msg[QUACKSIZE] = '\0';		// just in case they're all not the null ptr

        sprintf(msgString, "topic %d %s", (rand() % MAXTOPICS), msg);
        sendMessage(msgString, readEnd, writeEnd);
        sendMessage("end", readEnd, writeEnd);
        sleep(1);
    }

    sendMessage("terminate", readEnd, writeEnd);
       

    return 0;
}
