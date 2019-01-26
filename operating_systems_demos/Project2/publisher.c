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

#include "P4header.h"

int sendMessage(char * msg, int readEnd, int writeEnd) {

    char buff[1024];

    while (1) {
        write(writeEnd, msg, strlen(msg) + 1);
        read(readEnd, buff, 1023);

        fprintf(stderr, "\n---[PUB %d] RECIEVED '%s' FROM SERVER\n", getpid(), buff);
        if (strcmp(buff, "reject") == 0) {
            continue;
        }
        else if (strcmp(buff, "accept") == 0) {
            break;
        }
        else if (strcmp(buff, "terminate") == 0) {
            return 0;	// sends false when given 'signal' to terminate
        }
        else {
            fprintf(stderr, "PUB %d MESSAGE HAS NO DEFINED RESPONSE\n", getpid());
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
    char id[20];
    sprintf(id, "%d", pubID);

    char connectString[1024];
    strcpy(connectString, "pub ");
    strcat(connectString, id);
    strcat(connectString, " connect");

    // generate connect message
    sendMessage(connectString, readEnd, writeEnd);   
    sendMessage("end", readEnd, writeEnd);   

    // generate topic message
    char topicHeader[1024];
    strcpy(topicHeader, "pub ");
    strcat(topicHeader, id);
    strcat(topicHeader, " topic ");

    srand((uintptr_t) &writeEnd);
    // generate messages to send (max 10, min 1)
    int i;
    for (i = 0; i < ((pubID % 10) + 1); i++) {
        int fakeTopicNum = rand() % MAXTOPICS;
        char fakeTopic[20];
        sprintf(fakeTopic, "%d", fakeTopicNum);

        char topicString[1024];
        strcpy(topicString, topicHeader);
        strcat(topicString, fakeTopic);
        sendMessage(topicString, readEnd, writeEnd);
        sendMessage("end", readEnd, writeEnd);
    }

    sendMessage("terminate", readEnd, writeEnd);
       

    return 0;
}
