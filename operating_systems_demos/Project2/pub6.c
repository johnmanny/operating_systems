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

#include "P6header.h"

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
        else if (strcmp(buff, "retry") == 0) {
            continue;
        }
        else if (strcmp(buff, "successful") == 0) {
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

    // generate topic header for msg
    char topicHeader[1024];
    strcpy(topicHeader, "topic ");

    srand((uintptr_t) &writeEnd);
    // generate messages to send (max 10, min 1)
    int i;
    for (i = 0; i < ((pubID % 2) + 1); i++) {
        // generate topic num to publish to
        int fakeTopicNum = rand() % MAXTOPICS;
        char fakeTopic[20];
        char topicString[1024];
        char msg[QUACKSIZE + 1];
        char toUse[] = { "abcdefghijklmnopqrstuvwxyz " };	// allowed chars for message

        sprintf(fakeTopic, "%d ", fakeTopicNum);
        // put generated topic num
        strcpy(topicString, topicHeader);
        strcat(topicString, fakeTopic);

        // generate chars to put in message to send of size QUACKSIZE
        int j;
        for (j = 0; j < QUACKSIZE; j++) {
            // want to include the null pointer to allow for string to terminate at any point
            msg[j] = toUse[rand() % 28];	// select char inside toUse string
        }
        msg[QUACKSIZE] = '\0';

        strcat(topicString, msg);
        sendMessage(topicString, readEnd, writeEnd);
        sendMessage("end", readEnd, writeEnd);

    }

    sendMessage("terminate", readEnd, writeEnd);
       

    return 0;
}
